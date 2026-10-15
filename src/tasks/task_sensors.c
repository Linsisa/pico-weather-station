#include "task_sensors.h"

#include "config.h"
#include "sensor_data.h"
#include "src/drivers/ldr_adc.h"
#include "src/drivers/dht11_pio.h"

#include <stdio.h>
#include <stdint.h>


/*! \brief Read the LDR value from the ADC and convert it to a percentage.
 *
 * This function reads the raw ADC value from the LDR and converts it to a percentage
 * based on the maximum ADC value (4095).
 *
 * \param sensor_data A pointer to the sensor_data_t structure to store the luminosity.
 */
static void read_ldr(sensor_data_t *sensor_data) {
    uint16_t ldr_value = ldr_adc_read();
    sensor_data->luminosity = (uint8_t)((ldr_value * 100) / 4095);
}

/*! \brief Read data from the DHT11 sensor using the PIO program.
 *
 * This function reads the raw data from the DHT11 sensor using the PIO program and
 * validates the checksum. If the data is valid, it updates the sensor_data structure
 * with the temperature and humidity values.
 *
 * \param sensor_data A pointer to the sensor_data_t structure to store temperature and humidity.
 * \param program_offset The offset of the initialized PIO program for DHT11.
 * \return true if the read was successful and data is valid, false otherwise.
 */
static bool read_dht11(sensor_data_t *sensor_data, int program_offset) {
    uint8_t dht11_bytes[DHT_NUM_BYTES];

    if (dht11_read_data(program_offset, dht11_bytes)) {
        if (dht11_valid_checksum(dht11_bytes)) {
            sensor_data->humidity = dht11_bytes[0];
            sensor_data->temperature = (float)dht11_bytes[2] + (float)dht11_bytes[3] / 10.0f;
            return true;
        } else {
            printf("DHT11 checksum failed.");
        }
    }

    return false;
}


void task_sensors(void *pvParameters) {
    sensor_data_t sensor_data = {0};

    int program_offset = 0; // Used to store program offset memory for DHT11 pio program.
    uint8_t dht11_bytes[DHT_NUM_BYTES]; // Buffer to hold the raw data read from the DHT11 sensor.
    bool dht11_readed = false;

    // Initialize ADC for LDR and PIO for DHT11.
    ldr_adc_init();
    program_offset = dht11_pio_program_init();

    if (program_offset < 0) {
        printf("Failed to initialize DHT11 PIO program.\n");
        vTaskDelete(NULL); 
        // Terminate the task if initialization fails. 
        // Maybe consider adding a retry mechanism or error handling here in the future.
    }

    // Time needed by dht11 to stabilize after power-up (cf. datasheet recommendation).
    vTaskDelay(pdMS_TO_TICKS(DHT_INITIAL_WAIT_MS));

    while (true) {
        read_ldr(&sensor_data); // Read LDR value and convert it to percentage.
        dht11_readed = read_dht11(&sensor_data, program_offset); // Read data from DHT11 (temperature and humidity).
        sensor_data.timestamp_ms = to_ms_since_boot(get_absolute_time()); // Update the timestamp in milliseconds since boot.

        if (dht11_readed) {
            xQueueOverwrite(sensor_data_queue, &sensor_data);

            if (display_task_handle != NULL) {
                xTaskNotify(display_task_handle, SENSOR_NOTIFY_BIT, eSetBits);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_DELAY_MS));
    }
}
