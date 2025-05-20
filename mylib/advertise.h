#ifndef ADVERTISE_H
#define ADVERTISE_H

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/printk.h>
#include <zephyr/settings/settings.h>
#include <stdio.h>

#define BLE_NAME_LEN 32
#define MAC_ADDR_LEN 6

#define STACKSIZE 4096
#define PRIORITY 7

#define ADVERTISE_MS 2500

struct __aligned(4) ble_adv
{
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

int ble_init(void);

void queue_data(uint8_t pressure, uint8_t humidity, uint8_t temeprature, uint8_t r, uint8_t g, uint8_t b, uint16_t tvoc, int8_t accel_x, int8_t accel_y, int8_t accel_z);

#endif