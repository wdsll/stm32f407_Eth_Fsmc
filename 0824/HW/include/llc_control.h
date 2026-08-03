/* llc_control.h - LLC 模拟控制接口 (MCU 仅设定 CV_PWM 电压基准 + CC_PWM 电流基准)
 * LLC 模拟IC 内部做电压闭环 + 限流闭环, MCU 不做 PI */
#ifndef _LLC_CONTROL_H_
#define _LLC_CONTROL_H_

#include "main.h"

typedef enum {
    LLC_STATE_IDLE = 0,
    LLC_STATE_SOFTSTART,    /* CV_PWM + CC_PWM 占空比缓升 */
    LLC_STATE_RUN,          /* 基准已设定, 模拟IC 闭环工作 */
    LLC_STATE_STOPPING,     /* 占空比缓降到最低 */
    LLC_STATE_FAULT
} llc_state_t;

typedef enum {
    LLC_MODE_CC = 0,        /* 恒流: CC_PWM=目标电流基准, CV_PWM=电压上限 */
    LLC_MODE_CV             /* 恒压: CV_PWM=目标电压基准, CC_PWM=电流上限 */
} llc_mode_t;

void llc_app_init(void);
void llc_app_tick_1khz(void);
void llc_app_tick_100us(void);
void llc_app_enable(void);
void llc_app_disable(void);
void llc_app_set_mode(llc_mode_t mode);
void llc_app_set_target(float target);   /* CC: 电流(A), CV: 电压(V) */
void llc_app_set_cv_limit(float v_max);  /* CC 模式下的电压上限 */
void llc_app_set_cc_limit(float i_max);  /* CV 模式下的电流上限 */
llc_state_t llc_app_get_state(void);

/* 占空比?物理量 标定 (待台架整定) */
float vout_to_cv_duty(float vout_v);     /* 目标电压 → CV_PWM 占空比 */
float iout_to_cc_duty(float iout_a);     /* 目标电流 → CC_PWM 占空比 */

/* 充电状态机 (实现在 llc_control.c) */
void charger_state_machine_tick(void);

#endif /* _LLC_CONTROL_H_ */
