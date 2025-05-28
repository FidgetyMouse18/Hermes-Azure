/*
 * Copyright (c) 2024 disco_mqtt_2 project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_main, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/dhcpv4.h>

#include "device.h"
#include "wifi_manager.h"
#include "mqtt_client.h"

/* MQTT client instance */
static struct mqtt_client client_ctx;

/* Work queue for periodic MQTT publishing */
// static struct k_work_delayable mqtt_publish_work;

/* Network connection semaphore */
K_SEM_DEFINE(net_conn_sem, 0, 1);

/* Network management callback */
static struct net_mgmt_event_callback net_l4_mgmt_cb;

/* Application state */
static bool app_running = true;
static bool network_connected = false;

static void net_l4_evt_handler(struct net_mgmt_event_callback *cb,
			       uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network L4 connected!");
		network_connected = true;
		device_write_led(LED_STATUS, LED_ON);
		k_sem_give(&net_conn_sem);
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network L4 disconnected!");
		network_connected = false;
		device_write_led(LED_STATUS, LED_OFF);
		device_write_led(LED_NET, LED_OFF);
		break;
	default:
		break;
	}
}

static void log_network_info(struct net_if *iface)
{
	char addr_str[NET_IPV4_ADDR_LEN];
	struct net_linkaddr *mac;

	/* Log MAC address */
	mac = net_if_get_link_addr(iface);
	if (mac) {
		LOG_INF("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
			mac->addr[0], mac->addr[1], mac->addr[2],
			mac->addr[3], mac->addr[4], mac->addr[5]);
	}

	/* Log IP address if available */
	if (wifi_get_ip_address(addr_str, sizeof(addr_str)) == 0) {
		LOG_INF("IP Address: %s", addr_str);
	}
}

static int initialize_network(void)
{
	struct net_if *iface;
	int rc;

	/* Get default network interface */
	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("No default network interface found");
		return -ENETDOWN;
	}

	log_network_info(iface);

	/* Register L4 connectivity events */
	net_mgmt_init_event_callback(&net_l4_mgmt_cb, net_l4_evt_handler,
				     NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED);
	net_mgmt_add_event_callback(&net_l4_mgmt_cb);

	/* Initialize WiFi manager */
	rc = wifi_manager_init();
	if (rc != 0) {
		LOG_ERR("WiFi manager initialization failed [%d]", rc);
		return rc;
	}

	LOG_INF("Network initialization complete");
	return 0;
}

static int connect_to_network(void)
{
	int rc;

	LOG_INF("Starting network connection...");

	/* Optional: Scan for networks first */
	rc = wifi_scan();
	if (rc != 0) {
		LOG_WRN("WiFi scan failed [%d], continuing anyway", rc);
	}

	/* Connect to WiFi */
	rc = wifi_connect(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
	if (rc != 0) {
		LOG_ERR("WiFi connection failed [%d]", rc);
		return rc;
	}

	/* Wait for L4 connectivity (DHCP) */
	LOG_INF("Waiting for IP address assignment...");
	if (k_sem_take(&net_conn_sem, K_SECONDS(30)) != 0) {
		LOG_ERR("Network connectivity timeout");
		return -ETIMEDOUT;
	}

	/* Allow network stack to stabilize */
	k_sleep(K_SECONDS(2));

	LOG_INF("Network connection established successfully");
	log_network_info(net_if_get_default());

	return 0;
}

static int mqtt_connection_loop(void)
{
	int rc;

	while (app_running && network_connected) {
		/* Initialize MQTT client */
		rc = app_mqtt_init(&client_ctx);
		if (rc != 0) {
			LOG_ERR("MQTT initialization failed [%d]", rc);
			k_sleep(K_SECONDS(5));
			continue;
		}

		/* Connect to MQTT broker */
		LOG_INF("Connecting to MQTT broker...");
		app_mqtt_connect(&client_ctx);

		if (mqtt_connected) {
			/* Publishing is now handled in app_mqtt_run() loop */
			// k_work_reschedule(&mqtt_publish_work, K_NO_WAIT);

			/* Run MQTT processing loop */
			app_mqtt_run(&client_ctx);

			/* No work-queue to cancel after disconnect */
			// k_work_cancel_delayable(&mqtt_publish_work);
		}

		/* Wait before attempting reconnection */
		if (app_running && network_connected) {
			LOG_INF("Attempting MQTT reconnection in 15 seconds...");
			k_sleep(K_SECONDS(15));
		}
	}

	return 0;
}

int main(void)
{
	int rc;

	LOG_INF("Starting Disco MQTT 2.0 Application");
	LOG_INF("Board: disco_l475_iot1");
	LOG_INF("Build timestamp: %s %s", __DATE__, __TIME__);

	/* Initialize devices */
	rc = devices_init();
	if (rc != 0) {
		LOG_ERR("Device initialization failed [%d]", rc);
		device_write_led(LED_ERROR, LED_ON);
		return rc;
	}

	/* Check device readiness */
	if (!devices_ready()) {
		LOG_WRN("Some devices are not ready, continuing anyway");
	}

	/* Initialize network */
	rc = initialize_network();
	if (rc != 0) {
		LOG_ERR("Network initialization failed [%d]", rc);
		device_write_led(LED_ERROR, LED_ON);
		return rc;
	}

	/* Connect to network */
	rc = connect_to_network();
	if (rc != 0) {
		LOG_ERR("Network connection failed [%d]", rc);
		device_write_led(LED_ERROR, LED_ON);
		return rc;
	}

	/* Main application loop - MQTT connection and processing */
	LOG_INF("Starting MQTT connection loop...");
	rc = mqtt_connection_loop();

	/* Cleanup */
	LOG_INF("Application shutting down");
	app_running = false;
	// k_work_cancel_delayable(&mqtt_publish_work);
	device_write_led(LED_STATUS, LED_OFF);
	device_write_led(LED_NET, LED_OFF);
	device_write_led(LED_ERROR, LED_OFF);

	return rc;
} 