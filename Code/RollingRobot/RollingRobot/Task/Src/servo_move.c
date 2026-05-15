#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "queue.h"

#include "tim.h"
#include <math.h>
#include <stdio.h>

#include "servo_move.h"
#include "wit_imu.h"
#include "nrf_recv.h"

// 引用外部消息队列
extern QueueHandle_t qMove; // from nrf_recv.c
extern QueueHandle_t qFeet; // from wit_imu.c

const uint16_t mid_pos[12] =
{
    1500,1500,1500,1500,
    1500,1500,1500,1500,
    1500,1500,1500,1500
}; // 对应舵机中位
const uint16_t long_pos[12] =
{ 
    2500,2500,2500,2500,
    2500,2500,2500,2500,
    2500,2500,2500,2500
};// 对应舵机伸长位
const uint16_t short_pos[12] =
{ 
    500,500,500,500,
    500,500,500,500,
    500,500,500,500
};// 对应舵机缩短位

// 12个足端当前位置缓存
static uint16_t servo_curr[12];

typedef struct
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
} ServoMap_t; // 舵机映射信息

// 舵机编号与定时器通道映射
static const ServoMap_t servo_map[12] =
{
    { &htim1, TIM_CHANNEL_1 }, // foot 0
    { &htim1, TIM_CHANNEL_2 }, // foot 1
    { &htim1, TIM_CHANNEL_3 }, // foot 2
    { &htim1, TIM_CHANNEL_4 }, // foot 3

    { &htim3, TIM_CHANNEL_1 }, // foot 4
    { &htim3, TIM_CHANNEL_2 }, // foot 5
    { &htim3, TIM_CHANNEL_3 }, // foot 6
    { &htim3, TIM_CHANNEL_4 }, // foot 7

    { &htim4, TIM_CHANNEL_1 }, // foot 8
    { &htim4, TIM_CHANNEL_2 }, // foot 9
    { &htim4, TIM_CHANNEL_3 }, // foot 10
    { &htim4, TIM_CHANNEL_4 }, // foot 11
};

// 返回需要伸长的足端下标
int choose_feet(MovePkt* move, FeetPkt* ground_feet)
{
    float dx = cosf(move->MoveDir);
    float dy = sinf(move->MoveDir);

    int best_idx = -1;
    float best_proj = 1e9f;

    for (int i = 0; i < 3; ++i)
    {
        float fa = ground_feet->feet[i].angle;
        float fx = cosf(fa);
        float fy = sinf(fa);

        float proj = fx * dx + fy * dy;
        if (proj < best_proj)
        {
            best_proj = proj;
            best_idx = i;
        }
    }
    return best_idx;
}

// PWM 初始化
static inline void servo_pwm_start(uint8_t foot_id)
{
    TIM_HandleTypeDef *htim = servo_map[foot_id].htim;
    uint32_t ch = servo_map[foot_id].channel;
    HAL_TIM_PWM_Start(htim, ch);
}

// 设置舵机占空比
static inline void servo_set_pulse(uint8_t foot_id, uint16_t pulse)
{
    TIM_HandleTypeDef *htim = servo_map[foot_id].htim;
    uint32_t ch = servo_map[foot_id].channel;
    __HAL_TIM_SET_COMPARE(htim, ch, pulse);
    servo_curr[foot_id] = pulse;
}

// 舵机初始化，设置中位
void servo_init_all(void)
{
    for (uint8_t id = 0; id < 12; ++id)
    {
        servo_pwm_start(id);
    }

    for (uint8_t id = 0; id < 12; ++id)
    {
        servo_set_pulse(id, mid_pos[id]);
    }
}

// 舵机分段平滑移动
void servo_move_slow(uint8_t ids[3], uint16_t targets[3], uint16_t step, TickType_t delay_tick)
{
    uint8_t finished;
    if (step == 0) step = 1;

    while (1)
    {
        finished = 1;
        for (int i = 0; i < 3; ++i)
        {
            uint8_t id = ids[i];
            if (id >= 12) continue;

            uint16_t cur = servo_curr[id];
            uint16_t tgt = targets[i];

            if (cur < tgt)
            {
                uint16_t next = cur + step;
                if (next > tgt) next = tgt;
                servo_set_pulse(id, next);
                if (next != tgt) finished = 0;
            }
            else if (cur > tgt)
            {
                uint16_t next = (cur > step) ? cur - step : 0;
                if (next < tgt) next = tgt;
                servo_set_pulse(id, next);
                if (next != tgt) finished = 0;
            }
        }

        if (finished) break;
        vTaskDelay(delay_tick);
    }
}

// 根据 back_index 控制伸长/缩短
void feet_move(FeetPkt* ground_feet, int back_index)
{
    uint8_t foot_ids[3];
    uint16_t targets[3];
    int j = 0;

    for (int i = 0; i < 3; ++i)
    {
        uint8_t id = ground_feet->feet[i].foot_id;

        if (i == back_index)
        {
            foot_ids[0] = id;
            targets[0] = long_pos[id];
        }
        else
        {
            foot_ids[j + 1] = id;
            targets[j + 1] = short_pos[id];
            ++j;
        }
    }

    // 平滑移动到目标
    servo_move_slow(foot_ids, targets, 75, 20);//舵机一次响应速度0.28s
    vTaskDelay(pdMS_TO_TICKS(1220));

    // 回中位
    uint16_t mid_targets[3];
    for (int i = 0; i < 3; ++i)
    {
        uint8_t id = foot_ids[i];
        mid_targets[i] = (id < 12) ? mid_pos[id] : 0;
    }
    servo_move_slow(foot_ids, mid_targets, 75, 20);
	vTaskDelay(pdMS_TO_TICKS(1220));
}//每一次运动3s


// 运动任务
void move_task(void *params)
{
    vTaskDelay(pdMS_TO_TICKS(500));

    servo_init_all();

    MovePkt move;
    FeetPkt ground_feet;
    while (1)
    {
        BaseType_t res = xQueueReceive(qMove, &move, portMAX_DELAY);
        if (res == pdTRUE)
        {
            if (move.valid == 1)
            {
                xQueuePeek(qFeet, &ground_feet, portMAX_DELAY);

                int back_index = choose_feet(&move, &ground_feet);

                feet_move(&ground_feet, back_index);
            }
        }
    }
}
