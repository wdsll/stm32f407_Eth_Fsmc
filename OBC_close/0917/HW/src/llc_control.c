#include "llc_control.h"
#include "float.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

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
	  uint32_t pfc_ready_begin_ms;
    uint32_t softstart_begin_ms;
    uint32_t stopping_begin_ms;
    uint32_t hold_last_adjust_ms;
	  uint32_t run_entry_hold_begin_ms;
    uint32_t run_entry_stable_ticks;
}llc_runtime_ctx_t;

static llc_runtime_ctx_t s_llc_rt;

static llc_app_ctx_t s_llc_app;
static llc_t s_llc;

typedef struct {
    float iref;
    float imeas;
    float kp;
    float ki;
    float integ;
    float f_min;
    float f_max;
    float f_cmd;
    float f_slew;
    float deadband_a;
    bool limit_active;
} llc_curr_t;

static llc_curr_t s_llc_curr;

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
static bool llc_pfc_ready_stable(bool enable_llc);
static bool llc_faults_present(void);
static void llc_enter_fault(void);
static float llc_ctrl_step(float e);

static float llc_current_ctrl_step(float e);
static float llc_current_limit_step(float i_meas);
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

static float llc_ctrl_step(float e)
{
	  float kp = s_llc.kp;
    float ki = s_llc.ki;
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_slew = s_llc.f_slew;
	
	  if (f_min <= 0.0f || f_min >= f_max) {
        f_min = 75000.0f;
        f_max = 130000.0f;
    }
    if (f_slew <= 0.0f) {
        f_slew = 5000.0f;
    }
    if (kp <= 0.0f) {
        kp = 10.0f;
    }
    if (ki < 0.0f) {
        ki = 0.0f;
    }
		float f_nom = 0.5f * (f_min + f_max);
    float f_prev = s_llc.f_cmd;
    if (f_prev < f_min || f_prev > f_max) {
        f_prev = f_nom;
    }
		
		float x_candidate = s_llc.integ + ki * e;
    float u = kp * e + x_candidate;
    float f_req = f_nom - u;
    float f_sat = f_clampf(f_req, f_min, f_max);
		
		if (f_req == f_sat) {
        float i_lim = f_max - f_min;
        if (x_candidate > i_lim) {
            x_candidate = i_lim;
        } else if (x_candidate < -i_lim) {
            x_candidate = -i_lim;
        }
        s_llc.integ = x_candidate;
    }
		
		float df = f_sat - f_prev;
    if (df > f_slew) {
        s_llc.f_cmd = f_prev + f_slew;
    } else if (df < -f_slew) {
        s_llc.f_cmd = f_prev - f_slew;
    } else {
        s_llc.f_cmd = f_sat;
    }
		
	  return s_llc.f_cmd;
}

static float llc_current_ctrl_step(float e)
{
	  float kp = s_llc_curr.kp;
    float ki = s_llc_curr.ki;
    float f_min = s_llc_curr.f_min;
    float f_max = s_llc_curr.f_max;
    float f_slew = s_llc_curr.f_slew;
	
	  if (f_min <= 0.0f || f_min >= f_max) {
        f_min = 75000.0f;
        f_max = 130000.0f;
    }
    if (f_slew <= 0.0f) {
        f_slew = 5000.0f;
    }
    if (kp <= 0.0f) {
        kp = 10.0f;
    }
    if (ki < 0.0f) {
        ki = 0.0f;
    }
		
		float f_nom = 0.5f * (f_min + f_max);
    float f_prev = s_llc_curr.f_cmd;
    if (f_prev < f_min || f_prev > f_max) {
        f_prev = f_nom;
    }
		float x_candidate = s_llc_curr.integ + ki * e;
    float u = kp * e + x_candidate;
    float f_req = f_nom - u;
    float f_sat = f_clampf(f_req, f_min, f_max);

    if (f_req == f_sat) {
        float i_lim = f_max - f_min;
        if (x_candidate > i_lim) {
            x_candidate = i_lim;
        } else if (x_candidate < -i_lim) {
            x_candidate = -i_lim;
        }
        s_llc_curr.integ = x_candidate;
    }

    float df = f_sat - f_prev;
    if (df > f_slew) {
        s_llc_curr.f_cmd = f_prev + f_slew;
    } else if (df < -f_slew) {
        s_llc_curr.f_cmd = f_prev - f_slew;
    } else {
        s_llc_curr.f_cmd = f_sat;
    }

    return s_llc_curr.f_cmd;
}

static float llc_current_limit_step(float i_meas)
{
    float iref = s_llc_curr.iref;
    float db = s_llc_curr.deadband_a;

    s_llc_curr.imeas = i_meas;

    if (s_llc_curr.limit_active) {
        if (i_meas < (iref - db)) {
            s_llc_curr.limit_active = false;
        }
    } else {
        if (i_meas > (iref + db)) {
            s_llc_curr.limit_active = true;
        }
    }

    if (!s_llc_curr.limit_active) {
        s_llc_curr.integ = 0.0f;
        s_llc_curr.f_cmd = s_llc_curr.f_min;
        return s_llc_curr.f_cmd;
    }

    float err = iref - i_meas;
    return llc_current_ctrl_step(err);
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
    return (s_llc_rt.meas.vbus_v >= LLC_VBUS_MIN_START_V);
}

static bool llc_pfc_ready_stable(bool enable_llc)
{
    if (!enable_llc) {
        s_llc_rt.pfc_ready_begin_ms = 0U;
        return false;
    }

    if (s_llc_rt.pfc_ready_begin_ms == 0U) {
        s_llc_rt.pfc_ready_begin_ms = g_ms;
    }

    return elapsed_reached(s_llc_rt.pfc_ready_begin_ms, PFC_READY_STABLE_BEFORE_LLC_MS);
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
		  s_llc_rt.pfc_ready_begin_ms = 0U;
		  s_llc_rt.run_entry_hold_begin_ms = 0U;
		  s_llc_rt.run_entry_stable_ticks = 0U;
		  break;
	 case ST_PRECHECK:  
	  	llc_driver_en_set(true);
    	llc_pwm_outputs_enable(0); // 禁用 PWM 输出
    	llc_set_freq(s_llc.f_max);
      break;
	 case ST_SOFTSTART:
			s_llc_rt.softstart_begin_ms = g_ms;
			llc_driver_en_set(true);
	 		llc_softstart_start(LLC_SOFTSTART_TARGET_HZ);
			llc_pwm_outputs_enable(1);
			break;
	 case ST_RUN_ENTRY_HOLD:
			llc_driver_en_set(true);
			s_llc_rt.run_entry_hold_begin_ms = g_ms;
			s_llc_rt.run_entry_stable_ticks = 0U;
			s_llc.f_cmd = llc_softstart_last_hz();
			llc_set_freq(s_llc.f_cmd);
			break;
	 case ST_LLC_RUN:
			llc_driver_en_set(true);
      s_llc_rt.hold_last_adjust_ms = g_ms;
			s_llc.integ = 0.0f;
	 		s_llc_curr.integ = 0.0f;
			s_llc_curr.limit_active = false;
			s_llc_curr.f_cmd = s_llc_curr.f_min;
			break;
	 case ST_STOPPING:
      s_llc_rt.stopping_begin_ms = g_ms;
		  llc_driver_en_set(true);
      llc_set_freq(s_llc.f_max);
      break;
	 case ST_FAULT:
		 	llc_softstart_on_fault();
			llc_pwm_outputs_enable(0);
			llc_driver_en_set(false);
			llc_set_freq(s_llc.f_max);
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
	
	s_llc_curr = (llc_curr_t){
				.iref=LLC_IOUT_TARGET_A, .imeas=0.0f, .kp=LLC_IOUT_CTRL_KP, .ki=LLC_IOUT_CTRL_KI, .integ=0.0f,
				.f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_MIN_HZ, .f_slew=LLC_F_SLEW_HZ,
				.deadband_a=LLC_IOUT_CTRL_DEADBAND_A, .limit_active=false
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
	bool pfc_ready_stable = llc_pfc_ready_stable(enable_llc);

	if (s_llc_rt.app.state != ST_IDLE && s_llc_rt.app.state != ST_FAULT) {
		if (llc_faults_present()) {
			llc_enter_fault();
			return;
		}
	}
	switch(s_llc_rt.app.state)
	{
		case ST_IDLE:
			if(pfc_ready_stable &&llc_precheck_ok())
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
				float e = s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V;   // з
				float ae = fabsf(e);
				(void)ae;
				llc_softstart_stop();
				s_llc.f_cmd = llc_softstart_last_hz();
				llc_state_enter(ST_RUN_ENTRY_HOLD);
				break;
			}
			break;
    case ST_RUN_ENTRY_HOLD:
			if(!enable_llc|| !llc_precheck_ok())
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			float hold_err = s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V;
			if (fabsf(hold_err) <= LLC_RUN_ENTRY_STABLE_WINDOW_V) {
				if (s_llc_rt.run_entry_stable_ticks < LLC_RUN_ENTRY_STABLE_TICKS) {
					s_llc_rt.run_entry_stable_ticks++;
				}
			} 
			else 
			{
				s_llc_rt.run_entry_stable_ticks = 0U;
			}
			if (elapsed_reached(s_llc_rt.run_entry_hold_begin_ms, LLC_RUN_ENTRY_HOLD_MS) && s_llc_rt.run_entry_stable_ticks >= LLC_RUN_ENTRY_STABLE_TICKS)
			{
				llc_state_enter(ST_LLC_RUN);
				break;
			}

			if (elapsed_reached(s_llc_rt.run_entry_hold_begin_ms, LLC_RUN_ENTRY_TIMEOUT_MS)) {
				llc_state_enter(ST_STOPPING);
				break;
			}
				break;		
		case ST_LLC_RUN:
			if(!enable_llc||(s_llc_rt.meas.vbus_v<(LLC_VBUS_MIN_START_V-LLC_VOUT_HYST_V)))
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			s_llc.vmeas = s_llc_rt.meas.vout_v;
			float err = s_llc.vref - s_llc.vmeas;
			float f_cmd_v = llc_ctrl_step(err);
			float f_cmd_i = llc_current_limit_step(s_llc_rt.meas.iout_a);
			float f_cmd = (f_cmd_i > f_cmd_v) ? f_cmd_i : f_cmd_v;
			llc_set_freq(f_cmd);			
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

			break;
			
		default:
			break;
	}
}


llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}





