/*********************************************************************************************************
* 模块名称：Debug_printf.c
* 摘    要：调试打印模块，基于DMA的非阻塞串口打印实现
* 当前版本：1.0.0
* 作    者：Rengar
* 完成日期：2024年09月17日 
* 内    容：
* 注    意：                                                                  
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/
/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "debug_printf.h"
#include "gd32f30x_conf.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
//#define DEBUG_TX_BUFFER_SIZE       512   // 环形缓冲区大小
#define DEBUG_DMA_BUFFER_SIZE      128   // DMA发送缓冲区大小
#define DEBUG_DMA_STATE_IDLE       0
#define DEBUG_DMA_STATE_BUSY       1

/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
#pragma pack(1) 
typedef struct
{
	uint8_t ring_buffer[DEBUG_TX_BUFFER_SIZE];     // 环形缓冲区
	uint8_t dma_tx_buffer[DEBUG_DMA_BUFFER_SIZE];  // DMA发送缓冲区（独立）
	uint8_t dma_tx_state;                         // DMA发送状态
	uint16_t head;                                // 环形缓冲区写指针
	uint16_t tail;                                // 环形缓冲区读指针
}DEBUG_DataInfo;
#pragma pack() 

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/
static DEBUG_DataInfo g_debug_DataInfo;

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void debug_dma_init(void);
static void debug_dma_send(uint8_t *p_buff, uint16_t data_len);
static void debug_dma_irq_handler(void);
static uint16_t debug_buffer_available(void);
static uint16_t debug_buffer_available_unsafe(uint16_t head, uint16_t tail);
static size_t debug_encoded_len(const uint8_t *data, size_t len, int with_newline_expand);

static int debug_buffer_get(uint8_t *data);
static int debug_buffer_put_packet(const uint8_t *data, size_t len, int with_newline_expand);
/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：debug_dma_init
* 函数功能：调试串口DMA初始化
* 输入参数：void
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
static void debug_dma_init(void)
{
	dma_parameter_struct dma_init_struct;
	rcu_periph_clock_enable(RCU_DMA0);
	
	/* 初始化DMA通道(USART2 TX) */
	dma_deinit(DMA0, DMA_CH1);
	
	dma_init_struct.periph_addr  = (uint32_t)(&USART_DATA(USART2));
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT; 
	dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.memory_addr  = NULL;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.number       = 0;
	dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.priority     = DMA_PRIORITY_ULTRA_HIGH;
	
	dma_init(DMA0, DMA_CH1, dma_init_struct);
	
	/* 配置DMA模式 */
	dma_circulation_disable(DMA0, DMA_CH1);
	dma_memory_to_memory_disable(DMA0, DMA_CH1);
	
	/* 使能USART DMA发送 */
	usart_dma_transmit_config(USART2, USART_DENT_ENABLE);
	
	/* 配置DMA中断 */
	nvic_irq_enable(DMA0_Channel1_IRQn, 2, 0);
	dma_interrupt_enable(DMA0, DMA_CH1, DMA_INT_FTF | DMA_INT_ERR);
	
	g_debug_DataInfo.dma_tx_state = DEBUG_DMA_STATE_IDLE;
}

/*********************************************************************************************************
* 函数名称：debug_dma_send
* 函数功能：调试串口DMA发送数据
* 输入参数：p_buff - 要发送的数据缓冲区，data_len - 数据长度
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
static void debug_dma_send(uint8_t *p_buff, uint16_t data_len)
{
	//if(data_len >= DEBUG_TX_BUFFER_SIZE || DEBUG_DMA_STATE_BUSY == g_debug_DataInfo.dma_tx_state)
	//	return;
  if ((data_len == 0U) ||
			(data_len > DEBUG_DMA_BUFFER_SIZE) ||
			(g_debug_DataInfo.dma_tx_state == DEBUG_DMA_STATE_BUSY)) {
			return;
	}
	memcpy(g_debug_DataInfo.dma_tx_buffer, p_buff, data_len);
	g_debug_DataInfo.dma_tx_state = DEBUG_DMA_STATE_BUSY;
	
	/* 配置DMA传输 */
	dma_channel_disable(DMA0, DMA_CH1);
	    /* 重新配置并启动DMA发送 */
    //dma_channel_disable(DMA0, DMA_CH1);
  dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_G);
	dma_memory_address_config(DMA0, DMA_CH1, (uint32_t)&g_debug_DataInfo.dma_tx_buffer[0]);
	dma_transfer_number_config(DMA0, DMA_CH1, data_len);
	dma_channel_enable(DMA0, DMA_CH1);
}

/*********************************************************************************************************
* 函数名称：debug_buffer_available
* 函数功能：获取环形缓冲区可用空间
* 输入参数：void
* 输出参数：void
* 返 回 值：可用空间字节数        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
static uint16_t debug_buffer_available(void)
{
	__disable_irq();
	uint16_t head = g_debug_DataInfo.head;
	uint16_t tail = g_debug_DataInfo.tail;
	__enable_irq();
	
	return debug_buffer_available_unsafe(head, tail);
}

/*********************************************************************************************************
* 函数名称：debug_buffer_available_unsafe
* 函数功能：在已知head/tail时计算环形缓冲区可用空间（调用方保证并发安全）
* 输入参数：head - 写指针，tail - 读指针
* 输出参数：void
* 返 回 值：可用空间字节数
* 创建日期：2026年03月27日
* 注    意：
*********************************************************************************************************/
static uint16_t debug_buffer_available_unsafe(uint16_t head, uint16_t tail)
{
	if (head >= tail) {
		return DEBUG_TX_BUFFER_SIZE - (head - tail) - 1U;
	}
	return tail - head - 1U;
}
/*********************************************************************************************************
* 函数名称：debug_encoded_len
* 函数功能：计算写入长度（可选换行展开）
* 输入参数：data - 数据指针，len - 原始长度，with_newline_expand - 是否将\n扩展为\r\n
* 输出参数：void
* 返 回 值：编码后长度
* 创建日期：2026年03月27日
* 注    意：
*********************************************************************************************************/
static size_t debug_encoded_len(const uint8_t *data, size_t len, int with_newline_expand)
{
	size_t encoded_len = len;

	if((with_newline_expand != 0) && (data != NULL)) {
		for(size_t i = 0U; i < len; ++i) {
			if(data[i] == (uint8_t)'\n') {
				encoded_len += 1U;
			}
		}
	}
	return encoded_len;
}

/*********************************************************************************************************
* 函数名称：debug_buffer_used
* 函数功能：获取环形缓冲区已使用空间
* 输入参数：void
* 输出参数：void
* 返 回 值：已使用空间字节数        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
 uint16_t debug_buffer_used(void)
{
	__disable_irq();
	uint16_t head = g_debug_DataInfo.head;
	uint16_t tail = g_debug_DataInfo.tail;
	__enable_irq();
	
	if (head >= tail) {
		return head - tail;
	} else {
		return DEBUG_TX_BUFFER_SIZE - (tail - head);
	}
}
/*********************************************************************************************************
* 函数名称：debug_buffer_put_packet
* 函数功能：按“整帧”写入环形缓冲区，空间不足时整帧丢弃
* 输入参数：data - 数据指针，len - 数据长度，with_newline_expand - 是否将\n扩展为\r\n
* 输出参数：void
* 返 回 值：成功返回1，失败返回0
* 创建日期：2026年03月27日
* 注    意：
*********************************************************************************************************/
static int debug_buffer_put_packet(const uint8_t *data, size_t len, int with_newline_expand)
{
	if((len > 0U) && (data == NULL)) {
		return 0;
	}

	size_t encoded_len = debug_encoded_len(data, len, with_newline_expand);
	if((encoded_len == 0U) || (encoded_len >= DEBUG_TX_BUFFER_SIZE)) {
		return 0;
	}

	__disable_irq();
	uint16_t head = g_debug_DataInfo.head;
	uint16_t tail = g_debug_DataInfo.tail;
	uint16_t available = debug_buffer_available_unsafe(head, tail);
	if(encoded_len > (size_t)available) {
		__enable_irq();
		return 0;
	}

	for(size_t i = 0U; i < len; ++i) {
		if((with_newline_expand != 0) && (data[i] == (uint8_t)'\n')) {
			g_debug_DataInfo.ring_buffer[head] = (uint8_t)'\r';
			head = (uint16_t)((head + 1U) % DEBUG_TX_BUFFER_SIZE);
		}
		g_debug_DataInfo.ring_buffer[head] = data[i];
		head = (uint16_t)((head + 1U) % DEBUG_TX_BUFFER_SIZE);
	}
	g_debug_DataInfo.head = head;
	__enable_irq();
	return 1;
}


/*********************************************************************************************************
* 函数名称：debug_buffer_get
* 函数功能：从环形缓冲区读取数据
* 输入参数：data - 读取的数据指针
* 输出参数：void
* 返 回 值：成功返回1，失败返回0        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
static int debug_buffer_get(uint8_t *data)
{
	__disable_irq();
	if (g_debug_DataInfo.head == g_debug_DataInfo.tail) {
		__enable_irq();
		return 0; /* 缓冲区空 */
	}
	
	*data = g_debug_DataInfo.ring_buffer[g_debug_DataInfo.tail];
	g_debug_DataInfo.tail = (g_debug_DataInfo.tail + 1) % DEBUG_TX_BUFFER_SIZE;
	__enable_irq();
	return 1;
}

/*********************************************************************************************************
* 函数名称：DMA0_Channel1_IRQHandler
* 函数功能：DMA中断处理函数
* 输入参数：void
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
void DMA0_Channel1_IRQHandler(void)
{
	if(dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INTF_ERRIF) != RESET)
	{
		dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INTF_ERRIF);
		g_debug_DataInfo.dma_tx_state = DEBUG_DMA_STATE_IDLE;
	}
	
	if(dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INTF_FTFIF) != RESET)
	{
		dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_FTF);
		g_debug_DataInfo.dma_tx_state = DEBUG_DMA_STATE_IDLE;
		//debug_tx_task();
	}
}

/*********************************************************************************************************
* 函数名称：debug_printf_init
* 函数功能：调试打印初始化
* 输入参数：baudrate - 波特率
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
void debug_printf_init(uint32_t baudrate)
{
	if(0U == baudrate) {
		baudrate = DEBUG_PRINTF_DEFAULT_BAUDRATE;
	}

	/* 初始化环形缓冲区 */
	g_debug_DataInfo.head = 0;
	g_debug_DataInfo.tail = 0;
	g_debug_DataInfo.dma_tx_state = DEBUG_DMA_STATE_IDLE;

	/* 初始化GPIO */
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(RCU_USART2);

	/* 配置USART引脚 */
	gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
	//gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_11);

	/* 配置USART */
	usart_deinit(USART2);
	usart_baudrate_set(USART2, baudrate);
	usart_word_length_set(USART2, USART_WL_8BIT);
	usart_stop_bit_set(USART2, USART_STB_1BIT);
	usart_parity_config(USART2, USART_PM_NONE);
	usart_hardware_flow_rts_config(USART2, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(USART2, USART_CTS_DISABLE);
	usart_receive_config(USART2, USART_RECEIVE_DISABLE);
	usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
	usart_enable(USART2);

#if DEBUG_TX_DMA_ENABLE
	/* 初始化DMA */
	debug_dma_init();
#endif
}

/*********************************************************************************************************
* 函数名称：debug_printf_deinit
* 函数功能：调试打印反初始化
* 输入参数：void
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
void debug_printf_deinit(void)
{
	usart_disable(USART2);
	gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
	gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
	rcu_periph_clock_disable(RCU_USART2);
	
#if DEBUG_TX_DMA_ENABLE
	dma_channel_disable(DMA0, DMA_CH1);
	nvic_irq_disable(DMA0_Channel1_IRQn);
	rcu_periph_clock_disable(RCU_DMA0);
#endif
}

/*********************************************************************************************************
* 函数名称：debug_putchar
* 函数功能：发送单个字符
* 输入参数：ch - 要发送的字符
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
void debug_putchar(char ch)
{
(void)debug_buffer_put_packet((const uint8_t *)&ch, 1U, 1);
}
/*********************************************************************************************************
* 函数名称：debug_write_raw
* 函数功能：发送原始数据（不做换行转换）
* 输入参数：data - 数据指针，len - 数据长度
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年03月26日
* 注    意：
*********************************************************************************************************/
void debug_write_raw(const uint8_t *data, size_t len)
{
	(void)debug_buffer_put_packet(data, len, 0);
}

/*********************************************************************************************************
* 函数名称：debug_write
* 函数功能：发送数据
* 输入参数：data - 数据指针，len - 数据长度
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
void debug_write(const uint8_t *data, size_t len)
{
(void)debug_buffer_put_packet(data, len, 1);
}

/*********************************************************************************************************
* 函数名称：debug_vprintf
* 函数功能：格式化输出（可变参数版本）
* 输入参数：fmt - 格式字符串，args - 参数列表
* 输出参数：void
* 返 回 值：输出的字符数        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
int debug_vprintf(const char *fmt, va_list args)
{
	char buffer[128];
	int written = vsnprintf(buffer, sizeof(buffer), fmt, args);
	
	if(written < 0) {
		return written;
	}

	size_t to_write = (written < (int)sizeof(buffer)) ? (size_t)written : (size_t)sizeof(buffer) - 1U;
	
	if (to_write > 0) {
		debug_write((const uint8_t *)buffer, to_write);
	}

	if(written >= (int)sizeof(buffer)) {
		static const char trunc_marker[] = "...";
		debug_write((const uint8_t *)trunc_marker, sizeof(trunc_marker) - 1U);
	}

	return written;
}

/*********************************************************************************************************
* 函数名称：debug_printf
* 函数功能：格式化输出
* 输入参数：fmt - 格式字符串，... - 可变参数
* 输出参数：void
* 返 回 值：输出的字符数        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
#if defined(__GNUC__)
int debug_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
int debug_printf(const char *fmt, ...);
#endif

int debug_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int written = debug_vprintf(fmt, args);
	va_end(args);
	return written;
}

/*********************************************************************************************************
* 函数名称：debug_hexdump
* 函数功能：十六进制数据转储
* 输入参数：data - 数据指针，len - 数据长度
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
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

/*********************************************************************************************************
* 函数名称：debug_tx_task
* 函数功能：DMA发送任务处理
* 输入参数：void
* 输出参数：void
* 返 回 值：void        
* 创建日期：2026年03月27日
* 注    意：
*********************************************************************************************************/
void debug_tx_task1(void)
{
	uint16_t used = debug_buffer_used();
	if(used > 0 && g_debug_DataInfo.dma_tx_state == DEBUG_DMA_STATE_IDLE) {
		uint16_t transfer_len = (used > 128) ? 128 : used;
		uint8_t temp_buffer[128];
		
		/* 从环形缓冲区读取数据到临时缓冲区 */
		for(uint16_t i = 0; i < transfer_len; i++) {
			uint8_t data;
			if(debug_buffer_get(&data)) {
				temp_buffer[i] = data;
			}
		}
		
		/* 启动DMA发送 */
		debug_dma_send(temp_buffer, transfer_len);
	}
}
void debug_tx_task(void)
{
	uint16_t used = debug_buffer_used();
	if ((used > 0U) && (g_debug_DataInfo.dma_tx_state == DEBUG_DMA_STATE_IDLE)) {
		uint16_t transfer_len = (used > DEBUG_DMA_BUFFER_SIZE) ? DEBUG_DMA_BUFFER_SIZE : used;
		uint8_t temp_buffer[DEBUG_DMA_BUFFER_SIZE];
		uint16_t copied = 0U;

		/* 从环形缓冲区读取数据到临时缓冲区 */
		for (uint16_t i = 0U; i < transfer_len; i++) {
			uint8_t data;
			if (debug_buffer_get(&data)) {
				temp_buffer[copied++] = data;
			} else {
				break;
			}
		}

		/* 启动DMA发送（只发送成功取出的数据，避免发出脏数据） */
		if (copied > 0U) {
			debug_dma_send(temp_buffer, copied);
		}
	}
}
/*********************************************************************************************************
* 函数名称：debug_tx_busy
* 函数功能：检查发送是否繁忙
* 输入参数：void
* 输出参数：void
* 返 回 值：繁忙返回1，空闲返回0        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
int debug_tx_busy(void)
{
	return (g_debug_DataInfo.dma_tx_state == DEBUG_DMA_STATE_BUSY) || (debug_buffer_used() > 0);
}

/*********************************************************************************************************
* 函数名称：debug_tx_flush
* 函数功能：强制刷新发送缓冲区
* 输入参数：void
* 输出参数：void
* 返 回 值：void        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
void debug_tx_flush(void)
{
	while(debug_tx_busy()) {
		debug_tx_task();
	}
}

/*********************************************************************************************************
* 函数名称：debug_tx_available
* 函数功能：获取发送缓冲区剩余空间
* 输入参数：void
* 输出参数：void
* 返 回 值：剩余空间字节数        
* 创建日期：2024年09月17日
* 注    意：
*********************************************************************************************************/
int debug_tx_available(void)
{
	return debug_buffer_available();
}