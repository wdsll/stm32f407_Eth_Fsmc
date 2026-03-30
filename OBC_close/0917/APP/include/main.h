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
#include <stdbool.h>
#include <math.h>
#include "add_dma.h"
#include "pwm_llc.h"
#include "RCU.h"
#include "debug_printf.h"
#include "pwm.h"
#include "protect_exti.h"
#include "pfc_control.h"
#include "llc_control.h"
#include "llc_soft_start.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
/* ==== Voltage sense dividers (top to bus, bottom to gnd) ==== */
#define VBUS_RTOP_OHM       (200000.0f)
#define VBUS_RBOT_OHM       (10000.0f)

#define VREF_ADC            (3.30f)


/* PFC sensing networks 待定未确认*/
#define PFC_VBUS_RTOP_OHM   (0.0f) 
#define PFC_VBUS_RBOT_OHM   (1.0f) 
/* PFC sensing networks - AC side (ZMPT + LM358 后端 RC) */
//这两个宏只反映“运放输出 → ADC”这一段 1/3 的分压。
#define PFC_AC_RTOP_OHM     (20000.0f)// R7 + R19
#define PFC_AC_RBOT_OHM     (10000.0f)// R8
#define PFC_NTC_PULLUP_OHM  (10000.0f)
#define PFC_NTC_R0_OHM      (10000.0f)
#define PFC_NTC_BETA        (3950.0f)

/* Dividers for voltage ADC channels */
#define VOUT_RTOP_OHM       (102000.0f)   /* PA5 */
#define VOUT_RBOT_OHM       (5100.0f)
#define VBT_RTOP_OHM        (200000.0f)   /* PC4 */
#define VBT_RBOT_OHM        (10000.0f)

#define V3V3_RTOP_OHM        (5100.0f)   
#define V3V3_RBOT_OHM        (10000.0f)

/* ==== ADC startup sanity check thresholds ==== */
#define ADC_STARTUP_V3V3_MIN_V        (2.8f)
#define ADC_STARTUP_V3V3_MAX_V        (3.6f)
#define ADC_STARTUP_SAMPLE_COUNT      (8U)
#define ADC_STARTUP_SAMPLE_DELAY_MS   (2U)

#define ADC_FAST_SAMPLE_US           (100U)
#define ADC_FAST_SAMPLE_HZ           (1000000U / ADC_FAST_SAMPLE_US)


/* ==== Current sense ==== */
#define ISHUNT_OHM          (0.00333f)   /* 5 mΩ */
#define IAMP_GAIN           (19.8198f)  /* INA gain */

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
#define LLC_SOFTSTART_DURATION_MS     300U      // 软启动总时长
#define LLC_SOFTSTART_STABILIZE_MS     50U      // 软启动后稳定等待时间
#define LLC_SOFTSTART_MIN_DURATION_MS 20U  
#define LLC_SOFTSTART_START_HZ     (LLC_F_MAX_HZ)
#define LLC_SOFTSTART_TARGET_HZ    (120000.0f)
#define LLC_SOFTSTART_FAILSAFE_HZ  (LLC_F_MAX_HZ)

#define LLC_SOFTSTART_USE_COSINE_EASE 1        // 1: 余弦S曲线；0: 指数曲线
#define LLC_SOFTSTART_EXP_K           3.0f     // 指数陡峭度（越大前期越缓）
#define LLC_SOFTSTART_EXTRA_MARGIN      (0.02f)   /* 安全窗额外余量（防抖） */
#define LLC_SOFTSTART_TICK_MS         (0.1f)

/* ==== LLC_SOFTSTOP ==== */
#define LLC_SOFTSTOP_DURATION_MS      200U      // 软关断总时长（比软启短一些）
#define LLC_SOFTSTOP_MIN_DURATION_MS  20U       // 最小软关断时长

/* ==== LLC_CYCLE_STOP 周期性软关断 - 禁用，改用Burst ==== */
#define LLC_CYCLE_STOP_ENABLE         0         // 0: 禁用周期性软关断
#define LLC_CYCLE_STOP_INTERVAL_MS    3000U     // 周期性软关断间隔（3秒）
#define LLC_CYCLE_STOP_RESTART_MS     500U      // 软关断后重新启动前等待时间

/* ==== LLC_BURST_MODE 轻载突发模式 ==== */
#define LLC_BURST_MODE_ENABLE         0         // 1: 使能Burst模式

/* Burst进入/退出 - 第一档轻载：1.5A进入，2.5A退出 */
#define LLC_BURST_IOUT_ENTER_A        (1.5f)    // 进入阈值 <1.5A (轻载)
#define LLC_BURST_IOUT_EXIT_A         (2.5f)    // 退出阈值 >2.5A (滞环1A)
#define LLC_BURST_ENTER_DELAY_MS      (1000U)   // 进入延迟1秒(慢进入防抖动)
#define LLC_BURST_EXIT_DELAY_MS       (50U)     // 退出延迟50ms(快退出)

/* Burst时间窗口 */
#define LLC_BURST_ON_MIN_MS           (2U)      // ON最短2ms
#define LLC_BURST_ON_MAX_MS           (20U)     // ON最长20ms，超时强制关
#define LLC_BURST_OFF_MIN_MS          (4U)      // OFF最短4ms
#define LLC_BURST_OFF_MAX_MS          (50U)     // OFF最长50ms，超时强制开

/* Burst电压控制 */
#define LLC_BURST_VOUT_HYST_V         (1.0f)    // 电压滞环1V
#define LLC_BURST_VOUT_TARGET_V       (38.0f)   // 目标电压44V

/* Burst频率 - 高频准备 */
#define LLC_BURST_F_HZ                (180000.0f) // Burst用高频180kHz
#define LLC_BURST_F_PREPARE_MS        (1U)        // 高频准备1ms后关PWM (最小单位)
#define LLC_BURST_ENTRY_RAMP_MS       (3U)        // 首次进入Burst升频时间3ms

/* ==== LLC RUN entry hold ==== */
#define LLC_RUN_ENTRY_HOLD_MS          (20U)
#define LLC_RUN_ENTRY_STABLE_TICKS     (2U)
#define LLC_RUN_ENTRY_STABLE_WINDOW_V  (6.0f)
#define LLC_RUN_ENTRY_TIMEOUT_MS       (400U)

/* ==== LLC frequency window ==== */
#define LLC_F_MIN_HZ        (75000.0f)
#define LLC_F_MAX_HZ        (250000.0f)
#define LLC_F_INIT_HZ       (100000.0f)
#define LLC_F_SLEW_HZ       (3000.0f)
#define LLC_F_DEBUG_HZ      (130000.0f)

/* ==== LLC open-loop safe bands ==== */
#define LLC_VBUS_MIN_START_V         (360.0f)
#define LLC_VOUT_TARGET_V            (38.0f)
#define LLC_VOUT_HYST_V              (15.0f)
#define LLC_VOUT_OVP_V               (58.0f)
#define LLC_IOUT_OCP_A               (50.0f)

#define LLC_SWEEP_START_HZ           (100000.0f)
#define LLC_SWEEP_STOP_HZ            (80000.0f)
#define LLC_SWEEP_STEP_HZ            (800.0f) 
#define LLC_SWEEP_STEP_MS            (5U)
#define LLC_SWEEP_TIMEOUT_MS         (1600U)
#define LLC_SWEEP_STABLE_COUNT       (2U)
#define LLC_SWEEP_TARGET_WINDOW_V    (2.0f)

#define LLC_HOLD_ADJUST_HZ           (400.0f)
#define LLC_HOLD_ADJUST_PERIOD_MS    (10U)
#define LLC_HOLD_RESCAN_DELTA_V      (5.0f)

#define LLC_STOPPING_FREQ_HOLD_MS    (40U)


#define DEBUG_PRINTF_LLCSOFTSTART 0
#define DEBUG_PRINTF_LLC_OPENLOOP 0
#define DEBUG_PRINTF_PFC_STATE 0
#define DEBUG_PRINTF_BURST_MODE   0    /* Burst模式调试打印 */

#define LLC_DIAG_PRINT_DISABLE_ALL   1U
#if LLC_DIAG_PRINT_DISABLE_ALL
#define LLC_CR_RESP_LOG_ENABLE       0U
#define LLC_COLLAPSE_TRACE_ENABLE    0U
#endif

/* ---- LLC collapse trace / fixed-frequency load test ---- */
#ifndef LLC_COLLAPSE_TRACE_ENABLE
#define LLC_COLLAPSE_TRACE_ENABLE           1U
#endif

/* ---- LLC collapse trace / fixed-frequency load test ---- */
#define LLC_COLLAPSE_TRACE_PRE_MS           10U
#define LLC_COLLAPSE_TRACE_POST_MS          10U
#define LLC_COLLAPSE_VOUT_DROP_V            (5.0f)
#define LLC_COLLAPSE_VOUT_ABS_MIN_V         (30.0f)
#define LLC_COLLAPSE_TRACE_REARM_MS         200U

#define LLC_FIXED_FREQ_LOAD_TEST_ENABLE     0U
#define LLC_FIXED_FREQ_LOAD_TEST_HZ         (LLC_F_DEBUG_HZ)

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

/* ADC channel map (PA0/PA1 removed) */
#define AC_VOL_SAMPLE      ADC_CHANNEL_1   /* PA1 AC 电压采样*/
#define FAN_CS      			 ADC_CHANNEL_2   //风扇电流/状态采样

#define BUS_VOL_SAMPLE     ADC_CHANNEL_3   //母线电压保护通道（用于 OVP/OCP 计算）

#define VOUT_SENSE_CH      ADC_CHANNEL_5   /* PA5 */
#define ADC_ISENSE_CH      ADC_CHANNEL_6   /* PA6 */
//#define T_SENSE_PFC_MOS    ADC_CHANNEL_7   /* PA7 PFC MOS 管温度 NTC 采样*/
#define AD_3V3_CH          ADC_CHANNEL_14  /* PC4 3.3V 模拟电源监测（AD_3V3）*/ 
#define VBT_SENSE_CH       ADC_CHANNEL_15  /* PC5 电池端电压采样*/
//#define T_SENSE_LLCMOS_CH  ADC_CHANNEL_9   /* PB1 LLC MOS 管温度 NTC 采样*/


#define OUT_RELAY     GPIOB  //直流输出继电器控制
#define OUT_RELAY_PIN     	GPIO_PIN_14 
#define OUT_RELAY_RCU			RCU_GPIOB

#define PFC_MAIN_RELAY_PORT     GPIOC  //PFC 预充/继电器使能控制
#define PFC_MAIN_RELAY_PIN     	GPIO_PIN_10  
#define PFC_MAIN_RELAY_RCU			RCU_GPIOC

#define PFC_FAULT_PORT          GPIOB
#define PFC_FAULT_PIN           GPIO_PIN_12

#define LLC_EN_PORT             GPIOC
#define LLC_EN_PIN             GPIO_PIN_11

//#define PWM_BKIN_PORT           GPIOC
//#define PWM_BKIN_PIN            GPIO_PIN_12

#define LED_Y_PORT              GPIOC
#define LED_Y_PIN               GPIO_PIN_7
#define LED_G_PORT              GPIOC
#define LED_G_PIN               GPIO_PIN_8
#define LED_R_PORT              GPIOC
#define LED_R_PIN               GPIO_PIN_9

#define FAN_CTL_PORT            GPIOC
#define FAN_CTL_PIN             GPIO_PIN_12


#ifndef PFC_VBUS_DROPOUT_MS_NEW
#define PFC_VBUS_DROPOUT_MS_NEW        (200U)                     /* 退出延时加长 */
#endif

/* LLC 启动前，PFC READY 需稳定一小段时间，避免竞态 */
#ifndef PFC_READY_STABLE_BEFORE_LLC_MS
#define PFC_READY_STABLE_BEFORE_LLC_MS (50U)
#endif



/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
extern volatile uint32_t g_ms;

static inline uint32_t elapsed_since(uint32_t start_ms)
{
  return (start_ms == 0U) ? 0U : (uint32_t)(g_ms - start_ms);
}

static inline bool elapsed_reached(uint32_t start_ms, uint32_t duration_ms)
{
	if (duration_ms == 0) {
					return true;
	}
	if (start_ms == 0) {
					return false;
	}
	return elapsed_since(start_ms) >= duration_ms;
}
/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/

static inline float f_clampf(float x,float lo,float hi)
{ 
	return x<lo?lo:(x>hi?hi:x); 
}
void delay_ms(uint32_t duration_ms);
#endif
