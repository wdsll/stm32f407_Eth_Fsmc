#include "llc_control.h"
#include "float.h"
#include "llc_cr_proto.h"
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
#define LLC_F_NOM_HZ                 (92000.0f)  
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
    uint32_t fault_recover_begin_ms;   /* FAULT恢复防抖计时起点 */
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
static inline float conv_adc_to_vout(uint16_t raw);  /* Vout 专用：实测标定 + 0V 钳位 */
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

static void llc_collapse_trace_tick(void);
static void llc_collapse_trace_push(uint32_t f_act_hz);
static void llc_collapse_trace_dump(uint8_t reason_state);
static void llc_collapse_trace_drain_budget(uint8_t budget);

static void llc_burst_fast_reset(void);
static void llc_burst_state_set(burst_state_t next);
static void llc_burst_pwm_set(uint8_t on);
static void llc_burst_ctl_100us(float vout_filt, float vout_raw);
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

/* Vout 专用转换：实测标定公式 + 0V 钳位 */
static inline float conv_adc_to_vout(uint16_t raw)
{
    float v = (float)raw * VOUT_K_V_PER_COUNT + VOUT_B_V;
    return v < 0.0f ? 0.0f : v;
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
    s_llc_rt.meas.vout_v = conv_adc_to_vout(g_adc_multi.vout_raw);
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
   /* 4840 原理图确认：BKIN(PB12) 通过光耦 LTV-817S 连接主板 OCP 比较器(LMV393)
    * 必须检查硬件故障锁存和 BKIN 当前电平，否则 BRK 中断丢失时 LLC 状态机不知道硬件故障。
    * BKIN 低有效：protect_fault_active_hw() 返回 1 表示 PB12 为低电平（故障存在） */
   if (protect_fault_active_hw() || protect_fault_latched()) {
       return true;
   }

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
*                                      Burst 100us 快环门控
*********************************************************************************************************/
static uint16_t s_burst_tick_100us = 0U;
static uint8_t  s_burst_pwm_on = 0U;

static void llc_burst_fast_reset(void)
{
    s_burst_tick_100us = 0U;
    s_burst_pwm_on = 0U;
}

static void llc_burst_state_set(burst_state_t next)
{
    if (s_llc_rt.burst_state == next) {
        return;
    }

    s_llc_rt.burst_state = next;
    s_burst_tick_100us = 0U;

    /*
     * 保留ms时间戳，方便现有调试打印/状态观察。
     * 但Burst实际ON/OFF门控由100us tick控制。
     */
    s_llc_rt.burst_begin_ms = g_ms;
}

static void llc_burst_pwm_set(uint8_t on)
{
    if (s_burst_pwm_on == on) {
        return;
    }

    if (on) {
        /*
         * 先设置高频，再开PWM。
         * 高频Burst包络能量更小，适合空载。
         */
        llc_set_freq(LLC_BURST_F_HZ, true);
        llc_pwm_outputs_enable(1);
        s_burst_pwm_on = 1U;
    } else {
        /*
         * 先推到最高频，再关PWM。
         * 避免停波前仍停留在低频大能量点。
         */
        llc_set_freq(LLC_F_MAX_HZ, true);
        llc_pwm_outputs_enable(0);
        s_burst_pwm_on = 0U;
    }
}

/**
 * @brief Burst 100us快环门控
 *
 * 只在 ST_BURST_MODE 下调用。
 * 1kHz状态机只负责进入/退出Burst，不再负责每个ON/OFF包络。
 */

static void llc_burst_ctl_100us(float vout_filt,float vout)
{
#if LLC_BURST_MODE_ENABLE
    const float vtarget = s_llc_rt.vout_burst_target;
    const float v_high  = vtarget + LLC_BURST_HIGH_HYS_V;
    const float v_low   = vtarget - LLC_BURST_LOW_HYS_V;
    const float v_force = vtarget + LLC_BURST_FORCE_OFF_V;

    if (s_llc_rt.app.state != ST_BURST_MODE) {
        llc_burst_fast_reset();
        return;
    }

    if (s_burst_tick_100us < 0xFFFFU) {
        s_burst_tick_100us++;
    }

    /*
     * 最高优先级：Burst快速过压关断。
     * 空载时如果单包能量偏大，先直接关PWM，避免靠OVP兜底。
     */
    if (vout > v_force) {
        llc_burst_pwm_set(0U);
        llc_burst_state_set(BURST_STATE_OFF);
        return;
    }

    switch (s_llc_rt.burst_state) {
			case BURST_STATE_ENTRY_PREPARE:
					/*
					 * 刚进入Burst时，先保持PWM开，频率拉到最高，短暂准备后关PWM。
					 */
					if (s_burst_tick_100us >= LLC_BURST_ENTRY_RAMP_TICKS) {
							llc_burst_pwm_set(0U);
							llc_burst_state_set(BURST_STATE_OFF);
					}
					break;

			case BURST_STATE_OFF:
					/*
					 * OFF阶段：
					 * 只有 Vout 真的低于下限，并且满足最小OFF时间，才允许补一个PWM包。
					 * 不做OFF_MAX无条件强制开通，避免空载把Vout越顶越高。
					 */
					if ((vout_filt < v_low) &&
							(s_burst_tick_100us >= LLC_BURST_OFF_MIN_TICKS)) {
							llc_burst_pwm_set(1U);
							llc_burst_state_set(BURST_STATE_ON);
					}
					break;

			case BURST_STATE_ON:
					/*
					 * ON阶段：
					 * 达到上限并满足最小ON时间，则进入关断准备。
					 * 如果一直未达到上限，也不能长时间ON，ON_MAX兜底进入关断准备。
					 */
					if (((vout_filt > v_high) &&
							 (s_burst_tick_100us >= LLC_BURST_ON_MIN_TICKS)) ||
							(s_burst_tick_100us >= LLC_BURST_ON_MAX_TICKS)) {
							llc_set_freq(LLC_F_MAX_HZ, true);
							llc_burst_state_set(BURST_STATE_ON_PREPARE);
					}
					break;

			case BURST_STATE_ON_PREPARE:
					/*
					 * 关PWM前保持最高频一个很短窗口，再真正关断。
					 */
					if (s_burst_tick_100us >= LLC_BURST_F_PREPARE_TICKS) {
							llc_burst_pwm_set(0U);
							llc_burst_state_set(BURST_STATE_OFF);
					}
					break;

			default:
					/*
					 * 异常子状态，直接回OFF，PWM关断。
					 */
					llc_burst_pwm_set(0U);
					llc_burst_state_set(BURST_STATE_OFF);
					break;
			}
	#else
			(void)vout;
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
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
		// 1. 故障恢复与状态重置
    // 2. 关闭功率输出（安全保护）
    // 3. 初始化所有运行时变量
    // 4. 重置突发模式状态
	 case ST_IDLE:
		  llc_softstart_on_fault(); //软启动故障处理：清理软启动过程中的故障状态
		  llc_softstop_reset();  /* 重置软关断状态 */
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
	    llc_burst_fast_reset();
		  break;
	 case ST_PRECHECK:  
	  	llc_driver_en_set(true);
    	llc_pwm_outputs_enable(0); // 禁用 PWM 输出
    	llc_set_freq(s_llc.f_max, true);  /* 强制更新 */
			llc_burst_fast_reset();
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
			s_burst_pwm_on = 1U;  /* 如果软关断需要PWM存在，则同步影子状态 */
      /* 启动软关断 */
      llc_softstop_start(s_llc.f_cmd);
      break;
	 case ST_BURST_MODE:
			llc_driver_en_set(true);
			s_llc_rt.vout_burst_target = LLC_VOUT_TARGET_V;
			/* 记录进入Burst前的频率，用于退出时bumpless */
			s_llc_rt.f_pre_burst_hz = f_clampf(s_llc.f_cmd, s_llc.f_min, s_llc.f_max);			
		 /*
     * 每次进入Burst都先走 ENTRY_PREPARE：
     * PWM保持开启，频率拉高，随后100us快环会关PWM。
     */
			s_llc_rt.burst_state = BURST_STATE_ENTRY_PREPARE;
			s_llc_rt.burst_first_entry = 0;
	    s_llc_rt.burst_begin_ms = g_ms;
      s_llc_rt.burst_exit_delay_ms = 0U;
      llc_burst_fast_reset();	 
			llc_pwm_outputs_enable(1);  /* 进入准备阶段保持PWM开启 */
			llc_set_freq(LLC_F_MAX_HZ, true); /* 强制更新：进入Burst升频准备 */
			s_burst_pwm_on = 1U;
			break;
	 case ST_FAULT:
	 		llc_softstart_on_fault();
			llc_softstop_reset();  /* 故障时重置软关断状态 */
			llc_pwm_outputs_enable(0);
			llc_driver_en_set(false);
			llc_set_freq(s_llc.f_max, true);  /* 强制更新：故障保护立即生效 */
			llc_burst_fast_reset();
			s_llc_rt.fault_recover_begin_ms = 0U;  /* 重置恢复计时 */
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
void llc_app_tick_100us(void)
{
	  static float vout_filt_v   = 0.0f;   /* 电压PI用滤波状态 */
    float err;
    float f_cmd;
	
	
    if (s_llc_rt.app.state != ST_LLC_RUN && s_llc_rt.app.state != ST_BURST_MODE) {
			vloop_div = 0;
			vout_filt_inited = 0U;
			//llc_cr_resp_tick();
      return;
    }	

		 /* =========================
     * 正常电压PI模式
     * raw -> 电压 -> 滤波 -> PI
     * err = vref - vmeas
     * ========================= */
    float vout_now = conv_adc_to_vout(g_adc_multi.vout_raw);
		
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
				llc_burst_ctl_100us(s_llc.vmeas, vout_now);

			  //llc_cr_resp_tick();
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
			//llc_cr_resp_tick();
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
	bus_vol_adj_follow_vout(s_llc.vref, s_llc_rt.meas.vout_v, s_llc_rt.meas.vbus_v,
	                      (pfc_is_ready() && (s_llc_rt.app.state != ST_IDLE) && (s_llc_rt.app.state != ST_FAULT)));

	//bool enable_llc = (LLC_ENABLE != 0) && pfc_is_ready();
	bool enable_llc = pfc_is_ready();
	bool pfc_ready_stable = llc_pfc_ready_stable(enable_llc);
	if (s_llc_rt.app.state != ST_IDLE && s_llc_rt.app.state != ST_FAULT) { //非idle&故障太才会进行故障检测
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
		/* Burst进入检测 - 慢进入，需持续轻载超过延迟时间 三重判断电流小于1.5A,电压要大于目标电压，频率大于百分之九十的最大频率*/
		if((s_llc_rt.meas.iout_a < LLC_BURST_IOUT_ENTER_A)&&(s_llc.vmeas >= (LLC_VOUT_TARGET_V - 0.5f))&&(s_llc.f_cmd >= LLC_BURST_F_ENTER_MAX_HZ))
		{
			if(s_llc_rt.burst_enter_delay_ms == 0)
			{
				s_llc_rt.burst_enter_delay_ms = g_ms; /* 开始计时 */
			}
			//慢进入200ms延时防抖 
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
			
			if (!enable_llc || !llc_precheck_ok()) {
        llc_pwm_outputs_enable(0);
        s_burst_pwm_on = 0U;
        llc_state_enter(ST_STOPPING);
        break;
			}
		/*
     * 1kHz只负责退出Burst。
     * Burst内部的PWM ON/OFF门控由 llc_burst_ctl_100us() 完成。
     */
    if (s_llc_rt.meas.iout_a > LLC_BURST_IOUT_EXIT_A) {
        if (s_llc_rt.burst_exit_delay_ms == 0U) {
					 s_llc_rt.burst_exit_delay_ms = g_ms;
				}else if(elapsed_reached(s_llc_rt.burst_exit_delay_ms, LLC_BURST_EXIT_DELAY_MS))
				{
					  float f_resume;
            s_llc_rt.burst_exit_delay_ms = 0U;
            /*
             * 退出Burst：先恢复PWM，再恢复进入Burst前的频率点，
             * 然后做bumpless，避免回RUN瞬间频率跳变。
             */			
				    f_resume = f_clampf(s_llc_rt.f_pre_burst_hz, s_llc.f_min, s_llc.f_max);

            llc_pwm_outputs_enable(1);
            s_burst_pwm_on = 1U;
						s_llc.f_cmd = f_resume;
            llc_set_freq(f_resume, true);
            llc_ctrl_bumpless_init(LLC_VOUT_TARGET_V, vout, f_resume);
            
#if DEBUG_PRINTF_BURST_MODE
            debug_printf("[BURST] EXIT: iout=%.2fA, f_resume=%.0fHz, vout=%.2fV, bst=%d\n",
                         s_llc_rt.meas.iout_a,
                         f_resume,
                         vout,
                         s_llc_rt.burst_state);
#endif
            llc_state_enter(ST_LLC_RUN);
        }
    } else {
        s_llc_rt.burst_exit_delay_ms = 0U;
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
			/*
			 * FAULT 恢复策略：
			 * 1. 硬件故障(BKIN)清除：等 BKIN 电平恢复高 + 软件锁存清除
			 * 2. 软件故障(OVP/OCP/UVP)清除：等测量值恢复正常
			 * 3. 两种情况都满足后，延时 FAULT_RECOVER_MS 防抖，然后回到 ST_IDLE
			 *
			 * 注意：protect_clear_fault() 由外部调用（如 PFC 恢复流程/重启命令），
			 * 此处仅检查故障是否已解除，不做主动清除。
			 */
			if (!llc_faults_present()) {
				if (s_llc_rt.fault_recover_begin_ms == 0U) {
					s_llc_rt.fault_recover_begin_ms = g_ms;
				}
				if (elapsed_reached(s_llc_rt.fault_recover_begin_ms, LLC_FAULT_RECOVER_MS)) {
					s_llc_rt.fault_recover_begin_ms = 0U;
					llc_state_enter(ST_IDLE);
				}
			} else {
				s_llc_rt.fault_recover_begin_ms = 0U;
			}
			break;
			
		default:
			break;
	 }
	
}

llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}





