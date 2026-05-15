#ifndef __SW_I2C_H_
#define __SW_I2C_H_

#include <stdint.h>

typedef struct
{
	void (*scl_wr)(uint8_t);
	void (*sda_wr)(uint8_t);
	uint8_t (*sda_rd)(void);
} sw_i2c_t;

void sw_i2c_start(const sw_i2c_t *i2c);
void sw_i2c_stop(const sw_i2c_t *i2c);
void sw_i2c_send_ack(const sw_i2c_t *i2c, uint8_t ack);
uint8_t sw_i2c_recv_ack(const sw_i2c_t *i2c);
void sw_i2c_send_byte(const sw_i2c_t *i2c, uint8_t byte);
uint8_t sw_i2c_recv_byte(const sw_i2c_t *i2c);

#endif
