#include "wifi.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>

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