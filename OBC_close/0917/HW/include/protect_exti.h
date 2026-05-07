#ifndef PROTECT_EXTI_H
#define PROTECT_EXTI_H
#include "main.h"

/* === 引脚定义：按 4840 原理图确认 === */
#define HARD_PRO_READ_PORT       GPIOB          // HARD_PRO_READ = PB12 (BKIN)
#define HARD_PRO_READ_PIN        GPIO_PIN_12

/*
 * PC11 在 4840 原理图上是 LLC_EN（驱动使能），不是故障清除引脚。
 * 光耦 LTV-817S 输出低电平自锁，OCP 解除后光耦自然恢复，
 * 无需软件通过 GPIO 清除硬件锁存。HARD_PRO_CL 定义已删除。
 *
 * 若后续硬件增加了专用故障清除引脚，在此重新定义。
 */

#if defined(GD32F30X_XD) || defined(GD32F30X_CL)
#define TIMER0_BRK_IRQn_VALUE      TIMER0_BRK_TIMER8_IRQn
#define TIMER0_BRK_IRQHandler_NAME TIMER0_BRK_TIMER8_IRQHandler
#else
#define TIMER0_BRK_IRQn_VALUE      TIMER0_BRK_IRQn
#define TIMER0_BRK_IRQHandler_NAME TIMER0_BRK_IRQHandler
#endif
/* BKIN 自检时 Break ISR 触发标志（bkin_selftest.c 读写，protect_exti.c ISR 置位） */
extern volatile uint8_t g_brk_irq_fired;

void protect_exti_init(void);
int  protect_fault_latched(void);
int  protect_fault_source_hw_latched(void);

void protect_clear_fault(void);
int  protect_fault_active_hw(void);
#endif

