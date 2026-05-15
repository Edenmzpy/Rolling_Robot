#include "delay_us.h"

void dwt_init(void)
{
    /* 使能 DWT CYCCNT */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // 打开跟踪单元
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            // 使能周期计数
}

void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (SystemCoreClock / 1000000U) * us;  // 需要的CPU周期数
    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
        __NOP();  // 可选：给编译器一个指令避免过度优化
    }
}
