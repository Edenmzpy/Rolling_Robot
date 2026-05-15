#ifndef __DELAY_US_H__
#define __DELAY_US_H__

#include "stm32f4xx_hal.h"

void dwt_init(void);
void delay_us(uint32_t us);

#endif
