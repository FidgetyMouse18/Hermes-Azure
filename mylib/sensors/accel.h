#ifndef ACCEL_H
#define ACCEL_H

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

#define STACKSIZE 1024
#define PRIORITY 7

struct accel_data {
    float x;
    float y;
    float z;
};

int accel_init(void);
void accel_get(struct accel_data *data);

#endif