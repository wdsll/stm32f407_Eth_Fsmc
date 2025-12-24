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
    float sweep_best_freq;
    float sweep_best_error;
    uint8_t sweep_stable_hits;
    uint32_t sweep_start_ms;
    uint32_t sweep_last_step_ms;
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
static void llc_handle_sweep(void);
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
    //adc_multi_copy();
    s_llc_rt.meas.vout_v = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    s_llc_rt.meas.iout_a = conv_adc_to_i(g_adc_multi.isense_raw);
    s_llc_rt.meas.vbus_v = pfc_bus_voltage();
    s_llc.vmeas = s_llc_rt.meas.vout_v;
}
static bool llc_precheck_ok(void)
{
    bool hw_fault = protect_fault_active_hw() || protect_fault_latched();
    bool pfc_fault = pfc_is_fault() || pfc_is_fault_latched();
    if (hw_fault || pfc_fault) {
        return false;
    }

    return (s_llc_rt.meas.vbus_v >= LLC_VBUS_MIN_START_V);
}
static bool llc_faults_present(void)
{
    if (protect_fault_active_hw() || protect_fault_latched()) {
        return true;
    }

    if (pfc_is_fault() || pfc_is_fault_latched()) {
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
	s_llc_app.state = next;
	s_llc_app.entry_ms = g_ms;
	#if Bus_Adj
	//bus_vol_adj_reset();   //重置总线电压调整逻辑 百分之五十的占空比
	#endif
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
		case ST_IDLE:
			llc_softstart_on_fault();
			pfc_hw_set_main(false);   // 强制关闭 PFC
			llc_pwm_outputs_enable(0); // 禁用 PWM 输出
			llc_driver_en_set(false); //disable llc
			s_llc.f_cmd = s_llc.f_max; // 设置频率为最大值
		    s_llc_rt.sweep_best_error = FLT_MAX; //重置最佳误差为无穷大，清除历史最优解
            s_llc_rt.sweep_best_freq = LLC_SWEEP_START_HZ; //重置最佳频率为扫描起始值，准备重新进行频率扫描
      		s_llc_rt.sweep_stable_hits = 0U; //重置稳定计数器，用于判断系统是否达到稳态
      	    s_llc_rt.softstart_begin_ms = 0U;  //软启动开始时间戳，用于控制软启动斜坡
      	    s_llc_rt.stopping_begin_ms = 0U;  //停机过程开始时间戳，用于控制停机时序
      		s_llc_rt.hold_last_adjust_ms = 0U; //保持最后调整的时间戳，用于频率调整的去抖
			break;
	   case ST_PRECHECK:  
	  	llc_driver_en_set(true);
    	llc_pwm_outputs_enable(0); // 禁用 PWM 输出
    	s_llc.f_cmd = s_llc.f_max; // 设置频率为最大值
        break;
		case ST_SOFTSTART:
			s_llc_rt.softstart_begin_ms = g_ms;
			llc_driver_en_set(true);
			llc_pwm_outputs_enable(1);
			llc_softstart_start(LLC_SOFTSTART_TARGET_DUTY);
			break;
		case ST_SWEEP:
			llc_driver_en_set(true);
			 s_llc_rt.sweep_best_error = FLT_MAX;  //重置最佳误差为无穷大，清除历史最优解
			 s_llc_rt.sweep_best_freq = LLC_SWEEP_START_HZ;
			 s_llc_rt.sweep_stable_hits = 0U;
			 s_llc_rt.sweep_start_ms = g_ms;
			 s_llc_rt.sweep_last_step_ms = g_ms;
			 llc_set_freq(LLC_SWEEP_START_HZ);
		break;
		case ST_LLC_RUN:
			llc_driver_en_set(true);
			 llc_set_freq(s_llc_rt.sweep_best_freq);
       s_llc_rt.hold_last_adjust_ms = g_ms;
			break;
		case ST_STOPPING:
        s_llc_rt.stopping_begin_ms = g_ms;
		llc_driver_en_set(true);
        llc_set_freq(s_llc.f_max);
        break;
		case ST_FAULT:
		default:
			  llc_softstart_on_fault();
        pfc_hw_set_main(false); //PFC_off
        llc_pwm_outputs_enable(0); //llc pwm disable
        llc_driver_en_set(false);
        llc_set_freq(s_llc.f_max);
        break;
	}
}
static void llc_handle_sweep(void)
{
	//超时保护机制
		uint32_t elapsed = elapsed_since(s_llc_rt.sweep_start_ms); 
		if (elapsed_reached(s_llc_rt.sweep_start_ms, LLC_SWEEP_TIMEOUT_MS)) {
			llc_state_enter(ST_STOPPING);
			return;
		}
		//每次循环都计算当前输出与目标的偏差
		float err = fabsf(s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V); //使用 fabsf() 确保误差始终为正值
		
		if (err < s_llc_rt.sweep_best_error) {
			s_llc_rt.sweep_best_error = err;
			//贪心策略：只要发现更好的解就立即更新
			s_llc_rt.sweep_best_freq = s_llc.f_cmd;
    }
		//不是追求完美匹配，而是允许一定误差范围
		if (err <= LLC_SWEEP_TARGET_WINDOW_V) {
			//需要连续多次都在误差范围内才算稳定,去抖动：防止瞬时波动导致的误判
        if (++s_llc_rt.sweep_stable_hits >= LLC_SWEEP_STABLE_COUNT) {
            llc_state_enter(ST_LLC_RUN);
            return;
        }
    } else {
        s_llc_rt.sweep_stable_hits = 0U;
    }
		//定时步进：按照固定时间间隔调整频率，给系统足够时间响应 
		//LLC_SWEEP_STEP_MS ：确保每次频率调整后系统有时间稳定
		if (elapsed_since(s_llc_rt.sweep_last_step_ms) >= LLC_SWEEP_STEP_MS) {
			//从高频向低频扫描（LLC特性：频率越低，功率越大）
        float next_freq = s_llc.f_cmd - LLC_SWEEP_STEP_HZ;
			// 增强边界检查：确保频率在有效范围内
        if (next_freq < LLC_SWEEP_STOP_HZ) {
					// 达到扫描下限，停止扫描
            llc_state_enter(ST_STOPPING);
            return;
        }
				// 额外检查：确保频率不超出系统允许范围
				next_freq = f_clampf(next_freq, s_llc.f_min, s_llc.f_max);
        llc_set_freq(next_freq);
        s_llc_rt.sweep_last_step_ms = g_ms;
    }
}

/*********************************************************************************************************
*                                              公共接口
*********************************************************************************************************/
void llc_app_init()
{
	llc_softstart_init();
		s_llc = (llc_t){
	        .vref=VBUS_TARGET_V, .vmeas=0.0f, .kp=0.01f, .ki=0.0005f,
	        .f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ, .f_slew=LLC_F_SLEW_HZ
	};
	llc_state_enter(ST_IDLE);
}

void llc_app_tick_1khz(void)
{
	llc_update_measurements();
	
	if (llc_faults_present()) {
      llc_enter_fault();
  		return;
  }
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
            llc_enter_fault();
					break;
				}
				if (!llc_precheck_ok()) {
					llc_state_enter(ST_STOPPING);
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
			if(elapsed_reached(s_llc_rt.softstart_begin_ms,LLC_SOFTSTART_DURATION_MS))
			{
				llc_state_enter(ST_SWEEP);
			}
			break;
		case ST_SWEEP:
			if(!enable_llc)
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			llc_handle_sweep();
			break;
		case ST_LLC_RUN:
			if(!enable_llc||(s_llc_rt.meas.vbus_v<(LLC_VBUS_MIN_START_V-LLC_VOUT_HYST_V)))
			{
				llc_state_enter(ST_STOPPING);
			}
			float err = s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V;
			if(fabsf(err)>LLC_SWEEP_TARGET_WINDOW_V && elapsed_reached(s_llc_rt.hold_last_adjust_ms,LLC_HOLD_ADJUST_PERIOD_MS))
			{
				 float delta = (err < 0.0f) ? -LLC_HOLD_ADJUST_HZ : LLC_HOLD_ADJUST_HZ;
				 llc_set_freq(s_llc.f_cmd + delta);
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
			
		default:
			break;
	}
}


llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}





