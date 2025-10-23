#ifndef PROTECT_EXTI_H
#define PROTECT_EXTI_H
#include "main.h"

/* === 引脚定义：按你的原理图修正 === */
#define HARD_PRO_READ_PORT       GPIOB          // HARD_PRO_READ 所在端口
#define HARD_PRO_READ_PIN        GPIO_PIN_12    // HARD_PRO_READ 管脚


#define HARD_PRO_CL_GPIO_PORT    GPIOC          // HARD_PRO_CL（清除锁存）输出
#define HARD_PRO_CL_GPIO_PIN     GPIO_PIN_12

#if defined(GD32F30X_XD) || defined(GD32F30X_CL)
#define TIMER0_BRK_IRQn_VALUE      TIMER0_BRK_TIMER8_IRQn
#define TIMER0_BRK_IRQHandler_NAME TIMER0_BRK_TIMER8_IRQHandler
#else
#define TIMER0_BRK_IRQn_VALUE      TIMER0_BRK_IRQn
#define TIMER0_BRK_IRQHandler_NAME TIMER0_BRK_IRQHandler
#endif
void protect_exti_init(void);
int  protect_fault_latched(void);
void protect_clear_fault(void);
int  protect_fault_active_hw(void);
#endif

