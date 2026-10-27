/* ATSAMD21G18A minimal application. Derived framework: Microchip Harmony 3.
 * Internal open-loop DFLL ~48 MHz; no external crystal required.
 * SysTick is polled, so do not add long blocking tasks here.
 */
#include <stdint.h>
#include <stddef.h>
#include "definitions.h"
#include "board.h"

volatile uint32_t g_uptime_ms = 0U;
volatile uint32_t g_heartbeat = 0U;
/* 静态缓冲区，发送完成前保持有效 */
static uint8_t s_tx_message[] = "ATSAMD21G18A UART OK\r\n";

/* 可在调试器 Watch 中观察 */
volatile bool g_uart_started = false;
volatile bool g_uart_done = false;
int main(void)
{
    uint32_t elapsed = 0U;
    SYS_Initialize(NULL);

    g_uart_started = SERCOM0_USART_Write(
        s_tx_message,
        sizeof(s_tx_message) - 1U
    );
    if (g_uart_started)
    {
        /* 初次联调：等待串口驱动完成发送 */
        while (SERCOM0_USART_WriteIsBusy())
        {
        }
        g_uart_done = true;
    }
    while (true)
    {
        /* 后续业务代码 */
    }
}
