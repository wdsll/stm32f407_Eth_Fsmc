#include "protect_exti.h"
#include "pwm_llc.h"
static volatile uint8_t s_fault = 0;
static volatile uint8_t s_fault_latched = 0;



/* 触发后的统一处理：置位软件锁存 + 关 PWM（BKIN 已经硬件关，但软件也一起做） */
static void protect_fault_trigger(void)
{
	s_fault = 1;
	llc_pwm_outputs_enable(0);
}


//extern volatile uint32_t g_ms;   // 你的系统毫秒计数
/* 硬件电平是否在“故障有效”状态（这里按低有效） 判断硬件 BKIN 输入电平是否仍处于故障状态 */
int protect_fault_active_hw(void)
{
  return gpio_input_bit_get(GPIOB, GPIO_PIN_12) == RESET;
}

void protect_exti_init(void){
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_AF);
	//gpio_init(HARD_PRO_READ_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, HARD_PRO_READ_PIN);
	
	timer_interrupt_flag_clear(TIMER0,TIMER_INT_FLAG_BRK);
	timer_interrupt_enable(TIMER0,TIMER_INT_BRK);
	//nvic_irq_enable(TIMER0_BRK_IRQn_VALUE,1,0);
	nvic_irq_enable(TIMER0_BRK_IRQn_VALUE, IRQ_PRIO_FAULT_PREEMPT, IRQ_PRIO_FAULT_SUB);
	gpio_init(HARD_PRO_CL_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, HARD_PRO_CL_GPIO_PIN);
  gpio_bit_reset(HARD_PRO_CL_GPIO_PORT, HARD_PRO_CL_GPIO_PIN);
	
	/* 3 上电自检 */
	if(protect_fault_active_hw())
	{
		protect_fault_trigger();
	}
}


void TIMER0_BRK_IRQHandler_NAME(void)
{
	if(RESET != timer_interrupt_flag_get(TIMER0,TIMER_INT_FLAG_BRK))
	{
		protect_fault_trigger();
		timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_BRK);
	}
}
int protect_fault_latched(void)
{ 
	return s_fault!=0; 
}
void protect_clear_fault(void)
{ 
	s_fault=0; 
}
