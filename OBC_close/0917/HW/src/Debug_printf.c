#include "debug_printf.h"
#include "gd32f30x_conf.h"
#include <stdarg.h>
#include <stdio.h>

#if DEBUG_PRINTF_ENABLE

#ifndef DEBUG_USART_PERIPH
#define DEBUG_USART_PERIPH        USART2
#endif

#ifndef DEBUG_USART_RCU
#define DEBUG_USART_RCU           RCU_USART2
#endif

#ifndef DEBUG_USART_GPIO_PORT
#define DEBUG_USART_GPIO_PORT     GPIOB
#endif

#ifndef DEBUG_USART_GPIO_RCU
#define DEBUG_USART_GPIO_RCU      RCU_GPIOB
#endif

#ifndef DEBUG_USART_TX_PIN
#define DEBUG_USART_TX_PIN        GPIO_PIN_10
#endif

#ifndef DEBUG_USART_RX_PIN
#define DEBUG_USART_RX_PIN        GPIO_PIN_11
#endif

#ifndef DEBUG_PRINTF_BUFFER_SIZE
#define DEBUG_PRINTF_BUFFER_SIZE  128U
#endif

/* DMA配置 */
#ifndef DEBUG_USART_DMA_PERIPH
#define DEBUG_USART_DMA_PERIPH    DMA0
#endif

#ifndef DEBUG_USART_DMA_CHANNEL
#define DEBUG_USART_DMA_CHANNEL   DMA_CH1
#endif

#ifndef DEBUG_USART_DMA_RCU
#define DEBUG_USART_DMA_RCU       RCU_DMA0
#endif

#ifndef DEBUG_USART_DMA_IRQn
#define DEBUG_USART_DMA_IRQn      DMA0_Channel1_IRQn
#endif

/* 声明外部变量 */
extern uint32_t g_ms;
static void debug_start_dma_transfer(void);
/* 环形缓冲区结构 */
typedef struct {
    uint8_t buffer[DEBUG_TX_BUFFER_SIZE];
    volatile uint16_t head;          /* 写指针 */
    volatile uint16_t tail;          /* 读指针 */
    volatile uint8_t dma_busy;       /* DMA传输状态 */
    uint8_t dma_buffer[128];         /* DMA传输缓冲区（放大到128字节） */
    uint16_t dma_len;                /* 当前DMA传输长度 */
} debug_tx_buffer_t;

static debug_tx_buffer_t s_tx_buf;

/* 环形缓冲区操作函数 */
static inline uint16_t debug_tx_buffer_free(void)
{
    __disable_irq();
    uint16_t head = s_tx_buf.head;
    uint16_t tail = s_tx_buf.tail;
    __enable_irq();
    
    if (head >= tail) {
        return DEBUG_TX_BUFFER_SIZE - (head - tail) - 1;
    } else {
        return tail - head - 1;
    }
}

static inline uint16_t debug_tx_buffer_used(void)
{
    __disable_irq();
    uint16_t head = s_tx_buf.head;
    uint16_t tail = s_tx_buf.tail;
    __enable_irq();
    
    if (head >= tail) {
        return head - tail;
    } else {
        return DEBUG_TX_BUFFER_SIZE - (tail - head);
    }
}

static int debug_tx_buffer_put(uint8_t data)
{
    __disable_irq();
    uint16_t next_head = (s_tx_buf.head + 1) % DEBUG_TX_BUFFER_SIZE;
    
    if (next_head == s_tx_buf.tail) {
        __enable_irq();
        return 0; /* 缓冲区满 */
    }
    
    s_tx_buf.buffer[s_tx_buf.head] = data;
    s_tx_buf.head = next_head;
    __enable_irq();
    return 1;
}

static int debug_tx_buffer_get(uint8_t *data)
{
    __disable_irq();
    if (s_tx_buf.head == s_tx_buf.tail) {
        __enable_irq();
        return 0; /* 缓冲区空 */
    }
    
    *data = s_tx_buf.buffer[s_tx_buf.tail];
    s_tx_buf.tail = (s_tx_buf.tail + 1) % DEBUG_TX_BUFFER_SIZE;
    __enable_irq();
    return 1;
}

/* DMA中断处理函数 */
static void debug_dma_irq_handler(void)
{
    __disable_irq();
    if(dma_interrupt_flag_get(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, DMA_INT_FLAG_FTF);
        s_tx_buf.dma_busy = 0;
        
        /* 自动续传下一包数据 */
        if (debug_tx_buffer_used() > 0) {
            /* 重新启用DMA传输 */
            debug_start_dma_transfer();
        }
    }
    __enable_irq();
}

/* 真实的DMA中断向量函数 */
void DMA0_Channel1_IRQHandler(void)
{
    debug_dma_irq_handler();
}

/* 启动DMA传输 */
static void debug_start_dma_transfer(void)
{
    if (s_tx_buf.dma_busy || debug_tx_buffer_used() == 0) {
        return;
    }
    
    /* 计算要传输的数据量 */
    uint16_t available = debug_tx_buffer_used();
    uint16_t transfer_len = (available > sizeof(s_tx_buf.dma_buffer)) ? 
                           sizeof(s_tx_buf.dma_buffer) : available;
    
    /* 从环形缓冲区复制数据到DMA缓冲区 */
    for (uint16_t i = 0; i < transfer_len; i++) {
        uint8_t data;
        if (debug_tx_buffer_get(&data)) {
            s_tx_buf.dma_buffer[i] = data;
        }
    }
    
    /* 配置DMA传输 */
    dma_channel_disable(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL);
    
    /* 使用GD32F30x标准DMA API */
    dma_memory_address_config(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, 
                             (uint32_t)s_tx_buf.dma_buffer);
    dma_memory_width_config(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, DMA_MEMORY_WIDTH_8BIT);
    dma_memory_increase_enable(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL);
    
    dma_periph_address_config(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, USART_DATA(DEBUG_USART_PERIPH));
    dma_periph_width_config(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, DMA_PERIPHERAL_WIDTH_8BIT);
    dma_periph_increase_disable(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL);
    
    dma_transfer_number_config(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, transfer_len);
    dma_priority_config(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, DMA_PRIORITY_LOW);
    
    /* 使能DMA传输完成中断 */
    dma_interrupt_enable(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, DMA_INT_FTF);
    
    /* 启动DMA传输 */
    dma_channel_enable(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL);
    usart_dma_transmit_config(DEBUG_USART_PERIPH, USART_DENT_ENABLE);
    
    s_tx_buf.dma_busy = 1;
    s_tx_buf.dma_len = transfer_len;
}

/* 非阻塞发送任务 */
void debug_tx_task(void)
{
    debug_start_dma_transfer();
}

/* 检查发送是否繁忙 */
int debug_tx_busy(void)
{
    return s_tx_buf.dma_busy || (debug_tx_buffer_used() > 0);
}

/* 强制刷新发送缓冲区 */
void debug_tx_flush(void)
{
    uint32_t timeout = g_ms + DEBUG_TX_TIMEOUT_MS;
    
    while (debug_tx_busy() && (g_ms < timeout)) {
        debug_tx_task();
    }
}

/* 获取发送缓冲区剩余空间 */
int debug_tx_available(void)
{
    return debug_tx_buffer_free();
}

/* 非阻塞字符发送 */
static int debug_usart_send_nonblock(uint8_t data)
{
    return debug_tx_buffer_put(data);
}

/* 初始化函数 */
void debug_printf_init(uint32_t baudrate)
{
    if(0U == baudrate) {
        baudrate = DEBUG_PRINTF_DEFAULT_BAUDRATE;
    }

    /* 初始化环形缓冲区 */
    s_tx_buf.head = 0;
    s_tx_buf.tail = 0;
    s_tx_buf.dma_busy = 0;
    s_tx_buf.dma_len = 0;

    /* 初始化GPIO */
    rcu_periph_clock_enable(DEBUG_USART_GPIO_RCU);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(DEBUG_USART_RCU);

    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, DEBUG_USART_TX_PIN);
    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, DEBUG_USART_RX_PIN);

    /* 配置USART */
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

#if DEBUG_TX_DMA_ENABLE
    /* 初始化DMA */
    rcu_periph_clock_enable(DEBUG_USART_DMA_RCU);
    
    dma_deinit(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL);
    
    /* 声明并初始化DMA配置结构体 */
    dma_parameter_struct dma_init_struct;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_LOW;
    dma_init_struct.number = 0;
    dma_init_struct.periph_addr = 0;
    dma_init_struct.memory_addr = 0;
    dma_init(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL, dma_init_struct);
    
    /* 配置DMA中断 */
    nvic_irq_enable(DEBUG_USART_DMA_IRQn, 0, 0);
#endif
}

void debug_printf_deinit(void)
{
    usart_disable(DEBUG_USART_PERIPH);
    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, DEBUG_USART_TX_PIN);
    gpio_init(DEBUG_USART_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, DEBUG_USART_RX_PIN);
    rcu_periph_clock_disable(DEBUG_USART_RCU);
    
#if DEBUG_TX_DMA_ENABLE
    dma_channel_disable(DEBUG_USART_DMA_PERIPH, DEBUG_USART_DMA_CHANNEL);
    nvic_irq_disable(DEBUG_USART_DMA_IRQn);
    rcu_periph_clock_disable(DEBUG_USART_DMA_RCU);
#endif
}

/* 非阻塞字符发送 */
void debug_putchar(char ch)
{
    if('\n' == ch) {
        debug_usart_send_nonblock('\r');
    }
    debug_usart_send_nonblock((uint8_t)ch);
}

/* 非阻塞数据发送 */
void debug_write(const uint8_t *data, size_t len)
{
    for(size_t i = 0U; i < len; ++i) {
        debug_putchar((char)data[i]);
    }
#if DEBUG_TX_DMA_ENABLE
    debug_tx_task();
#endif
}

/* 丢弃数据计数器 */
static volatile uint32_t s_drop_count = 0U;

int debug_vprintf(const char *fmt, va_list args)
{
    char buffer[DEBUG_PRINTF_BUFFER_SIZE];
    int written = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if(written < 0) {
        return written;
    }

    size_t to_write = (written < (int)sizeof(buffer)) ? (size_t)written : (size_t)sizeof(buffer) - 1U;
    
    /* 写不下就丢并计数：只写能容纳的部分，剩余部分丢弃 */
    uint16_t available_space = debug_tx_buffer_free();
    if (available_space < to_write) {
        /* 丢弃超出部分，只写能容纳的数据 */
        to_write = available_space;
        s_drop_count++;
    }
    
    if (to_write > 0) {
        debug_write((const uint8_t *)buffer, to_write);
    }

    if(written >= (int)sizeof(buffer)) {
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

    while(offset < len) {
        size_t line_len = len - offset;
        if(line_len > 16U) {
            line_len = 16U;
        }

        debug_printf("%08X: ", (unsigned int)offset);
        for(size_t i = 0U; i < line_len; ++i) {
            debug_printf("%02X ", bytes[offset + i]);
        }
        for(size_t i = line_len; i < 16U; ++i) {
            debug_printf("   ");
        }

        debug_printf(" |");
        for(size_t i = 0U; i < line_len; ++i) {
            char c = (char)bytes[offset + i];
            if((c < 32) || (c > 126)) {
                c = '.';
            }
            debug_putchar(c);
        }
        debug_printf("|\n");

        offset += line_len;
    }
}

#endif



















