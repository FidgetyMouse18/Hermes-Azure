#ifndef WIFI_H
#define WIFI_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize Wi-Fi management event callbacks.
 */
void wifi_init(void);

/**
 * @brief Connect to a Wi-Fi network (blocking until association).
 *
 * @param ssid Null-terminated SSID string.
 * @param psk  Null-terminated pre-shared key string.
 * @return 0 on success, negative error code on failure.
 */
int wifi_connect(const char *ssid, const char *psk);

/**
 * @brief Block until an IPv4 address is obtained.
 */
void wifi_wait_for_ip_addr(void);

/**
 * @brief Disconnect from the current Wi-Fi network.
 *
 * @return 0 on success, negative error code on failure.
 */
int wifi_disconnect(void); 

#endif /* WIFI_H */