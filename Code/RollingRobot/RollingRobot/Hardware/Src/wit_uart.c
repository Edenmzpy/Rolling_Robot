#include "wit_uart.h"
#include "wit_c_sdk.h"

extern UART_HandleTypeDef huart2;
static uint8_t g_usart2_rx_byte;

void wit_uart_init(void)
{
    HAL_UART_Receive_IT(&huart2, &g_usart2_rx_byte, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
	{
        WitSerialDataIn(g_usart2_rx_byte);                 // 每字节喂给 SDK
        HAL_UART_Receive_IT(&huart2, &g_usart2_rx_byte, 1);// 继续下一字节
    }
}
