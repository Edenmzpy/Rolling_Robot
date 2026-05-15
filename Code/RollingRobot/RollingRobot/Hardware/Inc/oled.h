#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>

#define OLED_SCL_Pin GPIO_PIN_10
#define OLED_SCL_GPIO_Port GPIOC
#define OLED_SDA_Pin GPIO_PIN_11
#define OLED_SDA_GPIO_Port GPIOC

void oled_init(void);
void oled_clear(void);
void oled_show_char(uint8_t Line, uint8_t Column, char Char);
void oled_show_string(uint8_t Line, uint8_t Column, char *String);
void oled_show_num(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void oled_show_signednum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void oled_show_hexnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void oled_show_binnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);


#endif
