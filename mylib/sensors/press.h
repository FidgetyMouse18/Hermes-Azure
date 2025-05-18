#ifndef PRESS_H
#define PRESS_H

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

#define STACKSIZE 1024
#define PRIORITY 7

int press_init(void);
void press_get(float *data);

#endif