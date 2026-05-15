#include "nrf24l01.h"
#include "spi.h"
#include "cmsis_os.h"
#include "debug_printf.h"

const uint8_t NRF_RX_ADDR[5] = {0x34,0x43,0x10,0x10,0x01};
const uint8_t NRF_TX_ADDR[5] = {0x34,0x43,0x10,0x10,0x01};

uint8_t rx[NRF_PAYLOAD_LEN] = {0};
uint8_t tx[NRF_PAYLOAD_LEN] = {0};

static uint8_t spi_xfer(uint8_t b)
{
    uint8_t r;
    HAL_SPI_TransmitReceive(&hspi2, &b, &r, 1, 1000);
    return r;
}

static void reg_write(uint8_t reg, uint8_t val)
{
    NRF24L01_CSN_L;
    spi_xfer(NRF_WRITE_REG + (reg & 0x1F));
    spi_xfer(val);
    NRF24L01_CSN_H;
}

static uint8_t reg_read(uint8_t reg)
{
    uint8_t v;
    NRF24L01_CSN_L;
    spi_xfer(NRF_READ_REG + (reg & 0x1F));
    v = spi_xfer(0xFF);
    NRF24L01_CSN_H;
    return v;
}

static void buf_write(uint8_t reg, const uint8_t *p, int len)
{
    NRF24L01_CSN_L;
    spi_xfer(NRF_WRITE_REG + (reg & 0x1F));
    for(int i=0;i<len;i++) spi_xfer(p[i]);
    NRF24L01_CSN_H;
}

static void buf_read(uint8_t reg, uint8_t *p, int len)
{
    NRF24L01_CSN_L;
    spi_xfer(NRF_READ_REG + (reg & 0x1F));
    for(int i=0;i<len;i++) p[i] = spi_xfer(0xFF);
    NRF24L01_CSN_H;
}

uint8_t nrf24_read_status(void)
{
    return reg_read(STATUS);
}

void nrf24_clear_irqs(void)
{
    reg_write(STATUS, RX_DR | TX_DS | MAX_RT);
}

void nrf24_write_txpayload(const uint8_t *buf)
{
    NRF24L01_CSN_L;
    spi_xfer(NRF_WR_TX_PAYLOAD);
    for(int i = 0; i < NRF_PAYLOAD_LEN; i++) spi_xfer(buf[i]);
    NRF24L01_CSN_H;
}

void nrf24_read_rxpayload(uint8_t *out32)
{
	NRF24L01_CSN_L;
	spi_xfer(NRF_RD_RX_PAYLOAD);
	for (int i = 0; i < NRF_PAYLOAD_LEN; ++i) out32[i] = spi_xfer(0xFF);
	NRF24L01_CSN_H;
}

void nrf24_flush_rx(void)
{
    NRF24L01_CSN_L;
	spi_xfer(NRF_FLUSH_RX);
	NRF24L01_CSN_H;
}

void nrf24_flush_tx(void)
{
    NRF24L01_CSN_L;
	spi_xfer(NRF_FLUSH_TX);
	NRF24L01_CSN_H;
}

// 自检测试
int nrf24_check(void)
{
    uint8_t bak[5];
    buf_read(TX_ADDR_REG, bak, 5);

    uint8_t test[5] = {'B','E','E','F','!'};
    buf_write(TX_ADDR_REG, test, 5);

    uint8_t rd[5];
    buf_read(TX_ADDR_REG, rd, 5);

    buf_write(TX_ADDR_REG, bak, 5);

    for(int i=0;i<5;i++)
	{
        if(rd[i] != test[i]) return -1;
    }
    return 0;
}

// 通用基础配置
void nrf24_init(void)
{
    NRF24L01_CE_L;
	
	reg_write(CONFIG, 0x08);
	reg_write(EN_AA, 0x3F);
	reg_write(EN_RXADDR, 0x01);
	reg_write(SETUP_AW, 0x03);
	reg_write(SETUP_RETR, 0x03);
	reg_write(RF_CH, 0x02);
	reg_write(RF_SETUP, 0x0E);
	
	buf_write(RX_ADDR_P0, NRF_RX_ADDR, 5);
    buf_write(TX_ADDR_REG, NRF_TX_ADDR, 5);
	
	reg_write(RX_PW_P0 , NRF_PAYLOAD_LEN);
	
    vTaskDelay(pdMS_TO_TICKS(2));
	
	nrf24_rxmode();
}

// 接收模式初始化
void nrf24_rxmode(void)
{
	uint8_t Config;
	
	NRF24L01_CE_L;
	
	Config = reg_read(CONFIG);
	Config |= 0x03;
	reg_write(CONFIG, Config);

    NRF24L01_CE_H;
}

// 发送模式初始化
void nrf24_txmode(void)
{
	uint8_t Config;
	
	NRF24L01_CE_L;
	
	Config = reg_read(CONFIG);	
	Config |= 0x02;
	Config &= ~0x01;
	reg_write(CONFIG, Config);
	
    NRF24L01_CE_H;
}

// 阻塞发送
int nrf24_send32(const uint8_t *buf, uint32_t timeout_ms)
{
	
	buf_write(TX_ADDR_REG, NRF_TX_ADDR, 5);
	nrf24_write_txpayload(buf);
	buf_write(RX_ADDR_P0, NRF_RX_ADDR, 5);
	
	nrf24_txmode();

    // 等待发送完成
    uint32_t t0 = HAL_GetTick();
    int res = -2;
    while ((HAL_GetTick() - t0) <= timeout_ms) {
        uint8_t st = nrf24_read_status();
        if (st & TX_DS) { res = 0;  break; }
        if (st & MAX_RT){ res = -1; break; }
    }
	
	nrf24_clear_irqs();
	nrf24_flush_tx();
	
	buf_write(RX_ADDR_P0, NRF_RX_ADDR, 5);
	
	nrf24_rxmode();
	
	return res;
}

// 接收
int nrf24_receive32(uint8_t *out32)
{
    uint8_t st = nrf24_read_status();
    if (st & RX_DR)
	{
		nrf24_read_rxpayload(out32);
		
        nrf24_clear_irqs();
        return 0;
    }
	
    uint8_t fifo = reg_read(FIFO_STATUS);
    if ((fifo & 0x01) == 0)
	{
		nrf24_read_rxpayload(out32);
		
        nrf24_clear_irqs();
        return 0;
    }
    return -1;
}

static void nrf_print_bytes(const char *tag, const uint8_t *p, int n){
    debug_printf("%s=", tag);
    for(int i=0;i<n;i++) debug_printf("%02X", p[i]);
    debug_printf("\r\n");
}

static void nrf_read_addr(uint8_t reg, uint8_t out[5]){
    NRF24L01_CSN_L;
    spi_xfer(NRF_READ_REG + (reg & 0x1F));
    for(int i=0;i<5;i++) out[i] = spi_xfer(0xFF);
    NRF24L01_CSN_H;
}

void nrf24_dump_tx_side(void){
    uint8_t st   = reg_read(STATUS);
    uint8_t ob   = reg_read(OBSERVE_TX); // ARC_CNT/PLOS_CNT
    uint8_t cfg  = reg_read(CONFIG);
    uint8_t rf   = reg_read(RF_SETUP);
    uint8_t ch   = reg_read(RF_CH);
    uint8_t retr = reg_read(SETUP_RETR);
    uint8_t fifo = reg_read(FIFO_STATUS);
    uint8_t aw   = reg_read(SETUP_AW);
    uint8_t txa[5], rxa0[5];
    nrf_read_addr(TX_ADDR_REG, txa);
    nrf_read_addr(RX_ADDR_P0 , rxa0);

    debug_printf("[TX] STATUS=%02X OBSERVE=%02X (ARC=%u PLOS=%u) CFG=%02X RF_SETUP=%02X CH=%u RETR=%02X FIFO=%02X AW=%02X\r\n",
           st, ob, ob & 0x0F, (ob>>4)&0x0F, cfg, rf, ch, retr, fifo, aw);
    nrf_print_bytes("[TX] TX_ADDR", txa, 5);
    nrf_print_bytes("[TX] RX_ADDR_P0", rxa0, 5);
}

void nrf24_dump_rx_side(void){
    uint8_t st   = reg_read(STATUS);
    uint8_t cfg  = reg_read(CONFIG);
    uint8_t enaa = reg_read(EN_AA);
    uint8_t enrx = reg_read(EN_RXADDR);
    uint8_t rf   = reg_read(RF_SETUP);
    uint8_t ch   = reg_read(RF_CH);
    uint8_t fifo = reg_read(FIFO_STATUS);
    uint8_t pw0  = reg_read(RX_PW_P0);
    uint8_t aw   = reg_read(SETUP_AW);
    uint8_t rxa0[5];
    nrf_read_addr(RX_ADDR_P0, rxa0);

    debug_printf("[RX] STATUS=%02X CFG=%02X EN_AA=%02X EN_RXADDR=%02X RF_SETUP=%02X CH=%u FIFO=%02X RX_PW_P0=%u AW=%02X\r\n",
           st, cfg, enaa, enrx, rf, ch, fifo, pw0, aw);
    nrf_print_bytes("[RX] RX_ADDR_P0", rxa0, 5);
}

