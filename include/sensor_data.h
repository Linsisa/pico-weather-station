#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include "config.h"
#include "pico/stdlib.h"

#include <stdint.h>


typedef struct {
    float temperature;
    uint8_t humidity;
    uint8_t luminosity;
    uint64_t timestamp_ms; // Timestamp in milliseconds since boot
} sensor_data_t;


/** \brief Check if the sensor data is fresh.
 *
 * This function checks if the sensor data is fresh (verifying that the measurements is lower or equal to 3 consecutive sensor readings).
 *
 * \param data The sensor data to check.
 * \return true if the sensor data is fresh, false otherwise.
 */
static inline bool sensor_data_is_fresh(sensor_data_t *data) {
    uint64_t sensor_data_age_ms = to_ms_since_boot(get_absolute_time()) - data->timestamp_ms;
    
    return sensor_data_age_ms <= SENSOR_DATA_MAX_AGE_MS;
}

#endif // SENSOR_DATA_H
