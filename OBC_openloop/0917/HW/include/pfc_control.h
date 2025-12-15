#ifndef PFC_CONTROL_H_
#define PFC_CONTROL_H_
#include "main.h"
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef enum {
    PFC_ST_OFF = 0,
    PFC_ST_WAIT_AC,
    PFC_ST_PRECHARGE,
    PFC_ST_WAIT_RELAY,
    PFC_ST_RAMP_UP,
    PFC_ST_RUN,
    PFC_ST_FAULT,
} pfc_state_t;
/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
void pfc_init(void);
void pfc_enable(void);
void pfc_disable(void);
void pfc_set_vbus_target(float vbus_v);
void pfc_tick_1khz(void);
pfc_state_t pfc_state(void);

float pfc_get_vbus(void);
float pfc_get_vac(void);
float pfc_get_temp_pfc(void);
bool pfc_is_ready(void);
bool pfc_is_fault(void);
bool pfc_is_fault_latched(void);
void pfc_clear_fault(void);

/* Compatibility wrappers used by existing application code */
static inline void pfc_app_init(void) { pfc_init(); }
static inline void pfc_app_request_start(void) { pfc_enable(); }
static inline void pfc_app_force_off(void) { pfc_disable(); }
static inline void pfc_app_tick_1khz(float vbus_v) { (void)vbus_v; pfc_tick_1khz(); }
static inline pfc_state_t pfc_app_state(void) { return pfc_state(); }
static inline bool pfc_app_ready(void) { return pfc_is_ready(); }
static inline float pfc_bus_voltage(void) { return pfc_get_vbus(); }
void pfc_hw_set_relay(bool closed);
#endif