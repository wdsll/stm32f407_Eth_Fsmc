/*********************************************************************************************************
* 模块名称：main.h
* 摘    要：1.5kW OBC 主头文件
* 当前版本：0.1.0 (初始版本)
* 作    者：Rengar
* 完成日期：2026年07月29日
* 硬件参考：Honda 1.5KW Charger 260729 原理图
* MCU 平台：GD32F303RE (Cortex-M4, 108MHz)
* 拓扑：PFC(NCP1654 模拟控制) + LLC(专用IC 模拟控制) + 隔离CAN(ISO1050)
* 控制方式：PFC/LLC 均为模拟芯片控制, MCU 仅做监控/使能/CV_PWM基准/CAN/充电状态机
* MCU 角色：监控器 (非功率环控制器), 通过 CV_PWM(PA8) 调节 LLC 输出电压基准
* 功率规格：1.5kW, Vin 220VAC, Vbus ~380V
*********************************************************************************************************/

#ifndef _MAIN_H_
#define _MAIN_H_

#include "gd32f30x_conf.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ========== 系统时钟 ========== */
#define SYSTEM_CLOCK_HZ         (120000000U)

/* MCU：U17，GD32F303RET6 */
#define BOARD_MCU_PART                  "GD32F303RET6"

/* ADC 参考参数 */
#define VREF_ADC_V                      (3.30f)
#define ADC_FULL_SCALE_COUNTS           (4095U)
/* 兼容旧代码的别名 */
#define VREF_ADC                VREF_ADC_V
#define ADC_RESOLUTION          ADC_FULL_SCALE_COUNTS

/* ========== 电压采样分压网络 (原理图) ========== */
/* VBUS: 4×330k(串联1.32M) + 8.6k → 分压比 1/154.65 */
#define VBUS_RTOP_OHM           (1320000.0f)
#define VBUS_RBOT_OHM           (8600.0f)

/* AC: 4×330k(1.32M) + 5.1k → 分压比 1/259.8 */
#define AC_RTOP_OHM             (1320000.0f)
#define AC_RBOT_OHM             (5100.0f)

/* VOUT: 240k + 4.7k → 分压比 1/52.06 */
#define VOUT_RTOP_OHM           (240000.0f)
#define VOUT_RBOT_OHM           (4700.0f)

/* VBT: 240k + 4.7k (同 VOUT) */
#define VBT_RTOP_OHM            (240000.0f)
#define VBT_RBOT_OHM            (4700.0f)

/* 3V3 监测 */
#define V3V3_RTOP_OHM           (5100.0f)
#define V3V3_RBOT_OHM           (10000.0f)

/* 风扇电流采样 (FAN_CS, PA2): 分流 + 运放, 待台架标定 */
#define FAN_CS_SHUNT_OHM        (0.020f)     /* 待标定 */
#define FAN_CS_AMP_GAIN         (10.0f)      /* 待标定 */

/* ========== 输出电流采样 ========== */
/* 3×0.01R 并联 = 0.00333Ω, NSI1312 隔离运放 */
#define ISHUNT_OHM              (0.00333f)
#define IAMP_GAIN               (20.0f)      /* 运放增益, 待标定 */

/* ========== NTC 温度参数 ========== */
#define NTC_PULLUP_OHM          (10000.0f)
#define NTC_R0_OHM              (10000.0f)
#define NTC_BETA                (3950.0f)
#define NTC_T0_K                (298.15f)    /* 25°C */
#define TEMP_DERATING_START_C   (65.0f)
#define TEMP_DERATING_FULL_C    (85.0f)
#define TEMP_SHUTDOWN_C         (95.0f)

/* ========== 控制目标 ========== */
#define VBUS_TARGET_V           (380.0f)     /* PFC 母线目标 */
#define VBUS_MIN_START_V        (350.0f)     /* LLC 启动最小母线 */
#define LLC_ENTRY_V             (360.0f)
#define RUN_OK_LOW_V            (370.0f)
#define RUN_OK_HIGH_V           (390.0f)

#define VOUT_TARGET_V           (58.0f)      /* 12S 锂电满充 4.2×12=50.4, CC/CV */
#define VOUT_CC_V               (43.0f)      /* CC 阶段输出电压上限 */
#define VOUT_CV_V               (58.0f)      /* CV 阶段目标电压 */
#define VOUT_OVP_V              (80.0f)      /* 输出过压保护 */
#define IOUT_RATED_A            (20.0f)      /* 1.5kW/75V ≈ 20A */
#define IOUT_OCP_A              (30.0f)      /* 过流保护 */
#define IOUT_SOFT_START_A       (3.0f)       /* 软启电流限制 */


/*
 * 模拟采样网络（引脚号为 U17 的 LQFP64 封装引脚号）。
 * PA0、PC0~PC3 在原理图中未连接到采样网络，不能加入 ADC 扫描序列。
 */
 
#define AC_VOL_SAMPLE_PORT              GPIOA
#define AC_VOL_SAMPLE_PIN               GPIO_PIN_1
#define AC_VOL_SAMPLE_CH                ADC_CHANNEL_1   /* U17.15: AC_VOL_SENSE */

#define FAN_CS_SAMPLE_PORT              GPIOA
#define FAN_CS_SAMPLE_PIN               GPIO_PIN_2
#define FAN_CS_SAMPLE_CH                ADC_CHANNEL_2   /* U17.16: FAN_CS */

#define BUS_VOL_SAMPLE_PORT             GPIOA
#define BUS_VOL_SAMPLE_PIN              GPIO_PIN_3
#define BUS_VOL_SAMPLE_CH               ADC_CHANNEL_3   /* U17.17: BUS_VOL_SENSE */

#define T_SENSE_CASE_PORT               GPIOA
#define T_SENSE_CASE_PIN                GPIO_PIN_4
#define T_SENSE_CASE_CH                 ADC_CHANNEL_4   /* U17.20: T_SENSE_CASE */

#define VOUT_SENSE_PORT                 GPIOA
#define VOUT_SENSE_PIN                  GPIO_PIN_5
#define VOUT_SENSE_CH                   ADC_CHANNEL_5   /* U17.21: VOUT_SENSE */

#define I_SENSE_PORT                    GPIOA
#define I_SENSE_PIN                     GPIO_PIN_6
#define I_SENSE_CH                      ADC_CHANNEL_6   /* U17.22: I_SENSE */

#define T_SENSE_PFC_MOS_PORT            GPIOA
#define T_SENSE_PFC_MOS_PIN             GPIO_PIN_7
#define T_SENSE_PFC_MOS_CH              ADC_CHANNEL_7   /* U17.23: T_SENSE_PFC_MOS */

#define T_SENSE_TR_PORT                 GPIOC
#define T_SENSE_TR_PIN                  GPIO_PIN_4
#define T_SENSE_TR_CH                   ADC_CHANNEL_14  /* U17.24: T_SENSE_TR */

#define VBT_SENSE_PORT                  GPIOC
#define VBT_SENSE_PIN                   GPIO_PIN_5
#define VBT_SENSE_CH                    ADC_CHANNEL_15  /* U17.25: VBT_SENSE */

#define T_SENSE_LLC_MOS_PORT            GPIOB
#define T_SENSE_LLC_MOS_PIN             GPIO_PIN_1
#define T_SENSE_LLC_MOS_CH              ADC_CHANNEL_9   /* U17.27: T_SENSE_LLCMOS */

#define ADC_CHANNEL_QTY                 (10U)

/*
 * 模拟基准输出。
 * CC/CV 是两个独立的模拟给定量，不应配置成互补 PWM。原理图却将 CV_PWM 接到
 * PA8/TIMER0_CH0、CUR_PWM 接到 PB13/TIMER0_CH0_ON；这两个引脚共用 TIMER0 CH0
 * 比较寄存器，只能输出同一 PWM 的主/互补波形，无法独立调节电流和电压给定。
 * 在修改硬件引脚或确认模拟电路确实需要互补关系前，禁止同时启用这两个定时器输出。
 */
#define CV_PWM_PORT                     GPIOA
#define CV_PWM_PIN                      GPIO_PIN_8      /* U17.41: CV_PWM */
#define CV_PWM_RCU                      RCU_GPIOA
#define CV_PWM_TIMER                    TIMER0
#define CV_PWM_CH                       TIMER_CH_0

#define CUR_PWM_PORT                    GPIOA
#define CUR_PWM_PIN                     GPIO_PIN_0     /* U17.34: CUR-PWM */
#define CUR_PWM_RCU                     RCU_GPIOA
#define CUR_PWM_TIMER                   TIMER1
#define CUR_PWM_CH                      TIMER_CH_0
#define CC_CV_PWM_REQUIRE_INDEPENDENT   (1U)
#define CC_CV_PWM_PIN_CONFLICT          (1U)

#define PWM_BASE_HZ                     (20000U)
#define PWM_DUTY_SAFE                   (0.0f)

/* CV_PWM (电压基准) 占空比限制 */
#define CV_PWM_DUTY_MIN         (0.05f)         /* 最低占空比 (最低电压基准) */
#define CV_PWM_DUTY_MAX         (0.95f)         /* 最高占空比 (最高电压基准) */
#define CV_PWM_DUTY_INIT        (0.05f)         /* 初始最低 (安全) */

/* CC_PWM (电流基准) 占空比限制 */
#define CC_PWM_DUTY_MIN         (0.02f)         /* 最低占空比 (最小电流基准) */
#define CC_PWM_DUTY_MAX         (0.95f)         /* 最高占空比 (最大电流基准) */
#define CC_PWM_DUTY_INIT        (0.02f)         /* 初始最低 (安全) */

/* 占空比斜率限制 (防突变, per 1ms tick) */
#define PWM_DUTY_SLEW           (0.002f)

#if 1
/* LLC 软启动 (CV_PWM + CC_PWM 占空比缓升) */
#define LLC_SOFTSTART_DURATION_MS  (500U)
#define LLC_SOFTSTART_CV_START     (CV_PWM_DUTY_MIN)
#define LLC_SOFTSTART_CV_TARGET    (0.50f)      /* 软启目标电压基准, 待台架精标 */
#define LLC_SOFTSTART_CC_START     (CC_PWM_DUTY_MIN)
#define LLC_SOFTSTART_CC_TARGET    (0.30f)      /* 软启目标电流基准, 待台架精标 */
#endif

/* 功率级及继电器控制 */
#define LLC_EN_PORT                     GPIOB
#define LLC_EN_PIN                      GPIO_PIN_15     /* U17.36: LLC_EN */
#define LLC_EN_RCU                      RCU_GPIOB

#define HARD_FAULT_CLR_PORT             GPIOB
#define HARD_FAULT_CLR_PIN              GPIO_PIN_14     /* U17.35: HARD_FAULT_CLR */
#define HARD_FAULT_CLR_RCU              RCU_GPIOB

#define PFC_RELAY_PORT                  GPIOC
#define PFC_RELAY_PIN                   GPIO_PIN_10     /* U17.51: RELAY_PFC_EN_1 */
#define PFC_RELAY_RCU                   RCU_GPIOC

#define OUT_RELAY_PORT                  GPIOA
#define OUT_RELAY_PIN                   GPIO_PIN_9      /* U17.42: OUT_RELAY */
#define OUT_RELAY_RCU                   RCU_GPIOA

#define FAN_CTL_PORT                    GPIOC
#define FAN_CTL_PIN                     GPIO_PIN_12     /* U17.53: FAN_CTL */
#define FAN_CTL_RCU                     RCU_GPIOC

/* 故障输入：原理图没有把 HARD_PRO/BKIN 网络直接接到 U17。 */
#define LLC_FAULT_CHECK_PORT            GPIOB
#define LLC_FAULT_CHECK_PIN             GPIO_PIN_12     /* U17.33: LLC_FAULT_CHECK */
#define LLC_FAULT_CHECK_RCU             RCU_GPIOB

/* 指示灯；原理图中没有 MCU 控制的黄色 LED 网络。 */
#define LED_RED_PORT                    GPIOC
#define LED_RED_PIN                     GPIO_PIN_8      /* U17.39: RED_LED */
#define LED_RED_RCU                     RCU_GPIOC

#define LED_GREEN_PORT                  GPIOC
#define LED_GREEN_PIN                   GPIO_PIN_9      /* U17.40: GREEN_LED */
#define LED_GREEN_RCU                   RCU_GPIOC

/* CAN：U17 使用 PA11/PA12，而不是 PB8/PB9。 */
#define CAN0_RX_PORT                    GPIOA
#define CAN0_RX_PIN                     GPIO_PIN_11     /* U17.44: CAN_RXD */
#define CAN0_TX_PORT                    GPIOA
#define CAN0_TX_PIN                     GPIO_PIN_12     /* U17.45: CAN_TXD */
#define CAN0_GPIO_RCU                   RCU_GPIOA
#define CAN0_RCU                        RCU_CAN0
#define CAN_BAUDRATE                    (500000U)

/* 调试串口：PB10/PB11 对应 USART2。 */
#define DEBUG_USART                     USART2
#define DEBUG_USART_TX_PORT             GPIOB
#define DEBUG_USART_TX_PIN              GPIO_PIN_10     /* U17.29: TXD */
#define DEBUG_USART_RX_PORT             GPIOB
#define DEBUG_USART_RX_PIN              GPIO_PIN_11     /* U17.30: RXD */
#define DEBUG_USART_GPIO_RCU            RCU_GPIOB
#define DEBUG_USART_RCU                 RCU_USART2
#define DEBUG_USART_BAUDRATE            (115200U)

/* 板上 I2C/WP 网络。 */
#define BOARD_I2C_SCL_PORT              GPIOB
#define BOARD_I2C_SCL_PIN               GPIO_PIN_6      /* U17.58: SCL */
#define BOARD_I2C_SDA_PORT              GPIOB
#define BOARD_I2C_SDA_PIN               GPIO_PIN_7      /* U17.59: SDA */
#define BOARD_WP_PORT                   GPIOB
#define BOARD_WP_PIN                    GPIO_PIN_5      /* U17.57: WP1 */

/* 调试接口 */
#define SWD_DIO_PORT                    GPIOA
#define SWD_DIO_PIN                     GPIO_PIN_13
#define SWD_CLK_PORT                    GPIOA
#define SWD_CLK_PIN                     GPIO_PIN_14

/* ========== 中断优先级 (NVIC_PRIGROUP_PRE2_SUB2) ========== */
#define IRQ_PRIO_FAULT_PREEMPT      (0U)      /* 故障保护 最高 */
#define IRQ_PRIO_FAULT_SUB          (0U)
#define IRQ_PRIO_DMA_PREEMPT        (1U)      /* ADC DMA */
#define IRQ_PRIO_DMA_SUB            (0U)
#define IRQ_PRIO_SYSTICK_PREEMPT    (2U)      /* SysTick 1kHz */
#define IRQ_PRIO_SYSTICK_SUB        (0U)
#define IRQ_PRIO_MEASURE_PREEMPT    (2U)      /* TIMER3 100us */
#define IRQ_PRIO_MEASURE_SUB        (1U)
#define IRQ_PRIO_CAN_PREEMPT        (3U)      /* CAN RX */
#define IRQ_PRIO_CAN_SUB            (0U)

static inline uint8_t irq_priority_encode(uint8_t preempt, uint8_t sub)
{
    return (uint8_t)(((uint32_t)preempt << 2U) | ((uint32_t)sub & 0x03U));
}

/* 状态机类型保留为应用层公共接口。 */
typedef enum {
    MAIN_STEP_INIT = 0,
    MAIN_STEP_STANDBY,
    MAIN_STEP_PRECHARGE,
    MAIN_STEP_CC,
    MAIN_STEP_CV,
    MAIN_STEP_TRICKLE,
    MAIN_STEP_FINISHED,
    MAIN_STEP_FAULT,
    MAIN_STEP_INVALID
} charger_state_t;

typedef enum {
    FAULT_NONE = 0,
    FAULT_HARDWARE_PRO,         /* 硬件保护 (兼容旧代码, 现用于启动预检失败) */
    FAULT_LLC_CHECK,
    FAULT_OCP,
    FAULT_OVP,
    FAULT_BUS_UVP,
    FAULT_OVER_TEMP_PFC,
    FAULT_OVER_TEMP_LLC,
    FAULT_OVER_TEMP_TR,
    FAULT_OVER_TEMP_CASE,
    FAULT_CAN_TIMEOUT,
    FAULT_INVALID
} fault_type_t;

/* ========== ADC 启动自检 ========== */
#define ADC_STARTUP_V3V3_MIN_V      (2.8f)
#define ADC_STARTUP_V3V3_MAX_V      (3.6f)
#define ADC_STARTUP_SAMPLE_COUNT    (8U)
#define ADC_STARTUP_SAMPLE_DELAY_MS (2U)

/* ========== 全局变量 ========== */
extern volatile uint32_t g_ms;
extern charger_state_t g_charger_state;
extern fault_type_t g_fault;

void delay_ms(uint32_t duration_ms);

static inline uint32_t elapsed_since(uint32_t start_ms)
{
    return (uint32_t)(g_ms - start_ms);
}

static inline bool elapsed_reached(uint32_t start_ms, uint32_t duration_ms)
{
    return elapsed_since(start_ms) >= duration_ms;
}

static inline float f_clampf(float value, float low, float high)
{
    return (value < low) ? low : ((value > high) ? high : value);
}

/* ========== 100us 快速采样 / 主循环节流 ========== */
#define ADC_FAST_SAMPLE_HZ          (10000U)   /* 100us 采样率 = 10kHz */
#define FAST_ADC_MAX_TICKS_PER_LOOP (10U)      /* 单次主循环最多追补的 100us tick */
#define MAX_TICKS_PER_LOOP          (10U)      /* 单次主循环最多追补的 1ms tick */

/* CC_PWM 别名 (与 CUR_PWM 同引脚 PA0/TIMER1_CH0, 兼容 pwm_llc.c 命名) */
#define CC_PWM_PORT                 CUR_PWM_PORT
#define CC_PWM_PIN                  CUR_PWM_PIN
#define CC_PWM_RCU                  CUR_PWM_RCU
#define CC_PWM_TIMER                CUR_PWM_TIMER
#define CC_PWM_CH                   CUR_PWM_CH

#endif
