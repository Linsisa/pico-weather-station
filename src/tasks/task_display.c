#include "task_display.h"

#include "stdio.h"
#include "stdint.h"
#include "limits.h"
#include "pico/stdlib.h"

#include "../drivers/lcd1602_i2c.h"
#include "config.h"
#include "display_mode.h"
#include "sensor_data.h"


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


// =====================================================================================================================
// LCD Display Functions
// =====================================================================================================================
/*! \brief Initialize the LCD display.
 *
 * This function initializes the I2C interface and the LCD1602 display.
 * It sets up the necessary configurations for the LCD to function properly.
 */
static void lcd_init(void) {
    i2c_init(I2C_PORT, 100 * 1000); 
    lcd1602_init();
}

/*! \brief Draw a luminosity icon on the LCD based on the luminosity percentage.
 *
 * This function draws a luminosity icon on the LCD using a series of blocks that represent
 * the luminosity level. The number of filled blocks corresponds to the luminosity percentage.
 *
 * \param luminosity The luminosity percentage (0-100) to determine how many blocks to fill.
 */
static void draw_luminosity_icon(uint8_t luminosity) {
    uint8_t block_range = 4; // Default to 4 blocks for luminosity up to 100%.
    uint8_t block_count = 0;

    // Determine the number of blocks to display based on luminosity percentage.
    for (uint8_t i = 0; i < block_range; i++) {
        if (luminosity > (i * 25)) { block_count++; }
    }

    lcd1602_write_char(LEFT_ROUND_BRACKET_CHAR);

    for (uint8_t i = 0; i < block_range; i++) {
        if (i < block_count) {
            lcd1602_write_char(PLAIN_BLOCK_CHAR);
        } 
        else {
            lcd1602_write_char(EMPTY_BLOCK_CHAR);
        }
    }

    lcd1602_write_char(RIGHT_ROUND_BRACKET_CHAR);
}

/*! \brief Display the full information mode on the LCD.
 *
 * This function displays temperature, humidity, and luminosity on the LCD.
 * It formats the sensor data into strings and writes them to the appropriate positions on the LCD.
 *
 * \param sensor_data The sensor data structure containing temperature, humidity, and luminosity values.
 * \param buffer A character buffer used for formatting strings to be displayed on the LCD.
 * \param buffer_size The size of the character buffer.
 */
static void lcd_full_info_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    // Temperature/Humidity Writing.
    snprintf(buffer, buffer_size, "T:%.1fC  H:%3d%%", sensor_data.temperature, sensor_data.humidity);
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string(buffer);

    // Luminosity Writing.
    snprintf(buffer, buffer_size, "Lum:%3d%%", sensor_data.luminosity);
    lcd1602_set_cursor_pos(1, 0);
    lcd1602_write_string(buffer);

    // Luminosity Icon Drawing.
    lcd1602_set_cursor_pos(1, 10);
    draw_luminosity_icon(sensor_data.luminosity);
}

/*! \brief Display the temperature mode on the LCD.
 *
 * This function displays the temperature on the LCD.
 * It formats the temperature value into a string and writes it to the appropriate position on the LCD.
 *
 * \param sensor_data The sensor data structure containing the temperature value.
 * \param buffer A character buffer used for formatting strings to be displayed on the LCD.
 * \param buffer_size The size of the character buffer.
 */
static void lcd_temperature_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string("Temperature:");

    lcd1602_set_cursor_pos(1, 6);
    snprintf(buffer, buffer_size, "%.1fC", sensor_data.temperature);
    lcd1602_write_string(buffer);
}

/*! \brief Display the humidity mode on the LCD.
 *
 * This function displays the humidity on the LCD.
 * It formats the humidity value into a string and writes it to the appropriate position on the LCD.
 *
 * \param sensor_data The sensor data structure containing the humidity value.
 * \param buffer A character buffer used for formatting strings to be displayed on the LCD.
 * \param buffer_size The size of the character buffer.
 */
static void lcd_humidity_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string("Humidity:");

    lcd1602_set_cursor_pos(1, 6);
    snprintf(buffer, buffer_size, "%3d%%", sensor_data.humidity);
    lcd1602_write_string(buffer);
}

/*! \brief Display the luminosity mode on the LCD.
 *
 * This function displays the luminosity on the LCD.
 * It formats the luminosity value into a string and writes it to the appropriate position on the LCD.
 * Additionally, it draws a luminosity icon based on the luminosity percentage.
 *
 * \param sensor_data The sensor data structure containing the luminosity value.
 * \param buffer A character buffer used for formatting strings to be displayed on the LCD.
 * \param buffer_size The size of the character buffer.
 */
static void lcd_luminosity_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    snprintf(buffer, buffer_size, "Luminosity:%3d%%", sensor_data.luminosity);
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string(buffer);

    lcd1602_set_cursor_pos(1, 5); // Position the cursor for the luminosity icon
    draw_luminosity_icon(sensor_data.luminosity);
}

/*! \brief Display a message indicating that the sensor data is outdated.
 *
 * This function displays a message on the LCD indicating that the sensor data is outdated.
 */
static void lcd_outdated_data_mode(void) {
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string("OUTDATED DATA");

    lcd1602_set_cursor_pos(1, 0);
    lcd1602_write_string("Check sensors");
}


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

    if (new_mode != last_display_mode) {
        last_display_mode = new_mode;
        lcd1602_clear(); // Clear the screen if the display mode has changed.
    }

    switch (new_mode) {
        case DISPLAY_MODE_FULL_INFO:
            lcd_full_info_mode(sensor_data, buffer, buffer_size);
            break;
        case DISPLAY_MODE_TEMPERATURE:
            lcd_temperature_mode(sensor_data, buffer, buffer_size);
            break;
        case DISPLAY_MODE_HUMIDITY:
            lcd_humidity_mode(sensor_data, buffer, buffer_size);
            break;
        case DISPLAY_MODE_LUMINOSITY:
            lcd_luminosity_mode(sensor_data, buffer, buffer_size);
            break;
        case DISPLAY_MODE_OUTDATED:
            lcd_outdated_data_mode();
            break;
        default:
            lcd_full_info_mode(sensor_data, buffer, buffer_size);
            break;
    }
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

        // Check if the notification is from the button press event and update the display mode accordingly.
        if (notified == pdTRUE && (notification_value & BUTTON_NOTIFY_BIT)) {
            current_display_mode = (display_mode_t)((current_display_mode + 1) % DISPLAY_MODE_COUNT);
        }

        /* Check the freshness of the latest sensor data after every wake-up,
        * whether caused by a notification or by the timeout.
        */
        data_retrieved_ok = (xQueuePeek(sensor_data_queue, &sensor_data, 0) == pdPASS) && 
                             sensor_data_is_fresh(&sensor_data);

        refresh_display(sensor_data, data_retrieved_ok, buffer, buffer_size);        
    }
}
