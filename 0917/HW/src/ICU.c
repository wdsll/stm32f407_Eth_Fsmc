#include "ICU.h"


static volatile uint32_t last0=0, last1=0, per0=0, per1=0;
static volatile uint8_t upd0=0, upd1=0;

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
	timer_interrupt_flag_clear(CAP0_TIMER, TIMER_INT_CH0 | TIMER_INT_CH1 | TIMER_INT_UP);
	nvic_irq_enable(CAP0_IRQN, 2, 0); /* same as CAP1_IRQN */
	timer_interrupt_enable(CAP0_TIMER, CAP0_INT_CH);
	timer_interrupt_enable(CAP1_TIMER, CAP1_INT_CH);
	timer_enable(CAP0_TIMER);
}


void TIMER1_IRQHandler()
{
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
}


int cap_pa0_read_period(uint32_t* ticks){
	if(upd0)
	{ 
		upd0=0;
		*ticks = per0; 
		return 1; 
	}
	return 0;
}

int cap_pa1_read_period(uint32_t* ticks){
	if(upd1)
	{ 
		upd1=0;
	  *ticks = per1;
		return 1; 
	}
	return 0;
}
