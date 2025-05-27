#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <stdio.h>
#include "wifi.h"
#include "ui.h"

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define STACK_SIZE_UI 4096
#define PRIORITY_UI    7
#define STACK_SIZE_WIFI 1024
#define PRIORITY_WIFI    7
// #define NUM_TILES      3
// #define DEBOUNCE_MS    50  

// lv_obj_t *tileview;
// static lv_obj_t *tap_btn;
// uint8_t current_tile;
// static uint32_t last_click_ms;
// K_SEM_DEFINE(screen_tap_sem, 1, 1);

// static void screen_tap_cb(lv_event_t *e)
// {
//     ARG_UNUSED(e);

//     if (k_sem_take(&screen_tap_sem, K_NO_WAIT) != 0) {
//         return;
//     }

//     uint32_t now = lv_tick_get();
//     if (now - last_click_ms < DEBOUNCE_MS) {
//         k_sem_give(&screen_tap_sem);
//         return;
//     }
//     last_click_ms = now;

//     current_tile = (current_tile + 1) % NUM_TILES;
//     lv_tileview_set_tile_by_index(tileview, current_tile, 0, LV_ANIM_OFF);

//     lv_obj_move_foreground(tap_btn);
    
//     k_sem_give(&screen_tap_sem);
// }

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
        lv_timer_handler();
        k_msleep(10);
    }
}

static void wifi_thread() {

    while (1) {
        k_msleep(1000);
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
    int rc = wifi_connect("Melrose", "Marinuca");
    if (rc) {
        LOG_ERR("Wi-Fi connect failed: %d", rc);
        return rc;
    }

    // 3) wait until you actually get an IP
    wifi_wait_for_ip_addr();

    // Start UI thread after WiFi connection is established
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

    // Start UI thread after WiFi connection is established
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