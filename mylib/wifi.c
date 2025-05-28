#include "wifi.h"
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
#include <zephyr/data/json.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
LOG_MODULE_REGISTER(wifi, LOG_LEVEL_INF);

#define WIFI_SSID   "csse4011"
#define WIFI_PASS   "csse4011wifi"

static struct mqtt_client client;
static uint8_t rx_buf[2048], tx_buf[1024];
static struct sockaddr_storage broker;

static struct k_sem wifi_scan_done;
static struct k_sem wifi_connect_done;

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

int http_post(const char *ip, uint16_t port, const char *path, char* data)
{
    int sock, ret;
    struct sockaddr_in addr;
    char request[512];
    // char recv_buf[512];

    sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        LOG_ERR("Failed to create socket (%d)", sock);
        return sock;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    ret = zsock_inet_pton(AF_INET, ip, &addr.sin_addr);
    if (ret < 0) {
        LOG_ERR("Invalid IPv4 address: %s (%d)", ip, ret);
        zsock_close(sock);
        return ret;
    }

    ret = zsock_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Socket connect failed (%d)", ret);
        zsock_close(sock);
        return ret;
    }

    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Content-Type: application/json\r\n"
             "Host: %s:%u\r\n"
             "Content-Length: %d\r\n\r\n"
             "%s\r\n"
             "\r\n", path, ip, port, strlen(data), data);
    printk("%s\n", request);
    ret = zsock_send(sock, request, strlen(request), 0);
    if (ret < 0) {
        LOG_ERR("Failed to send HTTP request (%d)", ret);
        zsock_close(sock);
        return ret;
    }

    zsock_close(sock);

    return (ret < 0) ? ret : 0;
}






void wifi_init(void) {
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
}



// int main(void)
// {
//     char response[1024];

//     while (1) {
//         int rc = http_get("192.168.0.49", 3000, "/device/ABC123", response);
//         if (rc) {
//             LOG_ERR("HTTP GET failed: %d", rc);
//         } else {
//             struct sensor_data data;
//             int err = json_obj_parse(response, strlen(response),
//                                      sensor_descr, ARRAY_SIZE(sensor_descr),
//                                      &data);
//             if (err < 0) {
//                 LOG_ERR("JSON parse failed: %d", err);
//             } else {
//                 LOG_ERR("Parsed data: pressure=%u humidity=%u temperature=%u r=%u g=%u b=%u tvoc=%u accel_x=%d accel_y=%d accel_z=%d",
//                         data.pressure, data.humidity, data.temperature,
//                         data.r, data.g, data.b, data.tvoc,
//                         data.accel_x, data.accel_y, data.accel_z);
//             }
//         }
//         k_sleep(K_MSEC(5000));
//     }
//     return 0;
// }

