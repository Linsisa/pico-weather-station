#include "lcd_ui.h"

#include "config.h"
#include "src/drivers/lcd1602_i2c.h"

#include <stdio.h>


/*! \brief Initialize the LCD display.
 *
 * This function initializes the I2C interface and the LCD1602 display.
 * It sets up the necessary configurations for the LCD to function properly.
 */
void lcd_init(void) {
    i2c_init(I2C_PORT, I2C_BAUDRATE_HZ); 
    lcd1602_init();
}

/*! \brief Clear the LCD display.
 *
 * This function clears the entire LCD display.
 */
void lcd_clear_ui(void) {
    lcd1602_clear();
}

/*! \brief Draw a luminosity icon on the LCD based on the luminosity percentage.
 *
 * This function draws a luminosity icon on the LCD using a series of blocks that represent
 * the luminosity level. The number of filled blocks corresponds to the luminosity percentage.
 *
 * \param luminosity The luminosity percentage (0-100) to determine how many blocks to fill.
 */
void draw_luminosity_icon(uint8_t luminosity) {
    uint8_t block_range = LUMINOSITY_BAR_BLOCKS; // Default to 4 blocks for luminosity up to 100%.
    uint8_t block_count = 0;

    // Determine the number of blocks to display based on luminosity percentage.
    for (uint8_t i = 0; i < block_range; i++) {
        if (luminosity > (i * LUMINOSITY_BAR_STEP_PERCENT)) { block_count++; }
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
void lcd_full_info_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    // Temperature/Humidity Writing.
    snprintf(buffer, buffer_size, "T:%.1fC  H:%3d%%", sensor_data.temperature, sensor_data.humidity);
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string(buffer);

    // Luminosity Writing.
    snprintf(buffer, buffer_size, "Lum:%3d%%", sensor_data.luminosity);
    lcd1602_set_cursor_pos(0, 1);
    lcd1602_write_string(buffer);

    // Luminosity Icon Drawing.
    lcd1602_set_cursor_pos(10, 1);
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
void lcd_temperature_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string("Temperature:");

    lcd1602_set_cursor_pos(6, 1);
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
void lcd_humidity_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string("Humidity:");

    lcd1602_set_cursor_pos(6, 1);
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
void lcd_luminosity_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size) {
    snprintf(buffer, buffer_size, "Luminosity:%3d%%", sensor_data.luminosity);
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string(buffer);

    lcd1602_set_cursor_pos(5, 1); // Position the cursor for the luminosity icon
    draw_luminosity_icon(sensor_data.luminosity);
}

/*! \brief Display a message indicating that the sensor data is outdated.
 *
 * This function displays a message on the LCD indicating that the sensor data is outdated.
 */
void lcd_outdated_data_mode(void) {
    lcd1602_set_cursor_pos(0, 0);
    lcd1602_write_string("OUTDATED DATA");

    lcd1602_set_cursor_pos(0, 1);
    lcd1602_write_string("Check sensors");
}
