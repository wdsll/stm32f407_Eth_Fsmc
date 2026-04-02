#include "llc_control.h"
#include "float.h"
#include "temp_control.h"
#include "llc_trace.h"
#include "llc_log_dump.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#ifndef LLC_VOUT_FILT_ALPHA
#define LLC_VOUT_FILT_ALPHA          (0.08f) /* 100us快环滤波系数 */
#endif

#ifndef LLC_F_NOM_USE_STAGE_CMD
#define LLC_F_NOM_USE_STAGE_CMD      (LLC_F_NOM_FOLLOW_SOFTSTART_END)
#endif

#ifndef LLC_F_NOM_HZ
#define LLC_F_NOM_HZ                 (75000.0f)  
#endif
/*********************************************************************************************************
*                                             CR模式负载动态响应测试
*********************************************************************************************************/
#ifndef LLC_CR_RESP_LOG_ENABLE
#define LLC_CR_RESP_LOG_ENABLE        (1U)
#endif

#ifndef LLC_CR_RESP_LOG_PERIOD_MS
#define LLC_CR_RESP_LOG_PERIOD_MS     (1000U)
#endif

#ifndef LLC_CR_RESP_STEP_IOUT_A
#define LLC_CR_RESP_STEP_IOUT_A       (1.5f)
#endif

#ifndef LLC_CR_RESP_STABLE_WIN_V
#define LLC_CR_RESP_STABLE_WIN_V      (1.0f)
#endif

#ifndef LLC_CR_RESP_STABLE_TICKS
#define LLC_CR_RESP_STABLE_TICKS      (5U)
#endif

#ifndef LLC_CR_RESP_TIMEOUT_MS
#define LLC_CR_RESP_TIMEOUT_MS        (300U)
#endif

#ifndef LLC_CR_RESP_LOG_CACHE_MAX
#define LLC_CR_RESP_LOG_CACHE_MAX      (256U)
#endif

#ifndef CRITICAL_LOG_ONLY
#define CRITICAL_LOG_ONLY               (0U)  /* 1:仅关键日志, 0:详细日志 */
#endif

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/
typedef struct {
    float vbus_v;
    float vout_v;
    float iout_a;
		float llc_temp_c;
		bool llc_temp_valid;
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
	  uint32_t cycle_stop_begin_ms;      /* 周期性软关断开始时间 */
    bool     cycle_stop_enable;        /* 周期性软关断使能 */
    /* Burst Mode 变量 */
    burst_state_t burst_state;         /* Burst 子状态 */
    uint32_t burst_begin_ms;           /* Burst 阶段开始时间 */
    float vout_burst_target;           /* Burst 模式目标电压 */
    uint32_t burst_enter_delay_ms;     /* Burst进入延迟计时 */
    uint32_t burst_exit_delay_ms;      /* Burst退出延迟计时 */
    float f_pre_burst_hz;              /* 进入Burst前的频率，用于退出时bumpless */
    uint8_t burst_first_entry;         /* 首次进入Burst标志 */
}llc_runtime_ctx_t;
static llc_runtime_ctx_t s_llc_rt;

static llc_app_ctx_t s_llc_app;

typedef struct {
    float iref; 	//参考电流
		float iref_cmd;
    float imeas;  	//测量电流
	  float i_err_sat; //误差限幅
    float kp;   	//比例系数
    float ki;  	    //积分系数 
    float integ;    //积分值
    float f_min; 	//最小频率
    float f_max; 	//最大频率
		float df_max;   //频率增量上限
    float df_slew;  //频率 slew
	  float df_prev;  //上一拍 df
    float i_on;     //激活电流
    float i_off;    //退出电流
	  uint32_t derate_step_ms;
	  uint32_t derate_recover_ms;
    bool limit_active; //限流激活标志
} llc_curr_t;

static llc_curr_t s_llc_curr;
enum{
	LLC_START_DELAY_MS = 10000
};

typedef struct
{
    uint8_t  active;
    uint32_t seq;
    uint32_t start_ms;
    uint32_t settled_ms;
    float    iout_prev_a;
    float    iout_from_a;
    float    iout_to_a;
    float    vout_pre_v;
    float    vout_min_v;
    float    vout_max_v;
    float    max_abs_err_v;
    uint16_t stable_ticks;
    uint32_t next_period_log_ms;
} llc_cr_resp_ctx_t;

static llc_cr_resp_ctx_t s_cr_resp;

typedef struct
{
    uint32_t t_ms;
    uint8_t state;
    float f_cmd_hz;
    uint32_t f_act_hz;
    float vout_v;
} llc_collapse_snap_t;

typedef struct
{
    llc_collapse_snap_t buf[LLC_COLLAPSE_TRACE_PRE_MS + LLC_COLLAPSE_TRACE_POST_MS + 2U];
    uint16_t wr;
    uint16_t size;
    uint8_t active;
    uint8_t post_left;
    uint32_t last_trigger_ms;
	  uint8_t trigger_inhibit;
    float last_vout_v;
} llc_collapse_trace_ctx_t;

static llc_collapse_trace_ctx_t s_collapse_trace;

typedef struct
{
    uint8_t pending;
    uint8_t emit_idx;
    llc_collapse_snap_t pre;
    llc_collapse_snap_t trig;
    llc_collapse_snap_t post;
    uint8_t verdict;
    uint8_t reason_state;
    float dv_v;
    float df_cmd_hz;
    float df_act_hz;
} llc_collapse_emit_ctx_t;

static llc_collapse_emit_ctx_t s_collapse_emit;
static llc_cr_log_item_t s_cr_log_buf[LLC_CR_RESP_LOG_CACHE_MAX];
static uint16_t s_cr_log_w = 0U;
static uint16_t s_cr_log_r = 0U;
static uint16_t s_cr_log_cnt = 0U;
static uint8_t  s_cr_log_pending_dump = 0U;
static uint32_t s_cr_log_overflow_cnt = 0U;
static uint32_t s_cr_log_overflow_reported = 0U;
/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void llc_state_enter(llc_state_t next);
static void llc_update_measurements(void);
static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot);
static inline float conv_adc_to_i(uint16_t raw);
static void llc_set_freq(float hz, bool force_update);
static bool llc_precheck_ok(void);
static bool llc_pfc_ready_stable(bool enable_llc);
static bool llc_faults_present(void);
static void llc_enter_fault(void);
static float llc_ctrl_step(float e);
static void llc_fan_tick(void); //风扇的task

static void llc_current_thresholds_update(void);
static void llc_derate_reset(void);
static void llc_derate_update(float i_meas, bool enable);
static bool llc_temp_derate_active(void);
static bool llc_overtemp_shutdown(void);
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
    float v_net = v - 0.327f;
    return v_net / (ISHUNT_OHM * IAMP_GAIN);
}

static inline float llc_get_f_nom(float f_min, float f_max, float f_stage_cmd)
{
#if  (LLC_F_NOM_FOLLOW_MODE == LLC_F_NOM_FOLLOW_TARGET_FREQ)
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

/**
 * @brief 控制风扇开关
 * 
 * 该函数用于控制冷却风扇的开启和关闭，采用静态变量实现初始化和状态缓存，
 * 避免重复的GPIO配置和无效的GPIO操作。
 * 
 * @param on true表示开启风扇，false表示关闭风扇
 * 
 * @note 首次调用时会自动初始化GPIO外设时钟和引脚配置
 * @note 风扇默认状态为关闭
 * @note 使用状态缓存机制，只有状态变化时才执行GPIO操作
 * 
 * @static 该函数为静态函数，仅在当前文件内可见
 */
static void llc_fan_set(bool on)
{
    static bool initialized = false;
    static bool last = false;

    if (!initialized) {
        rcu_periph_clock_enable(RCU_GPIOC);
        gpio_init(FAN_CTL_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, FAN_CTL_PIN);
        gpio_bit_reset(FAN_CTL_PORT, FAN_CTL_PIN);   /* 默认关风扇 */
        initialized = true;
    }

    if (last == on) {
        return;
    }
    last = on;

    if (on) {
        gpio_bit_set(FAN_CTL_PORT, FAN_CTL_PIN);
    } else {
        gpio_bit_reset(FAN_CTL_PORT, FAN_CTL_PIN);
    }
}
/**
 * @brief LLC风扇控制周期任务
 * 
 * 根据LLC工作状态和输出电流自动控制风扇的启停，实现迟滞控制逻辑。
 * 
 * 控制策略：
 * - LLC非工作状态（软启动、运行、突发模式以外）：立即关闭风扇
 * - 输出电流 >= FAN_ON_IOUT_A：立即开启风扇
 * - 输出电流 <= FAN_OFF_IOUT_A：延时FAN_OFF_DELAY_MS后关闭风扇
 * - 电流介于两者之间：保持风扇开启状态，重置关闭延时
 * 
 * @note 该函数应在主循环中周期性调用
 * @note 使用静态变量fan_off_delay_ms记录风扇关闭延时时间戳
 * @note 迟滞控制避免风扇在阈值附近频繁启停
 */
static void llc_fan_tick(void)
{
    static uint32_t fan_off_delay_ms = 0U;
    float iout = s_llc_rt.meas.iout_a;
    bool llc_active;

    llc_active = (s_llc_rt.app.state == ST_SOFTSTART) ||
                 (s_llc_rt.app.state == ST_RUN_ENTRY_HOLD) ||
                 (s_llc_rt.app.state == ST_LLC_RUN) ||
                 (s_llc_rt.app.state == ST_BURST_MODE);

    if (!llc_active) {
        fan_off_delay_ms = 0U;
        llc_fan_set(false);
        return;
    }

    if (iout >= FAN_ON_IOUT_A) {
        fan_off_delay_ms = 0U;
        llc_fan_set(true);
        return;
    }

    if (iout <= FAN_OFF_IOUT_A) {
        if (fan_off_delay_ms == 0U) {
            fan_off_delay_ms = g_ms;
        } else if (elapsed_reached(fan_off_delay_ms, FAN_OFF_DELAY_MS)) {
            llc_fan_set(false);
        }
    } else {
        fan_off_delay_ms = 0U;
    }
}
/**
 * @brief 设置LLC开关频率
 * @param hz 目标频率
 * @param force_update 是否强制立即更新（见llc_pwm_set_freq说明）
 */
static void llc_set_freq(float hz, bool force_update)
{
    float f = f_clampf(hz, s_llc.f_min, s_llc.f_max);
    s_llc.f_cmd = f;
    llc_pwm_set_freq((uint32_t)f, force_update);
}

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
#if (LLC_F_NOM_FOLLOW_MODE == LLC_F_NOM_FOLLOW_TARGET_FREQ)
		f_nom = llc_get_f_nom(f_min, f_max, s_llc.f_cmd);
		s_llc.f_nom = f_nom;
#endif
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




/*********************************************************************************************************
* 函数名称：llc_current_thresholds_update
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年02月10日
* 注    意：
*********************************************************************************************************/
static void llc_current_thresholds_update(void)
{
    float i_on  = s_llc_curr.iref_cmd + LLC_IOUT_ON_DELTA_A;
    float i_off = s_llc_curr.iref_cmd - LLC_IOUT_OFF_DELTA_A;

    if (i_on  < 0.0f) i_on  = 0.0f;
    if (i_off < 0.0f) i_off = 0.0f;
 
    if (i_off >= i_on) {
        i_off = i_on - 0.1f;          // 给个最小滞回（0.1A 可按需调整）
        if (i_off < 0.0f) i_off = 0.0f;
    }
 
    s_llc_curr.i_on  = i_on;
    s_llc_curr.i_off = i_off;
}
/*********************************************************************************************************
* 函数名称：llc_derate_reset
* 函数功能：此函数是 LLC（谐振变换器）电流控制模块 的一部分，专用于管理 降额（Derating）状态 的复位操作
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年02月10日
* 注    意：“降额”指在检测到过流、过热等异常工况时，主动降低电流参考值以保护功率器件。该函数作为一个状态重置枢纽，确保系统能从降额模式安全、一致地恢复到正常工作点
*********************************************************************************************************/
static void llc_derate_reset(void)
{
    s_llc_curr.iref_cmd = s_llc_curr.iref;  // (1) 复位命令电流
    s_llc_curr.derate_step_ms = 0U;   // (2) 清除降额步进计时：记录上一次降额步进（减少电流）的时间戳
    s_llc_curr.derate_recover_ms = 0U; // (3) 清除降额恢复计时：记录上一次降额恢复（增加电流）的时间戳
    llc_current_thresholds_update();  // (4) 更新电流阈值
}

/*********************************************************************************************************
* 函数名称：llc_derate_update
* 函数功能：实现了 自适应降额（Adaptive Derating） 机制
* 输入参数：i_meas    enable
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年02月10日
* 注    意：当检测到持续过流或过热等异常工况时，系统不能立即关断（可能引起负载突变），而应逐步降低电流设定值，直至异常消除。该函数通过 迟滞比较 + 时间窗口 
的双重条件，实现了平滑、抗扰的降额控制。
*********************************************************************************************************/
static void llc_derate_update(float i_meas, bool enable)
{
	//提供外部开关，允许上层逻辑动态启用/禁用降额功能。
    if (!enable) {
			//一旦禁用，立即调用 llc_derate_reset() 确保所有状态变量归零，电流参考值恢复原始设定点。
        llc_derate_reset(); 
        return;
    }
  //惰性初始化（Lazy Initialization）：仅在第一次进入启用状态时，将计时器设置为当前系统时间 g_ms。
    if (s_llc_curr.derate_step_ms == 0U) {
        s_llc_curr.derate_step_ms = g_ms;
    }

    if (s_llc_curr.derate_recover_ms == 0U) {
        s_llc_curr.derate_recover_ms = g_ms;
    }
    bool updated = false;
    if ((i_meas >= s_llc_curr.i_on) && elapsed_reached(s_llc_curr.derate_step_ms, LLC_DERATE_STEP_PERIOD_MS)) {
        s_llc_curr.iref_cmd -= LLC_DERATE_STEP_A; // 过流：逐步降低电流
        s_llc_curr.derate_step_ms = g_ms;  // 重置步进计时器
        updated = true;
    }
    if ((i_meas <= s_llc_curr.i_off) && elapsed_reached(s_llc_curr.derate_recover_ms, LLC_DERATE_RECOVER_PERIOD_MS)) {
        s_llc_curr.iref_cmd += LLC_DERATE_RECOVER_STEP_A; // 恢复正常：逐步提高电流
        s_llc_curr.derate_recover_ms = g_ms;   // 重置恢复计时器
        updated = true;
    }
    if (updated) {
        s_llc_curr.iref_cmd = f_clampf(s_llc_curr.iref_cmd, LLC_DERATE_MIN_A, s_llc_curr.iref);
        llc_current_thresholds_update();
    }
}
/*********************************************************************************************************
* 函数名称：llc_temp_derate_active
* 函数功能：检测LLC温度降额条件是否激活
* 输入参数：无
* 输出参数：无
* 返 回 值：bool true-温度降额条件已激活，false-温度降额条件未激活
* 创建日期：2026年04月01日
* 注    意：当LLC温度测量有效且温度值达到或超过降额启动阈值（LLC_TEMP_DERATE_START_C）时，
*          返回true，表示系统需要进行温度降额控制。该函数用于实时监测温度状态，
*          为温度保护策略提供判断依据。
*********************************************************************************************************/
static bool llc_temp_derate_active(void)
{
    return s_llc_rt.meas.llc_temp_valid && (s_llc_rt.meas.llc_temp_c >= LLC_TEMP_DERATE_START_C);
}
static bool llc_overtemp_shutdown(void)
{
    return s_llc_rt.meas.llc_temp_valid && (s_llc_rt.meas.llc_temp_c >= LLC_TEMP_SHUTDOWN_C);
}
/*********************************************************************************************************
* 函数名称：llc_update_measurements
* 函数功能：实现了LLC变换器测量数据的更新与同步，包括温度、电压、电流等关键参数的采集和转换
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月01日
* 注    意：该函数通过温度控制模块获取LLC温度数据，并同步温度有效性标志；将ADC原始数据转换为实际物理量（输出电压、输出电流），同时获取PFC总线电压。所有测量数据统一存储在运行时数据结构中，供控制算法使用。
*********************************************************************************************************/
static void llc_update_measurements(void)
{
    //adc_multi_copy();
	  temp_sensor_data_t llc_temp = {0};  //(1)温度数据容器初始化
    temp_control_get_llc(&llc_temp);  //(2)获取温度传感器数据
    s_llc_rt.meas.llc_temp_valid = llc_temp.valid; //(3)同步温度有效性标志
    s_llc_rt.meas.llc_temp_c = llc_temp.temperature_c; //(4)同步温度值
		
		
    s_llc_rt.meas.vout_v = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    s_llc_rt.meas.iout_a = conv_adc_to_i(g_adc_multi.isense_raw);
    s_llc_rt.meas.vbus_v = pfc_bus_voltage();
    s_llc.vmeas = s_llc_rt.meas.vout_v;
}

static bool llc_precheck_ok(void)
{
    return (s_llc_rt.meas.vbus_v >= LLC_VBUS_MIN_START_V);
}
/*********************************************************************************************************
* 函数名称：llc_pfc_ready_stable
* 函数功能：检测PFC（功率因数校正）就绪状态是否稳定，通过时间窗口判断PFC是否已准备好启动LLC
* 输入参数：enable_llc  LLC使能标志，false时重置计时器并返回false
* 输出参数：void
* 返 回 值：bool  true表示PFC就绪状态已稳定，false表示未稳定或被禁用
* 创建日期：2026年04月01日
* 注    意：采用时间窗口机制，从首次使能LLC开始计时，经过PFC_READY_STABLE_BEFORE_LLC_MS时间
*          后才认为PFC就绪状态稳定，避免因瞬时波动导致误判。禁用时自动重置计时器。
*********************************************************************************************************/
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
/*********************************************************************************************************
* 函数名称：llc_faults_present
* 函数功能：检测LLC变换器是否存在故障状态
* 输入参数：void
* 输出参数：void
* 返 回 值：bool true-存在故障，false-无故障
* 创建日期：2026年04月01日
* 注    意：该函数综合检测以下故障条件：
*           1. 输出过压（VOUT_OVP_V）：输出电压超过58V保护阈值
*           2. 输出过流（IOUT_OCP_A）：输出电流超过41A保护阈值
*           3. 母线欠压（VBUS_MIN_START_V - VOUT_HYST_V）：母线电压低于启动最小值减去迟滞电压
*           4. 过温保护：通过llc_overtemp_shutdown()检测温度故障
*           该函数为LLC控制系统的核心故障检测入口，用于实时监控变换器运行状态，
*           一旦检测到任一故障条件立即返回true，触发相应的保护动作。
*********************************************************************************************************/
static bool llc_faults_present(void)
{
   // if (protect_fault_active_hw() || protect_fault_latched()) {
   //     return true;
   // }

   // if (pfc_is_fault()) {
   //     return true;
   // }

    if (s_llc_rt.meas.vout_v > LLC_VOUT_OVP_V) {  //58
        return true;
    }

    if (s_llc_rt.meas.iout_a > LLC_IOUT_OCP_A) { //41
        return true;
    }

    if (s_llc_rt.meas.vbus_v < (LLC_VBUS_MIN_START_V - LLC_VOUT_HYST_V)) {
        return true;
    }
		if (llc_overtemp_shutdown()) {
        return true;
    }

    return false;
}
/*********************************************************************************************************
* 函数名称：llc_enter_fault
* 函数功能：进入LLC故障状态，将状态机切换到故障状态
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月01日
* 注    意：该函数为静态函数，仅在模块内部使用。当检测到严重故障时调用，触发状态机进入故障处理流程
*********************************************************************************************************/
static void llc_enter_fault(void)
{
    llc_state_enter(ST_FAULT);
}

/**
 * @brief 将CR响应测试日志项推入环形缓冲区
 * 
 * 该函数将单个日志项写入CR测试日志缓存，采用环形缓冲区管理策略。
 * 当缓冲区未满时直接写入；当缓冲区已满时，覆盖最旧的日志项。
 * 
 * @param item 指向待写入的日志项的指针，若为NULL则直接返回
 * 
 * @note 仅在LLC_CR_RESP_LOG_ENABLE宏定义时生效
 * @note 写入操作会自动设置待转储标志，触发后续的日志转储流程
 * @note 缓冲区大小由LLC_CR_RESP_LOG_CACHE_MAX宏定义
 */
static void llc_cr_resp_log_push(const llc_cr_log_item_t *item)
{
#if LLC_CR_RESP_LOG_ENABLE
    uint16_t idx;
    uint32_t primask;
	
		//检查空指针，防止无效数据写入
    if (item == NULL) {
        return;
    }
	
    // 进入临界区 - 禁用中断
    primask = __get_PRIMASK();
    __disable_irq();
	
		//使用模运算实现环形缓冲区 LLC_CR_RESP_LOG_CACHE_MAX 定义为32，提供32个日志项的缓存空间
    idx = s_cr_log_w;
    s_cr_log_buf[idx] = *item;
    s_cr_log_w = (uint16_t)((s_cr_log_w + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);

    if (s_cr_log_cnt < LLC_CR_RESP_LOG_CACHE_MAX) {
        s_cr_log_cnt++; //当缓冲区未满时，只增加计数
    } else {
			  //当缓冲区已满时，移动读指针，实现先进先出的覆盖策略
        s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX); 
			  s_cr_log_overflow_cnt++;
    }
    //设置待处理标志，通知其他任务进行异步日志输出
    s_cr_log_pending_dump = 1U;
	
    // 退出临界区 - 恢复中断状态
    __set_PRIMASK(primask);
#endif
}
/**
 * @brief 转储CR响应日志缓冲区中的所有日志项
 * 
 * 该函数遍历环形缓冲区，将所有待处理的CR测试日志项通过调试接口输出。
 * 支持三种类型的日志格式：
 * - CR_LOG_MON: 周期性监控日志，包含时间戳、输出电压、输出电流、误差和频率
 * - CR_LOG_EVT_BEGIN: 测试开始事件，包含测试ID、负载电流变化和初始电压
 * - CR_LOG_EVT_END: 测试结束事件，包含测试结果、持续时间、稳定时间、电压范围和最大误差
 * 
 * @note 该函数仅在 LLC_CR_RESP_LOG_ENABLE 宏定义时编译
 * @note 函数执行后会清除 s_cr_log_pending_dump 标志
 * @note 调用该函数会清空缓冲区中的所有日志项
 */
static void llc_cr_resp_log_dump_test(void)
{
#if LLC_CR_RESP_LOG_ENABLE
  
    while (s_cr_log_cnt > 0U) {
        llc_cr_log_item_t item = s_cr_log_buf[s_cr_log_r];
        s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
        s_cr_log_cnt--;
				//监控数据输出 输出电压、输出电流、误差、频率
        if (item.type == CR_LOG_MON) {
            debug_printf("[CR_MON] t=%lu vout=%.1fV iout=%.1fA err=%.1fV f=%.0fHz\n",
                         (unsigned long)item.t_ms,
                         item.vout_v,
                         item.iout_a,
                         item.err_v,
                         item.f_cmd_hz);
        }
				//测试开始事件 记录测试开始前的输出电压,还有电流的变化范围
				else if (item.type == CR_LOG_EVT_BEGIN) {
            debug_printf("[CR_EVT_BEGIN] id=%lu iout=%.1f->%.1fA vout=%.1fV\n",
                         (unsigned long)item.id,
                         item.iout_from_a,
                         item.iout_to_a,
                         item.vout_pre_v);
        } 
				//测试结束事件  pass=%u 显示测试是否通过 响应时间、稳定时间 电压波动范围、最大误差
				else 
				{
            debug_printf("[CR_EVT_END] id=%lu pass=%u dt=%lums settle=%lums "
                         "vmin=%.1fV vmax=%.1fV maxerr=%.1fV\n",
                         (unsigned long)item.id,
                         (unsigned)item.pass,
                         (unsigned long)item.dt_ms,
                         (unsigned long)item.settle_ms,
                         item.vmin_v,
                         item.vmax_v,
                         item.maxerr_v);
        }
    }
    s_cr_log_pending_dump = 0U;
    
#endif
}
/*********************************************************************************************************
* 函数名称：llc_cr_log_frame_bytes
* 函数功能：计算LLC控制日志数据帧的字节长度
* 输入参数：item  指向LLC控制日志项的指针
* 输出参数：void
* 返 回 值：uint16_t  返回日志数据帧的总字节数，若输入无效或类型未知则返回0
* 创建日期：2026年04月01日
* 注    意：根据日志项类型（监控、事件开始、事件结束）确定payload长度，并计算完整帧长度。
*          帧结构 = 帧头(0xA5) + 类型 + 长度 + 序号 + payload + 校验和 + 帧尾(0x5A) = 6 + payload
*********************************************************************************************************/
static uint16_t llc_cr_log_frame_bytes(const llc_cr_log_item_t *item)
{
    uint8_t payload_len = 0U;

    if (item == NULL) {
        return 0U;
    }

    switch (item->type) {
    case CR_LOG_MON:
        payload_len = 10U;
        break;

    case CR_LOG_EVT_BEGIN:
        payload_len = 8U;
        break;

    case CR_LOG_EVT_END:
        payload_len = 14U;
        break;

    default:
        return 0U;
    }

    /* frame = A5 + type + len + seq + payload + chk + 5A = 6 + payload */
    return (uint16_t)(6U + payload_len);
}

/*********************************************************************************************************
* 函数名称：llc_cr_resp_log_dump
* 函数功能：导出并打印LLC电流环响应日志缓存中的记录
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月01日
* 注    意：通过中断保护机制安全访问环形缓冲区，依次读取并格式化输出监控数据、事件开始和事件结束三类日志记录，
	直至缓冲区清空。输出格式包含时间戳、电压、电流、误差、频率等关键参数，用于调试分析和性能评估。
*********************************************************************************************************/
static void llc_cr_resp_log_dump(void)
{
#if LLC_CR_RESP_LOG_ENABLE
    while (1) {
        llc_cr_log_item_t item;
        uint32_t primask;

        primask = __get_PRIMASK();
        __disable_irq();

        if (s_cr_log_cnt == 0U) {
            s_cr_log_pending_dump = 0U;
            __set_PRIMASK(primask);
            break;
        }

        item = s_cr_log_buf[s_cr_log_r];
        s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
        s_cr_log_cnt--;
        s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;

        __set_PRIMASK(primask);

        switch (item.type) {
        case CR_LOG_MON:
            debug_printf("[M] %lu %.1f %.1f %.1f %.0f\r\n",
                         (unsigned long)item.t_ms,
                         item.vout_v,
                         item.iout_a,
                         item.err_v,
                         item.f_cmd_hz);
            break;

        case CR_LOG_EVT_BEGIN:
            debug_printf("[B] %lu %.1f %.1f %.1f\r\n",
                         (unsigned long)item.id,
                         item.iout_from_a,
                         item.iout_to_a,
                         item.vout_pre_v);
            break;

        case CR_LOG_EVT_END:
            debug_printf("[E] %lu %u %lu %lu %.1f %.1f %.1f\r\n",
                         (unsigned long)item.id,
                         (unsigned)item.pass,
                         (unsigned long)item.dt_ms,
                         (unsigned long)item.settle_ms,
                         item.vmin_v,
                         item.vmax_v,
                         item.maxerr_v);
            break;

        default:
            break;
        }
    }
#endif
}

/**
 * @brief LLC电流响应(CR)测试周期处理函数
 * 
 * 该函数在系统运行时定期调用，负责：
 * 1. 周期性记录系统运行状态（输出电压、电流、误差、频率）
 * 2. 检测负载阶跃事件（输出电流变化超过阈值）
 * 3. 执行CR动态响应测试，记录瞬态性能指标
 * 4. 判断系统是否达到稳定状态，计算稳定时间
 * 
 * 测试触发条件：输出电流变化 ≥ LLC_CR_RESP_STEP_IOUT_A
 * 稳定判断条件：误差在 LLC_CR_RESP_STABLE_WIN_V 范围内持续 LLC_CR_RESP_STABLE_TICKS 个周期
 * 超时保护：测试时长超过 LLC_CR_RESP_TIMEOUT_MS 自动结束
 * 
 * @note 仅在 LLC_CR_RESP_LOG_ENABLE 宏定义时编译
 * @note 非ST_LLC_RUN状态下不执行测试，仅更新状态变量
 * @note 所有测试数据通过 llc_cr_resp_log_push() 异步推送到日志缓冲区
 */
static void llc_cr_resp_tick(void)
{
#if LLC_CR_RESP_LOG_ENABLE
    float iout = s_llc_rt.meas.iout_a;
    float vout = s_llc_rt.meas.vout_v;
    float err = s_llc.vref - vout;
	/* 本次电流与上一次记录电流的差值绝对值，用于判断是否发生负载阶跃 */
    float i_step = fabsf(iout - s_cr_resp.iout_prev_a);
  /* 只有 LLC 处于正常运行态才做动态响应监测 */
    if (s_llc_rt.app.state != ST_LLC_RUN) {
			 /* 不在 RUN 状态时，更新上次电流值，避免重新进入 RUN 时误触发阶跃 */
        s_cr_resp.iout_prev_a = iout;
			 /* 重置下一次周期监控日志的时间点 */
        s_cr_resp.next_period_log_ms = g_ms + LLC_CR_RESP_LOG_PERIOD_MS;
		  	/* 退出当前动态响应事件 */
			  s_cr_resp.active = 0U;
			  /* 清零稳定计数 */
        s_cr_resp.stable_ticks = 0U;
			  /* 清零稳定时间 */
        s_cr_resp.settled_ms = 0U;
        return;
    }
#if !CRITICAL_LOG_ONLY
		 /* 到达周期监控日志时间点时，输出一条 MON 监控日志 */
    if (g_ms >= s_cr_resp.next_period_log_ms) {
        llc_cr_log_item_t item;
			/* 日志类型：周期监控 */
        item.type = CR_LOG_MON;
			/* 当前时间戳 */
        item.t_ms = g_ms;
			/* 当前输出电压 */
        item.vout_v = vout;
			 /* 当前输出电流 */
        item.iout_a = iout;
			  /* 当前电压误差 */
        item.err_v = err;
			 /* 当前 LLC 频率指令 */
        item.f_cmd_hz = s_llc.f_cmd;
			 /* 压入 CR 日志缓冲区，后续由 1kHz 慢任务统一导出 */
        llc_cr_resp_log_push(&item);
			 /* 更新下一次周期监控日志时间 */
        s_cr_resp.next_period_log_ms = g_ms + LLC_CR_RESP_LOG_PERIOD_MS;
    }
#endif
     /* 如果当前没有响应事件在进行，并且检测到电流跳变量超过阈值，则认为发生了一次负载阶跃 */
		if (!s_cr_resp.active && (i_step >= LLC_CR_RESP_STEP_IOUT_A)) {
			/* 标记动态响应事件开始 */
        s_cr_resp.active = 1U;
			 /* 事件序号自增，用于区分每一次响应事件 */
        s_cr_resp.seq++;
			 /* 记录事件开始时间 */
        s_cr_resp.start_ms = g_ms;
			 /* 清零稳定时间，后续满足稳定条件时再赋值 */
        s_cr_resp.settled_ms = 0U;
			 /* 记录阶跃前电流值 */
        s_cr_resp.iout_from_a = s_cr_resp.iout_prev_a;
			/* 记录阶跃后当前电流值 */
        s_cr_resp.iout_to_a = iout;
			/* 记录事件开始瞬间的输出电压，作为阶跃前参考电压 */
        s_cr_resp.vout_pre_v = vout;
			 /* 初始化事件期间最小输出电压 */
        s_cr_resp.vout_min_v = vout;
			 /* 初始化事件期间最大输出电压 */
        s_cr_resp.vout_max_v = vout;
			 /* 初始化事件期间最大绝对误差 */
        s_cr_resp.max_abs_err_v = fabsf(err);
			  /* 清零稳定计数 */
        s_cr_resp.stable_ticks = 0U;
        {
					 
            llc_cr_log_item_t item;
					 /* 日志类型：事件开始 */
            item.type = CR_LOG_EVT_BEGIN;
					/* 当前事件 ID */
            item.id = s_cr_resp.seq;
					 /* 阶跃前电流 */
            item.iout_from_a = s_cr_resp.iout_from_a;
					/* 阶跃后电流 */
            item.iout_to_a = s_cr_resp.iout_to_a;
					 /* 阶跃前输出电压 */
            item.vout_pre_v = s_cr_resp.vout_pre_v;
					 /* 压入事件开始日志 */
            llc_cr_resp_log_push(&item);
        }
    }
/* 如果当前有动态响应事件正在进行，则持续跟踪其响应过程 */
    if (s_cr_resp.active) {
			/* 更新事件期间的最小输出电压 */
        if (vout < s_cr_resp.vout_min_v) {
            s_cr_resp.vout_min_v = vout;
        }
				 /* 更新事件期间的最大输出电压 */
        if (vout > s_cr_resp.vout_max_v) {
            s_cr_resp.vout_max_v = vout;
        }
				/* 更新事件期间的最大绝对误差 */
        if (fabsf(err) > s_cr_resp.max_abs_err_v) {
            s_cr_resp.max_abs_err_v = fabsf(err);
        }
				/* 如果当前误差已经进入稳定窗口，则累加稳定计数 */
        if (fabsf(err) <= LLC_CR_RESP_STABLE_WIN_V) {
					 /* 稳定计数不超过设定上限 */
            if (s_cr_resp.stable_ticks < LLC_CR_RESP_STABLE_TICKS) {
                s_cr_resp.stable_ticks++;
            }
						 /* 当稳定计数首次达到要求时，记录稳定时间 */
            if ((s_cr_resp.stable_ticks >= LLC_CR_RESP_STABLE_TICKS) && (s_cr_resp.settled_ms == 0U)) {
                s_cr_resp.settled_ms = g_ms - s_cr_resp.start_ms;
            }
        } else {
					 /* 一旦误差超出稳定窗口，稳定计数清零，重新开始累计 */
            s_cr_resp.stable_ticks = 0U;
        }
				 /* 如果已经稳定，或者已经超时，则结束本次事件 */
        if ((s_cr_resp.settled_ms > 0U) ||
            elapsed_reached(s_cr_resp.start_ms, LLC_CR_RESP_TIMEOUT_MS)) {
						/* 当前事件总耗时 */
            uint32_t event_ms = g_ms - s_cr_resp.start_ms;
						/* 是否判定通过：稳定时间有效则 pass=1，否则 pass=0 */
            uint8_t pass = (s_cr_resp.settled_ms > 0U) ? 1U : 0U;
            {
                llc_cr_log_item_t item;
							/* 日志类型：事件结束 */
                item.type = CR_LOG_EVT_END;
							 /* 当前事件 ID */
                item.id = s_cr_resp.seq;
							 /* 是否通过 */
                item.pass = pass;
							/* 事件总持续时间 */
                item.dt_ms = event_ms;
							  /* 实际稳定时间；若未稳定则一般为 0 */
                item.settle_ms = s_cr_resp.settled_ms;
							 /* 事件期间的最小输出电压 */
                item.vmin_v = s_cr_resp.vout_min_v;
							/* 事件期间的最大输出电压 */
                item.vmax_v = s_cr_resp.vout_max_v;
							 /* 事件期间最大绝对误差 */
                item.maxerr_v = s_cr_resp.max_abs_err_v;
							  /* 压入事件结束日志 */
                llc_cr_resp_log_push(&item);
            }
						 /* 清除活动标志，表示本次动态响应事件结束 */
            s_cr_resp.active = 0U;
						 /* 清零稳定计数 */
            s_cr_resp.stable_ticks = 0U;
						/* 清零稳定时间 */
            s_cr_resp.settled_ms = 0U;
        }
    }

    s_cr_resp.iout_prev_a = iout;
#endif
}
/**
 * @file llc_cr_resp_tick_test
 * @brief LLC CR（电流阶跃）动态响应监控处理函数
 * 
 * 本函数在LLC控制周期中被调用，用于实时监测和记录负载电流阶跃变化时的
 * 输出电压动态响应特性。通过检测输出电流的阶跃变化，自动触发响应测试，
 * 并记录关键性能指标包括响应时间、超调量、稳定时间等。
 * 
 * @note 仅当LLC_CR_RESP_LOG_ENABLE宏定义时生效
 * 
 * 功能说明：
 * - 周期性记录运行状态（电压、电流、误差、频率）
 * - 检测输出电流阶跃变化（阈值：LLC_CR_RESP_STEP_IOUT_A）
 * - 自动触发响应测试并记录初始状态
 * - 实时跟踪输出电压极值（最小值、最大值）
 * - 监测电压误差并判断是否进入稳定窗口
 * - 计算稳定时间（误差在稳定窗口内持续LLC_CR_RESP_STABLE_TICKS个周期）
 * - 测试超时保护（LLC_CR_RESP_TIMEOUT_MS）
 * - 输出测试结果报告（通过/失败、响应时间、稳定时间、电压极值、最大误差）
 * 
 * 测试触发条件：
 * - 当前未处于测试状态（s_cr_resp.active == 0）
 * - 输出电流变化幅度达到或超过设定阈值（|iout - iout_prev| >= LLC_CR_RESP_STEP_IOUT_A）
 * - LLC处于运行状态（s_llc_rt.app.state == ST_LLC_RUN）
 * 
 * 测试完成条件：
 * - 误差进入稳定窗口并持续LLC_CR_RESP_STABLE_TICKS个周期（测试通过）
 * - 或达到超时时间LLC_CR_RESP_TIMEOUT_MS（测试失败）
 * 
 * @param void 无参数
 * @return void 无返回值
 * 
 * @see LLC_CR_RESP_LOG_ENABLE - CR响应日志使能宏
 * @see LLC_CR_RESP_LOG_PERIOD_MS - 周期性日志记录间隔（毫秒）
 * @see LLC_CR_RESP_STEP_IOUT_A - 触发CR测试的电流阶跃阈值（安培）
 * @see LLC_CR_RESP_STABLE_WIN_V - 稳定判断的误差窗口（伏特）
 * @see LLC_CR_RESP_STABLE_TICKS - 稳定判断需要的持续周期数
 * @see LLC_CR_RESP_TIMEOUT_MS - 测试超时时间（毫秒）
 * 
 * @note 本函数为内部静态函数，仅在llc_control.c文件内可见
 * @note 使用全局变量g_ms获取系统时间
 * @note 使用全局结构体s_llc_rt、s_llc、s_cr_resp存储状态和参数
 */
static void llc_cr_resp_tick_test(void)
{
#if LLC_CR_RESP_LOG_ENABLE
    float iout = s_llc_rt.meas.iout_a;
    float vout = s_llc_rt.meas.vout_v;
    float err = s_llc.vref - vout;
    float i_step = fabsf(iout - s_cr_resp.iout_prev_a);
    //确保只有在LLC正常运行时才记录CR响应数据
    if (s_llc_rt.app.state != ST_LLC_RUN) {
        s_cr_resp.iout_prev_a = iout; //将当前输出电流值保存为"前一次"电流值",为下一次CR响应计算提供基准点
        s_cr_resp.next_period_log_ms = g_ms + LLC_CR_RESP_LOG_PERIOD_MS; //设置下一次CR响应数据记录的时间点
        return;
    }
    //基于当前时间触发，而非计数器
    if (g_ms >= s_cr_resp.next_period_log_ms) {
        debug_printf("[CR_MON] t=%lu vout=%.2fV iout=%.2fA err=%.2fV f=%luHz\n",
                     (unsigned long)g_ms,
                     vout,
                     iout,
                     err,
                     (unsigned long)s_llc.f_cmd);
				//更新下次进入时间
        s_cr_resp.next_period_log_ms = g_ms + LLC_CR_RESP_LOG_PERIOD_MS;
    }
    //智能事件检测器，当检测到输出电流发生显著变化时，自动启动CR动态响应测试，并记录完整的测试上下文信息。
    if (!s_cr_resp.active && (i_step >= LLC_CR_RESP_STEP_IOUT_A)) {
        s_cr_resp.active = 1U;   // 激活测试标志
        s_cr_resp.seq++; // 测试序列号递增
        s_cr_resp.start_ms = g_ms;  // 记录测试开始时间
        s_cr_resp.settled_ms = 0U;  // 清零稳定时间计数器
			// 电流变化轨迹
        s_cr_resp.iout_from_a = s_cr_resp.iout_prev_a;  // 变化前电流
        s_cr_resp.iout_to_a = iout;  // 变化后的电流
			// 电压基准和极值跟踪
        s_cr_resp.vout_pre_v = vout; // 变化前电压
        s_cr_resp.vout_min_v = vout; // 初始化最小电压
        s_cr_resp.vout_max_v = vout; // 初始化最大电压
			// 控制性能指标
        s_cr_resp.max_abs_err_v = fabsf(err); // 最大误差绝对值
        s_cr_resp.stable_ticks = 0U;// 稳定计数器清零
        debug_printf("[CR_EVT_BEGIN] id=%lu iout=%.2f->%.2fA vout=%.2fV\n",
                     (unsigned long)s_cr_resp.seq,
                     s_cr_resp.iout_from_a,
                     s_cr_resp.iout_to_a,
                     s_cr_resp.vout_pre_v);
    }
		//确保只在活跃测试期间执行监控逻辑，避免不必要的计算开销
    if (s_cr_resp.active) {
        if (vout < s_cr_resp.vout_min_v) {
            s_cr_resp.vout_min_v = vout; // 更新最小电压
        }
        if (vout > s_cr_resp.vout_max_v) {
            s_cr_resp.vout_max_v = vout; // 更新最大电压
        }
				//控制性能指标：反映PI控制器在瞬态过程中的最大偏差。
        if (fabsf(err) > s_cr_resp.max_abs_err_v) {
            s_cr_resp.max_abs_err_v = fabsf(err); // 更新最大绝对误差
        }

        if (fabsf(err) <= LLC_CR_RESP_STABLE_WIN_V) {  // 误差在稳定窗口内
            if (s_cr_resp.stable_ticks < LLC_CR_RESP_STABLE_TICKS) {
                s_cr_resp.stable_ticks++; // 稳定计数器递增
            }
            if ((s_cr_resp.stable_ticks >= LLC_CR_RESP_STABLE_TICKS) && (s_cr_resp.settled_ms == 0U)) {
                s_cr_resp.settled_ms = g_ms - s_cr_resp.start_ms;  // 记录稳定时间
            }
        } else {
            s_cr_resp.stable_ticks = 0U; // 误差超出窗口，重置计数器
        }

        if ((s_cr_resp.settled_ms > 0U) || // 条件1：已达到稳定
            elapsed_reached(s_cr_resp.start_ms, LLC_CR_RESP_TIMEOUT_MS)) {  // 条件2：超时
            uint32_t event_ms = g_ms - s_cr_resp.start_ms;
            uint8_t pass = (s_cr_resp.settled_ms > 0U) ? 1U : 0U;
            debug_printf("[CR_EVT_END] id=%lu pass=%u dt=%lums settle=%lums "
                         "vmin=%.2fV vmax=%.2fV maxerr=%.2fV\n",
                         (unsigned long)s_cr_resp.seq,
                         (unsigned)pass,  //pass：测试是否通过（1=通过，0=超时）
                         (unsigned long)event_ms,
                         (unsigned long)s_cr_resp.settled_ms, //稳定时间（核心指标）
                         s_cr_resp.vout_min_v,
                         s_cr_resp.vout_max_v,
                         s_cr_resp.max_abs_err_v); //最大控制误差
            s_cr_resp.active = 0U;
            s_cr_resp.stable_ticks = 0U;
            s_cr_resp.settled_ms = 0U;
        }
    }

    s_cr_resp.iout_prev_a = iout; // 为下一次阶跃检测准备
#endif
}




/*********************************************************************************************************
*                                              状态机核心
*********************************************************************************************************/
//频率版的在切入闭环瞬间做"无扰切换（bumpless transfer）"

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
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
	 case ST_IDLE:
		  llc_softstart_on_fault();
		  llc_softstop_reset();  /* 重置软关断状态 */
		  //pfc_hw_set_main(false);   // 强制关闭 PFC
			//pfc_disable();
		  llc_pwm_outputs_enable(0); // 禁用 PWM 输出
		  llc_driver_en_set(false); //disable llc
		  llc_set_freq(s_llc.f_max, true);  /* 强制更新：状态切换确保立即生效 */		
	 	  s_llc_rt.softstart_begin_ms = 0U;  //软启动开始时间戳，用于控制软启动斜坡
	 	  s_llc_rt.stopping_begin_ms = 0U;  //停机过程开始时间戳，用于控制停机时序
	 	  s_llc_rt.hold_last_adjust_ms = 0U; //保持最后调整的时间戳，用于频率调整的去抖
		  s_llc_rt.pfc_ready_begin_ms = 0U;
		  s_llc_rt.run_entry_hold_begin_ms = 0U;
		  s_llc_rt.run_entry_stable_ticks = 0U;
		  s_llc_rt.cycle_stop_begin_ms = 0U;  /* 重置周期性软关断计时 */
		  s_llc_rt.burst_state = BURST_STATE_OFF;
		  s_llc_rt.burst_begin_ms = 0U;
		  s_llc_rt.burst_first_entry = 1;     /* 重置首次进入标志 */
		  break;
	 case ST_PRECHECK:  
	  	llc_driver_en_set(true);
    	llc_pwm_outputs_enable(0); // 禁用 PWM 输出
    	llc_set_freq(s_llc.f_max, true);  /* 强制更新 */
	 
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
			llc_set_freq(s_llc.f_cmd, true);  /* 强制更新：从软启过渡到闭环 */

			break;
	 case ST_LLC_RUN:
			llc_driver_en_set(true);
      s_llc_rt.hold_last_adjust_ms = g_ms;
	 #if (LLC_F_NOM_FOLLOW_MODE == LLC_F_NOM_FOLLOW_SOFTSTART_END)
	    s_llc.f_nom = f_clampf(s_llc.f_cmd, s_llc.f_min, s_llc.f_max);
	 #else
	 	    s_llc.f_nom = llc_get_f_nom(s_llc.f_min, s_llc.f_max, s_llc.f_cmd);
#endif
      /* 重置Burst延迟计时器 */
      s_llc_rt.burst_enter_delay_ms = 0U;
      s_llc_rt.burst_exit_delay_ms = 0U;

      llc_ctrl_bumpless_init(LLC_VOUT_TARGET_V, s_llc_rt.meas.vout_v, s_llc.f_cmd);
			break;
	 case ST_STOPPING:
      s_llc_rt.stopping_begin_ms = g_ms;
		  llc_driver_en_set(true);
      /* 启动软关断：从当前频率平滑上升到最高频率 */
      llc_softstop_start(s_llc.f_cmd);
      break;
	 case ST_CYCLE_STOPPING:
      s_llc_rt.stopping_begin_ms = g_ms;
      llc_driver_en_set(true);
      /* 启动软关断 */
      llc_softstop_start(s_llc.f_cmd);
      break;
	 case ST_BURST_MODE:
			llc_driver_en_set(true);
			s_llc_rt.vout_burst_target = LLC_VOUT_TARGET_V;
			/* 记录进入Burst前的频率，用于退出时bumpless */
			s_llc_rt.f_pre_burst_hz = f_clampf(s_llc.f_cmd, s_llc.f_min, s_llc.f_max);
			
			/* 每次进入Burst都走ENTRY_PREPARE，确保软进入 */
			s_llc_rt.burst_state = BURST_STATE_ENTRY_PREPARE;
			s_llc_rt.burst_first_entry = 0;
			llc_pwm_outputs_enable(1);  /* 进入准备阶段保持PWM开启 */
			llc_set_freq(LLC_F_MAX_HZ, true); /* 强制更新：进入Burst升频准备 */
			s_llc_rt.burst_begin_ms = g_ms;
			
#if DEBUG_PRINTF_BURST_MODE
			debug_printf("[BURST] ENTER: iout=%.2fA, f_pre=%.0fHz, state=%d\n", 
			             s_llc_rt.meas.iout_a, s_llc_rt.f_pre_burst_hz, s_llc_rt.burst_state);
#endif
			break;
	 case ST_FAULT:
	 		llc_softstart_on_fault();
			llc_softstop_reset();  /* 故障时重置软关断状态 */
			llc_pwm_outputs_enable(0);
			llc_driver_en_set(false);
			llc_set_freq(s_llc.f_max, true);  /* 强制更新：故障保护立即生效 */

			break;
	 
	 default:
		  llc_softstart_on_fault();
      pfc_hw_set_main(false); //PFC_off
      llc_pwm_outputs_enable(0); //llc pwm disable
      llc_driver_en_set(false);
      llc_set_freq(s_llc.f_max, true);  /* 强制更新 */
      break;
	}
}


/*********************************************************************************************************
*                                              公共接口
*********************************************************************************************************/
void llc_app_init(void)
{
    llc_softstart_init();
    s_llc = (llc_t){
        .vref    = LLC_VOUT_TARGET_V,
        .vmeas   = 0.0f,

        /* 正常电压PI参数 */
        .kp      = 1900.0f,
        .ki      = 18.0f,

        .integ   = 0.0f,
        .iref    = 0.0f,
        .imeas   = 0.0f,

        .f_min   = LLC_F_MIN_HZ,
        .f_max   = LLC_F_MAX_HZ,
        .f_cmd   = LLC_F_INIT_HZ,
        .f_slew  = LLC_F_SLEW_HZ,
        .f_nom  = LLC_F_INIT_HZ,
        .f_cmd_v = 0.0f,
        .f_cmd_i = 0.0f
    };

    /* 初始化周期性软关断 */
    s_llc_rt.cycle_stop_enable = false;  /* 禁用周期性软关，使用Burst */
    s_llc_rt.cycle_stop_begin_ms = 0U;
    s_llc_rt.burst_state = BURST_STATE_OFF;
    s_llc_rt.burst_begin_ms = 0U;
    s_llc_rt.vout_burst_target = LLC_VOUT_TARGET_V;
    s_llc_rt.burst_enter_delay_ms = 0U;
    s_llc_rt.burst_exit_delay_ms = 0U;
    s_llc_rt.f_pre_burst_hz = LLC_F_NOM_HZ;  /* 默认用标称频率 */
    s_llc_rt.burst_first_entry = 1;          /* 标记首次进入 */
#if LLC_COLLAPSE_TRACE_ENABLE
    s_collapse_trace.wr = 0U;
    s_collapse_trace.size = 0U;
    s_collapse_trace.active = 0U;
    s_collapse_trace.post_left = 0U;
		s_collapse_trace.trigger_inhibit = 0U;
    s_collapse_trace.last_trigger_ms = 0U;
    s_collapse_trace.last_vout_v = 0.0f;
		
		s_collapse_emit.pending = 0U;
    s_collapse_emit.emit_idx = 0U;
#endif
    llc_state_enter(ST_IDLE);
}


static uint8_t vloop_div = 0;
static uint8_t vout_filt_inited = 0U;
#if 1
/*********************************************************************************************************
* 函数名称：llc_app_tick_100us
* 函数功能：实现了 LLC 变换器的 100μs 周期性控制任务，包括输出电压滤波、电压闭环控制及 Burst 模式管理
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月01日
* 注    意：该函数在 100μs 定时器中断中调用，实现以下功能：
*           1. 状态判断：仅在 ST_LLC_RUN 或 ST_BURST_MODE 状态下执行控制逻辑
*           2. 电压滤波：采用一阶低通滤波器对输出电压 ADC 原始值进行滤波，滤波系数为 LLC_VOUT_FILT_ALPHA
*           3. Burst 模式：在 Burst 模式下仅保留电压滤波，不执行电压 PI 控制
*           4. 电压闭环：每 5 个 tick（500μs）执行一次电压 PI 控制，计算频率指令并更新开关频率
*           5. 频率更新：通过 llc_set_freq 函数自然更新频率，实现闭环微调
*           设计要点：电压滤波采用静态变量 vout_filt_v 保存滤波状态，确保滤波连续性；
*                     vout_filt_inited 标志用于滤波器初始化，避免启动瞬态；
*                     vloop_div 计数器实现 500μs 的慢环控制周期，降低计算负载
*********************************************************************************************************/
void llc_app_tick_100us(void)
{
	  static float vout_filt_v   = 0.0f;   /* 电压PI用滤波状态 */
    float err;
    float f_cmd;
	
	
    if (s_llc_rt.app.state != ST_LLC_RUN && s_llc_rt.app.state != ST_BURST_MODE) {
			vloop_div = 0;
			vout_filt_inited = 0U;
			llc_cr_resp_tick();
      return;
    }	

		 /* =========================
     * 正常电压PI模式
     * raw -> 电压 -> 滤波 -> PI
     * err = vref - vmeas
     * ========================= */
    float vout_now = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
		
		if (!vout_filt_inited) {
				vout_filt_v = vout_now;
				vout_filt_inited = 1U;
		}
	  if (vout_filt_v < 0.001f) {
					vout_filt_v = vout_now;
			}
    /* 100us快环滤波，alpha可后续再调 */
    vout_filt_v += LLC_VOUT_FILT_ALPHA * (vout_now - vout_filt_v);
    s_llc.vmeas = vout_filt_v;
		/* Burst模式：只保留滤波，不跑PI */
    if (s_llc_rt.app.state == ST_BURST_MODE) {
        vloop_div = 0U;
			  llc_cr_resp_tick();
        return;
    }
#if LLC_FIXED_FREQ_LOAD_TEST_ENABLE
    vloop_div = 0U;
    llc_set_freq(LLC_FIXED_FREQ_LOAD_TEST_HZ, false);
    llc_cr_resp_tick();
    return;
#endif
    err = s_llc.vref - s_llc.vmeas;
		if (++vloop_div >= 5U) {   // 500us
			vloop_div = 0;
			f_cmd = llc_ctrl_step(err);
			llc_set_freq(f_cmd, false);  /* 自然更新：闭环微调 */
			llc_cr_resp_tick();
			}
		}
		
#endif 
static bool llc_is_active_state(llc_state_t st)
{
    return (st == ST_PRECHECK) ||
           (st == ST_SOFTSTART) ||
           (st == ST_RUN_ENTRY_HOLD) ||
           (st == ST_LLC_RUN) ||
           (st == ST_BURST_MODE) ||
           (st == ST_CYCLE_STOPPING) ||
           (st == ST_STOPPING);
}


static bool power_stage_all_idle(void)
{
    /* PFC 还ready，说明前级仍处于工作/可工作态，不允许刷日志 */
    if (pfc_is_ready()) {
        return false;
    }

    /* LLC 仍在活动态，也不允许刷日志 */
    if (llc_is_active_state(s_llc_rt.app.state)) {
        return false;
    }
		return true;
}
/**
 * @brief 限制性地导出LLC电流响应日志数据
 * 
 * 该函数从环形日志缓冲区中读取指定数量的日志项，并通过二进制协议发送。
 * 支持溢出状态报告和监控数据过滤功能。
 * 
 * @param max_items 本次导出的最大日志项数量，为0时直接返回
 * @param allow_mon 是否允许导出监控类型日志(CR_LOG_MON)，0表示跳过监控数据
 * 
 * @note 该函数使用临界区保护环形缓冲区的读取操作
 * @note 溢出状态变化时会自动报告溢出统计信息
 * @note 日志导出后自动更新待导出标志(s_cr_log_pending_dump)
 * 
 * @attention 仅在LLC_CR_RESP_LOG_ENABLE宏定义时有效
 */
static void llc_cr_resp_log_dump_limited_PRO(uint8_t max_items, uint8_t allow_mon)
{
#if LLC_CR_RESP_LOG_ENABLE
    uint8_t dumped = 0U;

    if (max_items == 0U) {
        return;
    }
		//优雅的溢出报告机制,只有溢出的时候才会打印
    if (s_cr_log_overflow_reported != s_cr_log_overflow_cnt) {
        debug_printf("[CR_OVF] total=%lu pending=%u\r\n",
                     (unsigned long)s_cr_log_overflow_cnt,
                     (unsigned)s_cr_log_cnt);
        s_cr_log_overflow_reported = s_cr_log_overflow_cnt;
    }
    while (dumped < max_items) {
        llc_cr_log_item_t item;
        uint32_t primask;

        primask = __get_PRIMASK();
        __disable_irq();

        if (s_cr_log_cnt == 0U) {
            s_cr_log_pending_dump = 0U;
            __set_PRIMASK(primask);
            break;
        }
				//环形缓冲区策略管理
        item = s_cr_log_buf[s_cr_log_r];
        s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
        s_cr_log_cnt--;
        s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;

        __set_PRIMASK(primask);

        /* 改成文本串口帧输出 */
        if ((item.type != CR_LOG_MON) || allow_mon) {
					#if 1
            llc_cr_proto_log_emit_bin(&item);
					#else
					            switch (item.type) {
            case CR_LOG_MON:
                debug_printf("[M] %lu %.1f %.1f %.1f %.0f\r\n",
                             (unsigned long)item.t_ms,
                             item.vout_v,
                             item.iout_a,
                             item.err_v,
                             item.f_cmd_hz);
                break;

            case CR_LOG_EVT_BEGIN:
                debug_printf("[B] %lu %.1f %.1f %.1f\r\n",
                             (unsigned long)item.id,
                             item.iout_from_a,
                             item.iout_to_a,
                             item.vout_pre_v);
                break;

            case CR_LOG_EVT_END:
                debug_printf("[E] %lu %u %lu %lu %.1f %.1f %.1f\r\n",
                             (unsigned long)item.id,
                             (unsigned)item.pass,
                             (unsigned long)item.dt_ms,
                             (unsigned long)item.settle_ms,
                             item.vmin_v,
                             item.vmax_v,
                             item.maxerr_v);
                break;

            default:
                break;
            }
						#endif
        }

        dumped++;
    }
#endif
}
/*
	把当前“先 pop 再决定发不发”的方式改成：

	先偷看队首 item
	判断类型
	判断空间
	满足条件再 pop
*/
static void llc_cr_resp_log_dump_limited(uint8_t max_items, uint8_t allow_mon)
{
#if LLC_CR_RESP_LOG_ENABLE
    uint8_t dumped = 0U;

    if (max_items == 0U) {
        return;
    }

    while (dumped < max_items) {
        llc_cr_log_item_t item;
        uint32_t primask;
        uint16_t frame_len;
        uint8_t can_pop = 0U;

        primask = __get_PRIMASK();
        __disable_irq();

        if (s_cr_log_cnt == 0U) {
            s_cr_log_pending_dump = 0U;
            __set_PRIMASK(primask);
            break;
        }

        /* 先偷看队首，不立即 pop */
        item = s_cr_log_buf[s_cr_log_r];
        __set_PRIMASK(primask);

        /* MON 且当前不允许输出，则直接丢弃这一条 */
        if ((item.type == CR_LOG_MON) && (allow_mon == 0U)) {
            primask = __get_PRIMASK();
            __disable_irq();

            if (s_cr_log_cnt > 0U) {
                s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
                s_cr_log_cnt--;
                s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;
            } else {
                s_cr_log_pending_dump = 0U;
            }

            __set_PRIMASK(primask);
            dumped++;
            continue;
        }

        frame_len = llc_cr_log_frame_bytes(&item);

        /* BEGIN/END：空间不够则不要 pop，下次再试 */
        if ((item.type == CR_LOG_EVT_BEGIN) || (item.type == CR_LOG_EVT_END)) {
            if (debug_tx_available() < (int)frame_len) {
                break;
            }
        }

        /* MON：空间不够允许丢 */
        if (item.type == CR_LOG_MON) {
            if (debug_tx_available() < (int)frame_len) {
                primask = __get_PRIMASK();
                __disable_irq();

                if (s_cr_log_cnt > 0U) {
                    s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
                    s_cr_log_cnt--;
                    s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;
                } else {
                    s_cr_log_pending_dump = 0U;
                }

                __set_PRIMASK(primask);
                dumped++;
                continue;
            }
        }

        /* 到这里说明可以真正 pop */
        primask = __get_PRIMASK();
        __disable_irq();

        if (s_cr_log_cnt > 0U) {
            item = s_cr_log_buf[s_cr_log_r];
            s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
            s_cr_log_cnt--;
            s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;
            can_pop = 1U;
        } else {
            s_cr_log_pending_dump = 0U;
        }

        __set_PRIMASK(primask);

        if (can_pop) {
            llc_cr_proto_log_emit_bin(&item);
            dumped++;
        } else {
            break;
        }
    }
#endif
}

static void llc_collapse_trace_push(uint32_t f_act_hz)
{
#if LLC_COLLAPSE_TRACE_ENABLE
    uint16_t cap = (uint16_t)(sizeof(s_collapse_trace.buf) / sizeof(s_collapse_trace.buf[0]));
    llc_collapse_snap_t *snap = &s_collapse_trace.buf[s_collapse_trace.wr];

    snap->t_ms = g_ms;
    snap->state = (uint8_t)s_llc_rt.app.state;
    snap->f_cmd_hz = s_llc.f_cmd;
    snap->f_act_hz = f_act_hz;
    snap->vout_v = s_llc_rt.meas.vout_v;

    s_collapse_trace.wr = (uint16_t)((s_collapse_trace.wr + 1U) % cap);
    if (s_collapse_trace.size < cap) {
        s_collapse_trace.size++;
    }
#else
    (void)f_act_hz;
#endif
}

static void llc_collapse_trace_dump(uint8_t reason_state)
{
#if LLC_COLLAPSE_TRACE_ENABLE
    uint16_t cap = (uint16_t)(sizeof(s_collapse_trace.buf) / sizeof(s_collapse_trace.buf[0]));
    uint16_t cnt = s_collapse_trace.size;
	
    uint16_t start_idx;
    uint16_t trig_off;
    uint16_t idx_pre;
    uint16_t idx_trig;
    uint16_t idx_post;
    const llc_collapse_snap_t *pre;
    const llc_collapse_snap_t *trig;
    const llc_collapse_snap_t *post;
    float dv_v;
    float df_cmd_hz;
    float df_act_hz;
    uint8_t verdict = 0U;
    if (cnt == 0U) {
        return;
    }

		    start_idx = (uint16_t)((s_collapse_trace.wr + cap - cnt) % cap);
    trig_off = (cnt > (LLC_COLLAPSE_TRACE_POST_MS + 1U)) ?
               (uint16_t)(cnt - LLC_COLLAPSE_TRACE_POST_MS - 1U) : 0U;

    idx_pre = start_idx;
    idx_trig = (uint16_t)((start_idx + trig_off) % cap);
    idx_post = (uint16_t)((start_idx + cnt - 1U) % cap);

    pre = &s_collapse_trace.buf[idx_pre];
    trig = &s_collapse_trace.buf[idx_trig];
    post = &s_collapse_trace.buf[idx_post];

    dv_v = trig->vout_v - pre->vout_v;
    df_cmd_hz = trig->f_cmd_hz - pre->f_cmd_hz;
    df_act_hz = (float)trig->f_act_hz - (float)pre->f_act_hz;

    if ((pre->state != trig->state) || (fabsf(df_cmd_hz) > 3000.0f)) {
        verdict = 1U; /* software_suspect */
    } else if (fabsf(trig->f_cmd_hz - (float)trig->f_act_hz) < 2500.0f) {
        verdict = 2U; /* hardware_suspect */
    }

		    s_collapse_emit.pre = *pre;
    s_collapse_emit.trig = *trig;
    s_collapse_emit.post = *post;
    s_collapse_emit.verdict = verdict;
    s_collapse_emit.reason_state = reason_state;
    s_collapse_emit.dv_v = dv_v;
    s_collapse_emit.df_cmd_hz = df_cmd_hz;
    s_collapse_emit.df_act_hz = df_act_hz;
    s_collapse_emit.emit_idx = 0U;
    s_collapse_emit.pending = 1U;
#endif
}

static void llc_collapse_trace_drain_budget(uint8_t budget)
{
#if LLC_COLLAPSE_TRACE_ENABLE
    while ((budget > 0U) && s_collapse_emit.pending) {
			  /* keep CR logs higher priority to avoid UART bandwidth contention */
        if (s_cr_log_pending_dump || (s_cr_log_cnt > 0U)) {
            break;
        }
        if (s_collapse_emit.emit_idx <= 2U) {
            if (debug_tx_available() < 18) {
                break;
            }

            if (s_collapse_emit.emit_idx == 0U) {
                llc_cr_proto_collapse_emit_bin(0U,
                                               s_collapse_emit.pre.t_ms,
                                               s_collapse_emit.pre.state,
                                               s_collapse_emit.pre.vout_v,
                                               s_collapse_emit.pre.f_cmd_hz,
                                               s_collapse_emit.pre.f_act_hz);
            } else if (s_collapse_emit.emit_idx == 1U) {
                llc_cr_proto_collapse_emit_bin(1U,
                                               s_collapse_emit.trig.t_ms,
                                               s_collapse_emit.trig.state,
                                               s_collapse_emit.trig.vout_v,
                                               s_collapse_emit.trig.f_cmd_hz,
                                               s_collapse_emit.trig.f_act_hz);
            } else {
                llc_cr_proto_collapse_emit_bin(2U,
                                               s_collapse_emit.post.t_ms,
                                               s_collapse_emit.post.state,
                                               s_collapse_emit.post.vout_v,
                                               s_collapse_emit.post.f_cmd_hz,
                                               s_collapse_emit.post.f_act_hz);
            }
            s_collapse_emit.emit_idx++;
        } else {
            if (debug_tx_available() < 14) {
                break;
            }
            llc_cr_proto_collapse_diag_emit_bin(s_collapse_emit.verdict,
                                                s_collapse_emit.reason_state,
                                                s_collapse_emit.dv_v,
                                                s_collapse_emit.df_cmd_hz,
                                                s_collapse_emit.df_act_hz);
            s_collapse_emit.pending = 0U;
            s_collapse_emit.emit_idx = 0U;
        }
        budget--;
    }
#else
    (void)budget;
#endif
}

static void llc_collapse_trace_tick(void)
{
#if LLC_COLLAPSE_TRACE_ENABLE
    float vout = s_llc_rt.meas.vout_v;
    uint32_t f_act = llc_pwm_get_freq_hz();

	  uint8_t in_arm_state = ((s_llc_rt.app.state == ST_LLC_RUN) ||(s_llc_rt.app.state == ST_BURST_MODE)) ? 1U : 0U;
    if (!in_arm_state) {
        s_collapse_trace.active = 0U;
        s_collapse_trace.post_left = 0U;
        s_collapse_trace.trigger_inhibit = 0U;
        s_collapse_trace.last_vout_v = vout;
        return;
    }

    llc_collapse_trace_push(f_act);
    if (s_collapse_trace.active) {
        if (s_collapse_trace.post_left > 0U) {
            s_collapse_trace.post_left--;
        }
        if (s_collapse_trace.post_left == 0U) {
            llc_collapse_trace_dump((uint8_t)s_llc_rt.app.state);
            s_collapse_trace.active = 0U;
					  s_collapse_trace.trigger_inhibit = 1U;
            s_collapse_trace.last_trigger_ms = g_ms;
        }
    } else  if (s_collapse_trace.trigger_inhibit == 0U) {
        float dv = s_collapse_trace.last_vout_v - vout;
        uint8_t abs_hit = (vout <= LLC_COLLAPSE_VOUT_ABS_MIN_V) ? 1U : 0U;
        uint8_t drop_hit = (dv >= LLC_COLLAPSE_VOUT_DROP_V) ? 1U : 0U;
        uint8_t rearmed = (s_collapse_trace.last_trigger_ms == 0U) ||
                          elapsed_reached(s_collapse_trace.last_trigger_ms, LLC_COLLAPSE_TRACE_REARM_MS);

        if (rearmed && (abs_hit || drop_hit)) {
            s_collapse_trace.active = 1U;
            s_collapse_trace.post_left = (uint8_t)LLC_COLLAPSE_TRACE_POST_MS;
        }
    }

    s_collapse_trace.last_vout_v = vout;
#endif
}

void llc_app_tick_1khz(void)
{
	llc_update_measurements();
	llc_fan_tick();
	bus_vol_adj_follow_vout(s_llc.vref, s_llc_rt.meas.vout_v, s_llc_rt.meas.vbus_v,
	                      (pfc_is_ready() && (s_llc_rt.app.state != ST_IDLE) && (s_llc_rt.app.state != ST_FAULT)));
	llc_collapse_trace_tick();
	static uint32_t s_cr_dump_last_ms = 0U;
	static uint32_t s_cr_mon_last_ms = 0U;
  bool run_state = (s_llc_rt.app.state == ST_LLC_RUN);
//自适应日志转储调度器
	//if (s_cr_log_pending_dump && elapsed_reached(s_cr_dump_last_ms, run_state ? 2U : 5U)) 
		if (s_cr_log_pending_dump &&(s_cr_dump_last_ms == 0U || elapsed_reached(s_cr_dump_last_ms, run_state ? 2U : 5U))){
			uint8_t allow_mon = 1U;
			uint8_t dump_n = 4U;

			if (run_state) {
					//allow_mon = elapsed_reached(s_cr_mon_last_ms, 100U) ? 1U : 0U;
				  allow_mon = (s_cr_mon_last_ms == 0U || elapsed_reached(s_cr_mon_last_ms, 100U)) ? 1U : 0U;
					if (allow_mon) {
							s_cr_mon_last_ms = g_ms;
					}
					dump_n = (s_cr_log_cnt > (LLC_CR_RESP_LOG_CACHE_MAX / 2U)) ? 12U : 6U;
			} else if (power_stage_all_idle()) {
				//负载自适应的转储量控制
					dump_n = (s_cr_log_cnt > (LLC_CR_RESP_LOG_CACHE_MAX / 2U)) ? 10U : 5U;
			}

			s_cr_dump_last_ms = g_ms;
			llc_cr_resp_log_dump_limited(dump_n, allow_mon);
	}
		
		llc_collapse_trace_drain_budget(1U);
	  bool enable_llc = (LLC_ENABLE != 0) && pfc_is_ready();
		//bool enable_llc = pfc_is_ready();
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
		{
			if(pfc_ready_stable &&llc_precheck_ok())
			{
				llc_state_enter(ST_PRECHECK);
			}
			break;
	}
		case ST_PRECHECK:
		{
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
			}
		case ST_SOFTSTART:
		{
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
		}
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
		{
			if(!enable_llc||(s_llc_rt.meas.vbus_v<(LLC_VBUS_MIN_START_V-LLC_VOUT_HYST_V)))
			{
				llc_state_enter(ST_STOPPING);
				break;
			}

#if LLC_BURST_MODE_ENABLE
#if !LLC_FIXED_FREQ_LOAD_TEST_ENABLE
		/* Burst进入检测 - 慢进入，需持续轻载超过延迟时间 */
		if(s_llc_rt.meas.iout_a < LLC_BURST_IOUT_ENTER_A)
		{
			if(s_llc_rt.burst_enter_delay_ms == 0)
			{
				s_llc_rt.burst_enter_delay_ms = g_ms; /* 开始计时 */
			}
			else if(elapsed_reached(s_llc_rt.burst_enter_delay_ms, LLC_BURST_ENTER_DELAY_MS))
			{
				/* 持续轻载超过1秒，进入Burst */
				s_llc_rt.burst_enter_delay_ms = 0;
				llc_state_enter(ST_BURST_MODE);
				break;
			}
		}
		else
		{
			s_llc_rt.burst_enter_delay_ms = 0; /* 电流回升，重置计时 */
		}
#endif
#endif
			break;
	 }
		case ST_BURST_MODE:
		{
			float vout = s_llc_rt.meas.vout_v;
			float vtarget = s_llc_rt.vout_burst_target;
			
			/* 首次进入Burst的软进入：先升频到最高，再进入OFF */
			if(s_llc_rt.burst_state == BURST_STATE_ENTRY_PREPARE)
			{
				if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_ENTRY_RAMP_MS))
				{
				/* 升频完成，进入OFF状态 */
				s_llc_rt.burst_state = BURST_STATE_OFF;
				s_llc_rt.burst_begin_ms = g_ms;
				llc_pwm_outputs_enable(0);
				llc_set_freq(LLC_BURST_F_HZ, true);  /* 强制更新：状态切换 */
				
#if DEBUG_PRINTF_BURST_MODE
				debug_printf("[BURST] ENTRY_PREPARE->OFF: vout=%.2fV, iout=%.2fA\n", 
				             s_llc_rt.meas.vout_v, s_llc_rt.meas.iout_a);
#endif
				}
			}
			else if(s_llc_rt.burst_state == BURST_STATE_OFF)
			{
				/* OFF阶段：电压低于阈值时打开 */
				if(vout < (vtarget - LLC_BURST_VOUT_HYST_V))
				{
					if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_OFF_MIN_MS))
					{
						s_llc_rt.burst_state = BURST_STATE_ON;
						s_llc_rt.burst_begin_ms = g_ms;
						/* 先设置频率，再使能输出 */
						llc_set_freq(LLC_BURST_F_HZ, true); /* 强制更新：OFF->ON切换 */
						llc_pwm_outputs_enable(1);
						
#if DEBUG_PRINTF_BURST_MODE
						debug_printf("[BURST] OFF->ON (voltage): vout=%.2fV, iout=%.2fA, off_time=%dms\n", 
						             vout, s_llc_rt.meas.iout_a, elapsed_since(s_llc_rt.burst_begin_ms));
#endif
					}
				}
			
				else if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_OFF_MAX_MS))
				{
				/* 强制打开，防止电压过低 */
				s_llc_rt.burst_state = BURST_STATE_ON;
				s_llc_rt.burst_begin_ms = g_ms;
				/* 先设置频率，再使能输出 */
				llc_set_freq(LLC_BURST_F_HZ, true); /* 强制更新：OFF->ON切换 */
				llc_pwm_outputs_enable(1);
				
#if DEBUG_PRINTF_BURST_MODE
				debug_printf("[BURST] OFF->ON (timeout): vout=%.2fV, iout=%.2fA, off_time=%dms\n", 
				             vout, s_llc_rt.meas.iout_a, elapsed_since(s_llc_rt.burst_begin_ms));
#endif
				}
			}
		else if(s_llc_rt.burst_state == BURST_STATE_ON_PREPARE)
			{
				/* 高频准备阶段：先将频率提升到最高，再关PWM (1ms准备时间) */
				if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_F_PREPARE_MS))
				{
					s_llc_rt.burst_state = BURST_STATE_OFF;
					s_llc_rt.burst_begin_ms = g_ms;
					llc_pwm_outputs_enable(0); /* 高频准备完成，关PWM */
					
#if DEBUG_PRINTF_BURST_MODE
					debug_printf("[BURST] ON_PREPARE->OFF: vout=%.2fV, iout=%.2fA\n", 
					             vout, s_llc_rt.meas.iout_a);
#endif
				}
			}
			else /* BURST_STATE_ON */
			{
				/* ON阶段：电压高于阈值时，先高频准备再关闭 */
				if(vout > (vtarget + LLC_BURST_VOUT_HYST_V))
				{
					if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_ON_MIN_MS))
					{
						/* 进入高频准备阶段，而不是立即关PWM */
						s_llc_rt.burst_state = BURST_STATE_ON_PREPARE;
						s_llc_rt.burst_begin_ms = g_ms;
						llc_set_freq(LLC_F_MAX_HZ, true); /* 强制更新：ON->ON_PREPARE切换 */
						
#if DEBUG_PRINTF_BURST_MODE
						debug_printf("[BURST] ON->ON_PREPARE (voltage): vout=%.2fV, iout=%.2fA, on_time=%dms\n", 
						             vout, s_llc_rt.meas.iout_a, elapsed_since(s_llc_rt.burst_begin_ms));
#endif
					}
				}
				else if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_ON_MAX_MS))
				{
					/* 强制高频准备后关闭 */
					s_llc_rt.burst_state = BURST_STATE_ON_PREPARE;
					s_llc_rt.burst_begin_ms = g_ms;
					llc_set_freq(LLC_F_MAX_HZ, true);  /* 强制更新 */
					
#if DEBUG_PRINTF_BURST_MODE
					debug_printf("[BURST] ON->ON_PREPARE (timeout): vout=%.2fV, iout=%.2fA, on_time=%dms\n", 
					             vout, s_llc_rt.meas.iout_a, elapsed_since(s_llc_rt.burst_begin_ms));
#endif
				}
			}
			
			/* 退出Burst检测 - 快退出 */
			if(s_llc_rt.meas.iout_a > LLC_BURST_IOUT_EXIT_A)
			{
				if(s_llc_rt.burst_exit_delay_ms == 0)
				{
					s_llc_rt.burst_exit_delay_ms = g_ms;
				}
				else if(elapsed_reached(s_llc_rt.burst_exit_delay_ms, LLC_BURST_EXIT_DELAY_MS))
				{
					 float f_resume;
					 s_llc_rt.burst_exit_delay_ms = 0U;
					 /* 用进入Burst前保存的频率恢复闭环接管点 */
					 f_resume = f_clampf(s_llc_rt.f_pre_burst_hz, s_llc.f_min, s_llc.f_max);
					 /* 先恢复PWM，再恢复接管频率 */
           llc_pwm_outputs_enable(1);
           llc_set_freq(f_resume, true);
					 /* 以恢复频率作为当前工作点做bumpless */
           llc_ctrl_bumpless_init(LLC_VOUT_TARGET_V, vout, f_resume);
					 /* 同步命令频率，避免ST_LLC_RUN入口再次改写时出现跳变 */
					 s_llc.f_cmd = f_resume;

#if DEBUG_PRINTF_BURST_MODE
					 debug_printf("[BURST] EXIT: iout=%.2fA, f_resume=%.0fHz, vout=%.2fV, state=%d\n", 
					              s_llc_rt.meas.iout_a, f_resume, vout, s_llc_rt.burst_state);
#endif
					 llc_state_enter(ST_LLC_RUN);
				}
			}
			else
			{
				s_llc_rt.burst_exit_delay_ms = 0; /* 电流回落，重置退出计时 */
			}

			break;
		}
		case ST_CYCLE_STOPPING:
		{
			/* 执行软关断tick */
			llc_softstop_tick_1khz();
			
			/* 检查软关断是否完成 */
			if(llc_softstop_is_done())
			{
				llc_pwm_outputs_enable(0);
				llc_softstop_reset();
				/* 等待一段时间后再重新软启动 */
				if(elapsed_reached(s_llc_rt.stopping_begin_ms, LLC_SOFTSTOP_DURATION_MS + LLC_CYCLE_STOP_RESTART_MS))
				{
					llc_driver_en_set(false);
					llc_state_enter(ST_PRECHECK);
				}
			}
			break;
		}
		case ST_STOPPING:
		{
			/* 执行软关断tick */
			llc_softstop_tick_1khz();
			
			/* 检查软关断是否完成 */
			if(llc_softstop_is_done())
			{
				llc_pwm_outputs_enable(0);
				llc_driver_en_set(false);

				llc_softstop_reset();  /* 重置软关断状态 */
				llc_state_enter(ST_IDLE);
			}
			break;
		}
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





