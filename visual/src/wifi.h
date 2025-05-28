#ifndef WIFI_H
#define WIFI_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/data/json.h>

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

/**
 * @brief Perform a basic HTTP GET to the given IP, port and path, and log the JSON response.
 *
 * @param ip   Null-terminated IPv4 address string.
 * @param port Port number in host byte order.
 * @param path Null-terminated HTTP path (must start with '/').
 * @return    0 on success, negative error code on failure.
 */
int http_get(const char *ip, uint16_t port, const char *path, char* response);

// Structure to hold parsed sensor data
struct sensor_data {
    char *uuid;
    uint64_t timestamp;
    uint8_t pressure;
    uint8_t humidity;
    uint8_t temperature;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint16_t tvoc;
    int8_t accel_x;
    int8_t accel_y;
    int8_t accel_z;
};

// JSON descriptor for sensor_data
static const struct json_obj_descr sensor_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct sensor_data, uuid, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, timestamp, JSON_TOK_UINT64),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, pressure,  JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, humidity,  JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, temperature, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, r,         JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, g,         JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, b,         JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, tvoc,      JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, accel_x,   JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, accel_y,   JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, accel_z,   JSON_TOK_NUMBER),
};

#endif /* WIFI_H */