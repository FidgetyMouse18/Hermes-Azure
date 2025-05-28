// -----------------------------------------------------------------------------
// ui.h  —  Public interface for sensor dashboard UI (M5Core2 + LVGL)
// Walter Nedov • May 2025
// -----------------------------------------------------------------------------
#ifndef UI_H_
#define UI_H_

#include <stdint.h>



/* ------------------------------------------------------------------------- */
/* Life‑cycle                                                                */
/* ------------------------------------------------------------------------- */
/** Initialise LVGL widgets and create the tile‑view.
 *  Call once after lv_init() and display/touch drivers are registered. */
void ui_init(void);

/* ------------------------------------------------------------------------- */
/* Environmental Sensors (Page 0)                                            */
/* ------------------------------------------------------------------------- */
void ui_env_set_temp(uint8_t degC);        /* °C */
void ui_env_set_hum(uint8_t rel_rh);       /* %RH */
void ui_env_set_press(uint8_t hPa);        /* hPa */
void ui_env_set_tvoc(uint16_t ppb);      /* parts‑per‑billion */

/* ------------------------------------------------------------------------- */
/* Motion Sensor (Page 1)                                                    */
/* ------------------------------------------------------------------------- */
/** Set accelerometer values in raw int8_t format. */
void ui_motion_set_xyz(int8_t x,
                       int8_t y,
                       int8_t z);

/* ------------------------------------------------------------------------- */
/* Light Sensor (Page 2)                                                     */
/* ------------------------------------------------------------------------- */
void ui_light_set_lux(uint32_t lux);     /* lx  */
void ui_light_set_rgb(uint8_t r, uint8_t g, uint8_t b);  /* RGB values */

#endif /* UI_H_ */
