#ifndef __OLED_H__
#define __OLED_H__

#include "stm32f1xx.h"

/*引脚配置*/
#define OLED_W_D0(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, (GPIO_PinState)(x))
#define OLED_W_D1(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, (GPIO_PinState)(x))
#define OLED_W_RES(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, (GPIO_PinState)(x))
#define OLED_W_DC(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, (GPIO_PinState)(x))
#define OLED_W_CS(x)		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, (GPIO_PinState)(x))


void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);


	
#endif
