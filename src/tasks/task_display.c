#include "task_display.h"

#include "config.h"
#include "display_mode.h"
#include "sensor_data.h"
#include "display_button.h"
#include "src/ui/lcd_ui.h"

#include <stdio.h>
#include <stdint.h>
#include <limits.h>


static volatile display_mode_t current_display_mode = DISPLAY_MODE_FULL_INFO; // Default display mode show full information.

/*! \brief Adapter for the lcd_outdated_data_mode function to match the signature of other display mode functions.
 *
 * This function is used as a placeholder for the outdated data display mode, which does not require sensor data or a buffer.
 * It simply calls the lcd_outdated_data_mode function to display the outdated data message.
 *
 * \param sensor_data_unused The sensor data structure (not used in this function).
 * \param buffer_unused A character buffer (not used in this function).
 * \param buffer_size_unused The size of the character buffer (not used in this function).
 */
static void lcd_outdated_data_mode_adapter(sensor_data_t sensor_data_unused, char *buffer_unused, uint8_t buffer_size_unused) {
    (void)sensor_data_unused; (void)buffer_unused; (void)buffer_size_unused; // Suppress unused parameter warnings
    lcd_outdated_data_mode();
}

typedef void (*display_mode_function_t)(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size);
static const display_mode_function_t display_handlers[] = {
    [DISPLAY_MODE_FULL_INFO]   = lcd_full_info_mode,
    [DISPLAY_MODE_TEMPERATURE] = lcd_temperature_mode,
    [DISPLAY_MODE_HUMIDITY]    = lcd_humidity_mode,
    [DISPLAY_MODE_LUMINOSITY]  = lcd_luminosity_mode,
    [DISPLAY_MODE_OUTDATED]    = lcd_outdated_data_mode_adapter
};

/*! \brief Refresh the LCD display based on the current display mode and sensor data.
 *
 * This function checks if the display mode has changed and clears the screen if necessary.
 * It then calls the appropriate function to update the display based on the current display mode.
 *
 * \param sensor_data The sensor data structure containing temperature, humidity, and luminosity values.
 * \param data_ok A boolean indicating whether the sensor data is fresh and valid.
 * \param buffer A character buffer used for formatting strings to be displayed on the LCD.
 * \param buffer_size The size of the character buffer.
 */
static void refresh_display(sensor_data_t sensor_data, bool data_ok, char *buffer, uint8_t buffer_size) {
    static display_mode_t last_display_mode = DISPLAY_MODE_FULL_INFO; // Track the last display mode to detect changes.

    // If data is outdated, force the display mode to be OUTDATED. Otherwise, use the current display mode.
    display_mode_t new_mode = data_ok ? current_display_mode : DISPLAY_MODE_OUTDATED;

    if (new_mode >= sizeof(display_handlers) / sizeof(display_handlers[0]) || !display_handlers[new_mode]) {
        // Case where the new_mode is invalid or not handled, default to full info mode.
        printf("Warning: Invalid display mode %d. Defaulting to DISPLAY_MODE_FULL_INFO.\n", new_mode);
        new_mode = DISPLAY_MODE_FULL_INFO;
    }

    if (new_mode != last_display_mode) {
        last_display_mode = new_mode;
        lcd_clear_ui(); // Clear the screen if the display mode has changed.
    }

    display_handlers[new_mode](sensor_data, buffer, buffer_size);
}


// =====================================================================================================================
// FreeRTOS Task: Display Task
// =====================================================================================================================
void task_display(void *pvParameters) {

    sensor_data_t sensor_data = {0}; // Initialize sensor data structure to hold the latest sensor readings.
    static char buffer[LCD_ROW_BUFFER_SIZE]; // Buffer to hold formatted strings for a single row of LCD display.
    uint8_t buffer_size = sizeof(buffer);
    uint32_t notification_value; // Variable used to store the RTOS task's notification value, when the task is notified.
    bool data_retrieved_ok = false; // Flag to indicate if new sensor data has been retrieved from the queue and is fresh.

    display_button_init(); 
    lcd_init(); 

    while(true) {
        // Wait for a notification from the sensor task or button press events to update the display accordingly.
        BaseType_t notified = xTaskNotifyWait(0, ULONG_MAX, &notification_value,
                                              pdMS_TO_TICKS(DISPLAY_WAIT_TIMEOUT_MS));

        /* Check the freshness of the latest sensor data after every wake-up,
        * whether caused by a notification or by the timeout.
        */
        data_retrieved_ok = (xQueuePeek(sensor_data_queue, &sensor_data, 0) == pdPASS) && 
                             sensor_data_is_fresh(&sensor_data);

        // Check if the notification is from the button press event and update the display mode accordingly.
        if (notified == pdTRUE && (notification_value & BUTTON_NOTIFY_BIT) && data_retrieved_ok) {
            current_display_mode = (display_mode_t)((current_display_mode + 1) % DISPLAY_MODE_COUNT);
        }

        refresh_display(sensor_data, data_retrieved_ok, buffer, buffer_size);
    }
}
