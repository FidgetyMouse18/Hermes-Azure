
#ifndef WIFI_H
#define WIFI_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/data/json.h>

#define TARGET_IP "192.168.0.49"
#define TARGET_PORT 3000

void wifi_init(void);

int http_post(const char *ip, uint16_t port, const char *path, char* data);

// Structure to hold parsed sensor data
struct sensor_data
{
    char *uuid;
    char *timestamp;
    char *pressure;
    char *humidity;
    char *temperature;
    char *r;
    char *g;
    char *b;
    char *tvoc;
    char *accel_x;
    char *accel_y;
    char *accel_z;
};

// JSON descriptor for sensor_data
static const struct json_obj_descr sensor_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct sensor_data, uuid, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, timestamp, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, pressure, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, humidity, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, temperature, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, r, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, g, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, b, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, tvoc, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, accel_x, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, accel_y, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct sensor_data, accel_z, JSON_TOK_STRING),
};

#endif