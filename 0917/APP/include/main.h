/*********************************************************************************************************
* 模块名称：main.h
* 摘    要：主模块
* 当前版本：1.0.0
* 作    者：Rengar
* 完成日期：2024年01月01日 
* 内    容：
* 注    意：                                                                  
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/
#ifndef _MAIN_H_
#define _MAIN_H_

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/


#include "gd32f30x_conf.h"
#include <stdint.h>
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
//#define SYSCLK_HZ           108000000UL

/* ==== Voltage sense dividers (top to bus, bottom to gnd) ==== */
#define VBUS_RTOP_OHM       (1000000.0f)
#define VBUS_RBOT_OHM       (4700.0f)

#define VREF_ADC            (3.3f)

/* Dividers for voltage ADC channels */
#define VOUT_RTOP_OHM       (1000000.0f)   /* PA5 */
#define VOUT_RBOT_OHM       (4700.0f)
#define VBT_RTOP_OHM        (1000000.0f)   /* PC5 */
#define VBT_RBOT_OHM        (4700.0f)

/* ==== Current sense ==== */
#define ISHUNT_OHM          (0.02f)   /* 20 mΩ */
#define IAMP_GAIN           (20.0f)   /* INA gain */

/* ==== Control targets/thresholds ==== */
#define VBUS_TARGET_V       (400.0f)
//#define AC_PRESENT_V        (30.0f)   /* |Vac| to start PFC */
#define LLC_ENTRY_V         (380.0f)  /* enter LLC when Vbus above this */
#define RUN_OK_LOW_V        (395.0f)
#define RUN_OK_HIGH_V       (405.0f)

/* ==== PWM defaults ==== */
#define LLC_PWM_BASE_HZ     (100000U)
#define LLC_PWM_DEAD_NS     (500U)
#define LLC_PWM_DUTY        (0.50f)

/* ==== LLC frequency window ==== */
#define LLC_F_MIN_HZ        (60000.0f)
#define LLC_F_MAX_HZ        (150000.0f)
#define LLC_F_INIT_HZ       (100000.0f)
#define LLC_F_SLEW_HZ       (1000.0f)
/*******************************************Board pin list********************************************************************/
/* TIMER0 (TIM1) complementary PWM */
#define LLC_PWM_TIMER  TIMER0
#define LLC_PWM_CH     TIMER_CH_0
#define LLC_H_PORT     GPIOA
#define LLC_H_PIN      GPIO_PIN_8
#define LLC_L_PORT     GPIOB
#define LLC_L_PIN      GPIO_PIN_13
#define BKIN_PORT      GPIOB
#define BKIN_PIN       GPIO_PIN_12


/* PB0 PWM (aux) */
#define PB0_PWM_TIMER  TIMER2
#define PB0_PWM_CH     TIMER_CH_2
#define PB0_PORT       GPIOB
#define PB0_PIN        GPIO_PIN_0
#define PB0_PWM_BASE_HZ     (20000U)

/* ===== PA0 & PA1 BOTH as timer input capture (no ADC on these) ===== */
#define CAP0_TIMER     TIMER1
#define CAP0_CH        TIMER_CH_0     /* PA0 → CH0 (adjust if needed) */
#define CAP0_PORT      GPIOA
#define CAP0_PIN       GPIO_PIN_0
#define CAP0_IRQN      TIMER1_IRQn
#define CAP0_INT_CH    TIMER_INT_CH0

#define CAP1_TIMER     TIMER1
#define CAP1_CH        TIMER_CH_1     /* PA1 → CH1 (adjust if needed) */
#define CAP1_PORT      GPIOA
#define CAP1_PIN       GPIO_PIN_1
#define CAP1_IRQN      TIMER1_IRQn
#define CAP1_INT_CH    TIMER_INT_CH1



/* ADC channel map (PA0/PA1 removed) */
#define VOUT_SENSE_CH      ADC_CHANNEL_5   /* PA5 */
#define ADC_ISENSE_CH      ADC_CHANNEL_6   /* PA6 */
#define ADC_TSENSE_CH      ADC_CHANNEL_7   /* PA7 */
#define AD_3V3_CH          ADC_CHANNEL_14  /* PC4 */
#define VBT_SENSE_CH       ADC_CHANNEL_15  /* PC5 */
#define T_SENSE_LLCMOS_CH  ADC_CHANNEL_9   /* PB1 */

/* PC11 EXTI protection */
#define PROT_GPIO_PORT     GPIOC
#define PROT_GPIO_PIN      GPIO_PIN_11
#define PROT_EXTI_LINE     EXTI_11
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef enum 
{ 
	ST_IDLE=0, ST_WAIT_VBUS, ST_LLC_RUN, ST_FAULT 
} llc_state_t;
typedef struct {
    float vref, vmeas;
    float kp, ki, integ;
    float f_min, f_max, f_cmd, f_slew;
} llc_t;


/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/


void llc_app_init(void);
void llc_app_tick_1khz(void);
llc_state_t llc_app_state(void);
void llc_step(llc_t* l);
static inline float f_clampf(float x,float lo,float hi);

#endif
