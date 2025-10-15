#include "debug_printf.h"


#if DEBUG_PRINTF_ENABLE

#include "gd32f30x_conf.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef DEBUG_USART_PERIPH
#define DEBUG_USART_PERIPH        USART0
#endif

#ifndef DEBUG_USART_RCU
#define DEBUG_USART_RCU           RCU_USART0
#endif

#ifndef DEBUG_USART_GPIO_PORT
#define DEBUG_USART_GPIO_PORT     GPIOA
#endif

#ifndef DEBUG_USART_GPIO_RCU
#define DEBUG_USART_GPIO_RCU      RCU_GPIOA
#endif

#ifndef DEBUG_USART_TX_PIN
#define DEBUG_USART_TX_PIN        GPIO_PIN_9
#endif

#ifndef DEBUG_USART_RX_PIN
#define DEBUG_USART_RX_PIN        GPIO_PIN_10
#endif

#ifndef DEBUG_PRINTF_BUFFER_SIZE
#define DEBUG_PRINTF_BUFFER_SIZE  128U
#endif


static void debug_usart_send(uint8_t data)
{
    while(RESET == usart_flag_get(DEBUG_USART_PERIPH, USART_FLAG_TBE))
    {
    }
    usart_data_transmit(DEBUG_USART_PERIPH, data);
}

void debug_printf_init(uint32_t baudrate)
{
    if(0U == baudrate)
    {
        baudrate = DEBUG_PRINTF_DEFAULT_BAUDRATE;
    }

    rcu_periph_clock_enable(DEBUG_USART_GPIO_RCU);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(DEBUG_USART_RCU);

    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, DEBUG_USART_TX_PIN);
    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, DEBUG_USART_RX_PIN);

    usart_deinit(DEBUG_USART_PERIPH);
    usart_baudrate_set(DEBUG_USART_PERIPH, baudrate);
    usart_word_length_set(DEBUG_USART_PERIPH, USART_WL_8BIT);
    usart_stop_bit_set(DEBUG_USART_PERIPH, USART_STB_1BIT);
    usart_parity_config(DEBUG_USART_PERIPH, USART_PM_NONE);
    usart_hardware_flow_rts_config(DEBUG_USART_PERIPH, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(DEBUG_USART_PERIPH, USART_CTS_DISABLE);
    usart_receive_config(DEBUG_USART_PERIPH, USART_RECEIVE_ENABLE);
    usart_transmit_config(DEBUG_USART_PERIPH, USART_TRANSMIT_ENABLE);
    usart_enable(DEBUG_USART_PERIPH);
}
void debug_printf_deinit(void)
{
    usart_disable(DEBUG_USART_PERIPH);
    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, DEBUG_USART_TX_PIN);
    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, DEBUG_USART_RX_PIN);
    rcu_periph_clock_disable(DEBUG_USART_RCU);
}

void debug_putchar(char ch)
{
    if('\n' == ch)
    {
        debug_usart_send('\r');
    }
    debug_usart_send((uint8_t)ch);
}

void debug_write(const uint8_t *data, size_t len)
{
    for(size_t i = 0U; i < len; ++i)
    {
        debug_putchar((char)data[i]);
    }
}
int debug_vprintf(const char *fmt, va_list args)
{
    char buffer[DEBUG_PRINTF_BUFFER_SIZE];
    int written = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if(written < 0)
    {
        return written;
    }

    size_t to_write = (written < (int)sizeof(buffer)) ? (size_t)written : (size_t)sizeof(buffer) - 1U;
    debug_write((const uint8_t *)buffer, to_write);

    if(written >= (int)sizeof(buffer))
    {
        static const char trunc_marker[] = "...";
        debug_write((const uint8_t *)trunc_marker, sizeof(trunc_marker) - 1U);
    }

    return written;
}
int debug_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = debug_vprintf(fmt, args);
    va_end(args);
    return written;
}
void debug_hexdump(const void *data, size_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;
    size_t offset = 0U;

    while(offset < len)
    {
        size_t line_len = len - offset;
        if(line_len > 16U)
        {
            line_len = 16U;
        }

        debug_printf("%08X: ", (unsigned int)offset);
        for(size_t i = 0U; i < line_len; ++i)
        {
            debug_printf("%02X ", bytes[offset + i]);
        }
        for(size_t i = line_len; i < 16U; ++i)
        {
            debug_printf("   ");
        }

        debug_printf(" |");
        for(size_t i = 0U; i < line_len; ++i)
        {
            char c = (char)bytes[offset + i];
            if((c < 32) || (c > 126))
            {
                c = '.';
            }
            debug_putchar(c);
        }
        debug_printf("|\n");

        offset += line_len;
    }
}
#endif









