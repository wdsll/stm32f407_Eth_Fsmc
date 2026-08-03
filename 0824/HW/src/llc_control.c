/*********************************************************************************************************
* 模块名称：llc_control.c
* 摘    要：
* 作    者：Rengar
* 内    容：llc_control.c - LLC 模拟控制状态机 (CV_PWM + CC_PWM 双基准)
						* MCU 仅设定 CV_PWM(电压基准) + CC_PWM(电流基准), 模拟IC 内部闭环
						* CC 模式: CC_PWM=目标电流, CV_PWM=电压上限
						* CV 模式: CV_PWM=目标电压, CC_PWM=电流上限 
* 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
*          
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "llc_control.h"
#include "pwm_llc.h"
#include "pfc_control.h"
#include "adc_dma.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static llc_state_t s_llc_state   = LLC_STATE_IDLE;
static llc_mode_t  s_llc_mode    = LLC_MODE_CV;
static float       s_target      = VOUT_CV_V;     /* CC:目标电流  CV:目标电压 */
static float       s_cv_limit    = VOUT_CC_V;     /* CC模式电压上限 */
static float       s_cc_limit    = IOUT_RATED_A;  /* CV模式电流上限 */
static uint32_t    s_state_ms    = 0U;
static float       s_cv_duty_cmd = CV_PWM_DUTY_INIT;
static float       s_cc_duty_cmd = CC_PWM_DUTY_INIT;
/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/

/* ========== 占空比物理量标定 (线性映射, 待台架精标) ========== */
/* CV_PWM: 0.05~0.95 → 0~80V (待标定) */
float vout_to_cv_duty(float vout_v)
{
    float d = vout_v / 80.0f;   /* 线性: 0V→0%, 80V→100% */
    return f_clampf(d, CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX);
}

/* CC_PWM: 0.02~0.95 → 0~20A (待标定) */
float iout_to_cc_duty(float iout_a)
{
    float d = iout_a / 20.0f;   /* 线性: 0A→0%, 20A→100% */
    return f_clampf(d, CC_PWM_DUTY_MIN, CC_PWM_DUTY_MAX);
}

/* ========== 状态机 ========== */
void llc_app_init(void)
{
    s_llc_state   = LLC_STATE_IDLE;
    s_cv_duty_cmd = CV_PWM_DUTY_INIT;
    s_cc_duty_cmd = CC_PWM_DUTY_INIT;
    cv_pwm_set_duty(s_cv_duty_cmd);
    cc_pwm_set_duty(s_cc_duty_cmd);
}

void llc_app_enable(void)
{
    if (s_llc_state == LLC_STATE_IDLE) {
        s_llc_state   = LLC_STATE_SOFTSTART;
        s_state_ms    = g_ms;
        s_cv_duty_cmd = LLC_SOFTSTART_CV_START;
        s_cc_duty_cmd = LLC_SOFTSTART_CC_START;
        cv_pwm_set_duty(s_cv_duty_cmd);
        cc_pwm_set_duty(s_cc_duty_cmd);
        gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);   /* 使能 LLC 模拟IC */
    }
}

void llc_app_disable(void)
{
    if (s_llc_state != LLC_STATE_IDLE && s_llc_state != LLC_STATE_FAULT) {
        s_llc_state = LLC_STATE_STOPPING;
        s_state_ms  = g_ms;
    }
}

void llc_app_set_mode(llc_mode_t mode) { s_llc_mode = mode; }
void llc_app_set_target(float target)  { s_target = target; }
void llc_app_set_cv_limit(float v_max) { s_cv_limit = v_max; }
void llc_app_set_cc_limit(float i_max) { s_cc_limit = i_max; }
llc_state_t llc_app_get_state(void)    { return s_llc_state; }

void llc_app_tick_1khz(void)
{
	  switch (s_llc_state) {
    case LLC_STATE_IDLE:
        break;
		case LLC_STATE_SOFTSTART: {
        uint32_t elapsed = elapsed_since(s_state_ms);
        if (elapsed >= LLC_SOFTSTART_DURATION_MS) {
            s_llc_state = LLC_STATE_RUN;
            s_state_ms  = g_ms;
        } else {
            float p = (float)elapsed / (float)LLC_SOFTSTART_DURATION_MS;
            float ease = 0.5f * (1.0f - cosf(3.14159f * p));
            /* CV: MIN → 软启目标 */
            s_cv_duty_cmd = LLC_SOFTSTART_CV_START +
                (LLC_SOFTSTART_CV_TARGET - LLC_SOFTSTART_CV_START) * ease;
            /* CC: MIN → 软启目标 */
            s_cc_duty_cmd = LLC_SOFTSTART_CC_START +
                (LLC_SOFTSTART_CC_TARGET - LLC_SOFTSTART_CC_START) * ease;
            cv_pwm_set_duty(s_cv_duty_cmd);
            cc_pwm_set_duty(s_cc_duty_cmd);
        }
        break;
    }
		case LLC_STATE_RUN:{
			  /* 根据充电模式设定 CV_PWM + CC_PWM 基准 (模拟IC内部闭环) */
        float cv_target_duty, cc_target_duty;

        if (s_llc_mode == LLC_MODE_CC) {
            /* 恒流: CC_PWM=目标电流, CV_PWM=电压上限 */
            cc_target_duty = iout_to_cc_duty(s_target);
            cv_target_duty = vout_to_cv_duty(s_cv_limit);
        } else {
            /* 恒压: CV_PWM=目标电压, CC_PWM=电流上限 */
            cv_target_duty = vout_to_cv_duty(s_target);
            cc_target_duty = iout_to_cc_duty(s_cc_limit);
        }
				
				/* 斜率限制 (防基准突变) */
        if (fabsf(cv_target_duty - s_cv_duty_cmd) > PWM_DUTY_SLEW)
            s_cv_duty_cmd += (cv_target_duty > s_cv_duty_cmd ? PWM_DUTY_SLEW : -PWM_DUTY_SLEW);
        else
            s_cv_duty_cmd = cv_target_duty;

        if (fabsf(cc_target_duty - s_cc_duty_cmd) > PWM_DUTY_SLEW)
            s_cc_duty_cmd += (cc_target_duty > s_cc_duty_cmd ? PWM_DUTY_SLEW : -PWM_DUTY_SLEW);
        else
            s_cc_duty_cmd = cc_target_duty;

        cv_pwm_set_duty(s_cv_duty_cmd);
        cc_pwm_set_duty(s_cc_duty_cmd);
        break;
		}
		case LLC_STATE_STOPPING:{
			  /* 软关断: 两路占空比缓降到最低 */
        s_cv_duty_cmd -= PWM_DUTY_SLEW * 5.0f;
        s_cc_duty_cmd -= PWM_DUTY_SLEW * 5.0f;
        if (s_cv_duty_cmd <= CV_PWM_DUTY_MIN && s_cc_duty_cmd <= CC_PWM_DUTY_MIN) {
            s_cv_duty_cmd = CV_PWM_DUTY_MIN;
            s_cc_duty_cmd = CC_PWM_DUTY_MIN;
            cv_pwm_set_duty(s_cv_duty_cmd);
            cc_pwm_set_duty(s_cc_duty_cmd);
            gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);
            s_llc_state = LLC_STATE_IDLE;
            break;
        }
        cv_pwm_set_duty(s_cv_duty_cmd);
        cc_pwm_set_duty(s_cc_duty_cmd);
        break;
		}
		case LLC_STATE_FAULT:
        cv_pwm_set_duty(CV_PWM_DUTY_MIN);
        cc_pwm_set_duty(CC_PWM_DUTY_MIN);
        gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);
        break;
    }
}

/* ========== 100us 快速环 (软件 OCP/OVP 前馈) ========== */
void llc_app_tick_100us(void)
{
    if (s_llc_state == LLC_STATE_RUN) {
        /* 软件过流: 快速降 CC_PWM 基准 (硬件 HARD_PRO 兜底) */
        if (g_adc_multi.iout_a > IOUT_OCP_A) {
            s_cc_duty_cmd -= PWM_DUTY_SLEW * 10.0f;
            if (s_cc_duty_cmd < CC_PWM_DUTY_MIN) s_cc_duty_cmd = CC_PWM_DUTY_MIN;
            cc_pwm_set_duty(s_cc_duty_cmd);
        }
        /* 软件过压: 快速降 CV_PWM 基准 */
        if (g_adc_multi.vout_v > VOUT_OVP_V) {
            s_cv_duty_cmd -= PWM_DUTY_SLEW * 10.0f;
            if (s_cv_duty_cmd < CV_PWM_DUTY_MIN) s_cv_duty_cmd = CV_PWM_DUTY_MIN;
            cv_pwm_set_duty(s_cv_duty_cmd);
        }
    }
}
/* ========== 充电状态机 (设定 CV_PWM + CC_PWM 基准) ========== */
void charger_state_machine_tick(void)
{
	extern adc_multi_t g_adc_multi;
	switch(g_charger_state) {
	    case MAIN_STEP_STANDBY:
        if (g_adc_multi.vbat_v > 10.0f && pfc_is_ready()) {
            g_charger_state = MAIN_STEP_PRECHARGE;
            /* 软启阶段: CC模式, 目标=额定电流, 电压上限=CC电压 */
            llc_app_set_mode(LLC_MODE_CC);
            llc_app_set_target(IOUT_RATED_A);
            llc_app_set_cv_limit(VOUT_CC_V);
            llc_app_enable();
            gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
            gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
        }
        break;
			case MAIN_STEP_PRECHARGE:
        if (llc_app_get_state() == LLC_STATE_RUN &&
            g_adc_multi.vout_v > (VOUT_CC_V * 0.8f)) {
            g_charger_state = MAIN_STEP_CC;
            gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
        }
        break;
			case MAIN_STEP_CV:
        /* 恒压: CV_PWM=CV电压, CC_PWM=额定电流上限 */
        if (g_adc_multi.iout_a < (IOUT_RATED_A * 0.1f)) {
            g_charger_state = MAIN_STEP_TRICKLE;
            /* 涓流: CC模式, 目标=1A, 电压上限=CV电压 */
            llc_app_set_mode(LLC_MODE_CC);
            llc_app_set_target(1.0f);
            llc_app_set_cv_limit(VOUT_CV_V);
        }
        break;
	
	     case MAIN_STEP_TRICKLE:
        if (g_adc_multi.iout_a < 0.3f) {
            g_charger_state = MAIN_STEP_FINISHED;
        }
        break;

      case MAIN_STEP_FINISHED:
        llc_app_disable();
        gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);
        gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
        break;

    case MAIN_STEP_FAULT:
        llc_app_disable();
        gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);
        gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
        break;
	
		default:
			break;
    }
	
}












