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

#ifndef LLC_F_NOM_USE_STAGE_CMD
#define LLC_F_NOM_USE_STAGE_CMD      (1)
#endif

#ifndef LLC_F_NOM_HZ
#define LLC_F_NOM_HZ                 (LLC_F_INIT_HZ)
#endif

#ifndef LLC_LOOP_DEBUG_PRINT_EVERY_N
#define LLC_LOOP_DEBUG_PRINT_EVERY_N (1U)
#endif

#ifndef LLC_TRACE_ENABLE
#define LLC_TRACE_ENABLE            (1)
#endif

#ifndef LLC_TRACE_MAX_SAMPLES
#define LLC_TRACE_MAX_SAMPLES       (500U)
#endif

#ifndef LLC_TRACE_AUTO_ARM_ON_RUN
#define LLC_TRACE_AUTO_ARM_ON_RUN   (1)
#endif

#ifndef LLC_TRACE_AUTO_ARM_SAMPLES
#define LLC_TRACE_AUTO_ARM_SAMPLES  (500U)
#endif

#ifndef LLC_TRACE_DUMP_LINES_PER_1MS
#define LLC_TRACE_DUMP_LINES_PER_1MS (2U)
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

typedef struct
{
    uint16_t vout_raw;
    int32_t  vout_mv;
    int32_t  err_mv;
    uint32_t f_cmd_hz;
} llc_trace_sample_t;

typedef struct
{
    uint8_t  armed;
    uint8_t  triggered;
    uint8_t  done;
    uint8_t  dumping;
    uint16_t wr;
    uint16_t count;
    uint16_t max_samples;
    uint16_t dump_idx;
} llc_trace_ctrl_t;
static llc_runtime_ctx_t s_llc_rt;

static llc_app_ctx_t s_llc_app;


#if LLC_TRACE_ENABLE
static llc_trace_sample_t s_llc_trace[LLC_TRACE_MAX_SAMPLES];
static llc_trace_ctrl_t   s_llc_trace_ctrl;
#endif

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
#if LLC_TRACE_ENABLE
static void llc_trace_init(void);
void llc_trace_arm(uint16_t samples);
static inline void llc_trace_push_sample(uint16_t vout_raw, float vout_v, float err_v, float f_cmd);
static void llc_trace_dump_task(void);
#endif
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

static inline float llc_get_f_nom(float f_min, float f_max, float f_stage_cmd)
{
#if LLC_F_NOM_USE_STAGE_CMD
    if (f_stage_cmd >= f_min && f_stage_cmd <= f_max) {
        return f_stage_cmd;
    }
#endif
	{
    float f_nom = LLC_F_NOM_HZ;
    if (f_nom < f_min || f_nom > f_max) {
        f_nom = 0.5f * (f_min + f_max);
    }
    return f_nom;
	}
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
#if LLC_TRACE_ENABLE
static void llc_trace_init(void)
{
    s_llc_trace_ctrl.armed      = 0U;
    s_llc_trace_ctrl.triggered  = 0U;
    s_llc_trace_ctrl.done       = 0U;
    s_llc_trace_ctrl.dumping    = 0U;
    s_llc_trace_ctrl.wr         = 0U;
    s_llc_trace_ctrl.count      = 0U;
    s_llc_trace_ctrl.max_samples= LLC_TRACE_MAX_SAMPLES;
    s_llc_trace_ctrl.dump_idx   = 0U;
}

void llc_trace_arm(uint16_t samples)
{
    if (samples == 0U) {
        samples = 200U;
    }
    if (samples > LLC_TRACE_MAX_SAMPLES) {
        samples = LLC_TRACE_MAX_SAMPLES;
    }

    s_llc_trace_ctrl.armed       = 1U;
    s_llc_trace_ctrl.triggered   = 0U;
    s_llc_trace_ctrl.done        = 0U;
    s_llc_trace_ctrl.dumping     = 0U;
    s_llc_trace_ctrl.wr          = 0U;
    s_llc_trace_ctrl.count       = 0U;
    s_llc_trace_ctrl.max_samples = samples;
    s_llc_trace_ctrl.dump_idx    = 0U;
}

/**
 * @brief 向追踪缓冲区推入一个采样数据
 * 
 * 该函数用于在触发状态下记录运行时数据，包括输出电压原始值、
 * 输出电压值、电压误差和频率命令。当缓冲区填满时自动停止记录。
 * 
 * @param vout_raw 输出电压ADC原始值
 * @param vout_v   输出电压值（单位：伏特）
 * @param err_v    电压误差值（单位：伏特）
 * 
 * @note 函数内部将浮点电压值转换为毫伏整数存储
 * @note 仅在 armed 置位时触发一次记录
 * @note 缓冲区满后设置 done 标志，停止接收新数据
 */
static inline void llc_trace_push_sample(uint16_t vout_raw, float vout_v, float err_v, float f_cmd)
{
    uint16_t i;

    if (s_llc_trace_ctrl.armed) {
        s_llc_trace_ctrl.armed = 0U;
        s_llc_trace_ctrl.triggered = 1U;
    }

    if (!s_llc_trace_ctrl.triggered || s_llc_trace_ctrl.done) {
        return;
    }

    i = s_llc_trace_ctrl.wr;
    if (i >= s_llc_trace_ctrl.max_samples) {
        s_llc_trace_ctrl.triggered = 0U;
        s_llc_trace_ctrl.done = 1U;
        s_llc_trace_ctrl.count = s_llc_trace_ctrl.max_samples;
        return;
    }

    s_llc_trace[i].vout_raw = vout_raw;
    s_llc_trace[i].vout_mv  = (int32_t)(vout_v * 1000.0f);
    s_llc_trace[i].err_mv   = (int32_t)(err_v * 1000.0f);
    s_llc_trace[i].f_cmd_hz = (uint32_t)(f_cmd);

    s_llc_trace_ctrl.wr++;

    if (s_llc_trace_ctrl.wr >= s_llc_trace_ctrl.max_samples) {
        s_llc_trace_ctrl.triggered = 0U;
        s_llc_trace_ctrl.done = 1U;
        s_llc_trace_ctrl.count = s_llc_trace_ctrl.max_samples;
    }
}

/**
 * @brief LLC 追踪数据转储任务
 * 
 * 该函数在 1ms 周期任务中调用，负责将缓存的 LLC 追踪数据通过 UART 输出。
 * 每次调用最多输出 LLC_TRACE_DUMP_LINES_PER_1MS 行数据，避免阻塞 1ms 任务。
 * 输出格式为 CSV，包含索引、输出电压原始值、输出电压(mV)、误差(mV)和命令频率(Hz)。
 * 
 * @note 该函数为内部实现细节，仅供 LLC 控制模块内部使用
 * @note 转储过程中 s_llc_trace_ctrl.dumping 标志位置 1，转储完成后自动清零
 * @note 转储完成后会重写追踪缓冲区控制结构，准备下一次追踪
 * 
 * @param  无
 * @return 无
 */
static void llc_trace_dump_task(void)
{
    uint16_t n = 0U;

    if (s_llc_trace_ctrl.done && !s_llc_trace_ctrl.dumping) {
        s_llc_trace_ctrl.dumping  = 1U;
        s_llc_trace_ctrl.dump_idx = 0U;
        debug_printf("\r\n[LLC_TRACE_BEGIN] count=%u Ts=100us\n", s_llc_trace_ctrl.count);
        debug_printf("idx,vout_raw,vout_mv,err_mv,f_cmd_hz\n");
    }

    if (!s_llc_trace_ctrl.dumping) {
        return;
    }

    while ((s_llc_trace_ctrl.dump_idx < s_llc_trace_ctrl.count) &&
           (n < LLC_TRACE_DUMP_LINES_PER_1MS)) {
        uint16_t i = s_llc_trace_ctrl.dump_idx;

   debug_printf("%u,%u,%ld,%ld,%lu\n",
             i,
             s_llc_trace[i].vout_raw,
             (long)s_llc_trace[i].vout_mv,
             (long)s_llc_trace[i].err_mv,
             (unsigned long)s_llc_trace[i].f_cmd_hz);

        s_llc_trace_ctrl.dump_idx++;
        n++;
    }

    if (s_llc_trace_ctrl.dump_idx >= s_llc_trace_ctrl.count) {
        debug_printf("[LLC_TRACE_END]\n");

        s_llc_trace_ctrl.dumping  = 0U;
        s_llc_trace_ctrl.done     = 0U;
        s_llc_trace_ctrl.count    = 0U;
        s_llc_trace_ctrl.wr       = 0U;
        s_llc_trace_ctrl.dump_idx = 0U;
    }
}
#endif
#if 1
//频率PI
/**
 * @brief LLC PI控制器单步计算
 * 
 * 根据误差信号计算并更新LLC开关频率，实现电压闭环控制。包含参数有效性检查、
 * 抗积分饱和处理和频率斜率限制。
 * 
 * @param e 控制误差，正值表示Vref > Vout（需要增加功率，降低频率）
 * @return 计算后的LLC开关频率（Hz），已应用斜率限制和边界约束
 * 
 * @note 控制逻辑：e > 0 时频率下降以增加输出功率
 * @note 当参数无效时使用默认值：f_min=75kHz, f_max=150kHz, f_slew=1kHz/s, kp=10
 * @note 使用抗积分饱和（anti-windup）机制防止积分器在饱和时持续累积
 * @note 频率变化受斜率限制，防止输出突变
 */
static float llc_ctrl_step(float e)
{
	  float kp = s_llc.kp;
    float ki = s_llc.ki;
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
		
		//float f_nom = 0.5f * (f_min + f_max);
		//float f_nom = llc_get_f_nom(f_min,f_max,s_llc.f_cmd);
		float f_nom = s_llc.f_nom;
		if (f_nom < f_min || f_nom > f_max) {
			f_nom = f_clampf(s_llc.f_cmd, f_min, f_max);
		}
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

/**
 * @brief 无扰动切换初始化
 * @details 计算PI控制器的积分项初始值，使得在给定参考电压和当前测量电压下，
 *          控制器输出的初始频率等于当前工作频率，从而实现模式切换时的无扰动过渡。
 * @param vref 参考电压（V）
 * @param vmeas 当前测量电压（V）
 * @param f_now 当前工作频率（Hz）
 * @note 积分项会被限制在 [f_max - f_min, -(f_max - f_min)] 范围内
 */
//static void llc_ctrl_bumpless_init(float vref, float vmeas, float f_now)
static void llc_ctrl_bumpless_init(float vref, float vmeas, float f_now)
{
		float kp = s_llc.kp;
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    //float f_nom = 0.5f * (f_min + f_max);
	  //float f_nom = llc_get_f_nom(f_min,f_max,s_llc.f_cmd);
		float f_nom = s_llc.f_nom;
		if (f_nom < f_min || f_nom > f_max) {
			f_nom = f_clampf(f_now, f_min, f_max);
		}
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

/**
 * @brief LLC 状态机状态切换函数
 * 
 * 执行从当前状态到目标状态的切换，包括状态更新、时间戳记录和各状态的初始化操作。
 * 如果目标状态与当前状态相同，则直接返回不执行任何操作。
 * 
 * @param next 目标状态，取值为 llc_state_t 枚举类型：
 *             - ST_IDLE: 空闲状态，关闭所有输出，重置时间戳
 *             - ST_PRECHECK: 预检查状态，使能驱动但保持PWM关闭
 *             - ST_SOFTSTART: 软启动状态，开始频率斜坡上升
 *             - ST_RUN_ENTRY_HOLD: 运行进入保持状态，等待稳定
 *             - ST_LLC_RUN: 正常运行状态，启动闭环控制
 *             - ST_STOPPING: 停机状态，频率回到最大值
 *             - ST_FAULT: 故障状态，关闭所有输出
 * 
 * @note 各状态切换时会执行相应的初始化操作：
 *       - ST_IDLE: 关闭PWM输出，禁用驱动，频率设为最大值，重置所有时间戳
 *       - ST_PRECHECK: 使能驱动，关闭PWM，频率设为最大值
 *       - ST_SOFTSTART: 记录启动时间，使能驱动和PWM，开始软启动斜坡
 *       - ST_RUN_ENTRY_HOLD: 记录保持时间，初始化频率为软启动结束频率
 *       - ST_LLC_RUN: 初始化无扰动控制，可选自动触发波形跟踪
 *       - ST_STOPPING: 记录停机时间，频率回到最大值
 *       - ST_FAULT: 关闭所有输出，频率设为最大值
 * 
 * @warning 函数内部会直接修改全局运行时状态 s_llc_rt.app.state
 */
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
	    s_llc.f_nom = f_clampf(s_llc.f_cmd, s_llc.f_min, s_llc.f_max);

#if LLC_USE_RAW_PI
    llc_ctrl_bumpless_init(LLC_RAW_PI_TARGET_CODE, (float)g_adc_multi.vout_raw, s_llc.f_cmd);
#else
    llc_ctrl_bumpless_init(LLC_VOUT_TARGET_V, s_llc_rt.meas.vout_v, s_llc.f_cmd);
#endif
#if LLC_TRACE_ENABLE && LLC_TRACE_AUTO_ARM_ON_RUN
      llc_trace_arm(LLC_TRACE_AUTO_ARM_SAMPLES);
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
#if LLC_TRACE_ENABLE
    llc_trace_init();
#endif
    s_llc = (llc_t){
        .vref    = LLC_VOUT_TARGET_V,
        .vmeas   = 0.0f,

        /* 正常电压PI参数 */
        .kp      = 1900.0f,
        .ki      = 18.0f,

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
        .f_nom  = LLC_F_INIT_HZ,
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
		 /* =========================
     * 正常电压PI模式
     * raw -> 电压 -> 滤波 -> PI
     * err = vref - vmeas
     * ========================= */
    float vout_now = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);

	  if (vout_filt_v < 0.001f) {
					vout_filt_v = vout_now;
			}
    /* 100us快环滤波，alpha可后续再调 */
     vout_filt_v += LLC_VOUT_FILT_ALPHA * (vout_now - vout_filt_v);

    s_llc.vmeas = vout_filt_v;
    err = s_llc.vref - s_llc.vmeas;

    f_cmd = llc_ctrl_step(err);
    llc_set_freq(f_cmd);
			
#if LLC_TRACE_ENABLE
    llc_trace_push_sample(g_adc_multi.vout_raw, s_llc.vmeas, err, s_llc.f_cmd);
#endif

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
#if LLC_TRACE_ENABLE
    llc_trace_dump_task();
#endif
}


llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}





