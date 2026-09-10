/* llc_control.h - LLC 模拟控制接口 (MCU 仅设定 CV_PWM 电压基准 + CC_PWM 电流基准)
 * LLC 模拟IC 内部做电压闭环 + 限流闭环, MCU 不做 PI */
#ifndef _LLC_CONTROL_H_
#define _LLC_CONTROL_H_

#include "main.h"

#define LLC_CV_REFERENCE_MIN_V  (32.3f)
#define LLC_CV_REFERENCE_MAX_V  (85.0f)
#define LLC_CC_REFERENCE_MIN_A  (0.0f)
#define LLC_CC_REFERENCE_MAX_A  (20.0f)

/* External analog ICs own the closed loops; these APIs only set PWM references. */
void llc_enable(void);
void llc_disable(void);

void power_supervisor_init(void);
void power_supervisor_tick_1khz(void);
void power_supervisor_enter_fault(void);
void power_supervisor_request(bool enable);
bool power_supervisor_requested(void);
bool power_supervisor_set_references(float voltage_v, float current_a);
float power_supervisor_voltage_reference(void);
float power_supervisor_current_reference(void);

#endif /* _LLC_CONTROL_H_ */
