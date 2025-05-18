#ifndef LIGHT_H
#define LIGHT_H

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

#define STACKSIZE 1024
#define PRIORITY 7

#define BH1745_I2C_ADDR 0x38
#define BH1745_REG_RED 0x50
#define BH1745_REG_GREEN 0x52
#define BH1745_REG_BLUE 0x54
#define BH1745_REG_WHITE 0x56

struct light_data {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t w;
};

int light_init(void);
void light_get(struct light_data *data);

#endif