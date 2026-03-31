#ifndef PFC_CONTROL_H_
#define PFC_CONTROL_H_
#include "main.h"
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef enum {
    PFC_ST_IDLE = 0,
	  PFC_ST_RAMP,
    PFC_ST_READY,
    PFC_ST_FAULT,
} pfc_state_t;

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
/* ===== Hardware scaling ===== */
#define PFC_VBUS_ADC_GAIN_V_PER_VIN   (175.060f)   // VBUS = ADC_V * 144.3464 175.06
#define PFC_AC_ADC_GAIN_V_PER_VIN     (369.98f)     // VAC  = ADC_V * 233.38  
#define PFC_AC_OFFSET                 (12.90f)

/* ===== 44/45/46V 临时测试版 =====
 * 这几个点的母线目标统一按 44*9 = 390V
 * 目的：先验证降低母线后，LLC 是否更接近谐振点，从而减轻输出坍陷
 */
#define PFC_VBUS_TARGET_TEST_V        (390.0f)
/* ===== 三态机：门限/时序 ===== */
#define PFC_STARTUP_DELAY_MS          (1000U)

/* READY判定：390V ± 10V */
#define PFC_VBUS_READY_V              (390.0f)
#define PFC_READY_DELAY_MS            (80U)

/* 硬件拉起后等待母线建立 */
#define PFC_VBUS_RAMP_DELAY_MS        (100U)
#define PFC_VBUS_RAMP_TIMEOUT_MS      (800U)
/* READY状态下的掉压保护
 * 369 - 24 = 345V
 * 这个值不要压得太低，否则母线已经明显掉下来了，状态机还不退出
 */
#define PFC_VBUS_OK_RESET_MARGIN_V    (10.0f)
#define PFC_VBUS_DROPOUT_THRESHOLD_V  (360.0f)
#define PFC_VBUS_DROPOUT_MS           (300U)

/* 过压保护
 * 临时测试版不再保留 450V 那么高的固定阈值
 * 这里先按 369 + 31 = 400V
 * 这样既能覆盖一定波动，又不会让目标 369V 的工况过压窗口过大
 */
#define PFC_VBUS_OVP_V                (420.0f)
#define PFC_FAULT_RESTART_MS          (1000U)

/* ===== AC check ===== */
#define PFC_AC_VALID_MIN_VRMS         (85.0f)
#define PFC_AC_VALID_MAX_VRMS         (265.0f)
#define PFC_AC_OVERVOLTAGE_MARGIN_V   (5.0f)
#define PFC_AC_LOSS_DEBOUNCE_MS       (200U)
#define PFC_AC_OK_DEBOUNCE_MS         (200U)
/* ===== 上电自检：VBUS≈1.414*VAC（仅在未使能硬件前检查） ===== */
#define PFC_VBUS_VAC_RATIO            (1.414f)
#define PFC_VBUS_VAC_RATIO_TOLERANCE  (0.05f)
#define PFC_VBUS_VAC_RATIO_STABLE_MS  (50U)


/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
void pfc_init(void);
void pfc_enable(void);
void pfc_disable(void);
void pfc_tick_1khz(void);
void adc_test(void);
float pfc_get_vbus(void);
float pfc_get_vac(void);

pfc_state_t pfc_state(void);
bool pfc_is_ready(void);
bool pfc_is_fault(void);
bool pfc_is_fault_latched(void);

void pfc_clear_fault(void);

/* 硬件控制（在 c 文件里实现） */
void pfc_hw_set_main(bool on);
/* 兼容/便捷接口：给 LLC 直接取母线电压 */
float pfc_bus_voltage(void);

#endif