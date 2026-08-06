/* PWM references converted to DC set-points by the board RC filters. */
#ifndef _PWM_LLC_H_
#define _PWM_LLC_H_

#include "main.h"

/* These outputs set analog references; they are not power-stage PWM drives. */
void cv_pwm_init(uint32_t freq_hz, float duty);
void cv_pwm_set_duty(float duty);
float cv_pwm_get_duty(void);

void cc_pwm_init(uint32_t freq_hz, float duty);
void cc_pwm_set_duty(float duty);
float cc_pwm_get_duty(void);

#endif
