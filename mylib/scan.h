#ifndef SCAN_H
#define SCAN_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include <math.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>
#include "node_list.h"

#define STACKSIZE 4096
#define PRIORITY 7
#define SCAN_TIME 100
struct __packed  ble_adv
{
    uint8_t ble_prefix[5];
    uint8_t prefix[4];
    uint8_t uuid[4];
    uint16_t timestamp;
    uint8_t pressure;
    uint8_t humidity;
    uint8_t temperature;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint16_t tvoc;
    int8_t accel_x;
    int8_t accel_y;
    int8_t accel_z;
};

#endif