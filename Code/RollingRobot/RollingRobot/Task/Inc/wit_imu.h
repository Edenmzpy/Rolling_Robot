#ifndef __WIT_IMU_H__
#define __WIT_IMU_H__

#include "stm32f4xx_hal.h"

typedef struct // 姿态角度数据包
{
    TickType_t ts;
    float roll, pitch, yaw;   // 单位：deg
} AnglePkt;

typedef struct // 姿态四元数数据包
{
    TickType_t ts;
    float Q0, Q1, Q2, Q3;
} QuaPkt;

typedef struct // 加速度数据包
{
    TickType_t ts;
    float ax, ay, az;
} AccPkt;

typedef struct // 陀螺仪数据包
{
    TickType_t ts;
    float gx, gy, gz;
} GyroPkt;

typedef struct // 单位向量格式
{
    float x, y, z;
} Vec3;

typedef struct
{
    uint8_t foot_id;  // 脚编号
    float   angle;    // 在世界 XY 平面上的方位角 atan2(y, x)，单位：rad
} FootPkt;

typedef struct {
    FootPkt feet[3];   // 队列打包
} FeetPkt;

typedef struct
{
    TickType_t ts;
    float vx;
    float vy;
    float v_dir;      // 沿控制器方向的速度
    float speed_xy;   // 水平速度大小
} VelPkt;

void imu_task(void *params);
#endif
