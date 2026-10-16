#ifndef CONFIG_H
#define CONFIG_H

#include "hardware/i2c.h"

// ON-BOARD LED
#define LED_ON 1
#define LED_OFF 0
#define CONNECTION_SUCCESS_BLINK_COUNT 3
#define CONNECTION_LED_ON_TIME_MS 800
#define CONNECTION_LED_OFF_TIME_MS 200
#define CONNECTION_FAILURE_LED_TIME_MS 1500
#define DATA_PUBLISHED_LED_TIME_MS 1500

// LDR ADC
#define LDR_ADC_CHANNEL 0
#define LDR_ADC_PIN 26
#define LDR_PERCENTAGE_MAX 100
#define LDR_ADC_MAX_VALUE 4095

// I2C
#define I2C_PORT i2c0
#define I2C_BAUDRATE_HZ 100000

// LCD1602
#define LCD_ADDR 0x27
#define LCD_SDA_PIN 4
#define LCD_SCL_PIN 5
#define LCD_COLS 16
#define LCD_ROWS 2
#define LCD_ROW_BUFFER_SIZE LCD_COLS
#define LUMINOSITY_BAR_BLOCKS 4
#define LUMINOSITY_BAR_STEP_PERCENT 25
#define LEFT_ROUND_BRACKET_CHAR 0x5B
#define RIGHT_ROUND_BRACKET_CHAR 0x5D
#define PLAIN_BLOCK_CHAR 0xFF
#define EMPTY_BLOCK_CHAR 0x20
#define DISPLAY_WAIT_TIMEOUT_MS 6000

// PCF8574 -> LCD
#define E         0x04   // P2
#define BACKLIGHT 0x08   // P3
#define PCF8574_WRITE_LENGTH 1 // byte

// DHT11
#define DHT11_PIN     15
#define DHT_SM        0
#define DHT_FREQ_HZ   1000000.0f
#define DHT_NUM_BYTES 5
#define DHT_DECIMAL_FACTOR 10.0f
#define DHT_TIMEOUT_US 10000 // 10 milliseconds timeout for reading DHT11 response
#define DHT_INITIAL_WAIT_MS 1500 // Wait time to allow the DHT11 sensor to stabilize before starting readings (cf. datasheet recommendation).
#define SENSOR_DATA_MAX_AGE_MS 12000 // Maximum age of sensor data in milliseconds (3 consecutive readings at 4 seconds each).

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
#define MQTT_ACK_TIMEOUT_MS 2000 // 2 seconds timeout for waiting for MQTT broker publish ACK response
#define MQTT_CONNECTION_STATUS_TIMEOUT_MS 15000
#define MQTT_CONNECTION_POLL_INTERVAL_MS 100
#define MQTT_PAYLOAD_BUFFER_SIZE 128


#endif // CONFIG_H
