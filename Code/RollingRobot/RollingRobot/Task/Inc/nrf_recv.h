#ifndef __NRF_RECV_H__
#define __NRF_RECV_H__

#include "stm32f4xx.h"

typedef struct // 移动指令数据包
{
    uint8_t valid;
    float MoveDir;
} MovePkt;

void nrf_recvtask(void *params);

#endif
