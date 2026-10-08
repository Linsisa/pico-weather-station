#include "task_comm.h"

#include "stdio.h"
#include "string.h"

#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"

#include "config.h"
#include "sensor_data.h"
#include "wifi_secrets.h"   // WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER_IP, MQTT_USER, MQTT_PASS


static mqtt_client_t *mqtt_client;
static volatile bool mqtt_connected = false;

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
    if (result != ERR_OK) {
        printf("MQTT publish failed: %d\n", result);
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
 */
static void publish_sensor_data(sensor_data_t *data) {
    if (!mqtt_connected) return;

    char payload[128];
    int len = snprintf(payload, sizeof(payload),
        "{\"temp\":%.1f,\"hum\":%d,\"lum\":%d}",
        data->temperature, data->humidity, data->luminosity);

    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(mqtt_client, MQTT_TOPIC, payload, len,
                              MQTT_QOS, MQTT_RETAIN, mqtt_publish_request_callback, NULL);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("mqtt_publish error: %d\n", err);
    }
}


void task_comm(void *pvParameters) {
    bool wifi_ok = wifi_init();
    bool mqtt_ok = wifi_ok && mqtt_setup();

    printf("task_comm started. Wi-Fi: %s, MQTT: %s\n",
           wifi_ok ? "OK" : "FAIL", mqtt_ok ? "OK" : "FAIL");

    sensor_data_t sensor_data = {0};
    while (true) {
        if (mqtt_ok && xQueuePeek(comm_sensor_data_queue, &sensor_data, 0) == pdPASS) {
            publish_sensor_data(&sensor_data);
        }

        vTaskDelay(pdMS_TO_TICKS(COMM_TASK_DELAY_MS));
    }
}
