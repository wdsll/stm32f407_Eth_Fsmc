/*********************************************************************************************************
* 模块名称：main.c
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


static inline uint32_t cap_plority_bit(uint16_t ch)
{
	switch(ch)
	{
		case TIMER_CH_0:
			return TIMER_CHCTL2_CH0P;
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
static inline void cap_set_polarity(uint32_t timer,uint16_t channel,uint16_t polarity)
{
	uint32_t bit = cap_plority_bit(channel);
	if(bit == 0)
	{
		return ;
	}
	if(polarity == TIMER_IC_POLARITY_FALLING)
	{
		TIMER_CHCTL2(timer) |= bit;
	}
	else{
		 TIMER_CHCTL2(timer) &= ~bit;
	}
}
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
	ic.icpolarity  = TIMER_IC_POLARITY_RISING;
	ic.icselection = TIMER_IC_SELECTION_DIRECTTI;
	ic.icprescaler = TIMER_IC_PSC_DIV1;
	ic.icfilter    = 0;
	timer_input_capture_config(CAP0_TIMER, CAP0_CH, &ic);
	timer_input_capture_config(CAP1_TIMER, CAP1_CH, &ic);

 /* 先清标志再开中断，再开 NVIC，最后启动计数器 */
	timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH | CAP1_INT_CH  | TIMER_INT_UP);
	nvic_irq_enable(CAP0_IRQN, 2, 0); /* same as CAP1_IRQN */
	timer_interrupt_enable(CAP0_TIMER, CAP0_INT_CH);
	timer_interrupt_enable(CAP1_TIMER, CAP1_INT_CH);
	timer_enable(CAP0_TIMER);
	
	rise0 = 0U;
	rise1 = 0U;
	period0 = 0U;
	period1 = 0U;
	expect_fall0 = 0U;
	expect_fall1 = 0U;
	have_rise0 = 0U;
	have_rise1 = 0U;
	upd0 = 0U;
	upd1 = 0U;
}


void TIMER1_IRQHandler()
{
	/*
	if(SET == timer_interrupt_flag_get(CAP0_TIMER,CAP0_INT_CH))
	{
		uint32_t now0 = timer_channel_capture_value_register_read(CAP0_TIMER, CAP0_CH); //读取当前捕获值
		per0 = (now0 - last0) & 0xFFFF;
		last0 = now0;
		upd0 = 1;
		timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH);
	}
	if(SET == timer_interrupt_flag_get(CAP1_TIMER, CAP1_INT_CH)){
		uint32_t now1 = timer_channel_capture_value_register_read(CAP1_TIMER, CAP1_CH);
		per1 = (now1 - last1) & 0xFFFF;
		last1 = now1;
		upd1 = 1;
		timer_interrupt_flag_clear(CAP1_TIMER, CAP1_INT_CH);
	}
	*/
	uint32_t now0 = 0;
	float duty = 0.0f; 
	if(SET == timer_interrupt_flag_get(CAP0_TIMER,CAP0_INT_CH))
	{
		now0 = timer_channel_capture_value_register_read(CAP0_TIMER, CAP0_CH);
		if(expect_fall0)
		{
			uint32_t high_ticks = (now0 - rise0) & 0xFFFFU;
			expect_fall0 = 0U;
			cap_set_polarity(CAP0_TIMER, CAP0_CH, TIMER_IC_POLARITY_RISING);
			if(period0 != 0U)
			{
				duty = (float)high_ticks / (float)period0;
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

	else{
		if(have_rise0)
		{
			period0 = (now0 - rise0)&0xFFFFU;
		}
		rise0 = now0;
		have_rise0 = 1U;
		expect_fall0 = 1U;
		cap_set_polarity(CAP0_TIMER, CAP0_CH, TIMER_IC_POLARITY_FALLING);
		}
		timer_interrupt_flag_clear(CAP0_TIMER, CAP0_INT_CH);
	}
	if(SET == timer_interrupt_flag_get(CAP1_TIMER, CAP1_INT_CH))
	{
			uint32_t now1 = timer_channel_capture_value_register_read(CAP1_TIMER, CAP1_CH);

			if(expect_fall1)
			{
					uint32_t high_ticks = (now1 - rise1) & 0xFFFFU;
					expect_fall1 = 0U;
					cap_set_polarity(CAP1_TIMER, CAP1_CH, TIMER_IC_POLARITY_RISING);
					if(period1 != 0U)
					{
							float duty = (float)high_ticks / (float)period1;
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
					if(have_rise1)
					{
							period1 = (now1 - rise1) & 0xFFFFU;
					}
					rise1 = now1;
					have_rise1 = 1U;
					expect_fall1 = 1U;
					cap_set_polarity(CAP1_TIMER, CAP1_CH, TIMER_IC_POLARITY_FALLING);
			}

			timer_interrupt_flag_clear(CAP1_TIMER, CAP1_INT_CH);
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
