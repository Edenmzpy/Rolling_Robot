#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "cmsis_os.h"

#include "pump_charge.h"
#include "adc.h"
#include "tim.h"

static uint16_t adc_buf[1];//用于存储从ADC读取的传感器数据
TaskHandle_t s_pumpTaskHandle = NULL; //创建空句柄
QueueHandle_t qPressure = NULL;
QueueHandle_t qPumpCmd = NULL;

extern QueueHandle_t qMax_pre;//引用外部消息队列，from nrf_recv.c

/* 软件PWM参数 */
#define CTRL_PERIOD_MS      20      // 控制周期 20ms
#define SOFT_PWM_PERIOD_MS  200     // 软件PWM窗口 200ms
#define PRESSURE_NEAR_TH    1.0f    // 距离目标1.0以内开始PID/细调
#define DUTY_MAX            100.0f
#define DUTY_MIN            0.0f

static void pump_on(void)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
}

static void pump_off(void)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
}

static float clampf(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

void charge_task(void *params)
{
	s_pumpTaskHandle = xTaskGetCurrentTaskHandle();//为当前任务赋值句柄
	qPressure = xQueueCreate(1, sizeof(float)); // 创建气压队列
	qPumpCmd  = xQueueCreate(1, sizeof(PumpCmd_t));
	
	// 开启ADC和定时器，周期性触发adc，并且将adc得到的值通过dma外设赋值在adc_buf中（相当与一个后台运行）
	HAL_TIM_Base_Start(&htim8);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, 1);
	
	float pressure = 0.0f;
    uint8_t max_pre = 10;
	
	/* PID参数 */
    float kp = 60.0f;
    float ki = 8.0f;
    float kd = 5.0f;

    float err = 0.0f, last_err = 0.0f;
    float integral = 0.0f;
    float derivative = 0.0f;
    float duty = 0.0f;

    uint8_t charging = 0;
	TickType_t lastWakeTime = xTaskGetTickCount();

    PumpCmd_t cmd = PUMP_CMD_NONE;
	
	while (1)
	{
		/* 非阻塞读取命令 */
        if (xQueueReceive(qPumpCmd, &cmd, 0) == pdPASS)
        {
            if (cmd == PUMP_CMD_START)
            {
                charging = 1;
                integral = 0.0f;
                last_err = 0.0f;
            }
            else if (cmd == PUMP_CMD_STOP)
            {
                charging = 0;
                pump_off();
                integral = 0.0f;
                last_err = 0.0f;
            }
        }

        /* 读取当前压力和目标压力 */
        xQueuePeek(qPressure, &pressure, 0);
        xQueuePeek(qMax_pre, &max_pre, 0);

        if (charging)
        {
            float target = (float)max_pre;
            err = target - pressure;

            if (err <= 0.0f)
            {
                /* 达到或超过目标，停止 */
                pump_off();
                integral = 0.0f;
                last_err = 0.0f;
            }
            else if (err > PRESSURE_NEAR_TH)
            {
                /* 误差较大，全速充气 */
                pump_on();
                integral = 0.0f;   // 防积分过大
                last_err = err;
            }
			else if ((err <= PRESSURE_NEAR_TH) && (err > 0.0f))

            {
                /* 目标附近：PID -> duty */
                float dt = CTRL_PERIOD_MS / 1000.0f;

                integral += err * dt;
                integral = clampf(integral, -5.0f, 5.0f);   // 抗积分饱和

                derivative = (err - last_err) / dt;
                last_err = err;

                duty = kp * err + ki * integral + kd * derivative;
                duty = clampf(duty, DUTY_MIN, DUTY_MAX);

                /* 软件PWM */
                uint32_t on_time  = (uint32_t)(SOFT_PWM_PERIOD_MS * duty / 100.0f);
                uint32_t off_time = SOFT_PWM_PERIOD_MS - on_time;

                if (on_time > 0)
                {
                    pump_on();
                    vTaskDelay(pdMS_TO_TICKS(on_time));
                }

                if (off_time > 0)
                {
                    pump_off();
                    vTaskDelay(pdMS_TO_TICKS(off_time));
                }

                /* 已经delay过了，本轮直接continue，避免再走下面固定周期delay */
                continue;
            }
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(CTRL_PERIOD_MS));
    
	}
}

// ADC中断回调函数读取气压传感器
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC1)
	{
		static uint32_t acc = 0;
		static uint8_t cnt = 0;

		acc += adc_buf[0];
		if (++cnt >= 10) // 取10次平均值
		{
			uint16_t avg = (uint16_t)(acc / 10u);
			float volt = ((float)avg / 4095) * 3.3;
			float pre = (volt - 0.2) * (100 / (2.7 - 0.2)); // 电压转气压
			
			acc = 0;
			cnt = 0;

			BaseType_t hpw = pdFALSE;
			xQueueOverwriteFromISR(qPressure, &pre, &hpw); // 写入气压队列
			portYIELD_FROM_ISR(hpw);
		}	
	}
}
