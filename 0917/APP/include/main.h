/*********************************************************************************************************
* 模块名称：main.h
* 摘    要：主模块
* 当前版本：1.0.0
* 作    者：Rengar
* 完成日期：2025年10月08日 
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
/* ==== Voltage sense dividers (top to bus, bottom to gnd) ==== */
#define VBUS_RTOP_OHM       (200000.0f)
#define VBUS_RBOT_OHM       (10000.0f)

#define VREF_ADC            (3.3f)

/* Dividers for voltage ADC channels */
#define VOUT_RTOP_OHM       (200000.0f)   /* PA5 */
#define VOUT_RBOT_OHM       (10000.0f)
#define VBT_RTOP_OHM        (200000.0f)   /* PC4 */
#define VBT_RBOT_OHM        (10000.0f)

#define V3V3_RTOP_OHM        (10000.0f)   /* PC5 */
#define V3V3_RBOT_OHM        (5100.0f)
/* ==== Auxiliary supply monitoring thresholds ==== */
#define AUX_V3V3_OK_MIN_V     (3.0f)
#define AUX_VBAT_OK_MIN_V     (10.0f)

#define AUX_OK_DEBOUNCE_MS          30U     /* 辅源恢复消抖时间 */
#define AUX_DROP_DEBOUNCE_MS        5U      /* 辅源掉电消抖时间（更快） */

#ifndef AUX_POWER_MONITOR_ENABLE
#define AUX_POWER_MONITOR_ENABLE    (0)
#endif
/* ==== Current sense ==== */
#define ISHUNT_OHM          (0.005f)   /* 5 mΩ */
#define IAMP_GAIN           (19.6f)   /* INA gain */

/* ==== Control targets/thresholds ==== */
#define VBUS_TARGET_V       (400.0f)
//#define AC_PRESENT_V        (30.0f)   /* |Vac| to start PFC */
#define LLC_ENTRY_V         (380.0f)  /* enter LLC when Vbus above this */
#define RUN_OK_LOW_V        (395.0f)
#define RUN_OK_HIGH_V       (405.0f)

/* ==== PWM defaults ==== */
#define LLC_PWM_BASE_HZ     (87600U)  //98753U
#define LLC_PWM_DEAD_NS     (500U)
#define LLC_PWM_DUTY        (0.50f)

#ifndef LLC_SOFTSTART_ENABLE
#define LLC_SOFTSTART_ENABLE        (1)
#endif
/* ==== LLC_SOFTSTART ==== */
#define LLC_SOFTSTART_DURATION_MS     100U      // 软启动总时长
#define LLC_SOFTSTART_MIN_DURATION_MS 20U  
#define LLC_SOFTSTART_START_DUTY      0.10f    // 起始占空（0~1）
#define LLC_SOFTSTART_TARGET_DUTY     0.50f    // 默认目标占空（0~1），可在 begin() 传入覆盖
#define LLC_SOFTSTART_FAILSAFE_DUTY   0.00f    // 故障时退回占空
#define LLC_SOFTSTART_USE_COSINE_EASE 1        // 1: 余弦S曲线；0: 指数曲线
#define LLC_SOFTSTART_EXP_K           3.0f     // 指数陡峭度（越大前期越缓）
#define LLC_SOFTSTART_EXTRA_MARGIN      (0.01f)   /* 安全窗额外余量（防抖） */
/* ==== LLC frequency window ==== */
#define LLC_F_MIN_HZ        (85000.0f)
#define LLC_F_MAX_HZ        (130000.0f)
#define LLC_F_INIT_HZ       (87500.0f)
#define LLC_F_SLEW_HZ       (500.0f)

#define DEBUG_PRINTF_LLCSOFTSTART 1

#define DEBUG_PRINTF_LLC_OPENLOOP 1

#define DEBUG_PRINTF_PFC_STATE 1

/* ==== Interrupt priority scheme (NVIC_PRIGROUP_PRE2_SUB2) ==== */
#define IRQ_PRIO_FAULT_PREEMPT        (0U)
#define IRQ_PRIO_FAULT_SUB            (0U)
#define IRQ_PRIO_DMA_PREEMPT          (1U)
#define IRQ_PRIO_DMA_SUB              (0U)
#define IRQ_PRIO_SYSTICK_PREEMPT      (2U)
#define IRQ_PRIO_SYSTICK_SUB          (0U)
#define IRQ_PRIO_MEASURE_PREEMPT      (2U)
#define IRQ_PRIO_MEASURE_SUB          (1U)
#define IRQ_PRIO_BACKGROUND_PREEMPT   (3U)
#define IRQ_PRIO_BACKGROUND_SUB       (0U)

static inline uint8_t irq_priority_encode(uint8_t preempt, uint8_t sub)
{
    return (uint8_t)(((uint32_t)preempt << 2U) | ((uint32_t)sub & 0x03U));
}
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

/* ===== PA3 & PA1 BOTH as timer input capture (no ADC on these) =====  交流的占空比和母线的占空比*/ 
#define CAP0_TIMER     TIMER1
#define CAP0_CH        TIMER_CH_3     /* PA3 → CH0 (adjust if needed) */
#define CAP0_PORT      GPIOA
#define CAP0_PIN       GPIO_PIN_3
#define CAP0_IRQN      TIMER1_IRQn
#define CAP0_INT_CH    TIMER_INT_CH3

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


#if 0
#define PFC_MAIN_RELAY_PORT     GPIOB
#define PFC_MAIN_RELAY_PIN     	GPIO_PIN_14 
#define PFC_MAIN_RELAY_RCU			RCU_GPIOB
#endif 

#define PFC_EN_PORT     GPIOC
#define PFC_EN_PIN     	GPIO_PIN_10  
#define PFC_EN_RCU			RCU_GPIOC

/* --- PFC 改进：更宽松的 READY 退出门限与延时 --- */
#ifndef PFC_VBUS_DROPOUT_THRESHOLD_V
#define PFC_VBUS_DROPOUT_THRESHOLD_V   (PFC_VBUS_READY_V - 15.0f) /* 例如 345V */
#endif

#ifndef PFC_VBUS_DROPOUT_MS_NEW
#define PFC_VBUS_DROPOUT_MS_NEW        (200U)                     /* 退出延时加长 */
#endif

/* LLC 启动前，PFC READY 需稳定一小段时间，避免竞态 */
#ifndef PFC_READY_STABLE_BEFORE_LLC_MS
#define PFC_READY_STABLE_BEFORE_LLC_MS (50U)
#endif

/* 在 IDLE→READY 判定中，对“明显掉回”才清零计时的缓冲带 */
#ifndef PFC_VBUS_OK_RESET_MARGIN_V
#define PFC_VBUS_OK_RESET_MARGIN_V     (5.0f)
#endif

/* ==== PFC control thresholds ==== */
#define PFC_VBUS_READY_V            (360.0f)
#define PFC_VBUS_READY_HYST_V       (10.0f)
#define PFC_READY_DELAY_MS          (200U)
#define PFC_STARTUP_DELAY_MS        (20U)
#define PFC_VBUS_DROPOUT_MS         (200U)
#define PFC_RESTART_DELAY_MS        (1000U)
#define PFC_VBUS_DROPOUT_THRESHOLD  (PFC_VBUS_READY_V - 15.0f)  /* 345V, increased from 350V */
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef enum 
{ 
	ST_IDLE=0, ST_WAIT_AUX, ST_WAIT_VBUS, ST_LLC_RUN, ST_FAULT 
} llc_state_t;
//vmeas：实际测量到的电压 
//integ：积分器的当前累积值（积分状态），通常会在饱和或模式切换时清零或软限制以防风up。
typedef struct {
	float vref, vmeas;
	float kp, ki, integ;
	float f_min, f_max, f_cmd, f_slew;
} llc_t;

typedef struct
{
	float v3v3_v;
	float vbat_v;
	float v3v3_min_v;
	float vbat_min_v;
	uint32_t last_update_ms;
	uint32_t drop_detected_ms; //电压下降被检测到的时间戳
	uint32_t restore_detected_ms; //电压恢复被检测到的时间戳
	bool power_ok;
} aux_power_monitor_t;

/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/


void llc_app_init(void);
void llc_app_tick_1khz(void);
llc_state_t llc_app_state(void);
void llc_step(llc_t* l);
static inline float f_clampf(float x,float lo,float hi);

#endif
