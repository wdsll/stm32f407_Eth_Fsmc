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
//#include "can_comm.h"
#include "pfc_control.h"
#include "protect.h"
#include "pwm_llc.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define BATTERY_PRESENT_V       (10.0f)   // 判定"电池已接入"的最低电压
#define OUTPUT_RELAY_MIN_V      (10.0f)   // CC 态判定输出已建起的电压门限
#define LLC_START_TIMEOUT_MS    (1500U)   // C2：名实不符，实际用于"CC 后 1.5s 输出未建起→报 BUS_UVP"

//#define PFC_READY_TIMEOUT_MS    (3000U)
//#define CAN_STATUS_PERIOD_MS    (100U)    //CAN 状态帧上报周期 100ms（10Hz）

#define CV_REFERENCE_MAX_V      (80.0f)   // CV_PWM 占空比换算满量程电压（344/80V）

#define CC_REFERENCE_MAX_A      (20.0f)  // CC_PWM 占空比换算满量程电流（全局 C3：满量程≈24.8A，OCP=30A 超量程）

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static bool s_enable_requested;   // 充电使能请求（来自 CAN）

static float s_voltage_reference_v;  // 当前的电压基准（V）,来自can的命令

static float s_current_reference_a; // 当前电流基准(A)，来自 CAN 命令

static uint32_t s_state_started_ms; // 进入当前状态的时间戳（状态内超时用）

//static uint32_t s_last_status_ms; // 上次 CAN 状态上报时间戳

static uint32_t s_qualification_started_ms; // 去抖计时起点（condition_held 用）

static bool s_qualification_active; // 去抖是否已激活（首次满足标记）

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
	  
}
/*********************************************************************************************************
* 函数名称：outputs_off
* 函数功能：断输出继电器，关LLC使能，断LLC
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：B1：三路同 tick 同步断开，无"先关 LLC→放电→再断继电器"时序
						C5：未复位 CV_PWM/CC_PWM 占空比，重新充电可能带旧值
*********************************************************************************************************/
static void outputs_off(void)
{
	  gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);
    gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);
    pfc_disable();
}
/*********************************************************************************************************
* 函数名称：condition_held
* 函数功能：祛抖函数
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月12日
* 注    意：
*********************************************************************************************************/
static bool condition_held(bool condition,uint32_t duration_ms)
{
	  if (!condition) {  //条件不满足避免误触发
        s_qualification_active = false;
        return false;
    }
		
		if (!s_qualification_active) {  //首次满足-->记录起点本次返回假
        s_qualification_active = true;
        s_qualification_started_ms = g_ms;
        return false;
    }
		
		return elapsed_reached(s_qualification_started_ms,duration_ms); //必须持续一段时间才为真
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
	  g_charger_state = state;
    s_state_started_ms = g_ms;
	
		s_qualification_active = false;
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
    outputs_off();  //默认所有都安全关闭
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
		s_enable_requested = enable;  // can命令设置使能
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
		outputs_off();
		gpio_bit_set(LED_RED_PORT,LED_RED_PIN);
		gpio_bit_reset(LED_GREEN_PORT,LED_GREEN_PIN);
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
				outputs_off();  //C3：每 tick 先全关
				gpio_bit_set(LED_RED_PORT,LED_RED_PIN);
				gpio_bit_reset(LED_GREEN_PORT,LED_GREEN_PIN);
				if(s_enable_requested && g_adc_multi.vbat_v >= BATTERY_PRESENT_V) //使能且电池已经接入(>=10V)
				{
					pfc_enable(); // C3：同一 tick 又开 PFC,不能每个task都开把
					enter_state(MAIN_STEP_PRECHARGE);
				}
				break;
			}
			case MAIN_STEP_PRECHARGE:
			{
				if (!s_enable_requested) {  //使能撤销->待机
					 outputs_off();
					 enter_state(MAIN_STEP_STANDBY);
				 } else if (pfc_is_ready()) { //母线挂起（360V）-->CC
					 apply_references(); // // 仅此处下发一次基准（见 A2）
					 gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);
					 enter_state(MAIN_STEP_CC); /* Legacy CAN state value: running. */
				 }
					else if (elapsed_reached(s_state_started_ms, PFC_READY_TIMEOUT_MS)) {
						protect_set_fault(FAULT_BUS_UVP);
				 }
					//A1：无第三个分支——pfc 永远不 ready 则永久卡死，不报错不回待机,现在新增第三个分支
				 break;
			}
			case MAIN_STEP_CC:
			{
				if(!s_enable_requested) // 使能撤销→待机
				{
					 outputs_off();
					 enter_state(MAIN_STEP_STANDBY);
				}
				else if(elapsed_reached(s_state_started_ms,CHARGE_CC_TIMEOUT_MS))
				{
					protect_set_fault(FAULT_CHARGE_TIMEOUT);   //B3：CC 超时=8s(调试值)，正常应 8h
				}
				//C4：判据用 vbat 非 vout
				else if(condition_held(s_voltage_reference_v > BATTERY_PRESENT_V && g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_CV_ENTRY_MARGIN_V),CHARGE_CV_ENTRY_DEBOUNCE_MS))
				{
					enter_state(MAIN_STEP_CV); // 电压到达→进 CV
				}
				else if(g_adc_multi.vout_v>OUTPUT_RELAY_MIN_V) //输出已经挂起
				{
					gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN); //闭合输出继电器
					gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
					gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
				}
				else if (elapsed_reached(s_state_started_ms, LLC_START_TIMEOUT_MS))
				{
					protect_set_fault(FAULT_BUS_UVP); //C2：名实不符(报母线欠压实则查输出)
				}
				break;
			}
			case MAIN_STEP_CV:
			{
				if(!s_enable_requested) // 使能撤销→待机
				{
					outputs_off();
					enter_state(MAIN_STEP_STANDBY);
				}
				else if(elapsed_reached(s_state_started_ms,CHARGE_CV_TIMEOUT_MS))
				{
					protect_set_fault(FAULT_CHARGE_TIMEOUT); //B3：CV 超时=3s(调试值)，正常应 3h
				}
				else
				{
				/* The analog IC owns the CV loop; the MCU only qualifies completion. */
					gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
					gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
					gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
					if(condition_held(s_voltage_reference_v>BATTERY_PRESENT_V&&g_adc_multi.vbat_v>=(s_voltage_reference_v - CHARGE_FINISH_VOLTAGE_MARGIN_V)
						&& g_adc_multi.iout_a >= 0 && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A,CHARGE_FINISH_DEBOUNCE_MS))
					{
						outputs_off();
						enter_state(MAIN_STEP_FINISHED);
					}
				}
				break;
			}
			case MAIN_STEP_FINISHED:
			{
				outputs_off();  // 保持断开输出
				gpio_bit_reset(LED_RED_PORT,LED_RED_PIN);
				gpio_bit_set(LED_GREEN_PORT,LED_GREEN_PIN);
				if(!s_enable_requested)
				{
					enter_state(MAIN_STEP_STANDBY); // 撤销使能→回待机
				}
				break;
			}
			default:
				outputs_off();
				enter_state(MAIN_STEP_STANDBY);  //C6：TRICKLE/INVALID 全靠此处兜底
				break;
		}
}

#if 0
void power_supervisor_tick_1khz_can(void)
{
	can_rx_data_t command;
	if (can_comm_get_cmd(&command)) {  //收到can的命令
		power_supervisor_set_references((float)command.vout_set_x10 * 0.1f,
                                        (float)command.iout_set_x10 * 0.1f);
		power_supervisor_request(command.enable != 0U);
	}
	if (elapsed_reached(s_last_status_ms, CAN_STATUS_PERIOD_MS)) {
		s_last_status_ms = g_ms;
		can_comm_tx_status();  //周期上报状态
  }
	if (s_enable_requested && can_comm_timeout()) {
		protect_set_fault(FAULT_CAN_TIMEOUT); //can 超时->锁故障
	}
	if (g_charger_state == MAIN_STEP_FAULT) { //B2：FAULT 在 CAN 处理后才判
		outputs_off();
		gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
		gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
		return;  //B2：return 后不再收 CAN，无法远程清故障
	}
	switch(g_charger_state)
	{ 
		case MAIN_STEP_STANDBY:
		{
			outputs_off();  //C3：每 tick 先全关
			gpio_bit_set(LED_RED_PORT,LED_RED_PIN);
			gpio_bit_reset(LED_GREEN_PORT,LED_GREEN_PIN);
			if(s_enable_requested && g_adc_multi.vbat_v >= BATTERY_PRESENT_V) //使能且电池已经接入(>=10V)
			{
				pfc_enable(); // C3：同一 tick 又开 PFC,不能每个task都开把
				enter_state(MAIN_STEP_PRECHARGE);
			}
			break;
		}
		case MAIN_STEP_PRECHARGE:
		{
			if (!s_enable_requested) {  //使能撤销->待机
         outputs_off();
         enter_state(MAIN_STEP_STANDBY);
       } else if (pfc_is_ready()) { //母线挂起（360V）-->CC
				 apply_references(); // // 仅此处下发一次基准（见 A2）
				 gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);
				 enter_state(MAIN_STEP_CC); /* Legacy CAN state value: running. */
			 }
			  //A1：无第三个分支——pfc 永远不 ready 则永久卡死，不报错不回待机
			 break;
		}
		case MAIN_STEP_CC:
		{
			if(!s_enable_requested) // 使能撤销→待机
			{
				 outputs_off();
				 enter_state(MAIN_STEP_STANDBY);
			}
			else if(elapsed_reached(s_state_started_ms,CHARGE_CC_TIMEOUT_MS))
			{
				protect_set_fault(FAULT_CHARGE_TIMEOUT);   //B3：CC 超时=8s(调试值)，正常应 8h
			}
			//C4：判据用 vbat 非 vout
			else if(condition_held(s_voltage_reference_v > BATTERY_PRESENT_V && g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_CV_ENTRY_MARGIN_V),CHARGE_CV_ENTRY_DEBOUNCE_MS))
			{
				enter_state(MAIN_STEP_CV); // 电压到达→进 CV
			}
			else if(g_adc_multi.vout_v>OUTPUT_RELAY_MIN_V) //输出已经挂起
			{
				gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN); //闭合输出继电器
				gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
				gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
			}
		  else if (elapsed_reached(s_state_started_ms, LLC_START_TIMEOUT_MS))
			{
				protect_set_fault(FAULT_BUS_UVP); //C2：名实不符(报母线欠压实则查输出)
			}
			break;
		}
		case MAIN_STEP_CV:
		{
			if(!s_enable_requested) // 使能撤销→待机
			{
				outputs_off();
				enter_state(MAIN_STEP_STANDBY);
			}
			else if(elapsed_reached(s_state_started_ms,CHARGE_CV_TIMEOUT_MS))
			{
				protect_set_fault(FAULT_CHARGE_TIMEOUT); //B3：CV 超时=3s(调试值)，正常应 3h
			}
			else
			{
			/* The analog IC owns the CV loop; the MCU only qualifies completion. */
				gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
				gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
				gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
				if(condition_held(s_voltage_reference_v>BATTERY_PRESENT_V&&g_adc_multi.vbat_v>=(s_voltage_reference_v - CHARGE_FINISH_VOLTAGE_MARGIN_V)
					&& g_adc_multi.iout_a >= 0 && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A,CHARGE_FINISH_DEBOUNCE_MS))
				{
					outputs_off();
					enter_state(MAIN_STEP_FINISHED);
				}
			}
			break;
		}
		case MAIN_STEP_FINISHED:
		{
			outputs_off();  // 保持断开输出
			gpio_bit_reset(LED_RED_PORT,LED_RED_PIN);
			gpio_bit_set(LED_GREEN_PORT,LED_GREEN_PIN);
			if(!s_enable_requested)
			{
				enter_state(MAIN_STEP_STANDBY); // 撤销使能→回待机
			}
			break;
		}
		default:
			outputs_off();
		  enter_state(MAIN_STEP_STANDBY);  //C6：TRICKLE/INVALID 全靠此处兜底
			break;
	}
	
}
#endif
