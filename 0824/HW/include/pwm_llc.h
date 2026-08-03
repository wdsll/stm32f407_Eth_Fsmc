/* pwm_llc.h - CV_PWM + CC_PWM 双独立基准输出
 * CV_PWM(PA8/TIMER0_CH0)  → RC滤波 → LLC IC 电压基准 (模拟IC内部电压闭环)
 * CC_PWM(PA0/TIMER1_CH0)  → RC滤波 → LLC IC 电流基准 (模拟IC内部限流闭环)
 * 两路独立定时器/比较单元, 占空比互不相关。MCU 仅设定两路基准, 不做 PI。 */
#ifndef _PWM_LLC_H_
#define _PWM_LLC_H_

#include "main.h"

/* CV_PWM: 电压基准 */
void cv_pwm_init(uint32_t freq_hz, float duty);
void cv_pwm_set_duty(float duty);
float cv_pwm_get_duty(void);

/* CC_PWM: 电流基准 */
void cc_pwm_init(uint32_t freq_hz, float duty);
void cc_pwm_set_duty(float duty);
float cc_pwm_get_duty(void);

#endif /* _PWM_LLC_H_ */
