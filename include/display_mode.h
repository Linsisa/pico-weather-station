#ifndef DISPLAY_MODE_H
#define DISPLAY_MODE_H


typedef enum {
    DISPLAY_MODE_FULL_INFO,
    DISPLAY_MODE_TEMPERATURE,
    DISPLAY_MODE_HUMIDITY,
    DISPLAY_MODE_LUMINOSITY,
    DISPLAY_MODE_COUNT, // Used to keep track of the number of display modes

    DISPLAY_MODE_OUTDATED // Special mode to indicate that the sensor data is outdated, not counted in DISPLAY_MODE_COUNT
} display_mode_t;

#endif // DISPLAY_MODE_H
