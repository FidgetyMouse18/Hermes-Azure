#include <zephyr/kernel.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
#include <zephyr/posix/sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* ====== EDIT THESE BEFORE BUILD ====== */
#define WIFI_SSID   "ss Wireless"
#define WIFI_PASS   "Vivian1123"
#define BROKER_ADDR "192.168.0.253"
#define BROKER_PORT 1883
/* ==================================== */

static struct mqtt_client client;
static uint8_t rx_buf[2048], tx_buf[2048];
static struct sockaddr_storage broker;

static struct k_sem wifi_scan_done;
static struct k_sem wifi_connect_done;

// MQTT connection status
static bool connected = false;
static int64_t connection_time = 0;

// MQTT event callback handler
static void mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
    switch (evt->type) {
    case MQTT_EVT_CONNACK:
        if (evt->result != 0) {
            LOG_ERR("MQTT connect failed %d", evt->result);
            break;
        }
        connected = true;
        connection_time = k_uptime_get();  // Record connection time
        LOG_INF("MQTT client connected!");
        LOG_INF("Socket fd: %d", client->transport.tcp.sock);
        break;
        
    case MQTT_EVT_DISCONNECT:
        LOG_INF("MQTT client disconnected %d", evt->result);
        LOG_INF("Disconnect - Socket fd: %d", client->transport.tcp.sock);
        connected = false;
        break;
        
    case MQTT_EVT_PUBACK:
        if (evt->result != 0) {
            LOG_ERR("MQTT PUBACK error %d", evt->result);
            break;
        }
        LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);
        break;
        
    default:
        LOG_DBG("MQTT event type %d", evt->type);
        break;
    }
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint32_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT) {
        const struct wifi_scan_result *scan_result = (const struct wifi_scan_result *)cb->info;
        if (scan_result && scan_result->ssid_length > 0) {
            LOG_INF("Found SSID: %.*s, Strength: %d dBm, Security: %s (%d), Channel: %d",
                    scan_result->ssid_length, scan_result->ssid,
                    scan_result->rssi,
                    scan_result->security == WIFI_SECURITY_TYPE_PSK ? "WPA/WPA2" :
                    scan_result->security == WIFI_SECURITY_TYPE_NONE ? "Open" :
                    "Other", scan_result->security, scan_result->channel);
        }
    } else if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
        LOG_INF("WiFi scan completed!");
        k_sem_give(&wifi_scan_done);
    } else if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        if (status && status->status == 0) {
            LOG_INF("WiFi connected (status OK)");
        } else {
            LOG_ERR("WiFi connection failed (status %d)", status ? status->status : -1);
        }
        k_sem_give(&wifi_connect_done);
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        LOG_INF("WiFi disconnected");
    }
}

static struct net_mgmt_event_callback net_cb;

static int wifi_scan(void)
{
    struct net_if *iface = net_if_get_default();
    
    if (!iface) {
        LOG_ERR("No network interface found");
        return -ENODEV;
    }

    k_sem_init(&wifi_scan_done, 0, 1);

    net_mgmt_init_event_callback(&net_cb, net_mgmt_event_handler,
                                 NET_EVENT_WIFI_SCAN_RESULT |
                                 NET_EVENT_WIFI_SCAN_DONE |
                                 NET_EVENT_WIFI_CONNECT_RESULT |
                                 NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&net_cb);

    LOG_INF("Starting WiFi scan...");
    int rc = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
    if (rc) {
        LOG_ERR("Wi-Fi scan request failed (%d)", rc);
        return rc;
    }

    if (k_sem_take(&wifi_scan_done, K_SECONDS(30)) != 0) {
        LOG_ERR("WiFi scan timeout");
        return -ETIMEDOUT;
    }

    LOG_INF("WiFi scan finished.");
    return 0;
}

static int wifi_connect(void)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No network interface found");
        return -ENODEV;
    }

    struct wifi_connect_req_params params = {
        .ssid = WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = WIFI_PASS,
        .psk_length = strlen(WIFI_PASS),
        .security = WIFI_SECURITY_TYPE_PSK,
        .channel = WIFI_CHANNEL_ANY,
    };

    k_sem_init(&wifi_connect_done, 0, 1);
    LOG_INF("Connecting to WiFi SSID: %s", WIFI_SSID);
    int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    if (rc) {
        LOG_ERR("WiFi connect request failed (%d)", rc);
        return rc;
    }

    if (k_sem_take(&wifi_connect_done, K_SECONDS(30)) != 0) {
        LOG_ERR("WiFi connect timeout");
        return -ETIMEDOUT;
    }

    LOG_INF("WiFi connection established");
    return 0;
}

// Simplified MQTT setup without polling
static int mqtt_setup(void)
{
    mqtt_client_init(&client);
    
    struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
    broker4->sin_family = AF_INET;
    broker4->sin_port = htons(BROKER_PORT);
    inet_pton(AF_INET, BROKER_ADDR, &broker4->sin_addr);

    static char client_id[32] = "disco_test_fixed";

    client.broker = &broker;
    client.evt_cb = mqtt_evt_handler;
    client.client_id.utf8 = client_id;
    client.client_id.size = strlen(client_id);
    client.protocol_version = MQTT_VERSION_3_1_1;
    client.keepalive = 120;  // Increase from 60 to 120 seconds
    client.clean_session = 1;
    client.transport.type = MQTT_TRANSPORT_NON_SECURE;
    client.rx_buf = rx_buf;
    client.rx_buf_size = sizeof(rx_buf);
    client.tx_buf = tx_buf;
    client.tx_buf_size = sizeof(tx_buf);

    LOG_INF("Connecting to MQTT broker %s:%d", BROKER_ADDR, BROKER_PORT);
    
    int rc = mqtt_connect(&client);
    if (rc != 0) {
        LOG_ERR("mqtt_connect failed: %d", rc);
        return rc;
    }

    // Add delay to let TCP connection stabilize
    k_sleep(K_MSEC(500));

    // Simple wait for connection
    for (int i = 0; i < 50; i++) {
        k_sleep(K_MSEC(200));  // Slower polling
        
        if (!connected) {  // Only call mqtt_input if not yet connected
            mqtt_input(&client);
        }
        
        if (connected) {
            LOG_INF("MQTT connected successfully");
            k_sleep(K_SECONDS(2));  // Longer stabilization
            return 0;
        }
    }
    
    LOG_ERR("MQTT connection timeout");
    return -1;
}

// Simple publish function
static int publish_temp(float temperature)
{
    if (!connected) {
        LOG_ERR("MQTT not connected, skipping publish");
        return -1;
    }

    char payload[32];
    snprintf(payload, sizeof(payload), "%.2f", temperature);

    struct mqtt_publish_param param = {
        .message.topic.topic.utf8 = "sensor/disco/temp",
        .message.topic.topic.size = strlen("sensor/disco/temp"),
        .message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE,
        .message.payload.data = payload,
        .message.payload.len = strlen(payload),
        .message_id = sys_rand16_get(),
        .dup_flag = 0,
        .retain_flag = 0
    };

    int result = mqtt_publish(&client, &param);
    if (result != 0) {
        LOG_ERR("mqtt_publish failed: %d", result);
        if (result == -ENOTCONN) {
            connected = false;
        }
    }
    return result;
}

int main(void)
{
    const struct device *dev = DEVICE_DT_GET_ONE(st_hts221);
    struct sensor_value temp;

    if (!device_is_ready(dev)) {
        LOG_ERR("Sensor not ready");
        return -1;
    }

    LOG_INF("Starting WiFi initialization for scanning...");
    int wifi_scan_result = wifi_scan();
    if (wifi_scan_result != 0) {
        LOG_ERR("WiFi scan failed.");
    }

    LOG_INF("Starting WiFi connection...");
    int wifi_connect_result = wifi_connect();
    if (wifi_connect_result != 0) {
        LOG_ERR("WiFi connection failed.");
        return -1;
    }

    LOG_INF("Checking for IP address assignment...");
    struct net_if *iface = net_if_get_default();
    bool ip_assigned = false;
    
    for (int i = 0; i < 30; i++) {
        if (iface) {
            struct in_addr *addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
            if (addr) {
                char addr_str[NET_IPV4_ADDR_LEN];
                net_addr_ntop(AF_INET, addr, addr_str, sizeof(addr_str));
                LOG_INF("IP address assigned: %s", addr_str);
                ip_assigned = true;
                break;
            }
        }
        
        LOG_DBG("Waiting for IP address... attempt %d/30", i + 1);
        k_sleep(K_SECONDS(1));
    }
    
    if (!ip_assigned) {
        LOG_ERR("No IP address assigned after 30 seconds");
        return -1;
    }

    LOG_INF("Network ready, allowing stack to stabilize...");
    k_sleep(K_SECONDS(2));

    LOG_INF("WiFi connected, setting up MQTT...");
    int mqtt_result = mqtt_setup();
    if (mqtt_result != 0) {
        LOG_ERR("MQTT setup failed, running in sensor-only mode");
        while (1) {
            sensor_sample_fetch(dev);
            sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            LOG_INF("Temperature: %d.%06d°C (No MQTT)", temp.val1, temp.val2);
            k_sleep(K_SECONDS(10));
        }
    }

    LOG_INF("MQTT connected, starting data publishing...");
    
    // Let connection stabilize before aggressive polling
    k_sleep(K_SECONDS(1));

    // Initialize sensor reading timestamp
    int64_t last_sensor_read = 0;
    const int64_t sensor_interval = 5000; // 5 seconds

    while (1) {
        // MQTT maintenance - but limit frequency to reduce eswifi stress
        if (connected) {
            static int64_t last_mqtt_process = 0;
            int64_t now = k_uptime_get();
            
            // Only process MQTT every 500ms to avoid eswifi race conditions
            if (now - last_mqtt_process >= 2000) {
                int rc = mqtt_input(&client);
                if (rc < 0 && rc != -EAGAIN) {
                    LOG_ERR("mqtt_input failed: %d", rc);
                    connected = false;
                }
                
                // Check keepalive
                int keepalive_time = mqtt_keepalive_time_left(&client);
                if (keepalive_time > 0 && keepalive_time < 10000) {
                    rc = mqtt_live(&client);
                    if (rc < 0 && rc != -EAGAIN) {
                        LOG_ERR("mqtt_live failed: %d", rc);
                        connected = false;
                    }
                }
                last_mqtt_process = now;
            }
        }
        
        // Sensor operations
        int64_t now = k_uptime_get();
        
        // Only publish if connected for at least 5 seconds AND sensor interval elapsed
        bool can_publish = connected && 
                          (now - connection_time >= 5000) && 
                          (now - last_sensor_read >= sensor_interval);
        
        if (can_publish) {
            // Try sensor read with timeout protection
            int rc = sensor_sample_fetch(dev);
            if (rc == 0) {
                rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
                if (rc == 0) {
                    float temperature = temp.val1 + (temp.val2 / 1000000.0);
                    
                    int result = publish_temp(temperature);
                    if (result == 0) {
                        LOG_INF("Published: %.2f°C", temperature);
                    } else {
                        LOG_ERR("Publish failed: %d", result);
                    }
                } else {
                    LOG_ERR("Sensor read failed: %d", rc);
                }
            } else {
                LOG_ERR("Sensor fetch failed: %d", rc);
            }
            last_sensor_read = now;
        } else if (!connected) {
            LOG_WRN("MQTT disconnected, waiting for reconnection");
        }
        
        // Small sleep to prevent CPU spinning
        k_sleep(K_MSEC(500));
    }
    return 0;
}