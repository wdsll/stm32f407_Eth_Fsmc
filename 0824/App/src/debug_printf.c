/* debug_printf.c - ·Ç×èÈûµ÷ÊÔ´®¿Ú (USART2, PB10/PB11) */
#include "debug_printf.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_BUF_SIZE  (1024U)

static char s_tx_buf[DEBUG_BUF_SIZE];
static volatile uint16_t s_tx_head = 0U;
static volatile uint16_t s_tx_tail = 0U;

void debug_printf_init(uint32_t baudrate)
{
    rcu_periph_clock_enable(DEBUG_USART_GPIO_RCU);
    rcu_periph_clock_enable(DEBUG_USART_RCU);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(DEBUG_USART_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, DEBUG_USART_TX_PIN);
    gpio_init(DEBUG_USART_RX_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, DEBUG_USART_RX_PIN);

    usart_deinit(DEBUG_USART);
    usart_baudrate_set(DEBUG_USART, baudrate);
    usart_word_length_set(DEBUG_USART, USART_WL_8BIT);
    usart_stop_bit_set(DEBUG_USART, USART_STB_1BIT);
    usart_parity_config(DEBUG_USART, USART_PM_NONE);
    usart_receive_config(DEBUG_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(DEBUG_USART, USART_TRANSMIT_ENABLE);
    usart_enable(DEBUG_USART);
}

void debug_printf(const char *fmt, ...)
{
    char line[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    if (len <= 0) return;

		if((size_t)len >= sizeof(line))
		{
			len = (int)sizeof(line) - 1;
		}
    for (int i = 0; i < len; i++) {
        uint16_t next = (s_tx_head + 1U) % DEBUG_BUF_SIZE;
        if (next == s_tx_tail) break;  /* »º³åÇøÂú, ¶ªÆú */
        s_tx_buf[s_tx_head] = line[i];
        s_tx_head = next;
    }
}

void debug_tx_task(void)
{
    while ((s_tx_tail != s_tx_head) && (SET == usart_flag_get(DEBUG_USART,USART_FLAG_TBE)))
		{    
        usart_data_transmit(DEBUG_USART, (uint8_t)s_tx_buf[s_tx_tail]);
        s_tx_tail = (s_tx_tail + 1U) % DEBUG_BUF_SIZE;
    }
}

uint32_t debug_buffer_used(void)
{
    return (uint32_t)((s_tx_head + DEBUG_BUF_SIZE - s_tx_tail) % DEBUG_BUF_SIZE);
}

bool debug_getchar(char *ch)
{
	  if ((ch == NULL) || (RESET == usart_flag_get(DEBUG_USART, USART_FLAG_RBNE))) {
        return false;
    }
    *ch = (char)(usart_data_receive(DEBUG_USART) & 0xFFU);
    return true;
}