#include "sw_i2c.h"

void sw_i2c_start(const sw_i2c_t *i2c)
{
	i2c->sda_wr(1);

	i2c->scl_wr(1);

	i2c->sda_wr(0);

	i2c->scl_wr(0);
}

void sw_i2c_stop(const sw_i2c_t *i2c)
{
	i2c->sda_wr(0);

	i2c->scl_wr(1);

	i2c->sda_wr(1);
}

void sw_i2c_send_ack(const sw_i2c_t *i2c, uint8_t ack)
{
	i2c->sda_wr(ack);

	i2c->scl_wr(1);

	i2c->scl_wr(0);
}

uint8_t sw_i2c_recv_ack(const sw_i2c_t *i2c)
{
	uint8_t ack = 1;

	i2c->sda_wr(1);

	i2c->scl_wr(1);

	ack = i2c->sda_rd();
	i2c->scl_wr(0);

	return ack;
}

void sw_i2c_send_byte(const sw_i2c_t *i2c, uint8_t byte)
{
	for (int8_t i = 7; i >= 0; --i)
	{
		i2c->sda_wr((byte >> i) & 0x01);

		i2c->scl_wr(1);

		i2c->scl_wr(0);
	}
}

uint8_t sw_i2c_recv_byte(const sw_i2c_t *i2c)
{
	uint8_t byte = 0;

	i2c->sda_wr(1);

	for (uint8_t i = 0; i < 8; ++i)
	{
		byte <<= 1;
		i2c->scl_wr(1);

		byte |= i2c->sda_rd();
		i2c->scl_wr(0);
	}

	return byte;
}
