#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "cmsis_os.h"
#include "servo_move.h"
#include <math.h>
#include <stdint.h>
#include "nrf_recv.h"
#include "wit_imu.h"
#include "wit_c_sdk.h"
#include "wit_uart.h"
#include "debug_printf.h"
#include "oled.h"
#include "nrf24l01.h"

#define ROBOT_RADIUS_M  0.30f
extern QueueHandle_t qMove;      	// from nrf_recv.c

static const float Rfix[3][3] = {
    { 1,  0,  0},
    { 0, -1,  0},
    { 0,  0, -1}
};

// 顶点单位向量，机体坐标系下的单位向量
const Vec3 vertex[20] =
{
	{     0,     0,     1},
	{     0, -0.66,  0.74},
	{ -0.58,  0.33,  0.74},
	{  0.58,  0.33,  0.74},
	{  0.58, -0.74,  0.33},
	{  0.93, -0.13,  0.33},
	{  0.36,  0.87,  0.33},
	{ -0.36,  0.87,  0.33},
	{  0.36, -0.87, -0.33},
	{  0.93,  0.13, -0.33},
	{  0.58,  0.74, -0.33},
	{ -0.58,  0.74, -0.33},
	{  0.58, -0.33, -0.74},
	{     0,  0.67, -0.74},
	{     0,     0,    -1},
	{ -0.93, -0.13,  0.33},
	{ -0.58, -0.74,  0.33},
	{ -0.93,  0.13, -0.33},
	{ -0.36, -0.87, -0.33},
	{ -0.58, -0.33, -0.74}
};

// 顶点-足端映射，舵机编号映射
const uint8_t vertex_to_feet[20][3] = 
{
	{0, 1, 2},
	{0, 1, 11},
	{0, 2, 10},
	{1, 2, 3},
	{1, 4, 11},
	{1, 3, 4},
	{2, 3, 6},
	{2, 6, 10},
	{4, 8, 11},
	{3, 4, 5},
	{3, 5, 6},
	{6, 7, 10},
	{4, 5, 8},
	{5, 6, 7},
	{5, 7, 8},
	{0, 9, 10},
	{0, 9, 11},
	{7, 9, 10},
	{8, 9, 11},
	{7, 8, 9}
};

// 足端单位向量，也是机体坐标系下的
const Vec3 feet[12] = 
{
	{ -0.5284, -0.3043,  0.7926},
	{  0.5190, -0.3133,  0.7953},
	{  0.0105,  0.6074,  0.7943},
	{  0.8555,  0.4818,  0.1898},
	{  0.8520, -0.4888, -0.1877},
	{  0.5284,  0.3043, -0.7926},
	{  0.0147,  0.9820, -0.1885},
	{ -0.5241,  0.3081, -0.7940},
	{ -0.0000, -0.6047, -0.7964},
	{ -0.8555, -0.4818, -0.1898},
	{ -0.8517,  0.4894,  0.1875},
	{ -0.0010, -0.9812,  0.1929}
};

extern UART_HandleTypeDef huart2;
// 消息队列，只存最新的（覆盖），取用使用glance
QueueHandle_t qAngle = NULL; // 最新角度
QueueHandle_t qQuat = NULL; // 最新四元数
QueueHandle_t qAcc = NULL; // 最新加速度
QueueHandle_t qGyro = NULL; // 最新陀螺仪
QueueHandle_t qDir = NULL; // 最新解算方向
QueueHandle_t qFeet = NULL; // 最新足端数据
QueueHandle_t qVel = NULL;//最新的速度数据
// SDK发送/延时回调
static void SensorUartSend(uint8_t *p, uint32_t n)
{
    HAL_UART_Transmit(&huart2, p, n, HAL_MAX_DELAY);
}

static void Delayms(uint16_t ms)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
	{
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else 
	{
        HAL_Delay(ms);
    }
}//检测任务调度是否启动，未启动则整机延时。

// 数据处理回调
static void SensorDataUpdata(uint32_t reg, uint32_t num)
{
    BaseType_t hpw = pdFALSE;
    for (uint32_t i = 0; i < num; ++i, ++reg)
	{
        // 欧拉角
        if (reg == Yaw)
		{  // Roll,Pitch,Yaw 到齐
            AnglePkt pkt;
            pkt.ts    = xTaskGetTickCountFromISR();
            pkt.roll  = sReg[Roll]  / 32768.0f * 180.0f;
            pkt.pitch = sReg[Pitch] / 32768.0f * 180.0f;
            pkt.yaw   = sReg[Yaw]   / 32768.0f * 180.0f;
            if (qAngle) xQueueOverwriteFromISR(qAngle, &pkt, &hpw);//覆盖消息队列数据，存在一个叫pkt的Anglepkt结构体里面，在覆写到qangle里
        }

        // 四元数
        if (reg == q3)
		{   // Q0~Q3 到齐
            QuaPkt qpkt;
            qpkt.ts = xTaskGetTickCountFromISR();
            qpkt.Q0 = sReg[q0] / 32768.0f;
            qpkt.Q1 = sReg[q1] / 32768.0f;
            qpkt.Q2 = sReg[q2] / 32768.0f;
            qpkt.Q3 = sReg[q3] / 32768.0f;
            if (qQuat) xQueueOverwriteFromISR(qQuat, &qpkt, &hpw);
        }
		
		// 加速度
		if (reg == AZ)
		{
			AccPkt apkt;
			apkt.ts = xTaskGetTickCountFromISR();
			apkt.ax = sReg[AX] / 32768.0f * 16.0f;
			apkt.ay = sReg[AY] / 32768.0f * 16.0f;
			apkt.az = sReg[AZ] / 32768.0f * 16.0f;
			if (qAcc) xQueueOverwriteFromISR(qAcc, &apkt, &hpw);
		}
		
		// 陀螺仪
		if (reg == GZ)
		{
			GyroPkt gpkt;
			gpkt.ts = xTaskGetTickCountFromISR();
			gpkt.gx = sReg[GX] / 32768.0f * 2000.0f;
			gpkt.gy = sReg[GY] / 32768.0f * 2000.0f;
			gpkt.gz = sReg[GZ] / 32768.0f * 2000.0f;
			if (qGyro) xQueueOverwriteFromISR(qGyro, &gpkt, &hpw);
		}
    }
    portYIELD_FROM_ISR(hpw);
}

static float dot2(float ax, float ay, float bx, float by)
{
    return ax * bx + ay * by;
}
//平面点乘函数
static Vec3 cross(Vec3 a, Vec3 b)
{
    Vec3 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}
//三维叉乘函数

static Vec3 scale(Vec3 a, float k)
{
    Vec3 r;
    r.x = a.x * k;
    r.y = a.y * k;
    r.z = a.z * k;
    return r;
}
//顶点向量×旋转半径

static Vec3 move_dir_to_world(float MoveDir)
{
    float rad = MoveDir;

    Vec3 d;
    d.x = cosf(rad);
    d.y = sinf(rad);
    d.z = 0.0f;

    return d;
}
//平面映射函数
// 由四元数求旋转矩阵
static void quat_to_Rwb(const QuaPkt *q, float R[3][3])
{
    float w = q->Q0, x = q->Q1, y = q->Q2, z = q->Q3;

    R[0][0] = 1.0f - 2.0f*(y*y + z*z);
    R[0][1] = 2.0f*(x*y - w*z);
    R[0][2] = 2.0f*(x*z + w*y);

    R[1][0] = 2.0f*(x*y + w*z);
    R[1][1] = 1.0f - 2.0f*(x*x + z*z);
    R[1][2] = 2.0f*(y*z - w*x);

    R[2][0] = 2.0f*(x*z - w*y);
    R[2][1] = 2.0f*(y*z + w*x);
    R[2][2] = 1.0f - 2.0f*(x*x + y*y);
}

// 世界坐标->机体坐标变换 (Vbody = (Rwb)T * Vworld)
static Vec3 world_to_body(float Rwb[3][3], Vec3 v_world)
{
    Vec3 v_body;
    v_body.x = Rwb[0][0]*v_world.x + Rwb[1][0]*v_world.y + Rwb[2][0]*v_world.z;
    v_body.y = Rwb[0][1]*v_world.x + Rwb[1][1]*v_world.y + Rwb[2][1]*v_world.z;
    v_body.z = Rwb[0][2]*v_world.x + Rwb[1][2]*v_world.y + Rwb[2][2]*v_world.z;
    return v_body;
}

// 机体坐标->世界坐标变化 (Vworld = (Rwb) * Vbody)
static Vec3 body_to_world(float Rwb[3][3], Vec3 v_body)
{
    Vec3 v_world;
    v_world.x = Rwb[0][0]*v_body.x + Rwb[0][1]*v_body.y + Rwb[0][2]*v_body.z;
    v_world.y = Rwb[1][0]*v_body.x + Rwb[1][1]*v_body.y + Rwb[1][2]*v_body.z;
    v_world.z = Rwb[2][0]*v_body.x + Rwb[2][1]*v_body.y + Rwb[2][2]*v_body.z;
    return v_world;
}

// 区分朝下顶点
static uint8_t dir_classify(float Rwb[3][3])
{
	Vec3 gravity = {0, 0, -1}; // 世界z轴向下坐标
	Vec3 vec = world_to_body(Rwb, gravity);
	
	uint8_t best = 20;
	float bestDot = .2f;
	for (uint8_t i = 0; i < 20; i++) // 与20个顶点向量求点积
	{
		float s = vertex[i].x * vec.x + vertex[i].y * vec.y + vertex[i].z * vec.z;
		if (s > bestDot)
		{
			best = i;
			bestDot = s;
		}
	}
	return best;
}

// 求三个足端在世界xy平面角度
static void feet_angle(float Rwb[3][3], uint8_t dir, FeetPkt* out)
{
	// 查询顶点所对应三个足端
    uint8_t f0 = vertex_to_feet[dir][0];
    uint8_t f1 = vertex_to_feet[dir][1];
    uint8_t f2 = vertex_to_feet[dir][2];
    uint8_t fs[3] = { f0, f1, f2 };

    for (int i = 0; i < 3; ++i)
    {
        uint8_t fid = fs[i];
        Vec3 n_body  = feet[fid];
        Vec3 n_world = body_to_world(Rwb, n_body);

        float hx = n_world.x;
        float hy = n_world.y;
        float ang = atan2f(hy, hx);

        out->feet[i].foot_id = fid;
        out->feet[i].angle   = ang;
    }
}

// 用陀螺仪估算机器人速度
static void calc_robot_velocity(
    float Rwb[3][3],
    uint8_t down_vertex,
    GyroPkt *gyro,
    float MoveDir,
    VelPkt *vel
)
{
    // 1. 陀螺仪 deg/s -> rad/s
    Vec3 omega_body;
    omega_body.x = gyro->gx * 3.1415926f / 180.0f;
    omega_body.y = gyro->gy * 3.1415926f / 180.0f;
    omega_body.z = gyro->gz * 3.1415926f / 180.0f;

    // 2. 角速度转世界坐标
    Vec3 omega_world = body_to_world(Rwb, omega_body);

    // 3. 当前朝下顶点作为接地点方向
    Vec3 r_body = scale(vertex[down_vertex], ROBOT_RADIUS_M);
    Vec3 r_world = body_to_world(Rwb, r_body);

    // 4. 滚动速度 v = -ω × r
    Vec3 v = cross(omega_world, r_world);

    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;

    // 5. 控制器目标方向
    Vec3 cmd_dir = move_dir_to_world(MoveDir);

    vel->ts = xTaskGetTickCount();
    vel->vx = v.x;
    vel->vy = v.y;
    vel->speed_xy = sqrtf(v.x * v.x + v.y * v.y);

    // 6. 投影到目标方向，得到沿 dir 的速度
    vel->v_dir = dot2(v.x, v.y, cmd_dir.x, cmd_dir.y);
}

void imu_task(void *params)
{	
    // 创建队列
    qAngle = xQueueCreate(1, sizeof(AnglePkt));
    qQuat = xQueueCreate(1, sizeof(QuaPkt));
	qAcc = xQueueCreate(1, sizeof(AccPkt));
	qGyro = xQueueCreate(1, sizeof(GyroPkt));
	qDir = xQueueCreate(1, sizeof(uint8_t));
	qFeet = xQueueCreate(1, sizeof(FeetPkt));
	qVel = xQueueCreate(1, sizeof(VelPkt));
	//结构体定义在头文件里

    // 串口和SDK初始化
    wit_uart_init();
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitSerialWriteRegister(SensorUartSend);
    WitRegisterCallBack(SensorDataUpdata);
    WitDelayMsRegister(Delayms);

    // 配置输出：欧拉角 + 四元数 + 加速度 +陀螺仪，50Hz
    (void)WitSetContent(RSW_ANGLE | RSW_Q | RSW_ACC | RSW_GYRO);
    (void)WitSetOutputRate(RRATE_50HZ);
//设置好回调之后应该一直在后台运行了，类似于中断，adc之类的
//	AnglePkt ang;
	QuaPkt quat;
	GyroPkt gyro;
	MovePkt mv = {0};
	
	uint8_t dir; // 顶点分类结果
	float Rwb[3][3]; // 旋转矩阵
	char buf[32];

    while (1)
	{	
		BaseType_t res = xQueuePeek(qQuat, &quat, portMAX_DELAY); // 读取四元数处理数据
        if (res == pdTRUE)
		{	float Rwb_raw[3][3];
			quat_to_Rwb(&quat, Rwb_raw);
			
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					Rwb[i][j] = Rwb_raw[i][0]*Rfix[0][j] +
								 Rwb_raw[i][1]*Rfix[1][j] +
								 Rwb_raw[i][2]*Rfix[2][j];
				}
			}
						
			dir = dir_classify(Rwb);
			
			
			FeetPkt ground_feet;
			feet_angle(Rwb, dir, &ground_feet);

			if (qMove)     (void)xQueuePeek(qMove, &mv, 0);
			if (qGyro && xQueuePeek(qGyro, &gyro, 0) == pdTRUE)
			{
				VelPkt vel;

				if (mv.valid)
				{
					calc_robot_velocity(Rwb, dir, &gyro, mv.MoveDir, &vel);
				}
				else
				{
					vel.ts = xTaskGetTickCount();
					vel.vx = 0.0f;
					vel.vy = 0.0f;
					vel.v_dir = 0.0f;
					vel.speed_xy = 0.0f;
				}

				xQueueOverwrite(qVel, &vel);
			}

			xQueueOverwrite(qDir, &dir);
			xQueueOverwrite(qFeet, &ground_feet);
        }
    }
}
