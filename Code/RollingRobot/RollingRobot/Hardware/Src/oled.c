#include "oled.h"
#include "oled_font.h"
#include "delay_us.h"
#include "sw_i2c.h"

// Line 行位置，范围：1~4
// Column 列位置，范围：1~16

static void oled_scl_wr(uint8_t value)
{
	HAL_GPIO_WritePin(OLED_SCL_GPIO_Port, OLED_SCL_Pin, (GPIO_PinState)value);
	delay_us(2);
}

static void oled_sda_wr(uint8_t value)
{
	HAL_GPIO_WritePin(OLED_SDA_GPIO_Port, OLED_SDA_Pin, (GPIO_PinState)value);
}

static uint8_t oled_sda_rd(void)
{
	return HAL_GPIO_ReadPin(OLED_SDA_GPIO_Port, OLED_SDA_Pin);
}

static sw_i2c_t oled_i2c = {
	.scl_wr = oled_scl_wr,
	.sda_wr = oled_sda_wr,
	.sda_rd = oled_sda_rd
};

static void oled_write_command(uint8_t command)
{
	sw_i2c_start(&oled_i2c);
	sw_i2c_send_byte(&oled_i2c, 0x78);		//从机地址
	sw_i2c_recv_ack(&oled_i2c);             //应答不处理
	sw_i2c_send_byte(&oled_i2c, 0x00);		//写命令
	sw_i2c_recv_ack(&oled_i2c);
	sw_i2c_send_byte(&oled_i2c, command); 
	sw_i2c_recv_ack(&oled_i2c);
	sw_i2c_stop(&oled_i2c);
}

static void oled_write_data(uint8_t data)
{
	sw_i2c_start(&oled_i2c);
	sw_i2c_send_byte(&oled_i2c, 0x78);		//从机地址
	sw_i2c_recv_ack(&oled_i2c);
	sw_i2c_send_byte(&oled_i2c, 0x40);		//写数据
	sw_i2c_recv_ack(&oled_i2c);
	sw_i2c_send_byte(&oled_i2c, data);
	sw_i2c_recv_ack(&oled_i2c);
	sw_i2c_stop(&oled_i2c);
}

static void oled_set_cursor(uint8_t Y, uint8_t X)
{
	oled_write_command(0xB0 | Y);					//设置Y位置
	oled_write_command(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	oled_write_command(0x00 | (X & 0x0F));			//设置X位置低4位
}

void oled_clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		oled_set_cursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			oled_write_data(0x00);
		}
	}
}

void oled_show_char(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	oled_set_cursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		oled_write_data(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	oled_set_cursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		oled_write_data(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

void oled_show_string(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		oled_show_char(Line, Column + i, String[i]);
	}
}

static uint32_t oled_pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

void oled_show_num(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		oled_show_char(Line, Column + i, Number / oled_pow(10, Length - i - 1) % 10 + '0');
	}
}

void oled_show_signednum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		oled_show_char(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		oled_show_char(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		oled_show_char(Line, Column + i + 1, Number1 / oled_pow(10, Length - i - 1) % 10 + '0');
	}
}

void oled_show_hexnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / oled_pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			oled_show_char(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			oled_show_char(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

void oled_show_binnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		oled_show_char(Line, Column + i, Number / oled_pow(2, Length - i - 1) % 2 + '0');
	}
}

void oled_init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	oled_write_command(0xAE);	//关闭显示
	
	oled_write_command(0xD5);	//设置显示时钟分频比/振荡器频率
	oled_write_command(0x80);
	
	oled_write_command(0xA8);	//设置多路复用率
	oled_write_command(0x3F);
	
	oled_write_command(0xD3);	//设置显示偏移
	oled_write_command(0x00);
	
	oled_write_command(0x40);	//设置显示开始行
	
	oled_write_command(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	oled_write_command(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	oled_write_command(0xDA);	//设置COM引脚硬件配置
	oled_write_command(0x12);
	
	oled_write_command(0x81);	//设置对比度控制
	oled_write_command(0xCF);

	oled_write_command(0xD9);	//设置预充电周期
	oled_write_command(0xF1);

	oled_write_command(0xDB);	//设置VCOMH取消选择级别
	oled_write_command(0x30);

	oled_write_command(0xA4);	//设置整个显示打开/关闭

	oled_write_command(0xA6);	//设置正常/倒转显示

	oled_write_command(0x8D);	//设置充电泵
	oled_write_command(0x14);

	oled_write_command(0xAF);	//开启显示
		
	oled_clear();				//OLED清屏
}

