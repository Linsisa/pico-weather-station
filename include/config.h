#ifndef CONFIG_H
#define CONFIG_H

#include "hardware/i2c.h"

// LDR ADC
#define LDR_ADC_CHANNEL 0
#define LDR_ADC_PIN 26

// I2C
#define I2C_PORT i2c0

// LCD1602
#define LCD_ADDR 0x27
#define LCD_SDA_PIN 4
#define LCD_SCL_PIN 5
#define LEFT_ROUND_BRACKET_CHAR 0x5B
#define RIGHT_ROUND_BRACKET_CHAR 0x5D
#define PLAIN_BLOCK_CHAR 0xFF
#define EMPTY_BLOCK_CHAR 0x20

// PCF8574 -> LCD
#define RS        0x01   // P0
#define RW        0x02   // P1
#define E         0x04   // P2
#define BACKLIGHT 0x08   // P3
#define D4        0x10   // P4
#define D5        0x20   // P5
#define D6        0x40   // P6
#define D7        0x80   // P7

// DHT11
#define DHT11_PIN     15
#define DHT_SM        0
#define DHT_FREQ_HZ   1000000.0f
#define DHT_NUM_BYTES 5
#define DHT_TIMEOUT_US 10000 // 10 milliseconds timeout for reading DHT11 response
#define DHT_INITIAL_WAIT_MS 1500 // Wait time to allow the DHT11 sensor to stabilize before starting readings (cf. datasheet recommendation).

// BUTTON
#define BUTTON_PIN    14
#define BUTTON_COOLDOWN_US 500000 // 500000 // 0.5 seconds in us

// FREE_RTOS
#define COMM_TASK_PRIORITY 1
#define SENSOR_TASK_PRIORITY 2
#define DISPLAY_TASK_PRIORITY 3
#define COMM_TASK_STACK_SIZE 2048
#define SENSOR_TASK_STACK_SIZE 256
#define DISPLAY_TASK_STACK_SIZE 256
#define COMM_TASK_DELAY_MS 10000
#define SENSOR_TASK_DELAY_MS 4000
#define BUTTON_NOTIFY_BIT 0x01
#define SENSOR_NOTIFY_BIT 0x02

// WIFI
#define WIFI_CONNECT_TIMEOUT_MS 15000
// MQTT
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "pico"
#define MQTT_TOPIC "home/pico-weather-station/data"
#define MQTT_KEEP_ALIVE 60
#define MQTT_QOS 1
#define MQTT_RETAIN 0


#endif // CONFIG_H
