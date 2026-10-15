#ifndef LCD1602_I2C_H
#define LCD1602_I2C_H

#include <stdint.h>


void lcd1602_init(void);
void lcd1602_clear(void);
void lcd1602_set_cursor_pos(uint8_t column, uint8_t row);
void lcd1602_write_char(char c);
void lcd1602_write_string(const char *str);

#endif // LCD1602_I2C_H
