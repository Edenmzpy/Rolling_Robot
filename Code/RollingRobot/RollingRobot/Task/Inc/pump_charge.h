#ifndef __PUMP_CHARGE_H__
#define __PUMP_CHARGE_H__

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef enum
{
    PUMP_CMD_NONE = 0,
    PUMP_CMD_START,
    PUMP_CMD_STOP
} PumpCmd_t;


extern TaskHandle_t s_pumpTaskHandle;

void charge_task(void *params);

#endif
