#include "llc_control.h"
#include "float.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#ifndef LLC_VOUT_FILT_ALPHA
#define LLC_VOUT_FILT_ALPHA          (0.08f) /* 100us快环滤波系数 */
#endif

#ifndef LLC_F_NOM_USE_STAGE_CMD
#define LLC_F_NOM_USE_STAGE_CMD      (1)
#endif

#ifndef LLC_F_NOM_HZ
#define LLC_F_NOM_HZ                 (LLC_F_INIT_HZ)
#endif
/*********************************************************************************************************
*                                             CR模式负载动态响应测试
*********************************************************************************************************/
#ifndef LLC_CR_RESP_LOG_ENABLE
#define LLC_CR_RESP_LOG_ENABLE        (1U)
#endif

#ifndef LLC_CR_RESP_LOG_PERIOD_MS
#define LLC_CR_RESP_LOG_PERIOD_MS     (100U)
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
#define LLC_CR_RESP_LOG_CACHE_MAX      (64U)
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

typedef enum
{
    CR_LOG_MON = 0,
    CR_LOG_EVT_BEGIN,
    CR_LOG_EVT_END
} cr_log_type_t;

typedef struct
{
    cr_log_type_t type;
    uint32_t t_ms;
    uint32_t id;
    uint8_t  pass;
    uint32_t dt_ms;
    uint32_t settle_ms;
    float    vout_v;
    float    iout_a;
    float    err_v;
    float    f_cmd_hz;
    float    iout_from_a;
    float    iout_to_a;
    float    vout_pre_v;
    float    vmin_v;
    float    vmax_v;
    float    maxerr_v;
} llc_cr_log_item_t;

static llc_cr_log_item_t s_cr_log_buf[LLC_CR_RESP_LOG_CACHE_MAX];
static uint16_t s_cr_log_w = 0U;
static uint16_t s_cr_log_r = 0U;
static uint16_t s_cr_log_cnt = 0U;
static uint8_t  s_cr_log_pending_dump = 0U;

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
static void llc_cr_resp_tick(void);

static void llc_cr_resp_log_push(const llc_cr_log_item_t *item);
static void llc_cr_resp_log_dump(void);
static void llc_cr_resp_log_dump_all(void);  /* STOP模式批量输出 */
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
/**
 * @brief 更新LLC测量值
 * 
 * 从ADC采集原始数据并转换为物理量，包括输出电压、输出电流和母线电压。
 * 更新后的测量值存储在运行时上下文和全局LLC结构中。
 * 
 * @note 该函数为静态内部函数，仅供模块内部调用
 */
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
/**
 * @brief 检查PFC就绪信号是否稳定
 * @param enable_llc 是否启用LLC控制
 * @return true PFC就绪信号已稳定达到设定时长
 * @return false PFC就绪信号未稳定或LLC未启用
 * @note 当enable_llc为false时重置计时器，为true时开始计时
 */
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

    if (s_llc_rt.meas.vout_v > LLC_VOUT_OVP_V) {  //58
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
    float i_step = fabsf(iout - s_cr_resp.iout_prev_a);

    if (s_llc_rt.app.state != ST_LLC_RUN) {
        s_cr_resp.iout_prev_a = iout;
        s_cr_resp.next_period_log_ms = g_ms + LLC_CR_RESP_LOG_PERIOD_MS;
        return;
    }
#if !CRITICAL_LOG_ONLY
    if (g_ms >= s_cr_resp.next_period_log_ms) {
        llc_cr_log_item_t item;
        item.type = CR_LOG_MON;
        item.t_ms = g_ms;
        item.vout_v = vout;
        item.iout_a = iout;
        item.err_v = err;
        item.f_cmd_hz = s_llc.f_cmd;
        llc_cr_resp_log_push(&item);
        s_cr_resp.next_period_log_ms = g_ms + LLC_CR_RESP_LOG_PERIOD_MS;
    }
#endif
    if (!s_cr_resp.active && (i_step >= LLC_CR_RESP_STEP_IOUT_A)) {
        s_cr_resp.active = 1U;
        s_cr_resp.seq++;
        s_cr_resp.start_ms = g_ms;
        s_cr_resp.settled_ms = 0U;
        s_cr_resp.iout_from_a = s_cr_resp.iout_prev_a;
        s_cr_resp.iout_to_a = iout;
        s_cr_resp.vout_pre_v = vout;
        s_cr_resp.vout_min_v = vout;
        s_cr_resp.vout_max_v = vout;
        s_cr_resp.max_abs_err_v = fabsf(err);
        s_cr_resp.stable_ticks = 0U;
        {
            llc_cr_log_item_t item;
            item.type = CR_LOG_EVT_BEGIN;
            item.id = s_cr_resp.seq;
            item.iout_from_a = s_cr_resp.iout_from_a;
            item.iout_to_a = s_cr_resp.iout_to_a;
            item.vout_pre_v = s_cr_resp.vout_pre_v;
            llc_cr_resp_log_push(&item);
        }
    }

    if (s_cr_resp.active) {
        if (vout < s_cr_resp.vout_min_v) {
            s_cr_resp.vout_min_v = vout;
        }
        if (vout > s_cr_resp.vout_max_v) {
            s_cr_resp.vout_max_v = vout;
        }

        if (fabsf(err) > s_cr_resp.max_abs_err_v) {
            s_cr_resp.max_abs_err_v = fabsf(err);
        }

        if (fabsf(err) <= LLC_CR_RESP_STABLE_WIN_V) {
            if (s_cr_resp.stable_ticks < LLC_CR_RESP_STABLE_TICKS) {
                s_cr_resp.stable_ticks++;
            }
            if ((s_cr_resp.stable_ticks >= LLC_CR_RESP_STABLE_TICKS) && (s_cr_resp.settled_ms == 0U)) {
                s_cr_resp.settled_ms = g_ms - s_cr_resp.start_ms;
            }
        } else {
            s_cr_resp.stable_ticks = 0U;
        }

        if ((s_cr_resp.settled_ms > 0U) ||
            elapsed_reached(s_cr_resp.start_ms, LLC_CR_RESP_TIMEOUT_MS)) {
            uint32_t event_ms = g_ms - s_cr_resp.start_ms;
            uint8_t pass = (s_cr_resp.settled_ms > 0U) ? 1U : 0U;
            {
                llc_cr_log_item_t item;
                item.type = CR_LOG_EVT_END;
                item.id = s_cr_resp.seq;
                item.pass = pass;
                item.dt_ms = event_ms;
                item.settle_ms = s_cr_resp.settled_ms;
                item.vmin_v = s_cr_resp.vout_min_v;
                item.vmax_v = s_cr_resp.vout_max_v;
                item.maxerr_v = s_cr_resp.max_abs_err_v;
                llc_cr_resp_log_push(&item);
            }
            s_cr_resp.active = 0U;
            s_cr_resp.stable_ticks = 0U;
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
	#if Bus_Adj
	//bus_vol_adj_reset();   //重置总线电压调整逻辑 百分之五十的占空比
	#endif
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
	    s_llc.f_nom = f_clampf(s_llc.f_cmd, s_llc.f_min, s_llc.f_max);
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
        .kp      = 1900.0f,
        .ki      = 18.0f,

        /* RAW PI参数 */
        .kp_raw  = 3.0f,
        .ki_raw  = 0.050f,

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

    llc_state_enter(ST_IDLE);
}
#endif
void llc_app_tick_adc_test(void)
{
	llc_update_measurements();
}

static uint8_t vloop_div = 0;
static uint8_t vout_filt_inited = 0U;
#if 1
void llc_app_tick_100us(void)
{
	  static float vout_filt_v   = 0.0f;   /* 电压PI用滤波状态 */
    static float vout_filt_raw = 0.0f;   /* RAW PI用滤波状态 */
    float err;
    float f_cmd;
	
	
    if (s_llc_rt.app.state != ST_LLC_RUN && s_llc_rt.app.state != ST_BURST_MODE) {
			vloop_div = 0;
			vout_filt_inited = 0U;
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
        return;
    }
    err = s_llc.vref - s_llc.vmeas;
		if (++vloop_div >= 5U) {   // 500us
			vloop_div = 0;
			f_cmd = llc_ctrl_step(err);
			llc_set_freq(f_cmd, false);  /* 自然更新：闭环微调 */
		}
}
#endif 
#if 0
void llc_app_tick_100us(void)
{
    static float vout_samples[5] = {0.0f};
    static uint8_t sample_idx = 0U;
    float vout_now;
    if (s_llc_rt.app.state != ST_LLC_RUN) {
        uint8_t i;
        vloop_div = 0U;
        sample_idx = 0U;
        for (i = 0U; i < 5U; i++) {
            vout_samples[i] = 0.0f;
        }
        return;
    }
    vout_now = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    vout_samples[sample_idx] = vout_now;
    sample_idx++;
    if (sample_idx >= 5U) {
        sample_idx = 0U;
    }
    if (++vloop_div >= 5U) {
        float vmax, vmin, vsum;
        float vmeas_500us;
        float err;
        uint8_t i;
        vloop_div = 0U;
        vmax = vout_samples[0];
        vmin = vout_samples[0];
        vsum = 0.0f;
        for (i = 0U; i < 5U; i++) {
            float v = vout_samples[i];
            if (v > vmax) {
                vmax = v;
            }
            if (v < vmin) {
                vmin = v;
            }
            vsum += v;
        }
        vmeas_500us = (vsum - vmax - vmin) / 3.0f;
        s_llc.vmeas = vmeas_500us;
				
        err = s_llc.vref - s_llc.vmeas;
        llc_set_freq(llc_ctrl_step(err), false);  /* 自然更新：闭环微调 */
    }
}
#endif
#if 0
#ifndef LLC_VLOOP_DECIMATE_N
#define LLC_VLOOP_DECIMATE_N          (5U)      /* 100us * 5 = 500us 执行一次电压环 */
#endif

#ifndef LLC_VOUT_USE_POST_IIR
#define LLC_VOUT_USE_POST_IIR         (1)       /* 0: 不做后级IIR  1: 做轻IIR */
#endif

#ifndef LLC_VOUT_POST_IIR_ALPHA
#define LLC_VOUT_POST_IIR_ALPHA       (0.25f)   /* 500us节拍上的轻IIR系数 */
#endif

static uint8_t vout_buf_inited  = 0U;
void llc_app_tick_100us(void)
{
    static float vout_samples[LLC_VLOOP_DECIMATE_N] = {0.0f};
    static uint8_t sample_idx = 0U;

#if LLC_VOUT_USE_POST_IIR
    static float vmeas_post_filt = 0.0f;
    static uint8_t vmeas_post_filt_inited = 0U;
#endif

    float vout_now;

    if (s_llc_rt.app.state != ST_LLC_RUN) {
        uint8_t i;

        vloop_div = 0U;
        sample_idx = 0U;
        vout_buf_inited = 0U;

        for (i = 0U; i < LLC_VLOOP_DECIMATE_N; i++) {
            vout_samples[i] = 0.0f;
        }

#if LLC_VOUT_USE_POST_IIR
        vmeas_post_filt = 0.0f;
        vmeas_post_filt_inited = 0U;
#endif
        return;
    }

    /* 当前100us采样点：ADC raw -> 实际输出电压 */
    vout_now = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);

    /* 首次进入RUN：用当前值灌满整个窗口，避免前5点被0污染 */
    if (!vout_buf_inited) {
        uint8_t i;
        for (i = 0U; i < LLC_VLOOP_DECIMATE_N; i++) {
            vout_samples[i] = vout_now;
        }
        sample_idx = 0U;
        vout_buf_inited = 1U;

#if LLC_VOUT_USE_POST_IIR
        vmeas_post_filt = vout_now;
        vmeas_post_filt_inited = 1U;
#endif
    }

    /* 更新滑窗 */
    vout_samples[sample_idx] = vout_now;
    sample_idx++;
    if (sample_idx >= LLC_VLOOP_DECIMATE_N) {
        sample_idx = 0U;
    }

    /* 500us执行一次电压环 */
    if (++vloop_div >= LLC_VLOOP_DECIMATE_N) {
        float vmax, vmin, vsum;
        float vmeas_raw;
        float vmeas_used;
        float err;
        float f_cmd;
        uint8_t i;

        vloop_div = 0U;

        /* 5点去极值平均 */
        vmax = vout_samples[0];
        vmin = vout_samples[0];
        vsum = 0.0f;

        for (i = 0U; i < LLC_VLOOP_DECIMATE_N; i++) {
            float v = vout_samples[i];
            if (v > vmax) {
                vmax = v;
            }
            if (v < vmin) {
                vmin = v;
            }
            vsum += v;
        }

        /* 对5点窗：去掉最大最小后，剩余3点平均 */
        vmeas_raw = (vsum - vmax - vmin) / (float)(LLC_VLOOP_DECIMATE_N - 2U);

#if LLC_VOUT_USE_POST_IIR
        if (!vmeas_post_filt_inited) {
            vmeas_post_filt = vmeas_raw;
            vmeas_post_filt_inited = 1U;
        }

        vmeas_post_filt += LLC_VOUT_POST_IIR_ALPHA * (vmeas_raw - vmeas_post_filt);
        vmeas_used = vmeas_post_filt;
#else
        vmeas_used = vmeas_raw;
#endif

        s_llc.vmeas = vmeas_used;
        err = s_llc.vref - s_llc.vmeas;

        f_cmd = llc_ctrl_step(err);
        llc_set_freq(f_cmd, false);  /* 自然更新：闭环微调 */


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

static void llc_cr_resp_log_dump_limited(uint8_t max_items)
{
#if LLC_CR_RESP_LOG_ENABLE
    uint8_t dumped = 0U;

    while (dumped < max_items) {
        llc_cr_log_item_t item;
        uint32_t primask;

        /* 只在出队时进入临界区，避免共享变量竞争 */
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

        /* 队列是否还有剩余数据 */
        s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;

        __set_PRIMASK(primask);

        /* 临界区外做慢操作：串口打印 */
        switch (item.type) {
        case CR_LOG_MON:
            debug_printf("[CR_MON] t=%lu vo=%.1fV io=%.1fA e=%.1fV f=%.0fHz\r\n",
                         (unsigned long)item.t_ms,
                         item.vout_v,
                         item.iout_a,
                         item.err_v,
                         item.f_cmd_hz);
            break;

        case CR_LOG_EVT_BEGIN:
            debug_printf("[CR_EVT_BEGIN] id=%lu io=%.1f->%.1fA vo=%.1fV\r\n",
                         (unsigned long)item.id,
                         item.iout_from_a,
                         item.iout_to_a,
                         item.vout_pre_v);
            break;

        case CR_LOG_EVT_END:
            debug_printf("[CR_EVT_END] id=%lu pass=%u dt=%lums st=%lums "
                         "vmin=%.1fV vmax=%.1fV me=%.1fV\r\n",
                         (unsigned long)item.id,
                         (unsigned)item.pass,
                         (unsigned long)item.dt_ms,
                         (unsigned long)item.settle_ms,
                         item.vmin_v,
                         item.vmax_v,
                         item.maxerr_v);
            break;

        default:
            debug_printf("[CR_LOG] unknown type=%u\r\n", (unsigned)item.type);
            break;
        }

        dumped++;
    }
#endif
}
void llc_app_tick_1khz(void)
{
	llc_update_measurements();
  llc_cr_resp_tick();
	
	static uint32_t s_cr_dump_last_ms = 0U;
/*
	if (s_cr_log_pending_dump &&
			power_stage_all_idle() &&
			elapsed_reached(s_cr_dump_last_ms, 5U)) {
			s_cr_dump_last_ms = g_ms;
			llc_cr_resp_log_dump_limited(4U);
	}
	*/
	if(s_cr_log_pending_dump)
	{
		llc_cr_resp_log_dump();
	}
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
		{
			if(pfc_ready_stable &&llc_precheck_ok())
			{
				llc_state_enter(ST_PRECHECK);
				//debug_printf("0");
				//debug_putc_blocking('0');
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
				//debug_printf("sf ok");
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
				//debug_printf("run ok");
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







