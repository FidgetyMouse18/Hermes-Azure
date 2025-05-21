#ifndef TVOC_H
#define TVOC_H

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

#define STACKSIZE 1024
#define PRIORITY 7

int tvoc_init(void);
void tvoc_get(float *data);

#endif