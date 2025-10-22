/*********************************************************************************************************
* 模块名称：ICU.c
* 摘    要：
* 当前版本：1.0.0
* 作    者：ICU.c
* 完成日期：2025年09月24日  
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
#include "ICU.h"
#include "gd32f30x_timer.h" 
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define CAP_TIMEOUT_MULTIPLIER   4U
#define CAP_MIN_TIMEOUT_TICKS    (0x10000U)

/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/

typedef struct
{
	uint32_t timer;
	uint16_t channel;
	uint32_t int_flag;
	uint32_t polarity_bit;
	uint32_t rise;
	uint32_t period;
	uint32_t timeout_deadline;
	float duty;
	uint8_t expect_fall;
	uint8_t have_rise;
	uint8_t updated;
}cap_channel_state_t;

enum
{
	CAP_CH_INDEX_PA3 = 0,
	CAP_CH_INDEX_PA1 = 1,
	CAP_CH_COUNT
};



/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static inline void cap_set_polarity(uint32_t timer,const volatile cap_channel_state_t* ch,uint16_t polarity);

static volatile cap_channel_state_t s_cap_channels[CAP_CH_COUNT];
static volatile uint32_t s_timer1_overflow = 0U;
/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：cap_polarity_bit
* 函数功能：其目的是根据输入的通道号（ ch ）返回对应的定时器通道优先级控制位（ TIMER_CHCTL2_CHxP ）
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年09月30日
* 注    意：表明这是一个静态内联函数，通常用于优化性能，避免函数调用的开销。
*********************************************************************************************************/

static inline uint32_t cap_polarity_bit(uint16_t ch) // 返回通道优先级控制位
{
	switch(ch)
	{
		case TIMER_CH_0:
			return TIMER_CHCTL2_CH0P; // 返回通道0优先级控制位
		case TIMER_CH_1:
			return TIMER_CHCTL2_CH1P;
		case TIMER_CH_2:
			return TIMER_CHCTL2_CH2P;
		case TIMER_CH_3:
			return TIMER_CHCTL2_CH3P;
		default:
			return 0;
	}
}

static inline uint32_t cap_timer_overflow_get(uint32_t timer)
{
	if (timer == TIMER1) {
		return s_timer1_overflow;
	}
	return 0U;
}

static inline void cap_timer_overflow_increment(uint32_t timer)
{
	if (timer == TIMER1) {
		++s_timer1_overflow;
	}
}

static inline uint32_t cap_timer_now(uint32_t timer)
{
	return (cap_timer_overflow_get(timer) << 16) | TIMER_CNT(timer);
}

static inline uint32_t cap_extend_timestamp(uint32_t timer, uint16_t capture)
{
	//其目的是将两个16位的计时器值（ timer 和 capture ）组合成一个32位的时间戳
	return (cap_timer_overflow_get(timer) << 16) | capture;
}
/*********************************************************************************************************
* 函数名称：cap_timeout_window
* 函数功能：主要目的是根据输入的 period_ticks （周期时间刻度）计算并返回一个“超时窗口”值。
这个窗口值会被限制在一个合理的范围内，避免过大或过小的情况
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年09月30日
* 注    意：表明这是一个静态内联函数，通常用于优化性能，避免函数调用的开销。
*********************************************************************************************************/
static inline uint32_t cap_timeout_window(uint32_t period_ticks)
{
	uint32_t base = period_ticks;
	if (base == 0U) {
					base = CAP_MIN_TIMEOUT_TICKS;
	}
	uint64_t window = (uint64_t)base * CAP_TIMEOUT_MULTIPLIER;
	if (window < CAP_MIN_TIMEOUT_TICKS) {
					window = CAP_MIN_TIMEOUT_TICKS;
	} else if (window > 0xFFFFFFFFULL) {
					window = 0xFFFFFFFFULL;
	}
	return (uint32_t)window;
}



static inline void cap_channel_handle_timeout(volatile cap_channel_state_t* ch, uint32_t now)
{
	if (!ch->have_rise) {
		return;
	}
	if ((int32_t)(now - ch->timeout_deadline) >= 0) {
		ch->have_rise = 0U;
		ch->expect_fall = 0U;
		ch->period = 0U;
		ch->duty = 0.0f;
		ch->updated = 1U;
	}
}
/*********************************************************************************************************
* 函数名称：cap_channel_reset_state
* 函数功能：重置通道状态
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年09月30日
* 注    意：表明这是一个静态内联函数，通常用于优化性能，避免函数调用的开销。
*********************************************************************************************************/
static inline void cap_channel_reset_state(volatile cap_channel_state_t* ch, uint32_t timer, uint16_t channel, uint32_t int_flag)
{
	ch->timer = timer;
	ch->channel = channel;
	ch->int_flag = int_flag;
	ch->polarity_bit = cap_polarity_bit(channel);
	ch->rise = 0U;
	ch->period = 0U;
	ch->timeout_deadline = 0U;
	ch->duty = 0.0f;
	ch->expect_fall = 0U;
	ch->have_rise = 0U;
	ch->updated = 0U;
}
/*********************************************************************************************************
* 函数名称：cap_channel_handle_irq
* 函数功能：
* 输入参数：volatile cap_channel_state_t* ch ：指向捕获通道状态的指针，包含定时器、通道号、时间戳等信息。
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年09月30日
* 注    意：它的主要功能是测量输入信号的周期和占空比，并通过捕获上升沿和下降沿的时间戳来计算这些参数。
*********************************************************************************************************/
static inline void cap_channel_handle_irq(volatile cap_channel_state_t* ch)
{
	uint32_t capture  = timer_channel_capture_value_register_read(ch->timer,ch->channel); //读取捕获值和扩展时间戳： capture ：当前捕获的原始值。
	uint32_t timestamp = cap_extend_timestamp(ch->timer, capture); //使用 cap_extend_timestamp 扩展时间戳，确保时间戳的范围足够大。
	if(ch->expect_fall)  //则计算高电平时间（ high_ticks ）和占空比（ duty ）。
	{
		ch->expect_fall = 0U;
		cap_set_polarity(ch->timer, ch, TIMER_IC_POLARITY_RISING); //cap_set_polarity ：设置捕获极性（上升沿或下降沿）。
		uint32_t high_ticks = timestamp  - ch->rise; //high_ticks ：高电平持续时间（从上升沿到下降沿的时间差）。
		
		float duty = 0.0f;
		if(ch->period != 0U)
		{
			duty = (float)high_ticks/(float)ch->period;
			if(duty<0.0f)
			{
				duty = 0.0f;
			}
			else if(duty > 1.0f)
			{
				duty = 1.0f;
			}
		}
		ch->duty = duty;
		ch->updated = 1U;
	}
	else //则记录上升沿时间戳（ ch->rise ），并设置下一次捕获为下降沿。
	{
		if(ch->have_rise)
		{
			ch->period = timestamp - ch->rise;
		}
		ch->rise = timestamp;
		ch->have_rise = 1U;
		ch->expect_fall = 1U;
		cap_set_polarity(ch->timer,ch,TIMER_IC_POLARITY_FALLING);
	}
	//cap_timeout_window ：计算超时窗口，用于检测信号丢失。
	//设置 timeout_deadline 防止信号丢失或异常情况下的无限等待
	 ch->timeout_deadline = timestamp + cap_timeout_window(ch->period);
}

/*********************************************************************************************************
* 函数名称：cap_set_polarity
* 函数功能：这段代码的主要功能是设置定时器（Timer）捕获通道（Channel）的极性（Polarity）。极性决定了捕获信号的触发方式（上升沿或下降沿）
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月10日
* 注    意：表明这是一个静态内联函数，通常用于优化性能，避免函数调用的开销。
*********************************************************************************************************/
static inline void cap_set_polarity(uint32_t timer,const volatile cap_channel_state_t* ch,uint16_t polarity)
{
	uint32_t bit =  ch->polarity_bit;
	if(bit == 0)
	{
		return ;
	}
	if(polarity == TIMER_IC_POLARITY_FALLING) //输入捕获下降沿
	{
		TIMER_CHCTL2(timer) |= bit;
	}
	else{
		 TIMER_CHCTL2(timer) &= ~bit;
	}
}

/*********************************************************************************************************
* 函数名称：cap_pa01_init
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年09月30日
* 注    意：
*********************************************************************************************************/
void cap_pa01_init()
{
	rcu_periph_clock_enable(RCU_GPIOA);
	gpio_init(CAP0_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, CAP0_PIN);
	gpio_init(CAP1_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, CAP1_PIN);

	rcu_periph_clock_enable(RCU_TIMER1);
	timer_parameter_struct t;
/* 初始化TIMER1相关结构体默认值参数 */
	timer_struct_para_init(&t);
	t.prescaler = 0;
	t.alignedmode = TIMER_COUNTER_EDGE;
	t.counterdirection = TIMER_COUNTER_UP;
	t.period = 0xFFFFU;
	t.clockdivision = TIMER_CKDIV_DIV1;
	t.repetitioncounter = 0;
	timer_init(CAP0_TIMER, &t); /* same as CAP1_TIMER */
	
	
	timer_ic_parameter_struct ic;
	// 已手动初始化 ic 结构体各字段，无需调用 timer_input_capture_struct_para_init(&ic)
	ic.icpolarity  = TIMER_IC_POLARITY_RISING; 			// 初始捕获上升沿
	ic.icselection = TIMER_IC_SELECTION_DIRECTTI;   // 直接输入模式。
	ic.icprescaler = TIMER_IC_PSC_DIV1;  //输入捕获不分频	
	ic.icfilter    = 0; //无滤波  8
	timer_input_capture_config(CAP0_TIMER, CAP0_CH, &ic);
	timer_input_capture_config(CAP1_TIMER, CAP1_CH, &ic);

 /* 先清标志再开中断，再开 NVIC，最后启动计数器 */
	//timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH | CAP1_INT_CH  | TIMER_INT_UP);
	//清除CAP0_TIMER的中断标志（包括CAP0、CAP1、更新和触发中断）
	timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH | CAP1_INT_CH | TIMER_INT_UP | TIMER_INT_TRG);
	nvic_irq_enable(CAP0_IRQN, 2, 0); /* 只需使能 CAP0_IRQN（TIMER1 IRQ），因为 CAP0 和 CAP1 共用同一个中断向量 */
	
	timer_interrupt_enable(CAP0_TIMER, CAP0_INT_CH);
	timer_interrupt_enable(CAP1_TIMER, CAP1_INT_CH);
	//是启用 CAP0_TIMER 的上升沿中断（ TIMER_INT_UP ）
	timer_interrupt_enable(CAP0_TIMER, TIMER_INT_UP);
	 
	timer_enable(CAP0_TIMER);
	
	cap_channel_reset_state(&s_cap_channels[CAP_CH_INDEX_PA3],CAP0_TIMER,CAP0_CH,CAP0_INT_CH);
	cap_channel_reset_state(&s_cap_channels[CAP_CH_INDEX_PA1],CAP1_TIMER,CAP1_CH,CAP1_INT_CH);
}

//通过捕获输入信号的上升沿和下降沿，计算输入信号的周期（ period ）和占空比（ duty ）
void TIMER1_IRQHandler()
{
	if(SET == timer_interrupt_flag_get(CAP0_TIMER, TIMER_INT_UP))
	{
		timer_interrupt_flag_clear(CAP0_TIMER,TIMER_INT_UP);
		cap_timer_overflow_increment(CAP0_TIMER); //并调用 cap_timer_overflow_increment 函数增加定时器的溢出计数。
		//获取当前定时器的计数值 now ，并调用 cap_channel_handle_timeout 函数处理两个通道（ CAP_CH_INDEX_PA3 和 CAP_CH_INDEX_PA1 ）的超时逻辑
		uint32_t now = cap_timer_now(CAP0_TIMER); 
		cap_channel_handle_timeout(&s_cap_channels[CAP_CH_INDEX_PA3], now);
		cap_channel_handle_timeout(&s_cap_channels[CAP_CH_INDEX_PA1], now);
	}
	
	volatile cap_channel_state_t* ch0 = &s_cap_channels[CAP_CH_INDEX_PA3];
	if(SET == timer_interrupt_flag_get(ch0->timer,ch0->int_flag))
	{
		cap_channel_handle_irq(ch0);
		timer_interrupt_flag_clear(ch0->timer,ch0->int_flag);
	}
	
	volatile cap_channel_state_t* ch1 = &s_cap_channels[CAP_CH_INDEX_PA1];
	if(SET == timer_interrupt_flag_get(ch1->timer, ch1->int_flag))
	{
		cap_channel_handle_irq(ch1);
		timer_interrupt_flag_clear(ch1->timer, ch1->int_flag);
	}
}
				
static inline int cap_read_duty(uint32_t index,float*duty)
{
	volatile cap_channel_state_t* ch = &s_cap_channels[index];
	if(ch->updated)
	{
		ch->updated = 0U;
		*duty = ch->duty;
		return 1;
	}
	return 0;
}
// 注意：cap_pa0_read_duty 实际读取的是 PA3 通道（CAP_CH_INDEX_PA3），请确保硬件连接与逻辑一致。
int cap_pa0_read_duty(float* duty){
	return cap_read_duty(CAP_CH_INDEX_PA3, duty);
}
// cap_pa1_read_duty corresponds to CAP_CH_INDEX_PA1, which represents the PA1 input capture channel.
int cap_pa1_read_duty(float* duty){
	return cap_read_duty(CAP_CH_INDEX_PA1, duty);
}
