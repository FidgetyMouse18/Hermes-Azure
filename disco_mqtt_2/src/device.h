/*
 * Copyright (c) 2024 disco_mqtt_2 project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/led.h>

/* LED identifiers */
enum led_id {
	LED_STATUS = 0,     /* General status LED */
	LED_NET = 1,        /* Network connectivity LED */
	LED_ERROR = 2       /* Error indication LED */
};

/* LED states */
enum led_state {
	LED_OFF = 0,
	LED_ON = 1
};

/* Sensor sample structure for JSON encoding */
struct sensor_sample {
	const char *unit;
	int32_t temperature;
	int32_t humidity;
	int64_t timestamp;
};

/* Device command structure */
struct device_cmd {
	const char *command;
	void (*handler)(void);
};

/* External array of supported commands */
extern struct device_cmd device_commands[];
extern const size_t num_device_commands;

/* Function prototypes */

/**
 * @brief Check if all devices are ready
 * @return true if all devices are ready, false otherwise
 */
bool devices_ready(void);

/**
 * @brief Read sensor data and populate sample structure
 * @param sample Pointer to sensor_sample structure to fill
 * @return 0 on success, negative error code on failure
 */
int device_read_sensor(struct sensor_sample *sample);

/**
 * @brief Control LED state
 * @param led_idx LED identifier
 * @param state LED state (on/off)
 * @return 0 on success, negative error code on failure
 */
int device_write_led(enum led_id led_idx, enum led_state state);

/**
 * @brief Handle device commands received via MQTT
 * @param command Command string to execute
 */
void device_command_handler(uint8_t *command);

/**
 * @brief Initialize all devices
 * @return 0 on success, negative error code on failure
 */
int devices_init(void);

#endif /* __DEVICE_H__ */ 