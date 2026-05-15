#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "cmsis_os.h"

#include <stdio.h>
#include <string.h>

#include "nrf_send.h"
#include "nrf24l01.h"
#include "oled.h"
#include "wit_imu.h"

// 引入外部相关数据队列
extern QueueHandle_t qVel;
extern QueueHandle_t qAngle;
extern QueueHandle_t qQuat;
extern QueueHandle_t qAcc;
extern QueueHandle_t qGyro;
extern QueueHandle_t qFeet;//from wit_imu
extern QueueHandle_t qDir;
extern QueueHandle_t qMax_pre;//from nrf_recv
extern QueueHandle_t qPressure;// from pump_charge
// 发送任务
void nrf_sendtask(void *para)
{
	vTaskDelay(pdMS_TO_TICKS(500));//任务启动之后等待，for 初始化操作或硬件准备
	uint8_t dir;
	float pressure;
	AnglePkt pkt;
	AccPkt apkt;
	uint8_t max_pre;
	VelPkt vel;
	char buf[32];
	while (1)
	{
		if (xQueuePeek(qPressure, &pressure, 0) == pdTRUE) // 读取队列写入tx，如果不为空
		{
			memcpy(&tx[1], &pressure, sizeof(float));
		}
		if (xQueuePeek(qMax_pre, &max_pre, 0) == pdTRUE)
		{
			tx[0] = max_pre;
		}
		if (qVel && xQueuePeek(qVel, &vel, 0) == pdTRUE)
        {
            memcpy(&tx[5], &vel.v_dir, sizeof(float));
        }
		
		nrf24_send32(tx, 50); // 发送数据
		vTaskDelay(50);
	}
}
