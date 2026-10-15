#ifndef DHT11_PIO_H
#define DHT11_PIO_H

#include "config.h"

#include <stdint.h>
#include <stdbool.h>


int dht11_pio_program_init();
bool dht11_read_data(int program_offset, uint8_t bytes[DHT_NUM_BYTES]);
bool dht11_valid_checksum(const uint8_t bytes[DHT_NUM_BYTES]);

#endif // DHT11_PIO_H
