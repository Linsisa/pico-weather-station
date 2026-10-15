#include "task_display.h"

#include "stdio.h"
#include "stdint.h"
#include "limits.h"
#include "pico/stdlib.h"

#include "config.h"
#include "display_mode.h"
#include "sensor_data.h"
#include "src/ui/lcd_ui.h"


static volatile display_mode_t current_display_mode = DISPLAY_MODE_FULL_INFO; // Default display mode show full information.
static volatile uint64_t last_display_change_button_press_time = 0; // Variable to track the last button press time.
static volatile bool has_display_mode_changed = false; // Flag to indicate if the display mode has changed and the screen needs to be cleared before updating.


// =====================================================================================================================
// Button Handling Functions
// =====================================================================================================================
/*! \brief Callback function for the display button press event.
 *
 * This function is called when the display button is pressed.
 * It tracks the time of the button press to avoid multiple rapid presses 
 * and notifies the display task that a screen mode change is requested.
 *
 * \param gpio The GPIO pin number that triggered the interrupt.
 * \param events The event type that triggered the interrupt (e.g., rising edge).
 */
static void display_button_callback(uint gpio, uint32_t events) {
    // This function is called when the display button is pressed and notify the display task that screen mode change is requested.
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_RISE)) {
        uint64_t current_time = time_us_64();

        if (current_time - last_display_change_button_press_time >= BUTTON_COOLDOWN_US) {
            last_display_change_button_press_time = current_time;

            // Notify the display task that a screen mode change is requested.
            if (display_task_handle != NULL) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                
                xTaskNotifyFromISR(display_task_handle, BUTTON_NOTIFY_BIT, eSetBits, &xHigherPriorityTaskWoken);

                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}

/*! \brief Initialize the display mode button GPIO and set up the interrupt.
 *
 * This function configures the GPIO pin for the display mode button as an input,
 * enables the pull-down resistor, and sets up an interrupt to trigger on a rising edge.
 * The interrupt will call the display_button_callback function when the button is pressed.
 */
static void display_mode_button_init(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); 
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &display_button_callback);
}


/*! \brief Adapter for the lcd_outdated_data_mode function to match the signature of other display mode functions.
 *
 * This function is used as a placeholder for the outdated data display mode, which does not require sensor data or a buffer.
 * It simply calls the lcd_outdated_data_mode function to display the outdated data message.
 *
 * \param sensor_data The sensor data structure (not used in this function).
 * \param buffer A character buffer (not used in this function).
 * \param buffer_size The size of the character buffer (not used in this function).
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
    static char buffer[16]; // Buffer to hold formatted strings for LCD display.
    uint8_t buffer_size = sizeof(buffer);
    uint32_t notification_value; // Variable used to store the RTOS task's notification value, when the task is notified.
    bool data_retrieved_ok = false; // Flag to indicate if new sensor data has been retrieved from the queue and is fresh.

    display_mode_button_init(); 
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
