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
void ui_env_set_temp(float degC);        /* °C */
void ui_env_set_hum(float rel_rh);       /* %RH */
void ui_env_set_press(float hPa);        /* hPa */
void ui_env_set_tvoc(uint16_t ppb);      /* parts‑per‑billion */

/* ------------------------------------------------------------------------- */
/* Motion Sensor (Page 1)                                                    */
/* ------------------------------------------------------------------------- */
/** Set accelerometer values in milli‑g (mg). */
void ui_motion_set_xyz(int16_t x_mg,
                       int16_t y_mg,
                       int16_t z_mg);

/* ------------------------------------------------------------------------- */
/* Light & Sound Sensors (Page 2)                                            */
/* ------------------------------------------------------------------------- */
void ui_light_set_lux(uint32_t lux);     /* lx  */
void ui_mic_set_db(float dB);            /* dB  */

#endif /* UI_H_ */
