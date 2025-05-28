// -----------------------------------------------------------------------------
// vibration.c  —  Vibration motor control for M5Core2 via AXP192 LDO3
// Walter Nedov • May 2025
// -----------------------------------------------------------------------------

#include "vibration.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>

#define VIBRATION_MOTOR_NODE DT_NODELABEL(vib_motor)
const struct device *vibration_regulator = DEVICE_DT_GET(VIBRATION_MOTOR_NODE);

static const struct sensor_thresholds thresholds = {
    .temperature_max = 30,
    .humidity_max = 70,
    .pressure_max = 200,
    .tvoc_max = 400,
    .motion_magnitude_max = 10,
    .light_rgb_max = 200
};

static bool vibration_initialized = false;

int vibration_init(void)
{
    if (!device_is_ready(vibration_regulator)) {
        return -1;
    }
    regulator_disable(vibration_regulator);
    vibration_initialized = true;
    return 0;
}

bool sensor_data_exceeds_thresholds(uint8_t temperature, uint8_t humidity, uint8_t pressure,
                                   uint16_t tvoc, int8_t accel_x, int8_t accel_y, int8_t accel_z,
                                   uint8_t r, uint8_t g, uint8_t b)
{
    if (temperature > thresholds.temperature_max) return true;
    if (humidity > thresholds.humidity_max) return true;
    if (pressure > thresholds.pressure_max) return true;
    if (tvoc > thresholds.tvoc_max) return true;
    
    int motion_squared = (accel_x * accel_x) + (accel_y * accel_y) + (accel_z * accel_z);
    int threshold_squared = thresholds.motion_magnitude_max * thresholds.motion_magnitude_max;
    if (motion_squared > threshold_squared) return true;
    
    if (r > thresholds.light_rgb_max || g > thresholds.light_rgb_max || b > thresholds.light_rgb_max) return true;
    
    return false;
}
