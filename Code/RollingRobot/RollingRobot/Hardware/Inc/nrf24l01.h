#ifndef __NRF24L01_H__
#define __NRF24L01_H__

#include "stm32f4xx.h"

//通信引脚
#define IRQ_Port   GPIOD
#define CE_Port    GPIOD
#define MISO_Port  GPIOB
#define MOSI_Port  GPIOB
#define SCK_Port   GPIOB
#define CSN_Port   GPIOB

#define IRQ_Pin    GPIO_PIN_8
#define CE_Pin     GPIO_PIN_9
#define MOSI_Pin   GPIO_PIN_15
#define MISO_Pin   GPIO_PIN_14
#define SCK_Pin    GPIO_PIN_13
#define CSN_Pin    GPIO_PIN_12

//寄存器地址代码
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR_REG 0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD       0x1C
#define FEATURE     0x1D

//操作指令代码
#define NRF_READ_REG      0x00
#define NRF_WRITE_REG     0x20
#define NRF_RD_RX_PAYLOAD 0x61
#define NRF_WR_TX_PAYLOAD 0xA0
#define NRF_FLUSH_TX      0xE1
#define NRF_FLUSH_RX      0xE2
#define NRF_REUSE_TX_PL	  0xE3
#define NRF_NOP           0xFF

//状态
#define RX_DR       0x40 //接收到数据后，第6位会置1
#define TX_DS       0x20 //发送数据后，第5位会置1
#define MAX_RT      0x10 //发送数据满，第4位会置1

//宏定义GPIO状态
#define NRF24L01_CSN_L  	HAL_GPIO_WritePin(CSN_Port,CSN_Pin,(GPIO_PinState)0)
#define NRF24L01_CSN_H  	HAL_GPIO_WritePin(CSN_Port,CSN_Pin,(GPIO_PinState)1)
#define NRF24L01_CE_L   	HAL_GPIO_WritePin(CE_Port,CE_Pin,(GPIO_PinState)0)
#define NRF24L01_CE_H   	HAL_GPIO_WritePin(CE_Port,CE_Pin,(GPIO_PinState)1)
#define NRF24L01_IRQ_Read 	HAL_GPIO_ReadPin(IRQ_Port, IRQ_Pin)

#define NRF_PAYLOAD_LEN   32
#define NRF_RF_CHANNEL    40    // 2400 + 40 = 2440 MHz
#define NRF_RF_SETUP_VAL  0x0E  // 2Mbps, 0dBm
#define NRF_RETR_VAL      0x1A  // 500us间隔，重发10次（仅TX用）

extern uint8_t rx[NRF_PAYLOAD_LEN];
extern uint8_t tx[NRF_PAYLOAD_LEN];

void nrf24_clear_irqs(void);
int nrf24_check(void);
void nrf24_init(void);
void nrf24_rxmode(void);
void nrf24_txmode(void);
int nrf24_send32(const uint8_t *buf, uint32_t timeout_ms);
int nrf24_receive32(uint8_t *out32);

void nrf24_dump_tx_side(void);
void nrf24_dump_rx_side(void);

#endif
