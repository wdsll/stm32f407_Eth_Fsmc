#include "charge_ctrl.h"
#include "llc_control.h"
#include "add_dma.h"

#ifndef CHARGE_OUT_RELAY_ACTIVE_LEVEL
#define CHARGE_OUT_RELAY_ACTIVE_LEVEL   1   /* 1: GPIO置位吸合; 0: GPIO复位吸合 */
#endif

typedef struct
{
    charge_state_t state;
    charge_stop_reason_t stop_reason;
    uint32_t entry_ms;
    uint32_t detect_ms;
    uint32_t term_ms;
    float vbat_v;
    float vout_v;
    float iout_a;
} charge_rt_t;

static charge_cfg_t s_chg_cfg =
{
    .cv_target_v         = 54.75f,
    .cc_target_a         = 40.0f,
    .precharge_current_a = 2.0f,
    .precharge_margin_v  = 1.5f,
    .v_match_window_v    = 1.0f,
    .vbat_present_min_v  = 20.0f,
    .vbat_absent_max_v   = 5.0f,
    .cv_enter_margin_v   = 0.5f,
    .term_current_a      = 2.0f,
    .detect_debounce_ms  = 200U,
    .precharge_hold_ms   = 200U,
    .relay_settle_ms     = 100U,
    .term_hold_ms        = 3000U,
};

static charge_rt_t s_chg_rt;

static inline float charge_conv_adc_to_v(uint16_t raw, float rtop, float rbot)
{
    float v = ((float)raw * VREF_ADC) / 4095.0f;
    return v * ((rtop + rbot) / rbot);
}

static void charge_out_relay_set(bool on)
{
    static bool init = false;

    if (!init) {
        rcu_periph_clock_enable(OUT_RELAY_RCU);
        gpio_init(OUT_RELAY, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, OUT_RELAY_PIN);

#if CHARGE_OUT_RELAY_ACTIVE_LEVEL
        gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
#else
        gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
#endif
        init = true;
    }

#if CHARGE_OUT_RELAY_ACTIVE_LEVEL
    if (on) 
			gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
    else    
			gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
#else
    if (on) gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
    else    gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
#endif
}
/*********************************************************************************************************
* 函数名称：relay_self_test
* 函数功能：输出继电器硬件自检，通过临时切换GPIO为输入模式读取IDR验证引脚状态是否与写入一致
* 输入参数：void
* 输出参数：void
* 返 回 值：bool - true自检通过，false自检失败（继电器或驱动电路异常）
* 创建日期：2026年04月11日
* 注    意：1.依赖OUT_RELAY_PIN已初始化(RCU+GPIO_MODE_OUT_PP)；2.测试过程会短暂切换GPIO模式约10us；
*         3.无硬件反馈触点时使用此软件回读方案；4.测试完成后恢复GPIO为PP输出状态并断开继电器
*********************************************************************************************************/
static bool relay_self_test(void)
{
    /* 确保GPIO已初始化（charge_out_relay_set有static init保护，多次调用安全） */
    charge_out_relay_set(false);  /* 确保init完成后GPIO为PP模式+断开状态 */

    uint32_t passed = 0U;

    /* 测试序列：高电平写入→读回；低电平写入→读回，各一轮 */
    for (uint8_t i = 0U; i < 2U; i++) {
        /* i=0: target=1（写高读高）; i=1: target=0（写低读低）*/
        bool target = (i == 0U);

#if CHARGE_OUT_RELAY_ACTIVE_LEVEL
        /* ACTIVE_LEVEL=1: GPIO高=吸合, GPIO低=断开 */
        /* 按 target 决定写入电平，而非两轮都固定写同一值 */
        if (target) {
            gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
        } else {
            gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
        }
        gpio_init(OUT_RELAY, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, OUT_RELAY_PIN);
        /* 切换为输入后等待一个GPIO响应周期，约1~2个APB时钟，这里延时10us确保稳定 */
        for (volatile uint32_t dly = 0U; dly < 240U; dly++) { (void)0; }
        bool level_detected = gpio_input_bit_get(OUT_RELAY, OUT_RELAY_PIN) != RESET;
        /* 期望：target=1时读回1，target=0时读回0 */
        if (level_detected == target) {
            passed++;
        }
        gpio_init(OUT_RELAY, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, OUT_RELAY_PIN);
        gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN); /* 每轮结束后恢复断开状态 */
#else
        /* ACTIVE_LEVEL=0: GPIO低=吸合, GPIO高=断开
         * 此时物理电平与 target 含义相反：target=1（吸合）对应写低
         * 按 target 取反决定写入电平 */
        if (!target) {
            gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
        } else {
            gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
        }
        gpio_init(OUT_RELAY, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, OUT_RELAY_PIN);
        for (volatile uint32_t dly = 0U; dly < 240U; dly++) { (void)0; }
        /* 读回实际电平，再取反还原为逻辑 target（因为 ACTIVE_LEVEL=0 时电平含义相反）*/
        bool level_detected = gpio_input_bit_get(OUT_RELAY, OUT_RELAY_PIN) == RESET;
        if (level_detected == target) {
            passed++;
        }
        gpio_init(OUT_RELAY, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, OUT_RELAY_PIN);
        gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN); /* 每轮结束后恢复断开状态（ACTIVE=0时高电平=断开）*/
#endif
    }

    /* 两轮写入-回读验证均通过才算自检OK */
    return (passed == 2U);
}

static void charge_enter(charge_state_t st)
{
    s_chg_rt.state = st;
    s_chg_rt.entry_ms = g_ms;
    s_chg_rt.term_ms = 0U;
}
/*********************************************************************************************************
* 函数名称：charge_update_meas
* 函数功能：更新充电运行时的实时测量数据，包括电池电压、输出电压和输出电流
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月11日
* 注    意：该函数为静态内部函数，仅在本文件内可见；从LLC状态结构体获取输出电压和电流，从ADC原始值计算电池电压
*********************************************************************************************************/
static void charge_update_meas(void)
{
    llc_status_t llc_st = {0};

    llc_get_status(&llc_st);

    s_chg_rt.vbat_v = charge_conv_adc_to_v(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
    s_chg_rt.vout_v = llc_st.vout_v;
    s_chg_rt.iout_a = llc_st.iout_a;
}


static bool charge_llc_ready_for_closed_relay(void)
{
    return (llc_app_state() == ST_LLC_RUN);
}
/*********************************************************************************************************
* 函数名称：charge_apply
* 函数功能：应用充电控制参数，设置LLC控制器的运行状态、电压/电流参考值，并控制输出继电器状态
* 输入参数：llc_run - LLC运行请求标志（true-启动，false-停止）；relay_on - 输出继电器状态（true-闭合，false-断开）；vref - 电压参考值（V）；iref - 电流参考值（A）
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月11日
* 注    意：该函数为静态内部函数，仅在本文件内可见
*********************************************************************************************************/
static void charge_apply(bool llc_run, bool relay_on, float vref, float iref)
{
    llc_set_vref(vref);
    llc_set_iref(iref);
    llc_set_run_request(llc_run);
    charge_out_relay_set(relay_on);
}
/*********************************************************************************************************
* 函数名称: charge_ctrl_init
* 函数功能: 充电控制模块初始化，复位状态机计时器和运行时变量，配置初始输出参数
* 输入参数: 无
* 输出参数: 无
* 返 回 值: 无
* 备    注: 初始化充电状态为空闲状态，关闭输出继电器和LLC运行请求，设置初始电压参考值和预充电流参考值
*********************************************************************************************************/
void charge_ctrl_init(void)
{
    s_chg_rt.state = CHG_ST_IDLE;
    s_chg_rt.stop_reason = CHG_STOP_NONE;
    s_chg_rt.entry_ms = 0U;
    s_chg_rt.detect_ms = 0U;
    s_chg_rt.term_ms = 0U;
    s_chg_rt.vbat_v = 0.0f;
    s_chg_rt.vout_v = 0.0f;
    s_chg_rt.iout_a = 0.0f;

    charge_out_relay_set(false);
    llc_set_run_request(false);
    llc_set_vref(s_chg_cfg.cv_target_v);
    llc_set_iref(s_chg_cfg.precharge_current_a);

    /* 上电继电器自检：LLC未启动，GPIO回读安全，失败直接锁入FAULT */
    if (!relay_self_test()) {
        s_chg_rt.stop_reason = CHG_STOP_RELAY_FAIL;
        s_chg_rt.state = CHG_ST_FAULT;
    } else {
        s_chg_rt.stop_reason = CHG_STOP_NONE;
    }
}

void charge_ctrl_set_cfg(const charge_cfg_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    s_chg_cfg = *cfg;
}

void charge_ctrl_set_target(float cv_v, float cc_a)
{
    s_chg_cfg.cv_target_v = cv_v;
    s_chg_cfg.cc_target_a = cc_a;
}

charge_state_t charge_ctrl_state(void)
{
    return s_chg_rt.state;
}

charge_stop_reason_t charge_ctrl_stop_reason(void)
{
    return s_chg_rt.stop_reason;
}

void charge_ctrl_get_status(charge_status_t *st)
{
    if (st == NULL) {
        return;
    }

    st->state       = s_chg_rt.state;
    st->stop_reason = s_chg_rt.stop_reason;
    st->vbat_v = s_chg_rt.vbat_v;
    st->vout_v = s_chg_rt.vout_v;
    st->iout_a = s_chg_rt.iout_a;
}
/*********************************************************************************************************
* 函数名称：charge_ctrl_tick_1khz
* 函数功能：充电控制器1kHz周期任务，实现锂电池充电状态机（预充→恒流→恒压→完成）
* 输入参数：void（使用全局状态s_chg_rt和配置s_chg_cfg）
* 输出参数：void（通过charge_apply输出到硬件LLC和继电器）
* 返 回 值：void
* 创建日期：2026年04月08日
* 注    意：每1ms调用一次，包含故障保护、电压匹配消抖、充电曲线控制
*********************************************************************************************************/

void charge_ctrl_tick_1khz(void)
{
	charge_update_meas();

    /* LLC自己已经故障，直接转FAULT */
    if (llc_is_fault_state()) {
        s_chg_rt.stop_reason = CHG_STOP_LLC_FAULT;
        charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);
        charge_enter(CHG_ST_FAULT);
        return;
    }

	switch (s_chg_rt.state)
	{
		default:
			
		case CHG_ST_IDLE:
		{
				// 默认状态：LLC关闭，继电器断开，处于安全待机模式
				charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);

				if (s_chg_rt.vbat_v >= s_chg_cfg.vbat_present_min_v) { // >=预充最小电压 
						if (s_chg_rt.detect_ms == 0U) {
								s_chg_rt.detect_ms = g_ms;  // 首次检测到，记录时间点
						} else if (elapsed_reached(s_chg_rt.detect_ms, s_chg_cfg.detect_debounce_ms)) {
								// 持续满足条件超过消抖时间，确认电池接入
								charge_enter(CHG_ST_PRECHARGE);
								// 进入新状态前清零计时器
								s_chg_rt.detect_ms = 0U;
						}
				} else {
						s_chg_rt.detect_ms = 0U; // 条件不满足立即重置，重新消抖
				}
				break;
		}
		//实现防打火预充机制：先开启LLC将输出电容电压提升至接近电池电压，再闭合继电器，避免大电流冲击损坏触点。
		case CHG_ST_PRECHARGE:
		{
				float vref = s_chg_rt.vbat_v + s_chg_cfg.precharge_margin_v; //最多54.75
				if (vref > s_chg_cfg.cv_target_v) {
						vref = s_chg_cfg.cv_target_v;
				}
				// 预充阶段：LLC开启，继电器断开
				charge_apply(true, false, vref, s_chg_cfg.precharge_current_a);
				// 关键检查：必须等待LLC进入RUN状态
				/* wait until LLC enters RUN before applying precharge completion checks */
				if (llc_app_state() != ST_LLC_RUN) {
						s_chg_rt.term_ms = 0U;
						break;
				}

				if (s_chg_rt.vbat_v < s_chg_cfg.vbat_absent_max_v) { //检查电池是否断开 电池电压小于5V继续进IDLE
						charge_enter(CHG_ST_IDLE);
						break;
				}

				if (fabsf(s_chg_rt.vout_v - s_chg_rt.vbat_v) <= s_chg_cfg.v_match_window_v) {
						if (s_chg_rt.term_ms == 0U) {
								s_chg_rt.term_ms = g_ms;
						} else if (elapsed_reached(s_chg_rt.term_ms, s_chg_cfg.precharge_hold_ms)) {  //压差大于1V才会进继电器控制模块
								charge_enter(CHG_ST_RELAY_ON);
								s_chg_rt.term_ms = 0U;
						}
				} else {
						s_chg_rt.term_ms = 0U;
				}
				break;
		}

		case CHG_ST_RELAY_ON:
		{
				float vref = s_chg_rt.vbat_v + s_chg_cfg.precharge_margin_v;
				if (vref > s_chg_cfg.cv_target_v) {
						vref = s_chg_cfg.cv_target_v;
				}

				charge_apply(true, true, vref, s_chg_cfg.precharge_current_a);//2A的预充电流
	
				/* 继电器闭合阶段若LLC已掉出RUN，立即走停机流程，避免“继电器闭合但LLC未稳态” */
			if (!charge_llc_ready_for_closed_relay()) {
					s_chg_rt.stop_reason = CHG_STOP_LLC_DROPOUT;
					charge_enter(CHG_ST_STOPPING);
					break;
			}
			//机械稳定延时
				if (elapsed_reached(s_chg_rt.entry_ms, s_chg_cfg.relay_settle_ms)) {
						charge_enter(CHG_ST_CC); //延时结束进入恒流充电
				}
				break;
		}

		case CHG_ST_CC:
		{
				charge_apply(true, true, s_chg_cfg.cv_target_v, s_chg_cfg.cc_target_a);

			if (!charge_llc_ready_for_closed_relay()) {
					s_chg_rt.stop_reason = CHG_STOP_LLC_DROPOUT;
					charge_enter(CHG_ST_STOPPING);
					break;
			}

			if (s_chg_rt.vbat_v >= (s_chg_cfg.cv_target_v - s_chg_cfg.cv_enter_margin_v)) { //电池电压大于 54.75 - 0.5
						charge_enter(CHG_ST_CV);
				}
				break;
		}

		case CHG_ST_CV:
		{
				charge_apply(true, true, s_chg_cfg.cv_target_v, s_chg_cfg.cc_target_a);
			
			if (!charge_llc_ready_for_closed_relay()) {
					s_chg_rt.stop_reason = CHG_STOP_LLC_DROPOUT;
					charge_enter(CHG_ST_STOPPING);
					break;
			}

			if (s_chg_rt.iout_a <= s_chg_cfg.term_current_a) {
					if (s_chg_rt.term_ms == 0U) {
							s_chg_rt.term_ms = g_ms;
					} else if (elapsed_reached(s_chg_rt.term_ms, s_chg_cfg.term_hold_ms)) {  //维持3000ms
							s_chg_rt.stop_reason = CHG_STOP_DONE;   /* 正常充满 */
							charge_enter(CHG_ST_STOPPING);   /* 先软停LLC，再断继电器 */
							s_chg_rt.term_ms = 0U;
						}
				} else {
						s_chg_rt.term_ms = 0U;
				}
				break;
		}

		case CHG_ST_STOPPING:
		{
				/* 第一步：停止LLC请求，继电器保持闭合（防止触点断弧） */
				llc_set_run_request(false);
				charge_out_relay_set(true);

				/* 第二步：等待LLC状态机回到IDLE（软关断走完） */
				if (llc_app_state() == ST_IDLE) {
						/* LLC已完全停止，安全断开继电器 */
						charge_out_relay_set(false);
						charge_enter(CHG_ST_DONE);
				}
				break;
		}

		case CHG_ST_DONE:
		{
				charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);

				if (s_chg_rt.vbat_v < s_chg_cfg.vbat_absent_max_v) {
						charge_enter(CHG_ST_IDLE);
				}
				break;
		}

		case CHG_ST_FAULT:
		{
				charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);
				break;
		}
	}
}