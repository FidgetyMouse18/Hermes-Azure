// -----------------------------------------------------------------------------
// ui.c  —  LVGL tile‑view UI for M5Core2 sensor dashboard
// Walter Nedov • May 2025
// -----------------------------------------------------------------------------
// This file *replaces* the old placeholder‑tile UI.  Call ui_init() once after
// lvgl is ready.  The existing screen‑tap callback that cycles tiles will still
// work because we keep the global symbols `tileview` and `current_tile`.
// -----------------------------------------------------------------------------

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <stdio.h>
#include "ui.h"

// -----------------------------------------------------------------------------
// Styles (memory-optimized pastel theme with 8-bit colors)
// -----------------------------------------------------------------------------
static lv_style_t style_tile;
static lv_style_t style_card;
static lv_style_t style_text_large;
static lv_style_t style_text_small;
static lv_style_t style_bar;

// 8-bit color palette (memory efficient)
#define COL_BG      lv_color_make(0xF0, 0xF0, 0xF0)  // Light gray
#define COL_CARD    lv_color_make(0xFF, 0xFF, 0xFF)  // White
#define COL_TEXT    lv_color_make(0x30, 0x30, 0x30)  // Dark gray

// Simplified pastel colors (8-bit)
#define COL_BLUE    lv_color_make(0xA0, 0xC0, 0xE0)  // Pastel blue
#define COL_PINK    lv_color_make(0xFF, 0xB0, 0xC0)  // Pastel pink
#define COL_GREEN   lv_color_make(0xC0, 0xE0, 0xC0)  // Pastel green
#define COL_ORANGE  lv_color_make(0xFF, 0xD0, 0xB0)  // Pastel orange

// RGB indicator colors (simplified)
#define COL_RED     lv_color_make(0xFF, 0x80, 0x80)  // Light red
#define COL_GRN     lv_color_make(0x80, 0xFF, 0x80)  // Light green
#define COL_BLU     lv_color_make(0x80, 0x80, 0xFF)  // Light blue

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------
static void create_environment_tile(lv_obj_t *parent);
static void create_motion_tile(lv_obj_t *parent);
static void create_light_sound_tile(lv_obj_t *parent);

// -----------------------------------------------------------------------------
// Tiles and tap handling state
// -----------------------------------------------------------------------------
#define NUM_TILES      3
#define DEBOUNCE_MS    50

// Shared state for tile view tap callback
lv_obj_t *tileview;
static lv_obj_t *tap_btn;
uint8_t   current_tile;
static uint32_t  last_click_ms;

// Semaphore for debouncing taps
K_SEM_DEFINE(screen_tap_sem, 1, 1);

static void screen_tap_cb(lv_event_t *e)
{
    if (k_sem_take(&screen_tap_sem, K_NO_WAIT) != 0) {
        return;
    }

    uint32_t now = lv_tick_get();
    if (now - last_click_ms < DEBOUNCE_MS) {
        k_sem_give(&screen_tap_sem);
        return;
    }
    last_click_ms = now;

    current_tile = (current_tile + 1) % NUM_TILES;
    lv_tileview_set_tile_by_index(tileview, current_tile, 0, LV_ANIM_OFF);

    lv_obj_move_foreground(tap_btn);
    
    k_sem_give(&screen_tap_sem);
}

// -----------------------------------------------------------------------------
// Public: initialise the whole UI
// -----------------------------------------------------------------------------
void ui_init(void)
{
    // ---------- Consolidated Styles (memory efficient) ----------
    lv_style_init(&style_tile);
    lv_style_set_bg_color(&style_tile, COL_BG);
    lv_style_set_pad_all(&style_tile, 12);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COL_CARD);
    lv_style_set_radius(&style_card, 8);
    lv_style_set_pad_all(&style_card, 10);
    lv_style_set_shadow_width(&style_card, 4);
    lv_style_set_shadow_opa(&style_card, LV_OPA_20);

    lv_style_init(&style_text_large);
    lv_style_set_text_color(&style_text_large, COL_TEXT);
    lv_style_set_text_font(&style_text_large, &lv_font_montserrat_24);

    lv_style_init(&style_text_small);
    lv_style_set_text_color(&style_text_small, COL_TEXT);
    lv_style_set_text_font(&style_text_small, &lv_font_montserrat_14);

    lv_style_init(&style_bar);
    lv_style_set_bg_color(&style_bar, lv_color_make(0xE0, 0xE0, 0xE0));
    lv_style_set_radius(&style_bar, 4);

    // ---------- Tile‑view ----------
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_add_style(tileview, &style_tile, 0);
    lv_obj_clear_flag(tileview, LV_OBJ_FLAG_SCROLL_ELASTIC);

    // Create tiles
    lv_obj_t *tile_env = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR | LV_DIR_VER);
    create_environment_tile(tile_env);

    lv_obj_t *tile_motion = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR | LV_DIR_VER);
    create_motion_tile(tile_motion);

    lv_obj_t *tile_ls = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR | LV_DIR_VER);
    create_light_sound_tile(tile_ls);

    // Initialize state
    current_tile = 0;
    last_click_ms = 0;

    // Transparent tap button
    tap_btn = lv_btn_create(lv_scr_act());
    lv_obj_remove_style_all(tap_btn);
    lv_obj_set_size(tap_btn, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(tap_btn);
    lv_obj_add_event_cb(tap_btn, screen_tap_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_opa(tap_btn, LV_OPA_TRANSP, 0);
    lv_obj_move_foreground(tap_btn); 
}

// -----------------------------------------------------------------------------
// Helpers (simplified for memory efficiency)
// -----------------------------------------------------------------------------
static lv_obj_t *make_card(lv_obj_t *parent, lv_color_t border_color)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_add_style(card, &style_card, 0);
    lv_obj_set_style_border_color(card, border_color, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_size(card, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);  // Remove scrollable indicator
    return card;
}

static void create_simple_value_block(lv_obj_t *card, const char *icon, const char *unit)
{
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *icon_lbl = lv_label_create(card);
    lv_obj_add_style(icon_lbl, &style_text_small, 0);
    lv_label_set_text(icon_lbl, icon);

    // Combined value and unit in one label
    lv_obj_t *val_lbl = lv_label_create(card);
    lv_obj_add_style(val_lbl, &style_text_large, 0);
    char combined_text[32];
    snprintf(combined_text, sizeof(combined_text), "-- %s", unit);
    lv_label_set_text(val_lbl, combined_text);
}

// -----------------------------------------------------------------------------
// Tile 0 – Environmental (smaller boxes to prevent spillover)
// -----------------------------------------------------------------------------
static void create_environment_tile(lv_obj_t *parent)
{
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);
    
    // Add padding to prevent spillover
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_pad_gap(parent, 6, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);  // Remove scrollable indicator

    // Temperature
    lv_obj_t *temp_card = make_card(parent, COL_BLUE);
    lv_obj_set_grid_cell(temp_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    create_simple_value_block(temp_card, LV_SYMBOL_HOME, "°C");

    // Humidity
    lv_obj_t *hum_card = make_card(parent, COL_PINK);
    lv_obj_set_grid_cell(hum_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    create_simple_value_block(hum_card, LV_SYMBOL_TINT, "%RH");

    // Pressure
    lv_obj_t *pres_card = make_card(parent, COL_GREEN);
    lv_obj_set_grid_cell(pres_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    create_simple_value_block(pres_card, LV_SYMBOL_UPLOAD, "hPa");

    // TVOC
    lv_obj_t *tvoc_card = make_card(parent, COL_ORANGE);
    lv_obj_set_grid_cell(tvoc_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    create_simple_value_block(tvoc_card, LV_SYMBOL_WARNING, "ppb");
}

// -----------------------------------------------------------------------------
// Tile 1 – Motion (rendered directly on background)
// -----------------------------------------------------------------------------
static void create_motion_tile(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);  // Remove scrollable indicator
    lv_obj_set_style_pad_all(parent, 20, 0);
    
    // Title directly on background
    lv_obj_t *title_lbl = lv_label_create(parent);
    lv_obj_add_style(title_lbl, &style_text_large, 0);
    lv_label_set_text(title_lbl, LV_SYMBOL_GPS " Acceleration");
    
    // Magnitude bar directly on background
    lv_obj_t *bar_label = lv_label_create(parent);
    lv_obj_add_style(bar_label, &style_text_small, 0);
    lv_label_set_text(bar_label, "Magnitude");
    lv_obj_set_style_pad_top(bar_label, 20, 0);
    
    lv_obj_t *mag_bar = lv_bar_create(parent);
    lv_obj_add_style(mag_bar, &style_bar, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mag_bar, COL_BLUE, LV_PART_INDICATOR);
    lv_obj_set_size(mag_bar, LV_PCT(80), 20);
    lv_bar_set_range(mag_bar, 0, 20);
    lv_bar_set_value(mag_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_pad_top(mag_bar, 5, 0);
    lv_obj_set_style_pad_bottom(mag_bar, 30, 0);
    
    // X, Y, Z values directly on background
    lv_obj_t *axes_cont = lv_obj_create(parent);
    lv_obj_remove_style_all(axes_cont);
    lv_obj_set_size(axes_cont, LV_PCT(80), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(axes_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(axes_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(axes_cont, 10, 0);
    
    const char *axes[] = {"X: -- g", "Y: -- g", "Z: -- g"};
    lv_color_t colors[] = {COL_BLUE, COL_PINK, COL_GREEN};
    
    for (int i = 0; i < 3; i++) {
        lv_obj_t *axis_lbl = lv_label_create(axes_cont);
        lv_obj_add_style(axis_lbl, &style_text_large, 0);
        lv_obj_set_style_text_color(axis_lbl, colors[i], 0);
        lv_label_set_text(axis_lbl, axes[i]);
    }
}

// -----------------------------------------------------------------------------
// Tile 2 – Light & Sound (longer boxes, better text positioning)
// -----------------------------------------------------------------------------
static void create_light_sound_tile(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(parent, 15, 0);
    lv_obj_set_style_pad_gap(parent, 15, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);  // Remove scrollable indicator

    // RGB Light card (longer)
    lv_obj_t *rgb_card = make_card(parent, COL_BLUE);
    lv_obj_set_width(rgb_card, LV_PCT(90));
    lv_obj_set_height(rgb_card, LV_PCT(35));
    lv_obj_set_flex_flow(rgb_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(rgb_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Light icon and title
    lv_obj_t *light_icon = lv_label_create(rgb_card);
    lv_obj_add_style(light_icon, &style_text_small, 0);
    lv_label_set_text(light_icon, LV_SYMBOL_EYE_OPEN " RGB Light");
    
    // RGB values (compact layout with units after values)
    lv_obj_t *rgb_cont = lv_obj_create(rgb_card);
    lv_obj_remove_style_all(rgb_cont);
    lv_obj_set_size(rgb_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(rgb_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rgb_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(rgb_cont, 8, 0);
    
    const char *rgb_text[] = {"R: --", "G: --", "B: --"};
    lv_color_t rgb_colors[] = {COL_RED, COL_GRN, COL_BLU};
    
    for (int i = 0; i < 3; i++) {
        lv_obj_t *rgb_lbl = lv_label_create(rgb_cont);
        lv_obj_add_style(rgb_lbl, &style_text_large, 0);
        lv_obj_set_style_text_color(rgb_lbl, rgb_colors[i], 0);
        lv_label_set_text(rgb_lbl, rgb_text[i]);
    }

    // Sound card (longer)
    lv_obj_t *mic_card = make_card(parent, COL_PINK);
    lv_obj_set_width(mic_card, LV_PCT(90));
    lv_obj_set_height(mic_card, LV_PCT(35));
    lv_obj_set_flex_flow(mic_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mic_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mic_icon = lv_label_create(mic_card);
    lv_obj_add_style(mic_icon, &style_text_small, 0);
    lv_label_set_text(mic_icon, LV_SYMBOL_AUDIO);

    lv_obj_t *mic_val = lv_label_create(mic_card);
    lv_obj_add_style(mic_val, &style_text_large, 0);
    lv_label_set_text(mic_val, "-- dB");
}

