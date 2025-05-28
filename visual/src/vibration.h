#ifndef VIBRATION_H_
#define VIBRATION_H_

#include <stdint.h>
#include <stdbool.h>

struct sensor_thresholds {
    uint8_t temperature_max;
    uint8_t humidity_max;
    uint8_t pressure_max;
    uint16_t tvoc_max;
    uint8_t motion_magnitude_max;
    uint8_t light_rgb_max;
};

int vibration_init(void);
bool sensor_data_exceeds_thresholds(uint8_t temperature, uint8_t humidity, uint8_t pressure,
                                   uint16_t tvoc, int8_t accel_x, int8_t accel_y, int8_t accel_z,
                                   uint8_t r, uint8_t g, uint8_t b);


#endif 