#include "task_comm.h"

#include "config.h"
#include "sensor_data.h"
#include "wifi_secrets.h"   // WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP, MQTT_USER, MQTT_PASS

#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "task.h"

#include <stdio.h>
#include <string.h>


static mqtt_client_t *mqtt_client;
static volatile bool mqtt_connected = false;
static volatile bool mqtt_connection_status_received = false;
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
    mqtt_connection_status_received = true;
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

    char payload[MQTT_PAYLOAD_BUFFER_SIZE];
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

/*! \brief Publish the latest sensor data from the queue if it is fresh.
 *
 * This function retrieves the latest sensor data from the queue and publishes it via MQTT
 * if the data is fresh (not outdated).
 *
 * \return true if the publish is successful, false otherwise.
 */
static bool publish_latest_data(void) {
    sensor_data_t latest_data;

    if (xQueuePeek(sensor_data_queue, &latest_data, 0) != pdPASS) {
        return false; // No data available in the queue.
    }

    if (!sensor_data_is_fresh(&latest_data)) {
        printf("Latest sensor data is outdated, not publishing.\n");
        return false; // Data is outdated.
    }

    return publish_sensor_data(&latest_data);
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

/*! \brief Blink the on-board LED to indicate connection status.
 *
 * This function blinks the on-board LED 3 times if Wi-Fi and MQTT connection is ok.
 * If one fails, the LED blinks once to indicate a failure.
 * 
 *  \param connection_ok A boolean indicating that both Wi-Fi and MQTT are successfully connected.
 */
static void blink_connection_status(bool connection_ok) {
    if (connection_ok) {
        for (int blink = 0; blink < CONNECTION_SUCCESS_BLINK_COUNT; blink++) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(CONNECTION_LED_ON_TIME_MS));
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(CONNECTION_LED_OFF_TIME_MS));
        }
    } else {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);
        vTaskDelay(pdMS_TO_TICKS(CONNECTION_FAILURE_LED_TIME_MS));
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);
    }
}

/*! \brief Blink the on-board LED to indicate a successful data publish.
 *
 * This function blinks the on-board LED once to indicate that sensor data has been successfully published.
 */
static void blink_data_published(void) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_ON);
    vTaskDelay(pdMS_TO_TICKS(DATA_PUBLISHED_LED_TIME_MS));
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, LED_OFF);
}


void task_comm(void *pvParameters) {
    bool wifi_ok = wifi_init();
    
    if (wifi_ok && mqtt_setup()) {
        // Define a timeout for the MQTT connection status callback to be called.
        // 15000ms / 100ms = 150 iterations = 15 seconds
        uint8_t timeout_counter = MQTT_CONNECTION_STATUS_TIMEOUT_MS / MQTT_CONNECTION_POLL_INTERVAL_MS;

        // Wait for the MQTT connection status callback to be called.
        while (!mqtt_connection_status_received && timeout_counter > 0) {
            vTaskDelay(pdMS_TO_TICKS(MQTT_CONNECTION_POLL_INTERVAL_MS));
            timeout_counter--;
        }
        if (timeout_counter == 0) {
            printf("MQTT connection status callback not received within the timeout period.\n");
            mqtt_connected = false;
        }
    }
    blink_connection_status(wifi_ok && mqtt_connected);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(COMM_TASK_DELAY_MS));

        if (mqtt_connected && publish_latest_data()) {
            blink_data_published();
        }
    }
}
