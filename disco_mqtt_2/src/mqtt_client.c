/*
 * Copyright (c) 2024 disco_mqtt_2 project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_mqtt, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/data/json.h>
#include <zephyr/random/random.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>

#include "mqtt_client.h"
#include "device.h"

/* MQTT buffers */
static uint8_t rx_buffer[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];
static uint8_t tx_buffer[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];
static uint8_t payload_buf[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];

/* MQTT broker address storage */
static struct sockaddr_storage broker;

/* Socket polling */
static struct zsock_pollfd fds[1];
static int nfds;

/* MQTT client ID buffer */
static uint8_t client_id[64];

/* MQTT connectivity status */
bool mqtt_connected = false;

/* JSON descriptor for sensor data */
static const struct json_obj_descr sensor_sample_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct sensor_sample, unit, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct sensor_sample, temperature, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct sensor_sample, humidity, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct sensor_sample, timestamp, JSON_TOK_NUMBER),
};

#if defined(CONFIG_MQTT_LIB_TLS)
/* TLS configuration */
#define TLS_SNI_HOSTNAME    CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME
#define APP_CA_CERT_TAG     1

static const sec_tag_t m_sec_tags[] = {
	APP_CA_CERT_TAG,
};

static int tls_init(void)
{
	/* For now, we'll use non-secure connection for simplicity */
	/* TLS configuration can be added here when certificates are available */
	LOG_INF("TLS support compiled in but using non-secure connection");
	return 0;
}
#endif

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}
#if defined(CONFIG_MQTT_LIB_TLS)
	else if (client->transport.type == MQTT_TRANSPORT_SECURE) {
		fds[0].fd = client->transport.tls.sock;
	}
#endif

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static void init_mqtt_client_id(void)
{
	/* Generate unique client ID with board name and random suffix */
	snprintk((char *)client_id, sizeof(client_id), 
		 "disco_l475_%08x", sys_rand32_get());
}

static inline void on_mqtt_connect(void)
{
	mqtt_connected = true;
	device_write_led(LED_NET, LED_ON);
	LOG_INF("Connected to MQTT broker!");
	LOG_INF("Hostname: %s", CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME);
	LOG_INF("Client ID: %s", client_id);
	LOG_INF("Port: %s", CONFIG_NET_SAMPLE_MQTT_BROKER_PORT);
}

static inline void on_mqtt_disconnect(void)
{
	mqtt_connected = false;
	clear_fds();
	device_write_led(LED_NET, LED_OFF);
	LOG_INF("Disconnected from MQTT broker");
}

static void on_mqtt_publish(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	int rc;
	uint8_t payload[CONFIG_NET_SAMPLE_MQTT_PAYLOAD_SIZE];
	size_t payload_len;

	if (evt->param.publish.message.payload.len >= sizeof(payload)) {
		LOG_ERR("MQTT payload too large: %d bytes", 
			evt->param.publish.message.payload.len);
		return;
	}

	rc = mqtt_read_publish_payload(client, payload, sizeof(payload) - 1);
	if (rc < 0) {
		LOG_ERR("Failed to read received MQTT payload [%d]", rc);
		return;
	}

	payload_len = rc;
	payload[payload_len] = '\0'; /* Null-terminate */

	LOG_INF("MQTT payload received!");
	LOG_INF("Topic: '%.*s'", 
		evt->param.publish.message.topic.topic.size,
		evt->param.publish.message.topic.topic.utf8);
	LOG_INF("Payload: %s", payload);

	/* Check if this is a command topic */
	if (strncmp((char *)evt->param.publish.message.topic.topic.utf8,
		    CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD,
		    evt->param.publish.message.topic.topic.size) == 0) {
		device_command_handler(payload);
	}
}

static void mqtt_event_handler(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed [%d]", evt->result);
			break;
		}
		on_mqtt_connect();
		break;

	case MQTT_EVT_DISCONNECT:
		on_mqtt_disconnect();
		break;

	case MQTT_EVT_PINGRESP:
		LOG_DBG("PINGRESP packet received");
		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error [%d]", evt->result);
			break;
		}
		LOG_DBG("PUBACK packet ID: %u", evt->param.puback.message_id);
		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREC error [%d]", evt->result);
			break;
		}
		LOG_DBG("PUBREC packet ID: %u", evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};
		mqtt_publish_qos2_release(client, &rel_param);
		break;

	case MQTT_EVT_PUBREL:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREL error [%d]", evt->result);
			break;
		}
		LOG_DBG("PUBREL packet ID: %u", evt->param.pubrel.message_id);

		const struct mqtt_pubcomp_param comp_param = {
			.message_id = evt->param.pubrel.message_id
		};
		mqtt_publish_qos2_complete(client, &comp_param);
		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBCOMP error [%d]", evt->result);
			break;
		}
		LOG_DBG("PUBCOMP packet ID: %u", evt->param.pubcomp.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result == MQTT_SUBACK_FAILURE) {
			LOG_ERR("MQTT SUBACK failure [%d]", evt->result);
			break;
		}
		LOG_INF("SUBACK packet ID: %d", evt->param.suback.message_id);
		break;

	case MQTT_EVT_PUBLISH:
		/* Handle incoming publish messages */
		const struct mqtt_publish_param *p = &evt->param.publish;

		/* Send ACK for QoS 1 */
		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack_param = {
				.message_id = p->message_id
			};
			mqtt_publish_qos1_ack(client, &ack_param);
		}
		/* Send REC for QoS 2 */
		else if (p->message.topic.qos == MQTT_QOS_2_EXACTLY_ONCE) {
			const struct mqtt_pubrec_param rec_param = {
				.message_id = p->message_id
			};
			mqtt_publish_qos2_receive(client, &rec_param);
		}

		on_mqtt_publish(client, evt);
		break;

	default:
		LOG_DBG("Unhandled MQTT event type: %d", evt->type);
		break;
	}
}

static int poll_mqtt_socket(struct mqtt_client *client, int timeout)
{
	int rc;

	prepare_fds(client);

	if (nfds <= 0) {
		return -EINVAL;
	}

	rc = zsock_poll(fds, nfds, timeout);
	if (rc < 0) {
		LOG_ERR("Socket poll error (from poll_mqtt_socket, zsock_poll rc): [%d]", rc);
	}

	return rc;
}

static int get_mqtt_payload(struct mqtt_binstr *payload)
{
	int rc;
	struct sensor_sample sample;

	/* Clear payload buffer first */
	memset(payload_buf, 0, sizeof(payload_buf));

	/* Read sensor data */
	rc = device_read_sensor(&sample);
	if (rc != 0) {
		LOG_ERR("Failed to read sensor data [%d]", rc);
		return rc;
	}

	if (sample.timestamp <= 0) {
		sample.timestamp = k_uptime_get();
	}

	LOG_DBG("Sensor values: T=%d, H=%d, timestamp=%lld", 
		sample.temperature, sample.humidity, sample.timestamp);

	/* Encode sensor data as JSON */
	rc = json_obj_encode_buf(sensor_sample_descr, ARRAY_SIZE(sensor_sample_descr),
				 &sample, payload_buf, sizeof(payload_buf) - 1);
	if (rc != 0) {
		LOG_ERR("Failed to encode JSON object [%d]", rc);
		return rc;
	}

	/* Ensure null termination */
	payload_buf[sizeof(payload_buf) - 1] = '\0';
	
	payload->data = payload_buf;
	payload->len = strlen((char *)payload->data);

	/* Validate payload size */
	if (payload->len == 0 || payload->len >= sizeof(payload_buf)) {
		LOG_ERR("Invalid payload length: %d", payload->len);
		return -EINVAL;
	}

	LOG_INF("JSON payload (%d bytes): %s", payload->len, payload->data);
	return 0;
}

int app_mqtt_publish(struct mqtt_client *client)
{
	int rc;
	struct mqtt_publish_param param;
	struct mqtt_binstr payload;
	static uint16_t msg_id = 1;

	if (!mqtt_connected) {
		LOG_WRN("MQTT not connected, skipping publish");
		return -ENOTCONN;
	}

	/* Get sensor data as JSON payload */
	rc = get_mqtt_payload(&payload);
	if (rc != 0) {
		LOG_ERR("Failed to get MQTT payload [%d]", rc);
		return rc;
	}

	/* Set up MQTT topic */
	struct mqtt_topic topic = {
		.topic = {
			.utf8 = (uint8_t *)CONFIG_NET_SAMPLE_MQTT_PUB_TOPIC,
			.size = strlen(CONFIG_NET_SAMPLE_MQTT_PUB_TOPIC)
		},
		.qos = IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_0_AT_MOST_ONCE) ? 0 :
		       (IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_1_AT_LEAST_ONCE) ? 1 : 2)
	};

	/* Set up publish parameters */
	memset(&param, 0, sizeof(param));
	param.message.topic = topic;
	param.message.payload = payload;
	param.message_id = msg_id++;
	param.dup_flag = 0;
	param.retain_flag = 0;

	/* Publish the message */
	rc = mqtt_publish(client, &param);
	if (rc != 0) {
		LOG_ERR("MQTT publish failed [%d]", rc);
		if (rc == -ENOTCONN || rc == -95) { // -95 is EOPNOTSUPP
			LOG_ERR("Connection lost due to publish failure, marking as disconnected");
			mqtt_connected = false;
		}
		/* Don't return error - let the system continue its reconnect logic */
		LOG_WRN("Continuing despite publish failure (reconnect logic will take over if disconnected)");
		return 0; /* Return success-like to prevent hanging in specific call chains, rely on mqtt_connected */
	}

	LOG_INF("Published to topic '%s', QoS %d, payload: %.*s",
		CONFIG_NET_SAMPLE_MQTT_PUB_TOPIC,
		param.message.topic.qos,
		(int)payload.len, payload.data);

	return 0;
}

int app_mqtt_subscribe(struct mqtt_client *client)
{
	int rc;
	struct mqtt_topic sub_topics[] = {
		{
			.topic = {
				.utf8 = (uint8_t *)CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD,
				.size = strlen(CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD)
			},
			.qos = IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_0_AT_MOST_ONCE) ? 0 :
			       (IS_ENABLED(CONFIG_NET_SAMPLE_MQTT_QOS_1_AT_LEAST_ONCE) ? 1 : 2)
		}
	};

	const struct mqtt_subscription_list sub_list = {
		.list = sub_topics,
		.list_count = ARRAY_SIZE(sub_topics),
		.message_id = sys_rand16_get()
	};

	LOG_INF("Subscribing to %d topic(s)", sub_list.list_count);

	rc = mqtt_subscribe(client, &sub_list);
	if (rc != 0) {
		LOG_ERR("MQTT subscribe failed [%d]", rc);
	} else {
		LOG_INF("Subscribed to command topic: %s", CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD);
	}

	return rc;
}

int app_mqtt_process(struct mqtt_client *client)
{
	int rc;

	// poll_mqtt_socket now logs its own "Socket poll error (from poll_mqtt_socket, zsock_poll rc): [rc_val]"
	rc = poll_mqtt_socket(client, mqtt_keepalive_time_left(client)); 

	if (rc > 0) { // Data available
		if (fds[0].revents & ZSOCK_POLLIN) {
			rc = mqtt_input(client);
			if (rc != 0) {
				LOG_ERR("MQTT input failed [%d]", rc);
				return rc; // Propagate error from mqtt_input
			}
		}
		if (fds[0].revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
			LOG_ERR("MQTT socket HUP or ERR event on poll");
			return -ENOTCONN; // Indicate connection is effectively lost
		}
	} else if (rc == 0) { // Poll timeout, time for keepalive
		k_sleep(K_MSEC(50)); 
		rc = mqtt_live(client);
		if (rc != 0 && rc != -EAGAIN) {
			LOG_ERR("MQTT live (keepalive) failed [%d]", rc);
			return rc; // Propagate error from mqtt_live
		}
		// If rc is -EAGAIN, mqtt_live will be called again. If 0, success.
		// Ensure rc is 0 on success from mqtt_live if no error.
		if (rc == -EAGAIN) return 0; // Treat EAGAIN as "try again later", not an immediate process error
                                             // but let mqtt_keepalive_time_left recalculate.
                                             // Or, mqtt_live should return 0 if it sent PINGREQ and expects PINGRESP.
                                             // Let's assume mqtt_live returns 0 on success (PINGREQ sent or not needed yet)
                                             // and only errors for actual failures.
	} else { // rc < 0, poll_mqtt_socket already logged its specific error
		// <<< MODIFICATION for clarity >>>
		// This log now clearly states it's from app_mqtt_process and shows both rc and errno
		LOG_ERR("Poll failure in app_mqtt_process: zsock_poll_rc=%d, current_errno=%d", rc, errno);
		return rc; // Return the original error code from zsock_poll (via poll_mqtt_socket)
	}

	return 0; // Success
}

// <<< NEW CODE / MODIFICATION from previous suggestion >>>
void app_mqtt_run(struct mqtt_client *client)
{
	int rc;
	uint64_t last_publish_time; 
	bool initial_delay_done = false; // Flag to ensure delay happens only once per run session

	rc = app_mqtt_subscribe(client);
	if (rc != 0) {
		LOG_WRN("Failed to subscribe to topics [%d]", rc);
		// Potentially critical: if subscribe fails, app might not function as expected.
		// Consider if this should lead to an attempt to disconnect and reconnect.
		// For now, it continues.
	}

	LOG_INF("Entering MQTT run loop...");

	while (mqtt_connected) {
		if (!initial_delay_done) {
			LOG_DBG("Initial short delay in MQTT run loop before first operation...");
			k_sleep(K_MSEC(1000)); // Delay for 1 second (adjustable)
			initial_delay_done = true;
			last_publish_time = k_uptime_get(); // Initialize after delay for correct first publish timing
		}

		uint64_t now = k_uptime_get();
		if ((now - last_publish_time) >= ((uint64_t)CONFIG_NET_SAMPLE_MQTT_PUBLISH_INTERVAL * 1000)) {
			if (!mqtt_connected) { // Re-check connection status before attempting publish
				break;
			}
			LOG_DBG("Attempting to publish data...");
			// app_mqtt_publish itself will set mqtt_connected = false on critical errors
			// and log the specifics.
			(void)app_mqtt_publish(client); // We rely on mqtt_connected state after this
			last_publish_time = now;
		}

		if (!mqtt_connected) { // Check if publish marked us as disconnected
			LOG_WRN("mqtt_connected is false (likely after publish attempt), exiting run loop.");
			break;
		}

		rc = app_mqtt_process(client); // This handles incoming messages and keepalives
		if (rc != 0) {
			// app_mqtt_process logs its own errors, including the modified poll error message.
			// If app_mqtt_process returns an error (e.g. from mqtt_input, mqtt_live, or poll error)
			// we log it here and sleep. If the error was critical enough to sever the connection,
			// the MQTT library's event handler (via mqtt_input) or our logic should set mqtt_connected = false.
			LOG_ERR("app_mqtt_process returned error [%d]. Current errno: %d. MQTT state: %s", rc, errno, mqtt_connected ? "connected" : "disconnected");
			
			if (mqtt_connected) { // Only sleep and continue if still marked as connected
				k_sleep(K_MSEC(1000)); // Wait 1 second before next iteration of the run loop
			}
		} else {
			// Success from app_mqtt_process (e.g., data handled, or keepalive sent/not needed yet)
			k_sleep(K_MSEC(50)); // Short sleep to yield CPU if everything is okay
		}
	}

	LOG_INF("MQTT run loop exiting (mqtt_connected: %s)", mqtt_connected ? "true" : "false");
}
// <<< END NEW CODE / MODIFICATION >>>


void app_mqtt_connect(struct mqtt_client *client)
{
	int rc;
	int retry_count = 0;
	const int max_retries = 5;

	mqtt_connected = false;

	while (!mqtt_connected && retry_count < max_retries) {
		LOG_INF("Attempting MQTT connection (attempt %d/%d)", 
			retry_count + 1, max_retries);

		rc = mqtt_connect(client);
		if (rc != 0) {
			LOG_ERR("MQTT connect call failed [%d]", rc);
			retry_count++;
			k_sleep(K_MSEC(MSECS_WAIT_RECONNECT));
			continue;
		}

		/* Poll for connection response */
		rc = poll_mqtt_socket(client, MSECS_NET_POLL_TIMEOUT); // poll_mqtt_socket logs its own error
		if (rc > 0) { // Data available (should be CONNACK)
			rc = mqtt_input(client); // Process the incoming CONNACK
			if (rc != 0) {
				LOG_ERR("MQTT input during connection (processing CONNACK) failed [%d]", rc);
				// mqtt_event_handler should set mqtt_connected if CONNACK result is bad
			}
		} else if (rc == 0) { // Timeout waiting for CONNACK
			LOG_WRN("Timeout waiting for CONNACK from broker.");
		}
		// If rc < 0, poll_mqtt_socket already logged the specific poll error code.
		// app_mqtt_process's more detailed errno log isn't called here.

		if (!mqtt_connected) { // mqtt_connected is set by on_mqtt_connect via event_handler
			LOG_WRN("MQTT connection not established after attempt, aborting current try.");
			mqtt_abort(client); // Cleanly abort the current connection attempt
			retry_count++;
			k_sleep(K_MSEC(MSECS_WAIT_RECONNECT));
		}
	}

	if (!mqtt_connected) {
		LOG_ERR("Failed to connect to MQTT broker after %d attempts", max_retries);
		device_write_led(LED_ERROR, LED_ON);
	}
}

int app_mqtt_init(struct mqtt_client *client)
{
	int rc;
	struct zsock_addrinfo *result;
	struct zsock_addrinfo hints;

	LOG_INF("Initializing MQTT client...");

	/* Resolve MQTT broker hostname via Zephyr socket API */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	rc = zsock_getaddrinfo(CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME,
			 CONFIG_NET_SAMPLE_MQTT_BROKER_PORT,
			 &hints, &result);
	if (rc != 0 || result == NULL) {
		LOG_ERR("Failed to resolve broker %s:%s, err=%d",
			CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME,
			CONFIG_NET_SAMPLE_MQTT_BROKER_PORT, rc);
		return -EIO;
	}

	/* Copy resolved address into broker storage */
	memcpy(&broker, result->ai_addr, result->ai_addrlen);
	zsock_freeaddrinfo(result);

	/* Log resolved IP address */
	char broker_ip[NET_IPV4_ADDR_LEN];
	net_addr_ntop(AF_INET,
		      &((struct sockaddr_in *)&broker)->sin_addr,
		      broker_ip, sizeof(broker_ip));
	LOG_INF("Resolved MQTT broker: %s -> %s:%s",
		CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME, broker_ip,
		CONFIG_NET_SAMPLE_MQTT_BROKER_PORT);

	/* Initialize MQTT client ID */
	init_mqtt_client_id();

	/* Initialize MQTT client */
	mqtt_client_init(client);
	client->broker = &broker;
	client->evt_cb = mqtt_event_handler;
	client->client_id.utf8 = client_id;
	client->client_id.size = strlen((char *)client_id);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;
	client->keepalive = 10; /* Drastically reduced keepalive to 10 seconds */
	client->clean_session = 1;

	/* Configure MQTT buffers */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* Configure transport */
#if defined(CONFIG_MQTT_LIB_TLS)
	rc = tls_init();
	if (rc != 0) {
		LOG_ERR("TLS initialization failed [%d]", rc);
		/* Continue with non-secure connection */
	}
	/* For now, use non-secure transport */
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#else
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif

	LOG_INF("MQTT client initialized successfully");
	LOG_INF("Client ID: %s", client_id);
	LOG_INF("Broker: %s:%s", CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME, 
		CONFIG_NET_SAMPLE_MQTT_BROKER_PORT);
	LOG_INF("Publish topic: %s", CONFIG_NET_SAMPLE_MQTT_PUB_TOPIC);
	LOG_INF("Command topic: %s", CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD);

	return 0;
}