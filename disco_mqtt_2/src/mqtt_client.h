/*
 * Copyright (c) 2024 disco_mqtt_2 project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>

/* Timeout values in milliseconds */
#define MSECS_NET_POLL_TIMEOUT      5000
#define MSECS_WAIT_RECONNECT        5000

/* External MQTT connectivity status flag */
extern bool mqtt_connected;

/* Function prototypes */

/**
 * @brief Initialize MQTT client
 * @param client Pointer to MQTT client structure
 * @return 0 on success, negative error code on failure
 */
int app_mqtt_init(struct mqtt_client *client);

/**
 * @brief Connect to MQTT broker (blocking)
 * @param client Pointer to MQTT client structure
 */
void app_mqtt_connect(struct mqtt_client *client);

/**
 * @brief Subscribe to MQTT topics
 * @param client Pointer to MQTT client structure
 * @return 0 on success, negative error code on failure
 */
int app_mqtt_subscribe(struct mqtt_client *client);

/**
 * @brief Publish sensor data to MQTT broker
 * @param client Pointer to MQTT client structure
 * @return 0 on success, negative error code on failure
 */
int app_mqtt_publish(struct mqtt_client *client);

/**
 * @brief Process incoming MQTT messages and keep connection alive
 * @param client Pointer to MQTT client structure
 * @return 0 on success, negative error code on failure
 */
int app_mqtt_process(struct mqtt_client *client);

/**
 * @brief Main MQTT run loop with subscription and message processing
 * @param client Pointer to MQTT client structure
 */
void app_mqtt_run(struct mqtt_client *client);

#endif /* __MQTT_CLIENT_H__ */ 