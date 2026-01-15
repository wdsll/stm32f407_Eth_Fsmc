#include "llc_control.h"
#include "float.h"
/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/
typedef struct {
    float vbus_v;
    float vout_v;
    float iout_a;
} llc_meas_t;
typedef struct {
    llc_app_ctx_t app;
    llc_meas_t meas;
    uint32_t softstart_begin_ms;
    uint32_t stopping_begin_ms;
    uint32_t hold_last_adjust_ms;
}llc_runtime_ctx_t;

static llc_runtime_ctx_t s_llc_rt;

static llc_app_ctx_t s_llc_app;
static llc_t s_llc;

enum{
	LLC_START_DELAY_MS = 10000
};

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void llc_state_enter(llc_state_t next);
static void llc_update_measurements(void);
static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot);
static inline float conv_adc_to_i(uint16_t raw);
static void llc_set_freq(float hz);
static bool llc_precheck_ok(void);
static bool llc_faults_present(void);
static void llc_enter_fault(void);

/*********************************************************************************************************
*                                              静态工具
*********************************************************************************************************/
static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot)
{
    float v = (raw * VREF_ADC) / 4095.0f;
    return v * (rtop + rbot) / rbot;
}
static inline float conv_adc_to_i(uint16_t raw)
{
    float v = (raw * VREF_ADC) / 4095.0f;
    float v_net = v - 0.17f;
    return v_net / (ISHUNT_OHM * IAMP_GAIN);
}

static void llc_driver_en_set(bool on)
{
	static bool initialized = false;
	static bool last = false;
	if(!initialized)
	{
		rcu_periph_clock_enable(RCU_GPIOC);
		gpio_init(LLC_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LLC_EN_PIN);
		gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);
		initialized = true;
	}
	if (last == on)
	{
		return;
	}
	last = on;
	if(on)
	{
		gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);
	}
	else
	{
		gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);
	}
}
static void llc_set_freq(float hz)
{
    float f = f_clampf(hz, s_llc.f_min, s_llc.f_max);
    s_llc.f_cmd = f;
    llc_pwm_set_freq((uint32_t)f);
}

static void llc_update_measurements(void)
{
    adc_multi_copy();
    s_llc_rt.meas.vout_v = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    s_llc_rt.meas.iout_a = conv_adc_to_i(g_adc_multi.isense_raw);
    s_llc_rt.meas.vbus_v = pfc_bus_voltage();
    s_llc.vmeas = s_llc_rt.meas.vout_v;
}
static bool llc_precheck_ok(void)
{
    return (s_llc_rt.meas.vbus_v >= LLC_VBUS_MIN_START_V);
}
static bool llc_faults_present(void)
{
    if (protect_fault_active_hw() || protect_fault_latched()) {
        return true;
    }

    if (pfc_is_fault()) {
        return true;
    }

    if (s_llc_rt.meas.vout_v > LLC_VOUT_OVP_V) {  //56
        return true;
    }

    if (s_llc_rt.meas.iout_a > LLC_IOUT_OCP_A) { //41
        return true;
    }

    if (s_llc_rt.meas.vbus_v < (LLC_VBUS_MIN_START_V - LLC_VOUT_HYST_V)) {
        return true;
    }

    return false;
}
static void llc_enter_fault(void)
{
    llc_state_enter(ST_FAULT);
}
/*********************************************************************************************************
*                                              状态机核心
*********************************************************************************************************/
static void llc_state_enter(llc_state_t next)
{
	// 更新 LLC 状态和进入时间
	if (s_llc_rt.app.state == next) 
		return;
	s_llc_rt.app.state = next;
  s_llc_rt.app.entry_ms = g_ms;
	//s_llc_app.entry_ms = g_ms;
	#if Bus_Adj
	//bus_vol_adj_reset();   //重置总线电压调整逻辑 百分之五十的占空比
	#endif
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
		case ST_IDLE:
		  llc_softstart_on_fault();
		  //pfc_hw_set_main(false);   // 强制关闭 PFC
			//pfc_disable();
		  llc_pwm_outputs_enable(0); // 禁用 PWM 输出
		  llc_driver_en_set(false); //disable llc
		  llc_set_freq(s_llc.f_max);

 		  s_llc_rt.softstart_begin_ms = 0U;  //软启动开始时间戳，用于控制软启动斜坡
 		  s_llc_rt.stopping_begin_ms = 0U;  //停机过程开始时间戳，用于控制停机时序
 		  s_llc_rt.hold_last_adjust_ms = 0U; //保持最后调整的时间戳，用于频率调整的去抖
		  break;
	 case ST_PRECHECK:  
	  	llc_driver_en_set(true);
    	llc_pwm_outputs_enable(0); // 禁用 PWM 输出
    	llc_set_freq(s_llc.f_max);
      break;
	 case ST_SOFTSTART:
			s_llc_rt.softstart_begin_ms = g_ms;
			llc_driver_en_set(true);
			llc_pwm_outputs_enable(1);
			llc_softstart_start(LLC_SOFTSTART_TARGET_HZ);
			break;

	 case ST_LLC_RUN:
			llc_driver_en_set(true);
			llc_set_freq(LLC_SOFTSTART_TARGET_HZ);
      s_llc_rt.hold_last_adjust_ms = g_ms;
			break;
	 case ST_STOPPING:
      s_llc_rt.stopping_begin_ms = g_ms;
		  llc_driver_en_set(true);
      llc_set_freq(s_llc.f_max);
      break;
	 case ST_FAULT:
			break;
	 
	 default:
		  llc_softstart_on_fault();
      pfc_hw_set_main(false); //PFC_off
      llc_pwm_outputs_enable(0); //llc pwm disable
      llc_driver_en_set(false);
      llc_set_freq(s_llc.f_max);
      break;
	}
}


/*********************************************************************************************************
*                                              公共接口
*********************************************************************************************************/
void llc_app_init()
{
	llc_softstart_init();
	s_llc = (llc_t){
				.vref=LLC_VOUT_TARGET_V, .vmeas=0.0f, .kp=0.01f, .ki=0.0005f,
				.f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ, .f_slew=LLC_F_SLEW_HZ
	};
	llc_state_enter(ST_IDLE);
}
void llc_app_tick_adc_test(void)
{
	llc_update_measurements();
}
void llc_app_tick_1khz(void)
{
	llc_update_measurements();

#if 0	
	if (llc_faults_present()) {
      llc_enter_fault();
  		return;
  }
#endif
	bool enable_llc = pfc_is_ready();
	switch(s_llc_rt.app.state)
	{
		case ST_IDLE:
			if(enable_llc&&llc_precheck_ok())
			{
				  llc_state_enter(ST_PRECHECK);
			}
			break;
		case ST_PRECHECK:
        if (!enable_llc) {
          llc_state_enter(ST_STOPPING);
					break;
				}
				if (!llc_precheck_ok()) {
					llc_state_enter(ST_STOPPING);
					break;
				}
				if (!elapsed_reached(s_llc_rt.app.entry_ms, 100U)) { // 100ms 稳定等待
					break;
				}
				llc_state_enter(ST_SOFTSTART);
        break;
		case ST_SOFTSTART:
			llc_softstart_tick_1khz();
			if(!enable_llc|| !llc_precheck_ok())
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			if(elapsed_reached(s_llc_rt.softstart_begin_ms,LLC_SOFTSTART_DURATION_MS + LLC_SOFTSTART_STABILIZE_MS))
			{
				float e = s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V;   // 有符号误差
        float ae = fabsf(e);
				llc_state_enter(ST_LLC_RUN);
        break;
			}
			break;

		case ST_LLC_RUN:
			if(!enable_llc||(s_llc_rt.meas.vbus_v<(LLC_VBUS_MIN_START_V-LLC_VOUT_HYST_V)))
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			float err = s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V;
			if(fabsf(err)>LLC_SWEEP_TARGET_WINDOW_V && elapsed_reached(s_llc_rt.hold_last_adjust_ms,LLC_HOLD_ADJUST_PERIOD_MS))
			{
				 float delta = (err < 0.0f) ? -LLC_HOLD_ADJUST_HZ : LLC_HOLD_ADJUST_HZ;
				
				 if((s_llc.f_cmd+delta) >= LLC_F_MIN_HZ && (s_llc.f_cmd+delta) <= LLC_F_MAX_HZ)
				 {
					 llc_set_freq(s_llc.f_cmd + delta);
				 }
				 
				 s_llc_rt.hold_last_adjust_ms = g_ms;
			}
			break;
		case ST_STOPPING:
			if(elapsed_reached(s_llc_rt.stopping_begin_ms,LLC_STOPPING_FREQ_HOLD_MS))
			{
				llc_pwm_outputs_enable(0);
				llc_driver_en_set(false);
				llc_state_enter(ST_IDLE);
			}
			break;
		case ST_FAULT:
			    llc_softstart_on_fault();

					// 1) 先停 LLC（最关键）
					llc_pwm_outputs_enable(0);
					llc_driver_en_set(false);
					llc_set_freq(s_llc.f_max);

					// 2) 再停 PFC
					pfc_hw_set_main(false);   // 或者 pfc_disable()
			
		default:
			break;
	}
}


llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}





