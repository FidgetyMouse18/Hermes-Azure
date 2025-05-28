/*
 * Copyright (c) 2024 disco_mqtt_2 project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_device, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>
#include <string.h>

#include "device.h"

/* HTS221 Temperature/Humidity sensor */
static const struct device *hts221_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(ambient_temp0));

/* GPIO LEDs using the built-in LED on disco l475 iot1 */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led_status = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Built-in LED2 for network status */
#define LED2_NODE DT_ALIAS(led2)
static const struct gpio_dt_spec led_net = GPIO_DT_SPEC_GET_OR(LED2_NODE, gpios, {0});

/* Command handler functions */
static void led_on_handler(void)
{
	device_write_led(LED_STATUS, LED_ON);
	LOG_INF("Status LED turned ON via command");
}

static void led_off_handler(void)
{
	device_write_led(LED_STATUS, LED_OFF);
	LOG_INF("Status LED turned OFF via command");
}

static void status_handler(void)
{
	LOG_INF("Device Status:");
	LOG_INF("  HTS221 sensor: %s", hts221_dev ? "Ready" : "Not available");
	LOG_INF("  Status LED: %s", gpio_is_ready_dt(&led_status) ? "Ready" : "Not ready");
	LOG_INF("  Network LED: %s", gpio_is_ready_dt(&led_net) ? "Ready" : "Not ready");
}

/* Supported device commands */
struct device_cmd device_commands[] = {
	{"led_on", led_on_handler},
	{"led_off", led_off_handler},
	{"status", status_handler}
};

const size_t num_device_commands = ARRAY_SIZE(device_commands);

void device_command_handler(uint8_t *command)
{
	if (command == NULL) {
		LOG_ERR("Null command received");
		return;
	}

	for (int i = 0; i < num_device_commands; i++) {
		if (strcmp((char *)command, device_commands[i].command) == 0) {
			LOG_INF("Executing device command: %s", device_commands[i].command);
			device_commands[i].handler();
			return;
		}
	}
	LOG_WRN("Unknown command: %s", command);
}

int device_read_sensor(struct sensor_sample *sample)
{
	static uint32_t counter = 1;

	if (sample == NULL) {
		return -EINVAL;
	}

	/* Initialize sample structure */
	memset(sample, 0, sizeof(*sample));
	sample->unit = "Count";
	sample->timestamp = k_uptime_get();
	
	/* Simple counting numbers instead of sensor data */
	sample->temperature = counter;
	sample->humidity = counter * 2;
	
	LOG_INF("Sending count values: temperature=%d, humidity=%d", 
		counter, counter * 2);
	
	counter++;
	if (counter > 1000) {
		counter = 1; /* Reset after 1000 */
	}

	return 0;
}

int device_write_led(enum led_id led_idx, enum led_state state)
{
	int rc = 0;
	const struct gpio_dt_spec *led_spec = NULL;

	/* Select appropriate LED */
	switch (led_idx) {
	case LED_STATUS:
		led_spec = &led_status;
		break;
	case LED_NET:
		led_spec = &led_net;
		break;
	case LED_ERROR:
		/* Use status LED for error indication with blinking pattern */
		led_spec = &led_status;
		break;
	default:
		LOG_ERR("Invalid LED index: %d", led_idx);
		return -EINVAL;
	}

	/* Check if LED GPIO is ready */
	if (!gpio_is_ready_dt(led_spec)) {
		LOG_DBG("LED %d GPIO not ready, simulating", led_idx);
		if (state == LED_ON) {
			LOG_INF("LED %d: ON (simulated)", led_idx);
		} else {
			LOG_INF("LED %d: OFF (simulated)", led_idx);
		}
		return 0;
	}

	/* Set LED state */
	switch (state) {
	case LED_OFF:
		rc = gpio_pin_set_dt(led_spec, 0);
		if (rc == 0) {
			LOG_DBG("LED %d turned OFF", led_idx);
		}
		break;
	case LED_ON:
		rc = gpio_pin_set_dt(led_spec, 1);
		if (rc == 0) {
			LOG_DBG("LED %d turned ON", led_idx);
		}
		break;
	default:
		LOG_ERR("Invalid LED state: %d", state);
		rc = -EINVAL;
		break;
	}

	if (rc != 0) {
		LOG_ERR("Failed to set LED %d state to %d [%d]", led_idx, state, rc);
	}

	return rc;
}

bool devices_ready(void)
{
	bool all_ready = true;

	/* Check HTS221 sensor readiness */
	if (hts221_dev != NULL) {
		if (!device_is_ready(hts221_dev)) {
			LOG_WRN("HTS221 sensor is not ready");
			all_ready = false;
		} else {
			LOG_INF("HTS221 sensor is ready");
		}
	} else {
		LOG_WRN("HTS221 sensor device not found in device tree");
	}

	/* Check status LED GPIO readiness */
	if (!gpio_is_ready_dt(&led_status)) {
		LOG_WRN("Status LED GPIO is not ready");
		all_ready = false;
	} else {
		LOG_INF("Status LED is ready");
	}

	/* Check network LED GPIO readiness (optional) */
	if (gpio_is_ready_dt(&led_net)) {
		LOG_INF("Network LED is ready");
	} else {
		LOG_DBG("Network LED not available (using status LED for network indication)");
	}

	return all_ready;
}

int devices_init(void)
{
	int rc = 0;

	LOG_INF("Initializing devices...");

	/* Configure status LED GPIO */
	if (gpio_is_ready_dt(&led_status)) {
		rc = gpio_pin_configure_dt(&led_status, GPIO_OUTPUT_INACTIVE);
		if (rc != 0) {
			LOG_ERR("Failed to configure status LED GPIO [%d]", rc);
			return rc;
		}
		LOG_INF("Status LED GPIO configured");
	}

	/* Configure network LED GPIO (if available) */
	if (gpio_is_ready_dt(&led_net)) {
		rc = gpio_pin_configure_dt(&led_net, GPIO_OUTPUT_INACTIVE);
		if (rc != 0) {
			LOG_WRN("Failed to configure network LED GPIO [%d]", rc);
			/* Don't fail initialization for optional LED */
		} else {
			LOG_INF("Network LED GPIO configured");
		}
	}

	/* Initialize device status with LEDs off */
	device_write_led(LED_STATUS, LED_OFF);
	device_write_led(LED_NET, LED_OFF);

	LOG_INF("Device initialization complete");
	return 0;
} 