#ifndef LCD_UI_H
#define LCD_UI_H

#include "sensor_data.h"

#include <stdint.h>


void lcd_init(void);
void lcd_clear_ui(void);
void draw_luminosity_icon(uint8_t luminosity);
void lcd_full_info_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size);
void lcd_temperature_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size);
void lcd_humidity_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size);
void lcd_luminosity_mode(sensor_data_t sensor_data, char *buffer, uint8_t buffer_size);
void lcd_outdated_data_mode(void);

#endif // LCD_UI_H
