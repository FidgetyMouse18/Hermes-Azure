/*
 * Copyright (c) 2024 disco_mqtt_2 project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_manager, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_event.h>
#include <string.h>

#include "wifi_manager.h"
#include "device.h"

/* WiFi management state */
static enum app_wifi_status wifi_status = APP_WIFI_DISCONNECTED;
static struct net_if *wifi_iface = NULL;

/* Synchronization primitives */
static K_SEM_DEFINE(wifi_scan_sem, 0, 1);
static K_SEM_DEFINE(wifi_connect_sem, 0, 1);

/* Network management callback */
static struct net_mgmt_event_callback net_mgmt_cb;

/* Current WiFi connection parameters */
static char current_ssid[WIFI_SSID_MAX_LEN];
static char current_password[WIFI_PSK_MAX_LEN];

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	const struct wifi_scan_result *scan_result;
	const struct wifi_status *status;

	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		scan_result = (const struct wifi_scan_result *)cb->info;
		if (scan_result && scan_result->ssid_length > 0) {
			LOG_DBG("Found SSID: %.*s, RSSI: %d dBm, Security: %d, Channel: %d",
				scan_result->ssid_length, scan_result->ssid,
				scan_result->rssi, scan_result->security, scan_result->channel);
		}
		break;

	case NET_EVENT_WIFI_SCAN_DONE:
		LOG_INF("WiFi scan completed");
		k_sem_give(&wifi_scan_sem);
		break;

	case NET_EVENT_WIFI_CONNECT_RESULT:
		status = (const struct wifi_status *)cb->info;
		if (status && status->status == 0) {
			LOG_INF("WiFi connected successfully");
			wifi_status = APP_WIFI_CONNECTED;
			device_write_led(LED_NET, LED_ON);
		} else {
			LOG_ERR("WiFi connection failed (status: %d)", 
				status ? status->status : -1);
			wifi_status = APP_WIFI_ERROR;
			device_write_led(LED_NET, LED_OFF);
		}
		k_sem_give(&wifi_connect_sem);
		break;

	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("WiFi disconnected");
		wifi_status = APP_WIFI_DISCONNECTED;
		device_write_led(LED_NET, LED_OFF);
		break;

	default:
		break;
	}
}

int wifi_manager_init(void)
{
	wifi_iface = net_if_get_default();
	if (!wifi_iface) {
		LOG_ERR("No default network interface found");
		return -ENODEV;
	}

	/* Initialize network management callback */
	net_mgmt_init_event_callback(&net_mgmt_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_SCAN_RESULT |
				     NET_EVENT_WIFI_SCAN_DONE |
				     NET_EVENT_WIFI_CONNECT_RESULT |
				     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&net_mgmt_cb);

	wifi_status = APP_WIFI_DISCONNECTED;

	LOG_INF("WiFi manager initialized");
	return 0;
}

int wifi_scan(void)
{
	int rc;

	if (!wifi_iface) {
		LOG_ERR("WiFi interface not initialized");
		return -ENODEV;
	}

	LOG_INF("Starting WiFi scan...");
	rc = net_mgmt(NET_REQUEST_WIFI_SCAN, wifi_iface, NULL, 0);
	if (rc) {
		LOG_ERR("WiFi scan request failed [%d]", rc);
		return rc;
	}

	/* Wait for scan completion with timeout */
	if (k_sem_take(&wifi_scan_sem, K_SECONDS(30)) != 0) {
		LOG_ERR("WiFi scan timeout");
		return -ETIMEDOUT;
	}

	LOG_INF("WiFi scan completed successfully");
	return 0;
}

int wifi_connect(const char *ssid, const char *password)
{
	int rc;
	struct wifi_connect_req_params params;

	if (!wifi_iface) {
		LOG_ERR("WiFi interface not initialized");
		return -ENODEV;
	}

	if (!ssid || strlen(ssid) == 0) {
		LOG_ERR("Invalid SSID");
		return -EINVAL;
	}

	if (!password || strlen(password) == 0) {
		LOG_ERR("Invalid password");
		return -EINVAL;
	}

	/* Store current connection parameters */
	strncpy(current_ssid, ssid, sizeof(current_ssid) - 1);
	current_ssid[sizeof(current_ssid) - 1] = '\0';
	strncpy(current_password, password, sizeof(current_password) - 1);
	current_password[sizeof(current_password) - 1] = '\0';

	/* Set up connection parameters */
	memset(&params, 0, sizeof(params));
	params.ssid = (uint8_t *)ssid;
	params.ssid_length = strlen(ssid);
	params.psk = (uint8_t *)password;
	params.psk_length = strlen(password);
	params.security = WIFI_SECURITY_TYPE_PSK;
	params.channel = WIFI_CHANNEL_ANY;
	params.timeout = 30000; /* 30 second timeout */

	wifi_status = APP_WIFI_CONNECTING;
	device_write_led(LED_NET, LED_OFF); /* Turn off LED during connection */

	LOG_INF("Connecting to WiFi SSID: %s", ssid);
	rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_iface, &params, sizeof(params));
	if (rc) {
		LOG_ERR("WiFi connect request failed [%d]", rc);
		wifi_status = APP_WIFI_ERROR;
		return rc;
	}

	/* Wait for connection result with timeout */
	if (k_sem_take(&wifi_connect_sem, K_SECONDS(35)) != 0) {
		LOG_ERR("WiFi connection timeout");
		wifi_status = APP_WIFI_ERROR;
		return -ETIMEDOUT;
	}

	if (wifi_status == APP_WIFI_CONNECTED) {
		LOG_INF("Successfully connected to WiFi");
		return 0;
	} else {
		LOG_ERR("WiFi connection failed");
		return -ECONNREFUSED;
	}
}

int wifi_disconnect(void)
{
	int rc;

	if (!wifi_iface) {
		LOG_ERR("WiFi interface not initialized");
		return -ENODEV;
	}

	if (wifi_status == APP_WIFI_DISCONNECTED) {
		LOG_INF("WiFi already disconnected");
		return 0;
	}

	LOG_INF("Disconnecting from WiFi...");
	rc = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, wifi_iface, NULL, 0);
	if (rc) {
		LOG_ERR("WiFi disconnect request failed [%d]", rc);
		return rc;
	}

	wifi_status = APP_WIFI_DISCONNECTED;
	device_write_led(LED_NET, LED_OFF);

	LOG_INF("WiFi disconnected");
	return 0;
}

enum app_wifi_status wifi_get_status(void)
{
	return wifi_status;
}

int wifi_wait_for_connection(int timeout_ms)
{
	int64_t start_time = k_uptime_get();
	int64_t timeout_absolute = timeout_ms;

	while (wifi_status != APP_WIFI_CONNECTED) {
		if (k_uptime_get() - start_time >= timeout_absolute) {
			LOG_ERR("WiFi connection wait timeout");
			return -ETIMEDOUT;
		}

		if (wifi_status == APP_WIFI_ERROR) {
			LOG_ERR("WiFi connection error");
			return -ECONNREFUSED;
		}

		k_sleep(K_MSEC(100));
	}

	return 0;
}

int wifi_get_ip_address(char *addr_str, size_t buf_size)
{
	struct in_addr *addr;

	if (!wifi_iface) {
		LOG_ERR("WiFi interface not initialized");
		return -ENODEV;
	}

	if (!addr_str || buf_size == 0) {
		return -EINVAL;
	}

	if (wifi_status != APP_WIFI_CONNECTED) {
		LOG_WRN("WiFi not connected");
		return -ENOTCONN;
	}

	/* Get IPv4 address */
	addr = net_if_ipv4_get_global_addr(wifi_iface, NET_ADDR_PREFERRED);
	if (!addr) {
		LOG_WRN("No IPv4 address assigned");
		return -ENODATA;
	}

	/* Convert to string */
	if (!net_addr_ntop(AF_INET, addr, addr_str, buf_size)) {
		LOG_ERR("Failed to convert IP address to string");
		return -EINVAL;
	}

	return 0;
} 