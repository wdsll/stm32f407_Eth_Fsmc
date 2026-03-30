#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include "gd32f30x.h"
#include <stdbool.h>
typedef struct
{
        float target_v;
        float kp;
        float ki;
        float integ;
        float neutral_duty;
        float duty_min;
        float duty_max;
        float duty_cmd;
} bus_vol_adj_ctrl_t;

void pb0_pwm_init(uint32_t pwm_hz);
void pb0_pwm_set_duty(float duty);

void bus_vol_adj_init(void);
void bus_vol_adj_reset(void);
void bus_vol_adj_tick(float vbus, bool enabled);

void bus_vol_adj_set_target_vbus(float target_vbus);
float bus_vol_adj_target_from_vout(float vout_ref);
void bus_vol_adj_follow_vout(float vout_ref, float vout_meas, float vbus_meas, bool enabled);
#endif

