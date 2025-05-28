#include "wifi.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(wifi, LOG_LEVEL_INF);

static K_SEM_DEFINE(sem_wifi, 0, 1);
static K_SEM_DEFINE(sem_ipv4, 0, 1);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

/**
 * @brief Handler for Wi-Fi connect/disconnect events.
 */
static void on_wifi_connection_event(struct net_mgmt_event_callback *cb,
                                     uint32_t mgmt_event,
                                     struct net_if *iface)
{
    const struct wifi_status *status = cb->info;

    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        if (status->status) {
            LOG_ERR("Wi-Fi connect failed (%d)", status->status);
        } else {
            LOG_INF("Wi-Fi connected");
            k_sem_give(&sem_wifi);
        }
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        if (status->status) {
            LOG_ERR("Wi-Fi disconnect failed (%d)", status->status);
        } else {
            LOG_INF("Wi-Fi disconnected");
            k_sem_take(&sem_wifi, K_NO_WAIT);
        }
    }
}

/**
 * @brief Handler for IPv4 address assigned event.
 */
static void on_ipv4_obtained(struct net_mgmt_event_callback *cb,
                             uint32_t mgmt_event,
                             struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        LOG_INF("IPv4 address assigned");
        k_sem_give(&sem_ipv4);
    }
}

void wifi_init(void)
{
    /* Register Wi-Fi connect/disconnect callback */
    net_mgmt_init_event_callback(&wifi_cb,
                                 on_wifi_connection_event,
                                 NET_EVENT_WIFI_CONNECT_RESULT |
                                 NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    /* Register IPv4 address assigned callback */
    net_mgmt_init_event_callback(&ipv4_cb,
                                 on_ipv4_obtained,
                                 NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&ipv4_cb);
}

int wifi_connect(const char *ssid, const char *psk)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No default network interface");
        return -ENODEV;
    }

    struct wifi_connect_req_params params = {
        .ssid = (const uint8_t *)ssid,
        .ssid_length = strlen(ssid),
        .psk = (const uint8_t *)psk,
        .psk_length = strlen(psk),
        .security = WIFI_SECURITY_TYPE_PSK,
        .channel = WIFI_CHANNEL_ANY,
        .band = WIFI_FREQ_BAND_2_4_GHZ,
        .mfp = WIFI_MFP_OPTIONAL,
    };

    LOG_INF("Connecting to SSID: %s", ssid);
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT,
                       iface, &params,
                       sizeof(params));
    if (ret) {
        LOG_ERR("Connection request failed (%d)", ret);
        return ret;
    }

    /* Wait indefinitely for connection event */
    k_sem_take(&sem_wifi, K_FOREVER);
    return 0;
}

void wifi_wait_for_ip_addr(void)
{
    /* Wait indefinitely for IPv4 address */
    k_sem_take(&sem_ipv4, K_FOREVER);
}

int wifi_disconnect(void)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        return -ENODEV;
    }
    return net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
}

int http_get(const char *ip, uint16_t port, const char *path, char* response)
{
    int sock, ret;
    struct sockaddr_in addr;
    char request[256];
    char recv_buf[512];

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
             "GET %s HTTP/1.1\r\n"
             "Host: %s:%u\r\n"
             "Connection: close\r\n"
             "\r\n", path, ip, port);
    ret = zsock_send(sock, request, strlen(request), 0);
    if (ret < 0) {
        LOG_ERR("Failed to send HTTP request (%d)", ret);
        zsock_close(sock);
        return ret;
    }

    while ((ret = zsock_recv(sock, recv_buf, sizeof(recv_buf) - 1, 0)) > 0) {
        recv_buf[ret] = '\0';
        char *line = strtok(recv_buf, "\r\n");
        char *last_line = NULL;
        
        while (line != NULL) {
            last_line = line;
            line = strtok(NULL, "\r\n");
        }
        
        if (last_line != NULL) {
            strcpy(response, last_line);
        }
    }

    if (ret < 0) {
        LOG_ERR("HTTP recv failed (%d)", ret);
    }

    zsock_close(sock);

    return (ret < 0) ? ret : 0;
}