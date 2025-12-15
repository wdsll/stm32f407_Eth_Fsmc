#include "llc_control.h"

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/

static llc_app_ctx_t s_llc_app;

enum{
	LLC_START_DELAY_MS = 10000
};

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void llc_state_enter(llc_state_t next);


static void llc_state_enter(llc_state_t next)
{
	// 更新 LLC 状态和进入时间
	s_llc_app.state = next;
	s_llc_app.entry_ms = g_ms;
	#if Bus_Adj
	bus_vol_adj_reset();   //重置总线电压调整逻辑 百分之五十的占空比
	#endif
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
		case ST_IDLE:
#if LLC_USE_OPEN_LOOP
		llc_open_loop_stop(&s_llc_open_loop); // 停止开环控制（如果启用）
		s_llc_open_loop_completed = false;
#endif
			llc_softstart_on_fault();
			pfc_app_force_off();   // 强制关闭 PFC
			llc_pwm_outputs_enable(0); // 禁用 PWM 输出
			pfc_hw_set_relay(false); // 关闭继电器
			s_llc.integ = 0.0f; // 重置积分项
			s_llc.f_cmd = s_llc.f_max; // 设置频率为最大值
			break;
	  case ST_WAIT_AUX:  /* 新增：只在辅源稳定后才进入 WAIT_VBUS */
#if LLC_USE_OPEN_LOOP
        llc_open_loop_stop(&s_llc_open_loop);
        s_llc_open_loop_completed = false;
#endif
        llc_softstart_on_fault();
        pfc_app_force_off();   // 强制关闭 PFC
				pfc_hw_set_relay(false); // 关闭继电器
        llc_pwm_outputs_enable(0); // 禁用 PWM 输出
        
        s_llc.integ = 0.0f;  // 重置积分项
        s_llc.f_cmd = s_llc.f_max; // 设置频率为最大值
        break;
		case ST_WAIT_VBUS:
#if LLC_USE_OPEN_LOOP
			llc_open_loop_stop(&s_llc_open_loop);
			s_llc_open_loop_completed = false;
#endif
			llc_softstart_on_fault();
			pfc_app_request_start(); //记录请求的起始时间
			pfc_hw_set_relay(false);
			llc_pwm_outputs_enable(0);
			s_llc.integ = 0.0f;
			s_llc.f_cmd = s_llc.f_max;
			break;
		case ST_LLC_RUN:
			s_llc.integ = 0.0f;
#if LLC_USE_OPEN_LOOP
		if(!s_llc_open_loop_completed)
		{
			llc_open_loop_start(&s_llc_open_loop);
			s_llc.f_cmd = f_clampf(llc_open_loop_get_freq(&s_llc_open_loop), s_llc.f_min, s_llc.f_max);
		}
		else
		{
			s_llc.f_cmd = f_clampf(s_llc_open_loop_final_freq, s_llc.f_min, s_llc.f_max);
		}
		
#else
			s_llc.f_cmd = f_clampf(LLC_F_INIT_HZ, s_llc.f_min, s_llc.f_max);
#endif
			llc_softstart_start(LLC_SOFTSTART_TARGET_DUTY);
			llc_pwm_outputs_enable(1);
			// 添加调试信息输出
			//debug_printf("[LLC] Starting open loop control. Initial frequency: %.1f Hz\n", s_llc.f_cmd);
			break;
		case ST_FAULT:
				
		default:	
#if LLC_USE_OPEN_LOOP
			llc_open_loop_stop(&s_llc_open_loop);
			s_llc_open_loop_completed = false;
#endif
			llc_softstart_on_fault();
			pfc_app_force_off();
			pfc_hw_set_relay(false);
			llc_pwm_outputs_enable(0);
			s_llc.f_cmd = s_llc.f_max;
			break;
	}
}

void llc_app_init()
{
	llc_softstart_init();
	llc_state_enter(ST_WAIT_VBUS);
}

void llc_app_tick_1khz(void)
{
	 static uint8_t s_llc_clear_sent = 0;
	
	if(protect_fault_latched() || protect_fault_active_hw()||pfc_app_state() == PFC_ST_FAULT)
	{
		llc_state_enter(ST_FAULT);
		llc_softstart_on_fault();
		return;
	}
	
	if (aux_power_brownout_stable(AUX_DROP_DEBOUNCE_MS)){
			llc_state_enter(ST_WAIT_AUX);
			return;
	}
	
	switch(s_llc_app.state)
	{
		case ST_IDLE:
			llc_state_enter(ST_WAIT_VBUS);
			break;
		case ST_WAIT_AUX:
        if (aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS)){
            llc_state_enter(ST_WAIT_VBUS);
        }
        break;
		case ST_WAIT_VBUS:
#if LLC_BYPASS_PFC_CONTROL
			if(s_llc.vmeas >= LLC_ENTRY_V) 
#else
			if(pfc_app_ready() && (s_llc.vmeas >= LLC_ENTRY_V)) 
#endif
			{
				if (s_llc_app.entry_ms == 0U) 
				{
					s_llc_app.entry_ms = g_ms; // 可用于入门延时(若需要)
				}
				else if((uint32_t)(g_ms - s_llc_app.entry_ms) >= LLC_START_DELAY_MS)
				{
					pfc_hw_set_relay(true);
					llc_state_enter(ST_LLC_RUN);
				}
			}
			else{
				 s_llc_app.entry_ms = 0U;
				 pfc_hw_set_relay(false);
			}
			break;
		case ST_LLC_RUN:
				llc_softstart_tick_1khz();
				if(!aux_power_ok_now()){
					pfc_hw_set_relay(0);
					llc_state_enter(ST_WAIT_AUX);
					break;
        }
#if LLC_BYPASS_PFC_CONTROL
			if(s_llc.vmeas<(LLC_ENTRY_V-PFC_VBUS_READY_HYST_V))
#else
			if(!pfc_app_ready()||s_llc.vmeas<(LLC_ENTRY_V-PFC_VBUS_READY_HYST_V))
#endif
			{
				pfc_hw_set_relay(0);
				llc_state_enter(ST_WAIT_VBUS);
			}
			break;
		case ST_FAULT:
			
		default:
			{
				bool hw_active = protect_fault_active_hw();
				bool sw_latched = protect_fault_latched();
				bool aux_ok = aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS);

				/* 满足恢复条件再清一次硬件锁存 */
				if (!hw_active && aux_ok && elapsed_reached(s_llc_app.entry_ms, PFC_RESTART_DELAY_MS)) {
						if (sw_latched) 
							protect_clear_fault();
						if (!s_llc_clear_sent) {
								//protect_hw_clear_pulse(10);
								s_llc_clear_sent = 1;
						}
						llc_state_enter(ST_WAIT_AUX);  // 先回到等辅源，再走 WAIT_VBUS → RUN
						s_llc_clear_sent = 0;
				}
			} break;
    }	
}

void llc_app_tick_1khz_withoutVbus(void)
{
	/* 只处理 LLC：不看母线电压、不看AUX，仅做故障保护 + 固定延时启动 + 开环/软启动推进 */
    static uint8_t s_llc_clear_sent = 0;
    static bool    ol_started = false;  /* 防止在 RUN 状态下重复 open-loop start */
	    if (protect_fault_latched() || protect_fault_active_hw()
#if !LLC_BYPASS_PFC_CONTROL
        || pfc_app_state() == PFC_ST_FAULT
#endif
        )
    {
        if (s_llc_app.state != ST_FAULT) {
            llc_state_enter(ST_FAULT);
            ol_started = false;
        }
        return;
    }
		
		//adc_multi_copy(); 
		//adc_multi_sample_aux_1khz();
	
    //float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
		
		//float v3v3 = (g_adc_multi.v3v3_raw * VREF_ADC) / 4095.0f;
		//float v3v3 = conv_adc_to_v_div(g_adc_multi.v3v3_raw,V3V3_RTOP_OHM,V3V3_RBOT_OHM);
		//float vbat = conv_adc_to_v_div(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
		//aux_power_monitor_update(v3v3, vbat);
		//s_llc.vmeas = vout;
		switch (s_llc_app.state)
		{
			  /* 这些状态统一当作“等待固定启动延时”，不做任何母线/AUX判断 */
        case ST_IDLE:
        case ST_WAIT_AUX:
        case ST_WAIT_VBUS:
				{
            if (s_llc_app.entry_ms == 0U) {
                s_llc_app.entry_ms = g_ms;
            }
            if (elapsed_reached(s_llc_app.entry_ms, LLC_START_DELAY_MS)) {
                //pfc_hw_set_relay(true);              /* 若未接硬件或宏未定义，此函数内部已做空操作保护 */
                llc_state_enter(ST_LLC_RUN);         /* 进入 RUN：在 llc_state_enter 中会做一次性初始化 */
                ol_started = false;
            }
        } break;

        case ST_LLC_RUN:
					llc_softstart_tick_1khz();
					llc_pwm_set_freq((uint32_t)s_llc.f_cmd);
						break;
						case ST_FAULT:
        default:
        {
            /* 自动重试：故障脚释放 + 到达重试延时 → 清软件/硬件锁存，回到 IDLE 等延时再起 */
            bool hw_active = protect_fault_active_hw();
            if (!hw_active && elapsed_reached(s_llc_app.entry_ms, PFC_RESTART_DELAY_MS)) {
                if (protect_fault_latched()) {
                    protect_clear_fault();
                }
                if (!s_llc_clear_sent) {
                    //protect_hw_clear_pulse(10); /* 10 ms；按你锁存清除时序需要可调 */
                    s_llc_clear_sent = 1;
                }
                pfc_hw_set_relay(false);
                llc_state_enter(ST_IDLE);
                s_llc_clear_sent = 0;
                ol_started = false;
            }
        } 
				break;
		}
}

llc_state_t llc_app_state(void)
{
	return s_llc_app.state;
}





