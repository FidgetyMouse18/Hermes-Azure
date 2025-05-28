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
#include "vibration.h"
#include <zephyr/drivers/regulator.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define STACK_SIZE_UI 4096
#define PRIORITY_UI    7
#define STACK_SIZE_WIFI 8192
#define PRIORITY_WIFI    7

// Definitions for Vibration Thread
#define STACK_SIZE_VIBRATION 2048
#define PRIORITY_VIBRATION    6 

// Message queue for sensor data communication between threads
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 10, 4);

extern const struct device *vibration_regulator;
bool thresholds_exceeded = false;

// Vibration thread components
static struct k_thread vibration_thread_data_obj;
static k_tid_t vibration_thread_handle = NULL;
K_THREAD_STACK_DEFINE(vibration_thread_stack_area, STACK_SIZE_VIBRATION);
static volatile bool stop_vibration_signal_flag = false;

static void vibration_management_thread() {

    while (true) {
        if (stop_vibration_signal_flag) {
            break;
        }

        k_msleep(500);
        regulator_enable(vibration_regulator); // Turn ON
        k_msleep(1000);

        regulator_disable(vibration_regulator); 

        if (stop_vibration_signal_flag) {
            break;
        }
    }

    regulator_disable(vibration_regulator);
}

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
        struct sensor_data current_sensor_data; // Variable to hold data from queue
        bool new_data_from_queue = false;       // Flag to track if new data was received

        if (k_msgq_get(&sensor_msgq, &current_sensor_data, K_NO_WAIT) == 0) {
            // New data is available, update UI
            ui_env_set_temp(current_sensor_data.temperature);
            ui_env_set_hum(current_sensor_data.humidity);
            ui_env_set_press(current_sensor_data.pressure);
            ui_env_set_tvoc(current_sensor_data.tvoc);
            ui_motion_set_xyz(current_sensor_data.accel_x, current_sensor_data.accel_y, current_sensor_data.accel_z);
            ui_light_set_rgb(current_sensor_data.r, current_sensor_data.g, current_sensor_data.b);
            new_data_from_queue = true;
        }

        thresholds_exceeded = sensor_data_exceeds_thresholds(
                current_sensor_data.temperature, current_sensor_data.humidity, current_sensor_data.pressure,
                current_sensor_data.tvoc, current_sensor_data.accel_x, current_sensor_data.accel_y, current_sensor_data.accel_z,
                current_sensor_data.r, current_sensor_data.g, current_sensor_data.b);

        if (thresholds_exceeded) {
            if (vibration_thread_handle == NULL) {
                stop_vibration_signal_flag = false;
                vibration_thread_handle = k_thread_create(&vibration_thread_data_obj,
                                                 vibration_thread_stack_area,
                                                 STACK_SIZE_VIBRATION,
                                                 vibration_management_thread,
                                                 NULL, NULL, NULL,
                                                 PRIORITY_VIBRATION,
                                                 0, K_NO_WAIT);
                if (vibration_thread_handle == NULL) {
                    LOG_ERR("Failed to create vibration thread");
                } else {
                    k_thread_name_set(vibration_thread_handle, "vibration_mgmt");
                }
            }
        } else { 
            if (vibration_thread_handle != NULL) {
                stop_vibration_signal_flag = true;
                int ret = k_thread_join(vibration_thread_handle, K_MSEC(500));
                if (ret == -EAGAIN) {
                    k_thread_abort(vibration_thread_handle);
                    regulator_disable(vibration_regulator);
                } 
                vibration_thread_handle = NULL;
            }
        }
        
        lv_timer_handler();
        k_msleep(10);
    }
}

static void wifi_thread() {
    char response[1024];
    while (1) {
        int rc = http_get("192.168.0.49", 3000, "/device/AB12", response);
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
                // LOG_ERR("Parsed data: pressure=%u humidity=%u temperature=%u r=%u g=%u b=%u tvoc=%u accel_x=%d accel_y=%d accel_z=%d",
                //         data.pressure, data.humidity, data.temperature,
                //         data.r, data.g, data.b, data.tvoc,
                //         data.accel_x, data.accel_y, data.accel_z);
                
                // Send sensor data to UI thread via message queue
                if (k_msgq_put(&sensor_msgq, &data, K_NO_WAIT) != 0) {
                    // Queue is full, purge and try again
                    k_msgq_purge(&sensor_msgq);
                    k_msgq_put(&sensor_msgq, &data, K_NO_WAIT);
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
    vibration_init();
    

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