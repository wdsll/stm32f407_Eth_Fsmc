/*********************************************************************************************************
* 模块名称：llc_control.c
* 摘    要：
* 作    者：Rengar
* 内    容：llc_control.c - LLC 模拟控制状态机 (CV_PWM + CC_PWM 双基准)
						* MCU 仅设定 CV_PWM(电压基准) + CC_PWM(电流基准), 模拟IC 内部闭环
						* CC 模式: CC_PWM=目标电流, CV_PWM=电压上限
						* CV 模式: CV_PWM=目标电压, CC_PWM=电流上限 
						//  A2：此注释与实现矛盾——代码里 CV 状态并没切换基准，见 A2

* 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
*          
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "llc_control.h"
#include "adc_dma.h"
#include "condition_held.h"
//#include "can_comm.h"
#include "pfc_control.h"
#include "protect.h"
#include "pwm_llc.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define BATTERY_PRESENT_V       (10.0f)   // 判定"电池已接入"的最低电压
#define OUTPUT_RELAY_MIN_V      (10.0f)   // CC 态判定输出已建起的电压门限也用于输出启动超时
#define OUTPUT_START_TIMEOUT_MS     (1500U)   //C2 已改名：CC 后 1.5s 输出未建起→FAULT_OUTPUT_START_TIMEOUT

//#define PFC_READY_TIMEOUT_MS    (3000U)
//#define CAN_STATUS_PERIOD_MS    (100U)    //CAN 状态帧上报周期 100ms（10Hz）

#define CV_REFERENCE_MAX_V      (80.0f)   // CV_PWM 占空比换算满量程电压（344/80V）

#define CC_REFERENCE_MAX_A      (20.0f)  // CC_PWM 占空比换算满量程电流（全局 C3：满量程≈24.8A，OCP=30A 超量程）

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static bool s_enable_requested;   // 充电使能请求（来自 CAN）

static float s_voltage_reference_v;  // 当前的电压基准（V）,来自外部 set_references

static float s_current_reference_a; // 当前电流基准(A)，来自外部 set_references

static uint32_t s_state_started_ms; // 进入当前状态的时间戳（状态内超时用）

static condition_qualification_t s_qualification; //去抖状态（CC→CV / 完成判据共用，按状态 reset）

/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：voltage_to_duty
* 函数功能：电压→占空比线性映射
* 输入参数：voltage_v
* 输出参数：float
* 返 回 值：float
* 创建日期：2026年08月12日
* 注    意：嵌在PWM的合法区间
*********************************************************************************************************/
static float voltage_to_duty(float voltage_v)
{
    return f_clampf(voltage_v / CV_REFERENCE_MAX_V,
                    CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX);
}

/*********************************************************************************************************
* 函数名称：current_to_duty
* 函数功能：电流→占空比线性映射
* 输入参数：current_a
* 输出参数：float
* 返 回 值：float
* 创建日期：2026年08月12日
* 注    意：嵌在PWM的合法区间
*********************************************************************************************************/
static float current_to_duty(float current_a)
{
    return f_clampf(current_a / CC_REFERENCE_MAX_A,
                    CC_PWM_DUTY_MIN, CC_PWM_DUTY_MAX);
}
/*********************************************************************************************************
* 函数名称：apply_references
* 函数功能：开环设定点，反馈在模拟 IC 内完成；MCU 不读 ADC 修正
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：
*********************************************************************************************************/
static void apply_references(void)
{
    /* Open-loop set-point conversion only; ADC feedback is not used here. */
    cv_pwm_set_duty(voltage_to_duty(s_voltage_reference_v));
    cc_pwm_set_duty(current_to_duty(s_current_reference_a)); 
	  //注释诚实：开环设定点，反馈在模拟 IC 内；MCU 不读 ADC 修正
}
/*********************************************************************************************************
* 函数名称：outputs_force_off
* 函数功能：断输出继电器，关LLC使能，断LLC
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：B1：三路同 tick 同步断开，无"先关 LLC→放电→再断继电器"时序
						C5：未复位 CV_PWM/CC_PWM 占空比，重新充电可能带旧值
*********************************************************************************************************/
static void outputs_force_off(void)
{
	gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN); // 关 LLC 使能
	cv_pwm_set_duty(PWM_DUTY_SAFE);  // C5 已修：PWM 复位到安全占空比(=0)
  cc_pwm_set_duty(PWM_DUTY_SAFE);
	gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN); // 断输出继电器
	pfc_disable(); // 断 PFC
}
 //CC/CV/FINISHED 共用：闭合继电器+绿亮红灭
static void outputs_on(void) 
{
	  gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
    gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
    gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
}

/*********************************************************************************************************
* 函数名称：enter_state
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：
*********************************************************************************************************/
static void enter_state(charger_state_t state)
{
	  if (g_charger_state == state) {  // 同态幂等守卫：重复进入不重复执行一次性动作
        return;
    }

	  g_charger_state = state;  // 切换全局状态
    s_state_started_ms = g_ms; // 重置状态计时
		condition_qualification_reset(&s_qualification); // 重置去抖，防跨状态残留
		
		switch(state) // 一次性动作集中在此（enter 范式）
		{
			case MAIN_STEP_STANDBY: //红灯亮，绿灯灭
			{
				outputs_force_off();
				gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
				gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
				break;
			}
			case MAIN_STEP_PRECHARGE: //C3 已修：PFC 使能挪到此处一次性
			{
				pfc_enable();
				break;
			}
			case MAIN_STEP_CC:
			{
				apply_references(); //仅此处下发一次基准
				gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN); //开LLC
				outputs_on();// N1：此处立即闭合继电器（行为变化，见 N1）
				break;
			}
			case MAIN_STEP_CV:
			{
				outputs_on(); // // CV 已闭合，保持即可
				break;
			}
			case MAIN_STEP_FINISHED:
			{
				outputs_force_off();
				gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
        gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
				break;
			}
			case MAIN_STEP_FAULT:
			{
				outputs_force_off();  // fail-safe 收敛点
				gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
				gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
				break;	
			}
			default:
				break;
		}
}

void power_supervisor_enter_fault(void) // protect 调此进 FAULT
{
    enter_state(MAIN_STEP_FAULT);
}

/* ========== 状态机 ========== */
/*********************************************************************************************************
* 函数名称：power_supervisor_init
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：
*********************************************************************************************************/
void power_supervisor_init(void)
{
    s_enable_requested = false;
	  s_voltage_reference_v = 0.0f;
    s_current_reference_a = 0.0f;
    //s_last_status_ms = g_ms;
	  cv_pwm_set_duty(CV_PWM_DUTY_INIT);  //初始安全占空比
    cc_pwm_set_duty(CC_PWM_DUTY_INIT);
		//outputs_force_off();
    enter_state(MAIN_STEP_STANDBY); //初始化进待机
}
/*********************************************************************************************************
* 函数名称：power_supervisor_request
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：
*********************************************************************************************************/
void power_supervisor_request(bool enable)
{
		s_enable_requested = enable;  // // 外部置使能
}
/*********************************************************************************************************
* 函数名称：power_supervisor_request
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：
*********************************************************************************************************/
bool power_supervisor_requested(void)
{
	return s_enable_requested; //仅仅给其他模块查询
}
/*********************************************************************************************************
* 函数名称：power_supervisor_set_references
* 函数功能：限幅更新PWM
* 输入参数：voltage_v  current_a
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：
*********************************************************************************************************/
void power_supervisor_set_references(float voltage_v, float current_a)
{
    s_voltage_reference_v = f_clampf(voltage_v, 0.0f, CV_REFERENCE_MAX_V);
    s_current_reference_a = f_clampf(current_a, 0.0f, CC_REFERENCE_MAX_A);
    apply_references();  // 立即下发新基准
}

float power_supervisor_voltage_reference(void) { return s_voltage_reference_v; }
float power_supervisor_current_reference(void) { return s_current_reference_a; }

/*********************************************************************************************************
* 函数名称：power_supervisor_tick_1khz
* 函数功能：基于systick 1ms更新
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月14日
* 注    意：
*********************************************************************************************************/
void power_supervisor_tick_1khz(void)
{
	/* Commands arrive through the transport-independent supervisor API. */
	if (g_charger_state == MAIN_STEP_FAULT) {
		outputs_force_off();

		if(!s_enable_requested&&!protect_fault_latched()&&!protect_fault_active_hw())
		{
			g_fault = FAULT_NONE;
			enter_state(MAIN_STEP_STANDBY);
		}
		return;
	}
	switch(g_charger_state)
		{ 
			case MAIN_STEP_STANDBY:
			{
				if(s_enable_requested && g_adc_multi.vbat_v >= BATTERY_PRESENT_V) //使能且电池已经接入(>=10V)
				{
					enter_state(MAIN_STEP_PRECHARGE); // → PRECHARGE（pfc_enable 在 enter 内）
				}
				break;
			}
			case MAIN_STEP_PRECHARGE:
			{
				if (!s_enable_requested) {  //使能撤销->待机
					 enter_state(MAIN_STEP_STANDBY);
				 } else if (pfc_is_ready()) { //母线挂起（360V）-->CC
					 enter_state(MAIN_STEP_CC); /* Legacy CAN state value: running. */
				 }
					else if (elapsed_reached(s_state_started_ms, PFC_READY_TIMEOUT_MS)) { //A1 已修：3s 超时
						protect_set_fault(FAULT_PRECHARGE_TIMEOUT);  // 母线建不起→故障，不再永久卡死
				 }
				 break;
			}
			case MAIN_STEP_CC:
			{
				if(!s_enable_requested) // 使能撤销→待机
				{
					 enter_state(MAIN_STEP_STANDBY);
				}
				else if(elapsed_reached(s_state_started_ms,CHARGE_CC_TIMEOUT_MS))
				{
					protect_set_fault(FAULT_CHARGE_TIMEOUT);   //B3：CC 超时=8s(调试值)，正常应 8h
				}
				//C4：判据用 vbat 非 vout
				else if(condition_held(&s_qualification,s_voltage_reference_v > BATTERY_PRESENT_V && g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_CV_ENTRY_MARGIN_V),CHARGE_CV_ENTRY_DEBOUNCE_MS))
				{
					enter_state(MAIN_STEP_CV); // 电压到达→进 CV
				}
				else if (g_adc_multi.vout_v <= OUTPUT_RELAY_MIN_V && elapsed_reached(s_state_started_ms, OUTPUT_START_TIMEOUT_MS))
				{
					protect_set_fault(FAULT_OUTPUT_START_TIMEOUT); //C2：名实不符(报母线欠压实则查输出)，现在更新故障类型
				}
				break;
			}
			case MAIN_STEP_CV:
			{
				if(!s_enable_requested) // 使能撤销→待机
				{
					enter_state(MAIN_STEP_STANDBY);
				}
				else if(elapsed_reached(s_state_started_ms,CHARGE_CV_TIMEOUT_MS))
				{
					protect_set_fault(FAULT_CHARGE_TIMEOUT); //B3：CV 超时=3s(调试值)，正常应 3h
				}
				else
				{
					if(condition_held(&s_qualification,s_voltage_reference_v>BATTERY_PRESENT_V&&g_adc_multi.vbat_v>=(s_voltage_reference_v - CHARGE_FINISH_VOLTAGE_MARGIN_V)
						&& g_adc_multi.iout_a >= 0 && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A,CHARGE_FINISH_DEBOUNCE_MS))
					{
						enter_state(MAIN_STEP_FINISHED);
					}
				}
				break;
			}
			case MAIN_STEP_FINISHED:
			{
				if(!s_enable_requested)
				{
					enter_state(MAIN_STEP_STANDBY); // 撤销使能→回待机
				}
				break;
			}
			default:
				enter_state(MAIN_STEP_STANDBY);  //C6：TRICKLE/INVALID 全靠此处兜底
				break;
		}
}
