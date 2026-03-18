#include "llc_control.h"
#include "float.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#ifndef LLC_IOUT_TARGET_A
#define LLC_IOUT_TARGET_A (12.0f)
#endif
#ifndef LLC_I_LOOP_KP
#define LLC_I_LOOP_KP (0.02f)
#endif
#ifndef LLC_I_LOOP_KI
#define LLC_I_LOOP_KI (0.0010f)
#endif

#ifndef LLC_USE_RAW_PI
#define LLC_USE_RAW_PI               (0)     /* 0: 电压PI  1: RAW PI */
#endif

#ifndef LLC_VOUT_FILT_ALPHA
#define LLC_VOUT_FILT_ALPHA          (0.08f) /* 100us快环滤波系数 */
#endif

#ifndef LLC_RAW_PI_TARGET_CODE
#define LLC_RAW_PI_TARGET_CODE       (2836.0f) /* 48V对应ADC码值，需按实测校准 */
#endif

#ifndef LLC_RAW_PI_USE_FILT
#define LLC_RAW_PI_USE_FILT          (1)     /* RAW PI模式下是否也做一阶滤波 */
#endif
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

static inline float llc_freq_to_period(float hz)
{
    if (hz < s_llc.f_min) {
        hz = s_llc.f_min;
    }
    if (hz > s_llc.f_max) {
        hz = s_llc.f_max;
    }

    /* 用归一化“等效周期量”：
     * 直接取 1/f 不方便数值调参，所以放大成 k/f
     * 这里用 1e9 只是为了让数值落在比较好看的范围
     */
    return 1.0e9f / hz;
}

static inline float llc_period_to_freq(float period)
{
    float pmin = 1.0e9f / s_llc.f_max;   /* 高频 -> 小周期 */
    float pmax = 1.0e9f / s_llc.f_min;   /* 低频 -> 大周期 */

    if (period < pmin) {
        period = pmin;
    }
    if (period > pmax) {
        period = pmax;
    }

    return 1.0e9f / period;
}

static void llc_set_freq(float hz)
{
    float f = f_clampf(hz, s_llc.f_min, s_llc.f_max);
    s_llc.f_cmd = f;
    llc_pwm_set_freq((uint32_t)f);
}

#if 1
//频率PI
static float llc_ctrl_step(float e)
{
	#if LLC_USE_RAW_PI
	  kp = s_llc.kp_raw;
    ki = s_llc.ki_raw;
	#else
	  float kp = s_llc.kp;
    float ki = s_llc.ki;
  #endif
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_slew = s_llc.f_slew;
	
	  if (f_min <= 0.0f || f_min >= f_max) {
        f_min = 75000.0f;
        f_max = 150000.0f;
    }
    if (f_slew <= 0.0f) {
        f_slew = 1000.0f;
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
		
		/* LLC频率控制：
     * e > 0 (Vref > Vout) => 需要加功率 => 频率下降
     */
    float f_req = f_nom - u;
    float f_sat = f_clampf(f_req, f_min, f_max);
		if (fabsf(f_req - f_sat) > 1e-6f) {
        /* 饱和：抗积分饱和 */
        s_llc.integ = (f_nom - f_sat) - kp * e;
    } else {
        /* 未饱和：正常积分 */
        s_llc.integ = x_candidate;
    }
    /* 频率slew限制 */
		float df = f_sat - f_prev;
    if (df > f_slew) {
        s_llc.f_cmd = f_prev + f_slew;
    } else if (df < -f_slew) {
        s_llc.f_cmd = f_prev - f_slew;
    } else {
        s_llc.f_cmd = f_sat;
    }
		s_llc.f_cmd = f_clampf(s_llc.f_cmd, f_min, f_max);
	  return s_llc.f_cmd;
}
#else
static float llc_ctrl_step(float e)
{
		float kp = s_llc.kp;
    float ki = s_llc.ki;
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_slew = s_llc.f_slew;

    float p_min, p_max;
    float p_prev;
    float p_candidate;
    float p_sat;
    float f_req;
    float f_prev;
    float df;
#if LLC_USE_RAW_PI
	  kp = s_llc.kp_raw;
    ki = s_llc.ki_raw;
	#else
	  kp = s_llc.kp;
    ki = s_llc.ki;
#endif
	  if (f_min <= 0.0f || f_min >= f_max) {
        f_min = 75000.0f;
        f_max = 150000.0f;
    }

    if (f_slew <= 0.0f) {
        f_slew = 1000.0f;
    }

    if (kp <= 0.0f) {
        kp = 10.0f;
    }

    if (ki < 0.0f) {
        ki = 0.0f;
    }
		
		p_prev = s_llc.period_cmd;
		
		p_min = s_llc.period_min;   /* 高频对应小周期 */
    p_max = s_llc.period_max;   /* 低频对应大周期 */
    if (p_prev < p_min || p_prev > p_max) {
        p_prev = llc_freq_to_period(s_llc.f_cmd);
    }
    if (p_prev < p_min || p_prev > p_max) {
        p_prev = 0.5f * (p_min + p_max);
    }

		
		/* 周期控制：
     * e > 0 (Vref > Vout) => period 增大 => frequency 降低
     */
    p_candidate = s_llc.period_integ + ki * e;
    p_sat = kp * e + p_candidate;

    /* 周期限幅 */
    p_sat = f_clampf(p_sat, p_min, p_max);

    /* 抗积分饱和 */
    if (fabsf((kp * e + p_candidate) - p_sat) > 1e-6f) {
        s_llc.period_integ = p_sat - kp * e;
    } else {
        s_llc.period_integ = p_candidate;
    }
		s_llc.period_cmd = p_sat;
    /* 再换算成频率命令 */
    f_req = llc_period_to_freq(p_sat);
		
		/* 保留你原来的频率slew限制 */
    f_prev = s_llc.f_cmd;
    if (f_prev < f_min || f_prev > f_max) {
        f_prev = f_req;
    }

		df = f_req - f_prev;
    if (df > f_slew) {
        s_llc.f_cmd = f_prev + f_slew;
    } else if (df < -f_slew) {
        s_llc.f_cmd = f_prev - f_slew;
    } else {
        s_llc.f_cmd = f_req;
    }
    
	  s_llc.f_cmd = f_clampf(s_llc.f_cmd, f_min, f_max);
    return s_llc.f_cmd;
}

#endif
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
   // if (protect_fault_active_hw() || protect_fault_latched()) {
   //     return true;
   // }

   // if (pfc_is_fault()) {
   //     return true;
   // }

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
//频率版的在切入闭环瞬间做“无扰切换（bumpless transfer）”
#if 1
static void llc_ctrl_bumpless_init(float vref, float vmeas, float f_now)
{
		#if LLC_USE_RAW_PI
				float kp = s_llc.kp_raw;
		#else
			 float kp = s_llc.kp;
		#endif
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_nom = 0.5f * (f_min + f_max);
    float e = vref - vmeas;
    float integ = (f_nom - f_now) - kp * e;
    float i_lim = f_max - f_min;

    if (integ > i_lim) {
        integ = i_lim;
    } else if (integ < -i_lim) {
        integ = -i_lim;
    }

    s_llc.integ = integ;
    s_llc.f_cmd = f_clampf(f_now, f_min, f_max);
}
#else
//周期版的
static void llc_ctrl_bumpless_init(float vref, float vmeas, float f_now)
{
    float kp = s_llc.kp;
    float e = vref - vmeas;

    float p_now = llc_freq_to_period(f_now);
    float p_min = s_llc.period_min;
    float p_max = s_llc.period_max;

    /* 周期PI：period = kp*e + integ */
    float integ = p_now - kp * e;

    if (integ < p_min) {
        integ = p_min;
    } else if (integ > p_max) {
        integ = p_max;
    }

    s_llc.period_integ = integ;
    s_llc.period_cmd = p_now;
    s_llc.f_cmd = f_clampf(f_now, s_llc.f_min, s_llc.f_max);
}
#endif
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
			llc_pwm_outputs_enable(1);
			llc_softstart_start(LLC_SOFTSTART_TARGET_HZ);
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
#if LLC_USE_RAW_PI
    llc_ctrl_bumpless_init(LLC_RAW_PI_TARGET_CODE, (float)g_adc_multi.vout_raw, s_llc.f_cmd);
#else
    llc_ctrl_bumpless_init(LLC_VOUT_TARGET_V, s_llc_rt.meas.vout_v, s_llc.f_cmd);
#endif
			//s_llc.integ = 0.0f;
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
#if 0
void llc_app_init()
{
	llc_softstart_init();
	s_llc = (llc_t){
				.vref=LLC_VOUT_TARGET_V, .vmeas=0.0f, .kp=900.0f, .ki=20.0f, 
		    .kp_raw = 3.0f, .kp_raw = 0.050f,
		    .integ = 0.0f, .iref = 0.0f, .imeas = 0.0f,
				.ikp = LLC_I_LOOP_KP,.iki = LLC_I_LOOP_KI,.i_integ = 0.0f,
				.f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ, .f_slew=LLC_F_SLEW_HZ,
        .f_cmd_v = 0.0f, .f_cmd_i = 0.0f, .period_cmd = 0.0f,.period_min = 0.0f,.period_max = 0.0f,.period_integ = 0.0f
	};
	s_llc.period_min = llc_freq_to_period(s_llc.f_max);  /* 高频 -> 小周期 */
	s_llc.period_max = llc_freq_to_period(s_llc.f_min);  /* 低频 -> 大周期 */
	
	s_llc.period_cmd = llc_freq_to_period(s_llc.f_cmd);
  s_llc.period_integ = s_llc.period_cmd;
	llc_state_enter(ST_IDLE);
}
#else
void llc_app_init(void)
{
    llc_softstart_init();

    s_llc = (llc_t){
        .vref    = LLC_VOUT_TARGET_V,
        .vmeas   = 0.0f,

        /* 正常电压PI参数 */
        .kp      = 1400.0f,
        .ki      = 10.0f,

        /* RAW PI参数 */
        .kp_raw  = 3.0f,
        .ki_raw  = 0.050f,

        .integ   = 0.0f,
        .iref    = 0.0f,
        .imeas   = 0.0f,

        .ikp     = LLC_I_LOOP_KP,
        .iki     = LLC_I_LOOP_KI,
        .i_integ = 0.0f,

        .f_min   = LLC_F_MIN_HZ,
        .f_max   = LLC_F_MAX_HZ,
        .f_cmd   = LLC_F_INIT_HZ,
        .f_slew  = LLC_F_SLEW_HZ,

        .f_cmd_v = 0.0f,
        .f_cmd_i = 0.0f
    };

    llc_state_enter(ST_IDLE);
}
#endif
void llc_app_tick_adc_test(void)
{
	llc_update_measurements();
}
#if 0
void llc_app_tick_100us(void)
{
    if (s_llc_rt.app.state != ST_LLC_RUN) {
        return;
    }

    //s_llc.vmeas = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    s_llc.vmeas = g_adc_multi.vout_raw;
    //float err = s_llc.vref - s_llc.vmeas;
		float err = 2836 - s_llc.vmeas;  // 2836  48V==>vtarget
    float f_cmd = llc_ctrl_step(err);
    llc_set_freq(f_cmd);
}
#endif

void llc_app_tick_100us(void)
{
	  static float vout_filt_v   = 0.0f;   /* 电压PI用滤波状态 */
    static float vout_filt_raw = 0.0f;   /* RAW PI用滤波状态 */
    float err;
    float f_cmd;

    if (s_llc_rt.app.state != ST_LLC_RUN) {
        return;
    }
#if LLC_USE_RAW_PI
		
    /* =========================
     * RAW PI模式
     * 直接使用ADC原始码值做控制
     * err = target_code - raw_code
     * ========================= */
		float vout_raw_now = (float)g_adc_multi.vout_raw;
		float vout_raw_used;
		#if LLC_RAW_PI_USE_FILT
        if (vout_filt_raw < 0.5f) {
            vout_filt_raw = vout_raw_now;
        }
        vout_filt_raw += LLC_VOUT_FILT_ALPHA * (vout_raw_now - vout_filt_raw);
        vout_raw_used = vout_filt_raw;
    #else
        vout_raw_used = vout_raw_now;
    #endif
		    /* 注意：
         * s_llc.vmeas 在RAW模式下保存的是“raw码值对应的float”
         * 仅供控制和调试使用，不再代表真实电压值
         */
        s_llc.vmeas = vout_raw_used;
        err = LLC_RAW_PI_TARGET_CODE - s_llc.vmeas;
			}
#else		
		 /* =========================
     * 正常电压PI模式
     * raw -> 电压 -> 滤波 -> PI
     * err = vref - vmeas
     * ========================= */
		{
    float vout_now = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);

	  if (vout_filt_v < 0.001f) {
					vout_filt_v = vout_now;
			}
    /* 100us快环滤波，alpha可后续再调 */
     vout_filt_v += LLC_VOUT_FILT_ALPHA * (vout_now - vout_filt_v);

    s_llc.vmeas = vout_filt_v;
    err = s_llc.vref - s_llc.vmeas;
		/* 周期控制PI，返回的是换算后的频率命令 */
#endif
    f_cmd = llc_ctrl_step(err);
    llc_set_freq(f_cmd);
	}
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
#if 0
	if (s_llc_rt.app.state != ST_IDLE && s_llc_rt.app.state != ST_FAULT) {
		if (llc_faults_present()) {
			llc_enter_fault();
			return;
		}
#endif
	
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
		{
			float hold_err;
			if(!enable_llc|| !llc_precheck_ok())
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			hold_err = s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V;
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
		}
		case ST_LLC_RUN:
			if(!enable_llc||(s_llc_rt.meas.vbus_v<(LLC_VBUS_MIN_START_V-LLC_VOUT_HYST_V)))
			{
				llc_state_enter(ST_STOPPING);
				break;
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

			break;
			
		default:
			break;
	}
}


llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}





