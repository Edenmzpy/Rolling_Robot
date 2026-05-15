#include "debug_printf.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define DEBUG_PRINTF_BUF_SIZE 256
static char debug_printf_buf[DEBUG_PRINTF_BUF_SIZE];

uint8_t debug_printf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    int n = vsnprintf(debug_printf_buf, sizeof(debug_printf_buf), fmt, arg);
    va_end(arg);
    if (n < 0) return 0;
    if (n > (int)sizeof(debug_printf_buf)) n = sizeof(debug_printf_buf);

    HAL_StatusTypeDef st = HAL_UART_Transmit(&huart1, (uint8_t*)debug_printf_buf, (uint16_t)n, 100);

    return (st == HAL_OK);
}
