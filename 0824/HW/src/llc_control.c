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
#include "adc_dma.h"
#include "can_comm.h"
#include "pfc_control.h"
#include "protect.h"
#include "pwm_llc.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define BATTERY_PRESENT_V       (10.0f)
#define OUTPUT_RELAY_MIN_V      (10.0f)
#define LLC_START_TIMEOUT_MS    (1500U)
#define CAN_STATUS_PERIOD_MS    (100U)
#define CV_REFERENCE_MAX_V      (80.0f)
#define CC_REFERENCE_MAX_A      (20.0f)
/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static bool s_enable_requested;
static float s_voltage_reference_v;
static float s_current_reference_a;
static uint32_t s_state_started_ms;
static uint32_t s_last_status_ms;
/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/
static float voltage_to_duty(float voltage_v)
{
    return f_clampf(voltage_v / CV_REFERENCE_MAX_V,
                    CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX);
}

static float current_to_duty(float current_a)
{
    return f_clampf(current_a / CC_REFERENCE_MAX_A,
                    CC_PWM_DUTY_MIN, CC_PWM_DUTY_MAX);
}

static void apply_references(void)
{
    /* Open-loop set-point conversion only; ADC feedback is not used here. */
    cv_pwm_set_duty(voltage_to_duty(s_voltage_reference_v));
    cc_pwm_set_duty(current_to_duty(s_current_reference_a));
}

static void outputs_off(void)
{
	  gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);
    gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);
    pfc_disable();
}

static void enter_state(charger_state_t state)
{
	  g_charger_state = state;
    s_state_started_ms = g_ms;
}
/* ========== 状态机 ========== */
void power_supervisor_init(void)
{
    s_enable_requested = false;
	  s_voltage_reference_v = 0.0f;
    s_current_reference_a = 0.0f;
    s_last_status_ms = g_ms;
	  cv_pwm_set_duty(CV_PWM_DUTY_INIT);
    cc_pwm_set_duty(CC_PWM_DUTY_INIT);
    outputs_off();
    enter_state(MAIN_STEP_STANDBY);
}

void power_supervisor_request(bool enable)
{
		s_enable_requested = enable;
}

bool power_supervisor_requested(void)
{
	return s_enable_requested;
}

void power_supervisor_set_references(float voltage_v, float current_a)
{
    s_voltage_reference_v = f_clampf(voltage_v, 0.0f, CV_REFERENCE_MAX_V);
    s_current_reference_a = f_clampf(current_a, 0.0f, CC_REFERENCE_MAX_A);
    apply_references();
}

float power_supervisor_voltage_reference(void) { return s_voltage_reference_v; }
float power_supervisor_current_reference(void) { return s_current_reference_a; }

void power_supervisor_tick_1khz(void)
{
	can_rx_data_t command;
	if (can_comm_get_cmd(&command)) {
		power_supervisor_set_references((float)command.vout_set_x10 * 0.1f,
                                        (float)command.iout_set_x10 * 0.1f);
		power_supervisor_request(command.enable != 0U);
	}
	if (elapsed_reached(s_last_status_ms, CAN_STATUS_PERIOD_MS)) {
		s_last_status_ms = g_ms;
		can_comm_tx_status();
  }
	if (s_enable_requested && can_comm_timeout()) {
		protect_set_fault(FAULT_CAN_TIMEOUT);
	}
	if (g_charger_state == MAIN_STEP_FAULT) {
		outputs_off();
		gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
		gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
		return;
	}
	switch(g_charger_state)
	{
		case MAIN_STEP_STANDBY:
		{
			outputs_off();
			gpio_bit_set(LED_RED_PORT,LED_RED_PIN);
			gpio_bit_reset(LED_GREEN_PORT,LED_GREEN_PIN);
			if(s_enable_requested && g_adc_multi.vbat_v >= BATTERY_PRESENT_V)
			{
				pfc_enable();
				enter_state(MAIN_STEP_PRECHARGE);
			}
			break;
		}
		case MAIN_STEP_PRECHARGE:
		{
			 if (!s_enable_requested) {
         outputs_off();
         enter_state(MAIN_STEP_STANDBY);
       } else if (pfc_is_ready()) {
				 apply_references();
				 gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);
				 enter_state(MAIN_STEP_CC); /* Legacy CAN state value: running. */
			 }
			 break;
		}
		case MAIN_STEP_CC:
		{
			if(!s_enable_requested)
			{
				 outputs_off();
				 enter_state(MAIN_STEP_STANDBY);
			}
			else if(g_adc_multi.vout_v > OUTPUT_RELAY_MIN_V)
			{
				 gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
				 gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
				 gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
			}
			else if(elapsed_reached(s_state_started_ms,LLC_START_TIMEOUT_MS))
			{
				protect_set_fault(FAULT_BUS_UVP);
			}
			break;
		}
		default:
			outputs_off();
		  enter_state(MAIN_STEP_STANDBY);
			break;
	}
	
}
