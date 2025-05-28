/*
 * Copyright (c) 2024 disco_mqtt_2 project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

/* WiFi connection status */
enum app_wifi_status {
	APP_WIFI_DISCONNECTED = 0,
	APP_WIFI_CONNECTING,
	APP_WIFI_CONNECTED,
	APP_WIFI_ERROR
};

/* Function prototypes */

/**
 * @brief Initialize WiFi manager
 * @return 0 on success, negative error code on failure
 */
int wifi_manager_init(void);

/**
 * @brief Connect to WiFi network
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return 0 on success, negative error code on failure
 */
int wifi_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from WiFi network
 * @return 0 on success, negative error code on failure
 */
int wifi_disconnect(void);

/**
 * @brief Get current WiFi status
 * @return Current WiFi status
 */
enum app_wifi_status wifi_get_status(void);

/**
 * @brief Scan for available WiFi networks
 * @return 0 on success, negative error code on failure
 */
int wifi_scan(void);

/**
 * @brief Wait for WiFi connection with timeout
 * @param timeout_ms Timeout in milliseconds
 * @return 0 if connected, negative error code on timeout/failure
 */
int wifi_wait_for_connection(int timeout_ms);

/**
 * @brief Get the IP address assigned to the WiFi interface
 * @param addr_str Buffer to store the IP address string
 * @param buf_size Size of the buffer
 * @return 0 on success, negative error code on failure
 */
int wifi_get_ip_address(char *addr_str, size_t buf_size);

#endif /* __WIFI_MANAGER_H__ */ 