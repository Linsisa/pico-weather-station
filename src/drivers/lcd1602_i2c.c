#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "lcd1602_i2c.h"
#include "config.h"


/***
 * LCD 1602 I2C Driver
 * This file contains the implementation for the LCD 1602 display connected via I2C.
 ***/


/*!
 * \brief Send a single byte to the PCF8574 I/O expander over I2C.
 *
 *  \param value The byte to be written to the PCF8574/LCD1602.
 */
static void pcf_write(uint8_t value) {
    int ret;

    ret = i2c_write_blocking(
        I2C_PORT,
        LCD_ADDR,
        &value,
        1,
        false
    );

    if (ret < 0) { printf("Error sending data over I2C to PCF8574\n"); }
}

/*!
 * \brief Generate the Enable (E) pulse required to perform a write operation to the LCD.
 *
 *  \param value The byte to be written to the PCF8574/LCD1602.
 */
static void lcd_pulse_enable(uint8_t value) {
    /*
     * According to the datasheet:
     * - E must remain HIGH for at least 230 ns (E pulse width).
     * - Data must be stable for at least 80 ns before E goes LOW
     *   (data setup time).
     * - Data must remain stable for at least 10 ns after E goes LOW
     *   (data hold time).
     * - The minimum E cycle time is 500 ns.
     *
     * The 1 us delays used here are longer than these minimum timings,
     * ensuring that the LCD has enough time to sample the data correctly.
     */

    // Set E HIGH and keep the data stable.
    pcf_write(value | E);
    sleep_us(1);

    // Set E LOW to complete the write operation.
    pcf_write(value & ~E);
    sleep_us(1);
}


/*!
 * \brief Send a single byte to the LCD1602 in 8-bit mode.
 *
 *  \param byte The byte to be sent to the LCD.
 *  \param rs Register Select flag: 0 for command, 1 for data.
 */
static void lcd_send_byte_8bits_mode(uint8_t byte, uint8_t rs) {
    lcd_pulse_enable(byte | BACKLIGHT | rs);

    /*!
     * According to the datasheet, the LCD needs a certain amount of time to process each command.
     * The datasheet specifies a minimum execution time of 39us for most commands, 
     * so we wait for 50us to ensure the command has been processed before sending any further commands or data.
     */
    sleep_us(50);
}

/*!
 * \brief Send a single byte to the LCD1602 in 4-bit mode.
 *
 *  \param byte The byte to be sent to the LCD.
 *  \param rs Register Select flag: 0 for command, 1 for data.
 */
static void lcd_send_byte_4bits_mode(uint8_t byte, uint8_t rs) {
    // Function to send a byte to the LCD in 4-bit mode. The byte is sent in two nibbles (high and low).

    uint8_t high_nibble;
    uint8_t low_nibble;

    // The high nibble is the upper 4 bits of the byte, and the low nibble is the lower 4 bits shifted left by 4 positions.
    high_nibble = byte & 0xF0; // ex 0x41 -> 0x40
    low_nibble  = (byte << 4) & 0xF0; // ex 0x41 -> 0x10

    lcd_pulse_enable(high_nibble | BACKLIGHT | rs); // ex = 0x40 | 0x08 | 0x01 = 0x49
    lcd_pulse_enable(low_nibble | BACKLIGHT | rs); // ex = 0x10 | 0x08 | 0x01 = 0x19

    /*!
     * According to the datasheet, the LCD needs a certain amount of time to process each command.
     * The datasheet specifies a minimum execution time of 39us for most commands, 
     * so we wait for 50us to ensure the command has been processed before sending any further commands or data.
     */
    sleep_us(50);
}


/*!
 * \brief Initialize the LCD1602 display connected via I2C.
 *
 * This function sets up the GPIO pins for I2C communication, sends the necessary
 * initialization commands to the LCD, and configures it for 4-bit mode operation.
 */
void lcd1602_init(void) {
    gpio_set_function(LCD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD_SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(LCD_SDA_PIN);
    gpio_pull_up(LCD_SCL_PIN);

    // Initialisation instruction applied to the LCD from the datasheet.
    // Required delay after power-on before sending commands to the LCD.
    sleep_ms(100);

    // First set of commands to initialize the LCD in 8-bit mode.
    lcd_send_byte_8bits_mode(0x30, 0);
    sleep_ms(5);
    lcd_send_byte_8bits_mode(0x30, 0);
    sleep_us(150);
    lcd_send_byte_8bits_mode(0x30, 0);

    // Now switch screen to 4-bit mode with th next command.
    lcd_send_byte_8bits_mode(0x20, 0);

    // Function set: 4-bit mode, 2 lines, 5x8 dots.
    lcd_send_byte_4bits_mode(0x28, 0);

    // Display OFF.
    lcd_send_byte_4bits_mode(0x08, 0);

    // Display clear.
    lcd1602_clear();

    // Entry mode set: Increment cursor, no display shift.
    lcd_send_byte_4bits_mode(0x06, 0);

    // Display ON, Cursor OFF, Blink OFF
    lcd_send_byte_4bits_mode(0x0C, 0);
}

/*!
 * \brief Send a clear command to the LCD1602 display.
 */
void lcd1602_clear(void) {
    lcd_send_byte_4bits_mode(0x01, 0); // Send clear command to display.
    
    /*
     * Wait for the clear command to complete. 
     * According to the datasheet, this command takes around 1.53ms to execute.
     * So we wait for 3ms to ensure the command has completed before sending any further commands or data.
     */
    sleep_ms(3);
}

/*!
 * \brief Set the cursor position on the LCD1602 display.
 *  
 *  \param column The column number (0 to 15) where the cursor should be placed.
 *  \param row The row number (0 or 1) where the cursor should be placed.
 */
void lcd1602_set_cursor_pos(uint8_t column, uint8_t row) {
    /*
     * The function calculates the appropriate DDRAM address based on the row and column values, 
     * and then sends the command to set the cursor position.
     * Default adress to send is 10000000 for row 0 and 11000000 for row 1.
    */

    uint8_t base_address = 0x80; // Base address for row 0

    if (row == 1) { base_address = 0xC0; }

    base_address += column; // Add the column to the base address.
    lcd_send_byte_4bits_mode(base_address, 0);
}

/*!
 * \brief Write a single character to the LCD1602 display at the current cursor position.
 *
 *  \param c The character to be written to the display.
 */
void lcd1602_write_char(char c) {
    lcd_send_byte_4bits_mode((uint8_t)c, 1);
}

/*!
 * \brief Write a string to the LCD1602 display at the current cursor position.
 *
 *  \param str The string to be written to the display.
 */
void lcd1602_write_string(const char *str) {
    while (*str) {
        lcd1602_write_char(*str);
        str++;
    }
}
