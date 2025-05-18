#ifndef HUMID_H
#define HUMID_H

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

#define STACKSIZE 1024
#define PRIORITY 7

int humid_init(void);
void humid_get(float *data);

#endif