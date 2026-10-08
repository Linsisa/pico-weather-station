#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdint.h>

typedef struct {
    float temperature;
    uint8_t humidity;
    uint8_t luminosity;
} sensor_data_t;

#endif // SENSOR_DATA_H
