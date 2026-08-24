/* debug_printf.c - 非阻塞调试串口 (USART2, PB10/PB11) */
#include "debug_printf.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//1KB 发送环形缓冲。以 115200 波特、每帧状态 ~80 字节、500ms 一帧来看余量充足
#define DEBUG_BUF_SIZE  (1024U)

static char s_tx_buf[DEBUG_BUF_SIZE];
//head=生产者写指针，tail=消费者(发送)读指针，均 volatile（被主循环两处访问）。但无临界区保护：当前仅任务上下文调用 debug_printf，安全；
//一旦将来在 ISR（如 5kHz ADC 节拍、1kHz 控制节拍）里加打印，head/tail 的「读-改-写」非原子会丢槽/乱序。属后续扩展隐患（P3）。
static volatile uint16_t s_tx_head = 0U;
static volatile uint16_t s_tx_tail = 0U;

void debug_printf_init(uint32_t baudrate)
{
    rcu_periph_clock_enable(DEBUG_USART_GPIO_RCU);
    rcu_periph_clock_enable(DEBUG_USART_RCU);
    rcu_periph_clock_enable(RCU_AF); //使能 AFIO 时钟。本设计未用重映射，严格说 PB10/11 的 AF 选择不依赖 RCU_AF，无害可保留。
	
    /* USART2 is routed to the board connector on PB10/PB11, not PA2/PA3. 
			ST32F303是这种USART2 对应的是PA2 PA3,GD32F303不需要重映射就是PB10,PB11*/
   // gpio_pin_remap_config(GPIO_USART2_FULL_REMAP, ENABLE);
	
    gpio_init(DEBUG_USART_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, DEBUG_USART_TX_PIN); //TX 复用推挽，正确。
    gpio_init(DEBUG_USART_RX_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, DEBUG_USART_RX_PIN);  //	RX 上拉输入，正确（悬空时不会误触发）。

    usart_deinit(DEBUG_USART); //先复位再配置，规范。
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
    char line[256]; //栈上 256 字节临时缓冲。使用场景全部在任务上下文（super-loop 内），栈深度足够；若将来在 ISR 调用需确认栈。
    va_list args;
    va_start(args, fmt); 
    int len = vsnprintf(line, sizeof(line), fmt, args); //用 vsnprintf（带长度上限）而非 sprintf，安全，无栈溢出。
    va_end(args);
    if (len <= 0) return;

		if((size_t)len >= sizeof(line)) //截断保护：vsnprintf 返回「本应写入长度」，≥256 即已截断，这里把可拷贝长度 clamp 到 255。逻辑正确。
		{
			len = (int)sizeof(line) - 1;
		}
    for (int i = 0; i < len; i++) { //	逐字符入环形缓冲。
        uint16_t next = (s_tx_head + 1U) % DEBUG_BUF_SIZE;
        if (next == s_tx_tail) break;  /* 缓冲区满, 丢弃 */
        s_tx_buf[s_tx_head] = line[i]; 	//写入并推进 head。
        s_tx_head = next; 
    }
}

void debug_tx_task(void) //发送任务由主循环调用
{
	//仅当「有数据」且「发送保持寄存器空」时才发，非阻塞。
    while ((s_tx_tail != s_tx_head) && (SET == usart_flag_get(DEBUG_USART,USART_FLAG_TBE)))
		{    
			usart_data_transmit(DEBUG_USART, (uint8_t)s_tx_buf[s_tx_tail]); //发送一个字节
        s_tx_tail = (s_tx_tail + 1U) % DEBUG_BUF_SIZE; //推进到tail里面
    }
}

uint32_t debug_buffer_used(void) //	返回已用缓冲长度，供主循环判断是否需要发、也可用于诊断阻塞。
{
    return (uint32_t)((s_tx_head + DEBUG_BUF_SIZE - s_tx_tail) % DEBUG_BUF_SIZE);
}

bool debug_getchar(char *ch)
{
	  if ((ch == NULL) || (RESET == usart_flag_get(DEBUG_USART, USART_FLAG_RBNE))) {
        return false;
    }
    *ch = (char)(usart_data_receive(DEBUG_USART) & 0xFFU); //读数据寄存器并屏蔽。
    return true;
}
