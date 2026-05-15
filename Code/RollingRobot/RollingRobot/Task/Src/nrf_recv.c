#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "queue.h"

#include "nrf_recv.h"
#include "nrf24l01.h"
#include "oled.h"
#include "pump_charge.h"
#include <string.h>

#define PUMP_EVE 0x01
#define ADD_PRE_EVE 0x02
#define SUB_PRE_EVE 0x04
#define PUMP_STOP_EVE  0x08
//设置事件标志位
extern QueueHandle_t qPumpCmd;
extern TaskHandle_t s_pumpTaskHandle;
//引用外部任务的句柄，在pump_charge.c有定义
static TaskHandle_t s_nrfTaskHandle = NULL;
//创建空句柄，为该.c下的某个任务句柄赋值做准备
QueueHandle_t qMove = NULL; // 运动指令消息队列
QueueHandle_t qMax_pre = NULL; // 充气压力队列

/* nrf24接收中断回调函数，nrf24接收信息后使能pin8触发中断，解除
 ulTaskNotifyTake阻塞*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_8)
	{
        BaseType_t hpw = pdFALSE;
        if (s_nrfTaskHandle)
		{
            vTaskNotifyGiveFromISR(s_nrfTaskHandle, &hpw); // 向nrf_recvtask发送指令读取任务通知
        }
        portYIELD_FROM_ISR(hpw);
    }
}

void nrf_recvtask(void *params)
{
    s_nrfTaskHandle = xTaskGetCurrentTaskHandle();//将当前任务的句柄赋值在这个空句柄上
	qMove = xQueueCreate(1, sizeof(MovePkt));
	qMax_pre = xQueueCreate(1, sizeof(uint8_t));
	
	uint8_t max_pre = 10;
	
	xQueueOverwrite(qMax_pre, &max_pre);

    while (nrf24_check() != 0)
	{
        vTaskDelay(pdMS_TO_TICKS(50));
    }//确认射频模块初始化
	
    nrf24_init();
	
	MovePkt move;//构建结构体，定义在头文件里
	

    while (1)
	{
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);//不接受中断通知一直处于阻塞状态

        while (nrf24_receive32(rx) == 0)
		{
			if (rx[5] != 0)
			{
				if (rx[5] & PUMP_EVE)
				{
					rx[5] &= ~PUMP_EVE;
					PumpCmd_t cmd = PUMP_CMD_START;
					if (qPumpCmd)
					{
						xQueueOverwrite(qPumpCmd, &cmd);
					}
				}

				if (rx[5] & PUMP_STOP_EVE)
				{
					rx[5] &= ~PUMP_STOP_EVE;
					PumpCmd_t cmd = PUMP_CMD_STOP;
					if (qPumpCmd)
					{
						xQueueOverwrite(qPumpCmd, &cmd);
					}
				}

				if (rx[5] & ADD_PRE_EVE)
				{
					rx[5] &= ~ADD_PRE_EVE;
					if (max_pre < 100)   
					{
						max_pre++;
					}
					xQueueOverwrite(qMax_pre, &max_pre);
				}

				if (rx[5] & SUB_PRE_EVE)
				{
					rx[5] &= ~SUB_PRE_EVE;
					if (max_pre > 0)     
					{
						max_pre--;
					}
					xQueueOverwrite(qMax_pre, &max_pre);
				}
			}
			if (rx[0] == 1) // 方向指令有效
			{
				move.valid = 1;
				memcpy(&move.MoveDir, &rx[1], sizeof(float));
				xQueueOverwrite(qMove, &move); // 写入队列
			}
			if (rx[0] == 0) // 无效
			{
				move.valid = 0;
				move.MoveDir = 0;
				xQueueOverwrite(qMove, &move);
			}
        }
    }
}
