#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#include <zephyr/arch/arch_interface.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include "wifi.h"
#include "ui.h"

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define STACK_SIZE_UI 4096
#define PRIORITY_UI    7
#define STACK_SIZE_WIFI 8192
#define PRIORITY_WIFI    7

// Message queue for sensor data communication between threads
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 10, 4);

static void ui_thread()
{
    const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(disp)) {
        LOG_ERR("Display not ready");
        return;
    }

    ui_init();
    display_blanking_off(disp);

    while (1) {
        // Check for new sensor data
        struct sensor_data data;
        if (k_msgq_get(&sensor_msgq, &data, K_NO_WAIT) == 0) {
            // Update UI with new sensor data
            ui_env_set_temp(data.temperature);
            ui_env_set_hum(data.humidity);
            ui_env_set_press(data.pressure);
            ui_env_set_tvoc(data.tvoc);
            ui_motion_set_xyz(data.accel_x, data.accel_y, data.accel_z);
            ui_light_set_rgb(data.r, data.g, data.b);
            
        }
        
        lv_timer_handler();
        k_msleep(10);
    }
}

static void wifi_thread() {
    char response[1024];
    while (1) {
        int rc = http_get("192.168.0.49", 3000, "/device/ABC123", response);
        if (rc) {
            LOG_ERR("HTTP GET failed: %d", rc);
        } else {
            struct sensor_data data;
            int err = json_obj_parse(response, strlen(response),
                                     sensor_descr, ARRAY_SIZE(sensor_descr),
                                     &data);
            if (err < 0) {
                LOG_ERR("JSON parse failed: %d", err);
            } else {
                LOG_INF("Parsed data: pressure=%u humidity=%u temperature=%u r=%u g=%u b=%u tvoc=%u accel_x=%d accel_y=%d accel_z=%d",
                        data.pressure, data.humidity, data.temperature,
                        data.r, data.g, data.b, data.tvoc,
                        data.accel_x, data.accel_y, data.accel_z);
                
                // Send sensor data to UI thread via message queue
                if (k_msgq_put(&sensor_msgq, &data, K_NO_WAIT) != 0) {
                    // Queue is full, purge and try again
                    k_msgq_purge(&sensor_msgq);
                    k_msgq_put(&sensor_msgq, &data, K_NO_WAIT);
                    LOG_WRN("Sensor message queue was full, purged old data");
                }
            }
        }
        k_msleep(5000);
    }
}

static struct k_thread ui_thread_data;
static k_tid_t ui_thread_id;
K_THREAD_STACK_DEFINE(ui_stack , STACK_SIZE_UI);

static struct k_thread wifi_thread_data;
static k_tid_t wifi_thread_id;
K_THREAD_STACK_DEFINE(wifi_stack , STACK_SIZE_WIFI);

int main(void) {
    wifi_init();

    // 2) join your network (blocks until associated)
    int rc = wifi_connect("csse4011", "csse4011wifi");
    if (rc) {
        LOG_ERR("Wi-Fi connect failed: %d", rc);
        return rc;
    }

    // 3) wait until you actually get an IP
    wifi_wait_for_ip_addr();

    ui_thread_id = k_thread_create(&ui_thread_data,
                                 ui_stack,
                                 STACK_SIZE_UI,
                                 ui_thread,
                                 NULL,
                                 NULL,
                                 NULL,
                                 PRIORITY_UI,
                                 0,
                                 K_NO_WAIT);
    if (ui_thread_id == NULL) {
        LOG_ERR("Failed to create UI thread");
        return -ENOMEM;
    }

    wifi_thread_id = k_thread_create(&wifi_thread_data,
                                 wifi_stack,
                                 STACK_SIZE_WIFI,
                                 wifi_thread,
                                 NULL,
                                 NULL,
                                 NULL,
                                 PRIORITY_WIFI,
                                 0,
                                 K_NO_WAIT);
    if (wifi_thread_id == NULL) {
        LOG_ERR("Failed to create WIFI thread");
        return -ENOMEM;
    }

    return 0;
}