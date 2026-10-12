#include "task_comm.h"

#include "stdio.h"
#include "string.h"
#include "task.h"

#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"

#include "config.h"
#include "sensor_data.h"
#include "wifi_secrets.h"   // WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP, MQTT_USER, MQTT_PASS

static mqtt_client_t *mqtt_client;
static volatile bool mqtt_connected = false;
static volatile err_t last_mqtt_publish_result = ERR_OK;

/*! \brief MQTT connection callback function.
 *
 * This function is called when the MQTT client connects or disconnects from the broker.
 *
 * \param client The MQTT client.
 * \param arg The argument passed to the callback.
 * \param status The connection status.
 */
static void mqtt_connection_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    mqtt_connected = (status == MQTT_CONNECT_ACCEPTED);
    if (!mqtt_connected) {
        printf("MQTT refuse, status=%d\n", status);
    }
}

/*! \brief MQTT publish request callback function.
 *
 * This function is called when the MQTT publish request is completed.
 *
 * \param arg The argument passed to the callback.
 * \param result The result of the publish request.
 */
static void mqtt_publish_request_callback(void *arg, err_t result) {
    TaskHandle_t comm_task_handle = (TaskHandle_t)arg;
    last_mqtt_publish_result = result;
    if (comm_task_handle != NULL) {
        xTaskNotifyGive(comm_task_handle);
    }

    if (result == ERR_OK) {
        printf("Callback: MQTT publish successful\n");
    } else {
        printf("Callback: MQTT publish failed: %d\n", result);
    }
}

/*! \brief Initialize Wi-Fi connection.
 *
 * This function initializes the Wi-Fi connection using the cyw43_arch driver 
 * and the provided SSID and password.
 *
 * \return true if the Wi-Fi connection is successful, false otherwise.
 */
static bool wifi_init(void) {
    if (cyw43_arch_init()) {
        printf("Wi-Fi initialisation failed\n");
        return false;
    }
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                            CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS)) {
        printf("Wi-Fi connection failed\n");
        return false;
    }

    printf("Wi-Fi connection success\n");
    return true;
}

/*! \brief Set up the MQTT client.
 *
 * This function initializes the MQTT client, sets up the connection parameters,
 * and tries to connects to the MQTT broker.
 *
 * \return true if the MQTT setup is successful, false otherwise.
 */
static bool mqtt_setup(void) {
    mqtt_client = mqtt_client_new();
    if (!mqtt_client) return false;

    ip_addr_t broker_addr;
    ipaddr_aton(MQTT_BROKER_IP, &broker_addr);

    struct mqtt_connect_client_info_t client_info = {0};
    client_info.client_id   = MQTT_CLIENT_ID;
    client_info.client_user = MQTT_USER;
    client_info.client_pass = MQTT_PASS;
    client_info.keep_alive  = MQTT_KEEP_ALIVE;

    cyw43_arch_lwip_begin();
    err_t err = mqtt_client_connect(mqtt_client, &broker_addr, MQTT_BROKER_PORT,
                                     mqtt_connection_callback, NULL, &client_info);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("mqtt_client_connect failed: %d\n", err);
        return false;
    }
    return true;
}

/*! \brief Publish sensor data via MQTT.
 *
 * This function publishes the sensor data to the MQTT topic.
 *
 * \param data The sensor data to publish.
 * \return true if the publish is successful, false otherwise.
 */
static bool publish_sensor_data(sensor_data_t *data) {
    if (!mqtt_connected) return false;

    char payload[128];
    int len = snprintf(payload, sizeof(payload),
        "{\"temp\":%.1f,\"hum\":%d,\"lum\":%d}",
        data->temperature, data->humidity, data->luminosity);

    // Purge the notification of the previous publish request, if any.
    ulTaskNotifyTake(pdTRUE, 0);

    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(mqtt_client, MQTT_TOPIC, payload, len,
                              MQTT_QOS, MQTT_RETAIN, mqtt_publish_request_callback, 
                              (void *)xTaskGetCurrentTaskHandle());
    cyw43_arch_lwip_end();

    // If the publish request was not accepted, return false immediately not waiting for the callback.
    if (err != ERR_OK) {
        printf("MQTT publish failed: %d\n", err);
        return false;
    }

    // Wait for the publish request to complete receiving a notification from the callback, with a timeout.
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(MQTT_ACK_TIMEOUT_MS)) != pdPASS) {
        printf("MQTT timeout, no response received from the broker.\n");
        return false;
    }

    return (last_mqtt_publish_result == ERR_OK);
}


void task_comm(void *pvParameters) {
    bool wifi_ok = wifi_init();
    bool mqtt_ok = wifi_ok && mqtt_setup();

    printf("task_comm started. Wi-Fi: %s, MQTT: %s\n",
           wifi_ok ? "OK" : "FAIL", mqtt_ok ? "OK" : "FAIL");

    // Make on-board LED blink 3 times if Wi-Fi and MQTT are OK.
    uint32_t blink_count = (wifi_ok && mqtt_ok) ? 3 : 0;
    for (int i = 0; i < blink_count; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(1500));
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    sensor_data_t sensor_data = {0};
    bool first_publish = true;

    while (true) {
        if (mqtt_ok && xQueuePeek(comm_sensor_data_queue, &sensor_data, 0) == pdPASS) {

            if (publish_sensor_data(&sensor_data)) {
                if (!first_publish) {              
                    // Visual validation of successful MQTT publish by turning on the LED for 1.5 seconds.
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);
                }
                
                first_publish = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(COMM_TASK_DELAY_MS));
    }
}
