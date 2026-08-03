/* pfc_control.h - PFC 控制 (NCP1654 外置, MCU 监控+继电器) */
#ifndef _PFC_CONTROL_H_
#define _PFC_CONTROL_H_
#include "main.h"

typedef enum {
    PFC_STATE_OFF = 0,
    PFC_STATE_RELAY_ON,    /* 继电器闭合, 等母线建立 */
    PFC_STATE_RUN,         /* NCP1654 工作, MCU 监控 */
    PFC_STATE_FAULT
} pfc_state_t;

void pfc_init(void);
void pfc_enable(void);
void pfc_disable(void);
void pfc_tick_1khz(void);
pfc_state_t pfc_get_state(void);
bool pfc_is_ready(void);

#endif
