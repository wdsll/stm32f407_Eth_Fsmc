#include "protect_exti.h"

static volatile uint8_t s_fault = 0;
static volatile uint8_t s_fault_source_hw = 0;

/* BKIN 自检用：Break ISR 触发标志 */
volatile uint8_t g_brk_irq_fired = 0;

/* 触发后的统一处理：置位软件锁存 + 关 PWM（BKIN 已经硬件关，但软件也一起做） */
static void protect_fault_trigger(void)
{
	s_fault = 1;
	s_fault_source_hw = protect_fault_active_hw() ? 1U : 0U;
	llc_pwm_outputs_enable(0);
}


/* 硬件电平是否在"故障有效"状态（低有效） 判断硬件 BKIN 输入电平是否仍处于故障状态 */
int protect_fault_active_hw(void)
{
  return gpio_input_bit_get(HARD_PRO_READ_PORT, HARD_PRO_READ_PIN) == RESET;
}

void protect_exti_init(void){
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_AF);
	
	/*
	 * BKIN 引脚 (PB12) 的 GPIO 初始化已在 llc_pwm_init() -> pins_init() 中完成：
	 *   gpio_init(BKIN_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, BKIN_PIN);
	 * IPU 上拉确保光耦未导通时 BKIN 为高电平（安全状态）。
	 * 此处不再重复初始化，避免 GPIO_MODE_IN_FLOATING 覆盖 IPU 配置。
	 *
	 * 4840 原理图信号链路：
	 *   主板 OCP 比较器(LMV393) → HARD_PRO → 光耦(LTV-817S,OP2) → PB12(BKIN)
	 *   光耦输出低有效：OCP 触发 → 光耦导通 → PB12 被拉低 → BKIN 触发
	 */
	
	/* 清除并使能故障中断 */
	timer_interrupt_flag_clear(TIMER0,TIMER_INT_FLAG_BRK);
	nvic_irq_enable(TIMER0_BRK_IRQn_VALUE, IRQ_PRIO_FAULT_PREEMPT, IRQ_PRIO_FAULT_SUB);
	timer_interrupt_enable(TIMER0,TIMER_INT_BRK);
	
	/*
	 * PC11 (HARD_PRO_CL) 已确认在 4840 原理图上是 LLC_EN（驱动使能），
	 * 不是故障清除引脚。光耦 LTV-817S 输出自锁特性：
	 *   - OCP 触发时光耦导通，PB12 拉低
	 *   - OCP 解除后光耦自然截止，PB12 被 IPU 上拉恢复高电平
	 *   - 无需软件通过 GPIO 清除硬件锁存
	 * 因此删除 HARD_PRO_CL 初始化代码。
	 */
	
	/* 上电自检：如果 BKIN 已经是低电平，说明硬件故障已存在 */
	if(protect_fault_active_hw())
	{
		protect_fault_trigger();
	}
}


void TIMER0_BRK_IRQHandler_NAME(void)
{
	if(RESET != timer_interrupt_flag_get(TIMER0,TIMER_INT_FLAG_BRK))
	{
		g_brk_irq_fired = 1U;  /* 自检标志 */
		protect_fault_trigger();
		timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_BRK);
	}
}

int protect_fault_latched(void)
{ 
	return s_fault!=0; 
}

int protect_fault_source_hw_latched(void)
{
	return s_fault_source_hw != 0U;
}

void protect_clear_fault(void)
{ 
	s_fault=0; 
	s_fault_source_hw = 0U;
}
