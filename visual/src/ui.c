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
// Styles (high‑contrast dark theme)
// -----------------------------------------------------------------------------
static lv_style_t style_tile;
static lv_style_t style_card;
static lv_style_t style_value;
static lv_style_t style_unit;
static lv_style_t style_icon;

// Shared colours
#define COL_BG      lv_color_black()
#define COL_CARD    lv_color_hex(0x222222)
#define COL_TEXT    lv_color_white()
#define COL_ACCENT1 lv_palette_main(LV_PALETTE_BLUE)
#define COL_ACCENT2 lv_palette_main(LV_PALETTE_RED)
#define COL_ACCENT3 lv_palette_main(LV_PALETTE_GREEN)

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
    // ---------- Styles ----------
    lv_style_init(&style_tile);
    lv_style_set_bg_color(&style_tile, COL_BG);
    lv_style_set_pad_all(&style_tile, 12);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COL_CARD);
    lv_style_set_radius(&style_card, 8);
    lv_style_set_pad_all(&style_card, 8);
    lv_style_set_shadow_width(&style_card, 8);
    lv_style_set_shadow_color(&style_card, lv_color_black());

    lv_style_init(&style_value);
    lv_style_set_text_color(&style_value, COL_TEXT);
    lv_style_set_text_font(&style_value, &lv_font_montserrat_28);

    lv_style_init(&style_unit);
    lv_style_set_text_color(&style_unit, COL_TEXT);
    lv_style_set_text_font(&style_unit, &lv_font_montserrat_14);

    lv_style_init(&style_icon);
    lv_style_set_text_color(&style_icon, COL_ACCENT1);

    // ---------- Tile‑view ----------
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_add_style(tileview, &style_tile, 0);
    lv_obj_clear_flag(tileview, LV_OBJ_FLAG_SCROLL_ELASTIC);

    // Page 0: Environmental dashboard -------------------------------------------------
    lv_obj_t *tile_env = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR | LV_DIR_VER);
    create_environment_tile(tile_env);

    // Page 1: Motion -------------------------------------------------------------------
    lv_obj_t *tile_motion = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR | LV_DIR_VER);
    create_motion_tile(tile_motion);

    // Page 2: Light + Sound -------------------------------------------------------------
    lv_obj_t *tile_ls = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR | LV_DIR_VER);
    create_light_sound_tile(tile_ls);

    // Start on the first tile
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

// -----------------------------------------------------------------------------
// Helpers ----------------------------------------------------------------------
// -----------------------------------------------------------------------------
static lv_obj_t *make_card(lv_obj_t *parent, lv_color_t accent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_add_style(card, &style_card, 0);
    lv_style_set_border_color(&style_card, accent);
    lv_style_set_border_width(&style_card, 2);
    lv_obj_set_size(card, LV_PCT(100), LV_PCT(100));
    return card;
}

static void create_value_block(lv_obj_t *card, const char *icon, const char *unit)
{
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *icon_lbl = lv_label_create(card);
    lv_obj_add_style(icon_lbl, &style_icon, 0);
    lv_label_set_text(icon_lbl, icon);

    lv_obj_t *val_lbl = lv_label_create(card);
    lv_obj_add_style(val_lbl, &style_value, 0);
    lv_label_set_text(val_lbl, "--");          // Placeholder value

    lv_obj_t *unit_lbl = lv_label_create(card);
    lv_obj_add_style(unit_lbl, &style_unit, 0);
    lv_label_set_text(unit_lbl, unit);
}

// -----------------------------------------------------------------------------
// Tile 0 – Environmental --------------------------------------------------------
// -----------------------------------------------------------------------------
static void create_environment_tile(lv_obj_t *parent)
{
    // 2×2 grid
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);

    // Temperature ------------------------------------------------------------------
    lv_obj_t *temp_card = make_card(parent, COL_ACCENT1);
    lv_obj_set_grid_cell(temp_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    create_value_block(temp_card, LV_SYMBOL_HOME, "°C");

    // Humidity ---------------------------------------------------------------------
    lv_obj_t *hum_card = make_card(parent, COL_ACCENT2);
    lv_obj_set_grid_cell(hum_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    create_value_block(hum_card, LV_SYMBOL_TINT, "%RH");

    // Pressure ---------------------------------------------------------------------
    lv_obj_t *pres_card = make_card(parent, COL_ACCENT3);
    lv_obj_set_grid_cell(pres_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    create_value_block(pres_card, LV_SYMBOL_UPLOAD, "hPa");

    // TVOC -------------------------------------------------------------------------
    lv_obj_t *tvoc_card = make_card(parent, lv_palette_main(LV_PALETTE_ORANGE));
    lv_obj_set_grid_cell(tvoc_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    create_value_block(tvoc_card, LV_SYMBOL_WARNING, "ppb");
}

// -----------------------------------------------------------------------------
// Tile 1 – Motion (Accelerometer) ----------------------------------------------
// -----------------------------------------------------------------------------
static void create_motion_tile(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char *axes[] = {"X", "Y", "Z"};
    lv_color_t colors[] = {COL_ACCENT1, COL_ACCENT2, COL_ACCENT3};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *card = make_card(parent, colors[i]);
        lv_obj_set_width(card, LV_PCT(90));
        lv_obj_set_height(card, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_ver(card, 4, 0);
        create_value_block(card, axes[i], "g");
    }
}

// -----------------------------------------------------------------------------
// Tile 2 – Light & Sound --------------------------------------------------------
// -----------------------------------------------------------------------------
static void create_light_sound_tile(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Light (lux) ------------------------------------------------------------------
    lv_obj_t *lux_card = make_card(parent, COL_ACCENT1);
    lv_obj_set_width(lux_card, LV_PCT(90));
    lv_obj_set_height(lux_card, LV_SIZE_CONTENT);
    create_value_block(lux_card, LV_SYMBOL_EYE_OPEN, "lux");

    // Sound (dB) -------------------------------------------------------------------
    lv_obj_t *mic_card = make_card(parent, COL_ACCENT2);
    lv_obj_set_width(mic_card, LV_PCT(90));
    lv_obj_set_height(mic_card, LV_SIZE_CONTENT);
    create_value_block(mic_card, LV_SYMBOL_AUDIO, "dB");
}

// -----------------------------------------------------------------------------
// EOF --------------------------------------------------------------------------
