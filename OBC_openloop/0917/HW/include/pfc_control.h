#ifndef PFC_CONTROL_H_
#define PFC_CONTROL_H_
#include "main.h"
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef enum {
    PFC_ST_IDLE = 0,
    PFC_ST_READY,
    PFC_ST_FAULT,
} pfc_state_t;

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
/* ===== Hardware scaling ===== */
#define PFC_VBUS_ADC_GAIN_V_PER_VIN   (144.3464f)   // VBUS = ADC_V * 144.3464
#define PFC_AC_ADC_GAIN_V_PER_VIN     (233.38f)     // VAC  = ADC_V * 233.38

/* ===== 三态机：门限/时序 ===== */
#define PFC_STARTUP_DELAY_MS          (200U)
#define PFC_VBUS_READY_V              (375.0f)
#define PFC_READY_DELAY_MS            (80U)
#define PFC_VBUS_OK_RESET_MARGIN_V    (10.0f)
#define PFC_VBUS_DROPOUT_THRESHOLD_V  (340.0f)
#define PFC_VBUS_DROPOUT_MS           (300U)
#define PFC_VBUS_OVP_V                (430.0f)
#define PFC_FAULT_RESTART_MS          (2000U)

/* ===== AC check ===== */
#define PFC_AC_VALID_MIN_VRMS         (85.0f)
#define PFC_AC_VALID_MAX_VRMS         (265.0f)
#define PFC_AC_OVERVOLTAGE_MARGIN_V   (5.0f)
#define PFC_AC_LOSS_DEBOUNCE_MS       (200U)
#define PFC_AC_OK_DEBOUNCE_MS         (200U)
/* ===== 上电自检：VBUS≈1.414*VAC（仅在未使能硬件前检查） ===== */
#define PFC_VBUS_VAC_RATIO            (1.414f)
#define PFC_VBUS_VAC_RATIO_TOLERANCE  (0.15f)
#define PFC_VBUS_VAC_RATIO_STABLE_MS  (50U)
/* --- 启动与 READY --- */
#define PFC_STARTUP_DELAY_MS          (200U)
#define PFC_VBUS_READY_V              (375.0f)
#define PFC_READY_DELAY_MS            (80U)
#define PFC_VBUS_OK_RESET_MARGIN_V    (10.0f)

/* --- READY 掉电退出 --- */
#define PFC_VBUS_DROPOUT_THRESHOLD_V  (340.0f)
#define PFC_VBUS_DROPOUT_MS           (300U)

/* --- 保护 --- */
#define PFC_VBUS_OVP_V                (430.0f)
#define PFC_FAULT_RESTART_MS          (2000U)
/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
void pfc_init(void);
void pfc_enable(void);
void pfc_disable(void);
void pfc_tick_1khz(void);

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