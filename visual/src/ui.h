#ifndef UI_H_
#define UI_H_

#include <stdint.h>




void ui_init(void);
void ui_env_set_temp(uint8_t degC);        
void ui_env_set_hum(uint8_t rel_rh);       
void ui_env_set_press(uint8_t hPa);        
void ui_env_set_tvoc(uint16_t ppb);      
void ui_motion_set_xyz(int8_t x,int8_t y,int8_t z);
void ui_light_set_rgb(uint8_t r, uint8_t g, uint8_t b); 

#endif /* UI_H_ */
