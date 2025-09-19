#include "protect_exti.h"

static volatile uint8_t s_fault=0;

void protect_exti_init(void){
	rcu_periph_clock_enable(RCU_GPIOC);
	gpio_init(PROT_GPIO_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, PROT_GPIO_PIN);
	rcu_periph_clock_enable(RCU_AF);
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOC, GPIO_PIN_SOURCE_11);
	exti_init(PROT_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
	exti_interrupt_flag_clear(PROT_EXTI_LINE);
	nvic_irq_enable(EXTI10_15_IRQn, 1, 0);
	exti_interrupt_enable(PROT_EXTI_LINE);
}

void EXTI10_15_IRQHandler(void){
    if(RESET != exti_interrupt_flag_get(PROT_EXTI_LINE)){
        s_fault = 1;
        exti_interrupt_flag_clear(PROT_EXTI_LINE);
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
