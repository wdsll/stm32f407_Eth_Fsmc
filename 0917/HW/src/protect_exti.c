#include "protect_exti.h"

static volatile uint8_t s_fault = 0;

static void protect_fault_trigger(void)
{
	s_fault = 1;
}

#if defined(GD32F30X_XD) || defined(GD32F30X_CL)
#define TIMER0_BRK_IRQn_VALUE      TIMER0_BRK_TIMER8_IRQn
#define TIMER0_BRK_IRQHandler_NAME TIMER0_BRK_TIMER8_IRQHandler
#else
#define TIMER0_BRK_IRQn_VALUE      TIMER0_BRK_IRQn
#define TIMER0_BRK_IRQHandler_NAME TIMER0_BRK_IRQHandler
#endif
void protect_exti_init(void){
	rcu_periph_clock_enable(RCU_GPIOC);
	gpio_init(PROT_GPIO_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, PROT_GPIO_PIN);
	rcu_periph_clock_enable(RCU_AF);
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOC, GPIO_PIN_SOURCE_11);
	exti_init(PROT_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
	exti_interrupt_flag_clear(PROT_EXTI_LINE);
	nvic_irq_enable(EXTI10_15_IRQn, 1, 0);
	exti_interrupt_enable(PROT_EXTI_LINE);
	
	timer_interrupt_flag_clear(TIMER0,TIMER_INT_FLAG_BRK);
	timer_interrupt_enable(TIMER0,TIMER_INT_BRK);
	nvic_irq_enable(TIMER0_BRK_IRQn_VALUE,1,0);
}

void EXTI10_15_IRQHandler(void){
    if(RESET != exti_interrupt_flag_get(PROT_EXTI_LINE)){
        //s_fault = 1;
      protect_fault_trigger();  
			exti_interrupt_flag_clear(PROT_EXTI_LINE);
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
