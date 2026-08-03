/*********************************************************************************************************
* 模块名称：pwm_llc.c
* 摘    要：CV_PWM + CC_PWM 双独立基准输出,CV_PWM(电压基准) + CC_PWM(电流基准) → RC滤波 → LLC 模拟IC 内部闭环
* 作    者：Rengar
* 内    容：CV_PWM: PA8 / TIMER0_CH0 (高级定时器, 仅主输出, 不使能互补; 需 MOE), CC_PWM: PA0 / TIMER1_CH0 (通用定时器, 独立比较单元)
* 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
*          
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "pwm_llc.h"
#include <string.h>
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static float s_cv_duty = CV_PWM_DUTY_INIT;
static float s_cc_duty = CC_PWM_DUTY_INIT;
/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/
/* TIMER1 在 APB1: 若 APB1 分频≠1, 定时器时钟 = 2×APB1 */
static uint32_t timer1_clk_hz(void)
{
    uint32_t apb1 = rcu_clock_freq_get(CK_APB1);
    return (RCU_CFG0 & RCU_CFG0_APB1PSC) ? (apb1 * 2U) : apb1;
}

/* ========== CV_PWM (TIMER0 CH0, PA8) ========== */
void cv_pwm_init(uint32_t freq_hz, float duty)
{
    rcu_periph_clock_enable(CV_PWM_RCU);      /* GPIOA */
    rcu_periph_clock_enable(RCU_TIMER0);    /* TIMER0 (APB2) */
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(CV_PWM_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, CV_PWM_PIN);
    timer_deinit(CV_PWM_TIMER);

    /* TIMER0 在 APB2 (APB2 分频=1, 定时器时钟 = APB2) */
    uint32_t tclk = rcu_clock_freq_get(CK_APB2);
    uint32_t period = tclk / freq_hz;
    if (period > 0U) period -= 1U;

    timer_parameter_struct t;
    timer_struct_para_init(&t);
    t.prescaler = 0U;
    t.counterdirection = TIMER_COUNTER_UP;
    t.period = period;
    t.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(CV_PWM_TIMER, &t);
	
	  /* CH0 主输出, 不使能互补 (仅作 RC 基准, 无需死区) */
    timer_oc_parameter_struct oc;
    memset(&oc, 0, sizeof(oc));
    oc.outputstate  = TIMER_CCX_ENABLE;
    oc.outputnstate = TIMER_CCXN_DISABLE;
    oc.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    oc.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    oc.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    oc.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(CV_PWM_TIMER, CV_PWM_CH, &oc);

    s_cv_duty = f_clampf(duty, CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX);
    timer_channel_output_pulse_value_config(CV_PWM_TIMER, CV_PWM_CH,
        (uint16_t)((float)period * s_cv_duty));
    timer_channel_output_mode_config(CV_PWM_TIMER, CV_PWM_CH, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(CV_PWM_TIMER, CV_PWM_CH, TIMER_OC_SHADOW_ENABLE);

    /* 高级定时器必须使能初级输出 (MOE), 否则 PA8 无波形 */
    timer_primary_output_config(CV_PWM_TIMER, ENABLE);
    timer_auto_reload_shadow_enable(CV_PWM_TIMER);
    timer_enable(CV_PWM_TIMER);
}

void cv_pwm_set_duty(float duty)
{
    s_cv_duty = f_clampf(duty, CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX);
    uint32_t period = TIMER_CAR(CV_PWM_TIMER);
    timer_channel_output_pulse_value_config(CV_PWM_TIMER, CV_PWM_CH,
        (uint16_t)((float)period * s_cv_duty));
}

float cv_pwm_get_duty(void) { return s_cv_duty; }

/* ========== CC_PWM (TIMER1 CH0, PA0) ========== */
void cc_pwm_init(uint32_t freq_hz, float duty)
{
    rcu_periph_clock_enable(CC_PWM_RCU);      /* GPIOA */
    rcu_periph_clock_enable(RCU_TIMER1);    /* TIMER1 (APB1) */
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(CC_PWM_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, CC_PWM_PIN);
    timer_deinit(CC_PWM_TIMER);

    uint32_t tclk = timer1_clk_hz();
    uint32_t period = tclk / freq_hz;
    if (period > 0U) period -= 1U;

    timer_parameter_struct t;
    timer_struct_para_init(&t);
    t.prescaler = 0U;
    t.counterdirection = TIMER_COUNTER_UP;
    t.period = period;
    t.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(CC_PWM_TIMER, &t);

    timer_oc_parameter_struct oc;
    memset(&oc, 0, sizeof(oc));
    oc.outputstate = TIMER_CCX_ENABLE;
    oc.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_channel_output_config(CC_PWM_TIMER, CC_PWM_CH, &oc);

    s_cc_duty = f_clampf(duty, CC_PWM_DUTY_MIN, CC_PWM_DUTY_MAX);
    timer_channel_output_pulse_value_config(CC_PWM_TIMER, CC_PWM_CH,
        (uint16_t)((float)period * s_cc_duty));
    timer_channel_output_mode_config(CC_PWM_TIMER, CC_PWM_CH, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(CC_PWM_TIMER, CC_PWM_CH, TIMER_OC_SHADOW_ENABLE);
    timer_auto_reload_shadow_enable(CC_PWM_TIMER);
    timer_enable(CC_PWM_TIMER);
}

void cc_pwm_set_duty(float duty)
{
    s_cc_duty = f_clampf(duty, CC_PWM_DUTY_MIN, CC_PWM_DUTY_MAX);
    uint32_t period = TIMER_CAR(CC_PWM_TIMER);
    timer_channel_output_pulse_value_config(CC_PWM_TIMER, CC_PWM_CH,
        (uint16_t)((float)period * s_cc_duty));
}

float cc_pwm_get_duty(void) { return s_cc_duty; }


