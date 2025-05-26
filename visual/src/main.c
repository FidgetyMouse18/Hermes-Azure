#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <stdio.h>
#include "wifi.h"

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define STACK_SIZE_UI 4096
#define PRIORITY_UI    7
#define NUM_TILES      3
#define DEBOUNCE_MS    50  

static lv_obj_t *tileview;
static lv_obj_t *tap_btn;        
static uint8_t   current_tile;
static uint32_t  last_click_ms;  
static struct k_thread ui_thread_data;
static k_tid_t ui_thread_id;


static void screen_tap_cb(lv_event_t *e)
{
    ARG_UNUSED(e);

    uint32_t now = lv_tick_get();
    if (now - last_click_ms < DEBOUNCE_MS) {  
        return;
    }
    last_click_ms = now;

    current_tile = (current_tile + 1) % NUM_TILES;
    lv_tileview_set_tile_by_index(tileview, current_tile, 0, LV_ANIM_OFF);

    lv_obj_move_foreground(tap_btn);
}

static void create_ui(void)
{

    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview,
                    lv_disp_get_hor_res(NULL),
                    lv_disp_get_ver_res(NULL));
    lv_obj_center(tileview);
    lv_obj_set_scroll_dir(tileview, LV_DIR_NONE);     

    for (int i = 0; i < NUM_TILES; i++) {
        lv_obj_t *tile = lv_tileview_add_tile(tileview, i, 0, LV_DIR_RIGHT);

        lv_obj_t *num = lv_label_create(tile);
        char buf[4]; snprintf(buf, sizeof(buf), "%d", i + 1);
        lv_label_set_text(num, buf);
        lv_obj_set_style_text_font(num, &lv_font_montserrat_48, 0);
        lv_obj_center(num);

        lv_obj_t *hint = lv_label_create(tile);
        lv_label_set_text(hint, "Tap anywhere to cycle");
        lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
    }

    current_tile = 0;
    last_click_ms = 0;

    tap_btn = lv_btn_create(lv_scr_act());
    lv_obj_remove_style_all(tap_btn);
    lv_obj_set_size(tap_btn,
                    lv_disp_get_hor_res(NULL),
                    lv_disp_get_ver_res(NULL));
    lv_obj_center(tap_btn);
    lv_obj_add_event_cb(tap_btn, screen_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_opa(tap_btn, LV_OPA_TRANSP, 0);

    lv_obj_move_foreground(tap_btn);  
}

static void ui_thread()
{
    const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(disp)) {
        LOG_ERR("Display not ready");
        return;
    }

    create_ui();
    display_blanking_off(disp);

    while (1) {
        lv_timer_handler();
        k_msleep(10);
    }
}



K_THREAD_STACK_DEFINE(ui_stack, STACK_SIZE_UI);

int main(void) {
    wifi_init();

    printk("here\n");

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

    return 0;
}