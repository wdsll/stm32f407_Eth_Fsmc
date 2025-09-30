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


/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/
static volatile uint32_t rise0 = 0, rise1 = 0;
static volatile uint32_t period0 = 0U, period1 = 0U;
static volatile float duty0 = 0.0f, duty1 = 0.0f;
static volatile uint8_t expect_fall0 = 0U, expect_fall1 = 0U;
static volatile uint8_t have_rise0 = 0U, have_rise1 = 0U;
static volatile uint8_t upd0 = 0U, upd1 = 0U;
static volatile uint32_t last0=0, last1=0, per0=0, per1=0;
//static volatile uint8_t upd0=0, upd1=0;
/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：cap_plority_bit
* 函数功能：其目的是根据输入的通道号（ ch ）返回对应的定时器通道优先级控制位（ TIMER_CHCTL2_CHxP ）
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年09月30日
* 注    意：表明这是一个静态内联函数，通常用于优化性能，避免函数调用的开销。
*********************************************************************************************************/

static inline uint32_t cap_plority_bit(uint16_t ch) // 返回通道优先级控制位
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

/*********************************************************************************************************
* 函数名称：cap_set_polarity
* 函数功能：这段代码的主要功能是设置定时器（Timer）捕获通道（Channel）的极性（Polarity）。极性决定了捕获信号的触发方式（上升沿或下降沿）
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年09月30日
* 注    意：表明这是一个静态内联函数，通常用于优化性能，避免函数调用的开销。
*********************************************************************************************************/
static inline void cap_set_polarity(uint32_t timer,uint16_t channel,uint16_t polarity)
{
	uint32_t bit = cap_plority_bit(channel); //调用 cap_plority_bit(channel) 获取与通道对应的极性位掩码（bitmask）
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
* 函数名称：cap_set_polarity
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
	//timer_input_capture_struct_para_init(&ic);
	ic.icpolarity  = TIMER_IC_POLARITY_RISING; 			// 初始捕获上升沿
	ic.icselection = TIMER_IC_SELECTION_DIRECTTI;   // 直接输入模式。
	ic.icprescaler = TIMER_IC_PSC_DIV1;  //输入捕获不分频	
	ic.icfilter    = 0; //无滤波
	timer_input_capture_config(CAP0_TIMER, CAP0_CH, &ic);
	timer_input_capture_config(CAP1_TIMER, CAP1_CH, &ic);

 /* 先清标志再开中断，再开 NVIC，最后启动计数器 */
	//timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH | CAP1_INT_CH  | TIMER_INT_UP);
	// // 清除CAP0_TIMER的中断标志（包括CAP0、CAP1、更新和触发中断）
	timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH | CAP1_INT_CH | TIMER_INT_UP | TIMER_INT_TRG);
	nvic_irq_enable(CAP0_IRQN, 2, 0); /* same as CAP1_IRQN */
	
	timer_interrupt_enable(CAP0_TIMER, CAP0_INT_CH);
	timer_interrupt_enable(CAP1_TIMER, CAP1_INT_CH);
	timer_enable(CAP0_TIMER);
	
	// 初始化全局变量：上升沿时间
	rise0 = 0U;
	rise1 = 0U;
	// 初始化全局变量：周期时间
	period0 = 0U;
	period1 = 0U;
	// 初始化全局变量：期望捕获下降沿标志
	expect_fall0 = 0U;
	expect_fall1 = 0U;
	// 初始化全局变量：已捕获上升沿标志
	have_rise0 = 0U;
	have_rise1 = 0U;
	upd0 = 0U;
	upd1 = 0U;
}

//通过捕获输入信号的上升沿和下降沿，计算输入信号的周期（ period ）和占空比（ duty ）
void TIMER1_IRQHandler()
{
	uint32_t now0 = 0; // 当前捕获值
	float duty = 0.0f;  // 占空比	
	if(SET == timer_interrupt_flag_get(CAP0_TIMER,CAP0_INT_CH))  // 如果捕获通道0中断标志被设置
	{
		now0 = timer_channel_capture_value_register_read(CAP0_TIMER, CAP0_CH); 		
		if(expect_fall0) // 如果期望捕获下降沿
		{
			uint32_t high_ticks = (now0 - rise0) & 0xFFFFU;  // 计算高电平时间
			expect_fall0 = 0U; //下降沿标志
			cap_set_polarity(CAP0_TIMER, CAP0_CH, TIMER_IC_POLARITY_RISING); // 设置捕获通道的极性为上升沿
			if(period0 != 0U) // 如果周期不为0
			{
				duty = (float)high_ticks / (float)period0; //计算占空比
				if(duty < 0.0f)
				{
					duty = 0.0f;
				}
				if(duty > 1.0f)
				{
					duty = 1.0f;
				}
			}
			duty0 = duty;
			upd0 = 1U;
		}
		else
		{
			if(have_rise0) // 如果已捕获上升沿
			{
				period0 = (now0 - rise0)&0xFFFFU; //计算周期
			}
		rise0 = now0; //保存本次的上升沿时间
		have_rise0 = 1U; //已捕获上升沿标志
		expect_fall0 = 1U; //期望捕获下降沿标志
		cap_set_polarity(CAP0_TIMER, CAP0_CH, TIMER_IC_POLARITY_FALLING); //设置捕获通道的极性为下降沿
		}
		timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH); //清除中断标志
	}
	if(SET == timer_interrupt_flag_get(CAP1_TIMER, CAP1_INT_CH)) // 如果捕获通道1中断标志被设置
	{
			uint32_t now1 = timer_channel_capture_value_register_read(CAP1_TIMER, CAP1_CH);  // 读取当前捕获值

			if(expect_fall1)   // 如果期望捕获下降沿
			{
					uint32_t high_ticks = (now1 - rise1) & 0xFFFFU; // 计算高电平时间
					expect_fall1 = 0U; //下降沿标志
					cap_set_polarity(CAP1_TIMER, CAP1_CH, TIMER_IC_POLARITY_RISING); // 设置捕获通道的极性为上升沿
					if(period1 != 0U) // 如果周期不为0
					{
							float duty = (float)high_ticks / (float)period1; //计算占空比
							if(duty < 0.0f)
							{
									duty = 0.0f;
							}
							if(duty > 1.0f)
							{
									duty = 1.0f;
							}
							duty1 = duty;
							upd1 = 1U;
					}
				}
			else
			{
					if(have_rise1) // 如果已捕获上升沿
					{
							period1 = (now1 - rise1) & 0xFFFFU; //计算周期
					}
					rise1 = now1; //保存本次的上升沿时间
					have_rise1 = 1U; //已捕获上升沿标志
					expect_fall1 = 1U; //期望捕获下降沿标志
					cap_set_polarity(CAP1_TIMER, CAP1_CH, TIMER_IC_POLARITY_FALLING); //设置捕获通道的极性为下降沿
			}
 
			timer_interrupt_flag_clear(CAP1_TIMER, CAP1_INT_CH); //清除中断标志
	}
}
				

int cap_pa0_read_duty(float* duty){
	if(upd0)
	{ 
		upd0=0;
		*duty = duty0;
		return 1; 
	}
	return 0;
}

int cap_pa1_read_duty(float* duty){
	if(upd1)
	{ 
		upd1=0;
	  *duty = duty1;
		return 1; 
	}
	return 0;
}
