#include "llc_control.h"
#include "float.h"
#include <stdarg.h>
#include <stdio.h>

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
    llc_app_ctx_t app; //应用状态上下文
    llc_meas_t meas; //测量值
	  uint32_t pfc_ready_begin_ms; //	pfc ready 开始时间
    uint32_t softstart_begin_ms; //软启动开始时间
    uint32_t stopping_begin_ms; //停机开始时间
    uint32_t hold_last_adjust_ms; //保持最后调整时间
	  uint32_t run_entry_hold_begin_ms; //运行入口保持开始时间
    uint32_t run_entry_stable_ticks; //运行入口稳定tick
}llc_runtime_ctx_t;

static llc_runtime_ctx_t s_llc_rt; //运行时上下文

static llc_app_ctx_t s_llc_app; //应用上下文
static llc_t s_llc; //llc上下文

typedef struct {
    float iref; 	//参考电流
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
    bool limit_active; //限流激活标志
} llc_curr_t;

static llc_curr_t s_llc_curr;

enum{
	LLC_START_DELAY_MS = 10000
};

static llc_mode_t s_llc_mode = LLC_MODE_NORMAL;
static bool s_llc_adc_enable = false;

/* 故障快照结构：20us快环记录，1ms慢环打印 */
typedef struct {
    bool ovp_active;        /* 过压故障 */
    bool ocp_active;        /* 过流故障 */
    bool vbus_uv_active;    /* 母线欠压故障 */
    float vout_snapshot;    /* 故障时Vout值 */
    float iout_snapshot;    /* 故障时Iout值 */
    float vbus_snapshot;    /* 故障时Vbus值 */
    bool state_changed;     /* 状态转换标志 */
    llc_state_t prev_state; /* 前一状态 */
    llc_state_t new_state;  /* 新状态 */
    uint32_t state_change_ms; /* 状态转换时间 */
    bool pending_print;     /* 待打印标志 */
} llc_fault_snapshot_t;

static llc_fault_snapshot_t s_fault_snapshot = {0};

/* 日志缓冲区：ISR记录，主循环打印 */
static llc_log_entry_t s_log_buf[LLC_LOG_BUF_SIZE];
static volatile uint8_t s_log_write_idx = 0;
static volatile uint8_t s_log_read_idx = 0;

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void llc_state_enter(llc_state_t next);
static void llc_update_measurements(void);
static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot);
static inline float conv_adc_to_i(uint16_t raw);
static void llc_set_freq(float hz);
static float llc_loop_scan_freq_from_adc(void);
static bool llc_precheck_ok(void);
static bool llc_pfc_ready_stable(bool enable_llc);
static bool llc_faults_present(void);
static void llc_enter_fault(void);

//static float llc_ctrl_step(float e, bool en, bool limit_active, float f_init, float f_track);
static float llc_ctrl_step(float e, bool en, bool limit_active, float f_init, float f_track, float f_nom_ff);
static float llc_bumpless_integ(float f0, float e0, float kp, float f_nom, float f_min, float f_max);
static float llc_current_limit_step(float i_meas,bool en);
/*********************************************************************************************************
*                                              静态工具
*********************************************************************************************************/
static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot)
{
    float v_adc = (raw * VREF_ADC) / 4095.0f;  /* ADC输入电压 */
    return v_adc * ((rtop + rbot) / rbot);      /* 分压比换算到实际电压 */
}
static inline float conv_adc_to_i(uint16_t raw)
{
    float v = (raw * VREF_ADC) / 4095.0f;
    float v_net = v - 0.327f;
    return v_net / (ISHUNT_OHM * IAMP_GAIN);
}
#if 0
static float llc_loop_scan_freq_from_adc(void)
{
	float k = 0.41240f;   //130 --> 0.465   80
	// 0.465

	/* 一阶低通滤波，alpha = 0.005 对应截止频率约 40Hz (50kHz采样)
	 * 公式：fc ≈ alpha * fs / (2*pi)，fs = 50000Hz
	 * 较小的alpha提供更平滑的滤波效果
	 */
	static float vadc_filtered = 0.0f;
	const float alpha = 0.1f;
	
	float vadc_v = ((float)g_adc_multi.vout_raw * VREF_ADC) / 4095.0f;
	
	/* 首次调用初始化 */
	if (vadc_filtered < 0.001f) {
		vadc_filtered = vadc_v;
	}
	
	/* 低通滤波: y[n] = (1-alpha)*y[n-1] + alpha*x[n] */
	vadc_filtered = (1.0f - alpha) * vadc_filtered + alpha * vadc_v;
	
	if (vadc_filtered < LLC_LOOP_SCAN_ADC_MIN_V) {
			return LLC_LOOP_SCAN_FREQ_HZ;
	}
	float vad_test = vadc_filtered/3.3*4095;
	// 1.565  1.625
	float pctrl = k * vad_test;


	return LLC_LOOP_SCAN_CTRL_CLK_HZ / pctrl;
}
#else
static float llc_loop_scan_freq_from_adc(void)
{
    enum {
			  /* Q12定点系数
         * K = 0.41240
         * K_Q12 = round(0.41240 * 4096) = round(1689.1904) = 1689
         * 实际等效系数 = 1689 / 4096 = 0.412353515625
         */
        K_Q12 = 1689    /* round(0.41240 * 4096) */
    };
		const float f_step_max = 500.0f;
		/* 最小ADC阈值:
     * adc_min_code = round(LLC_LOOP_SCAN_ADC_MIN_V / VREF_ADC * 4095)
     * 若ADC低于该值，则认为注入无效，保持上一频率
     */
    const uint16_t adc_min_code = (uint16_t)((LLC_LOOP_SCAN_ADC_MIN_V / VREF_ADC) * 4095.0f + 0.5f);
		
		static uint32_t adc_filtered_q4 = 0;   // ADC滤波值，Q4提高一点分辨率
    static float f_last = LLC_LOOP_SCAN_FREQ_HZ;
		
		uint16_t adc_now = g_adc_multi.vout_raw;
    /* Q4滤波状态:
     * adc_filtered_q4 = adc_filtered * 16
     * 用Q4提高滤波过程的小数分辨率
     */
    if (adc_filtered_q4 == 0U) {
		/* 首次初始化:
     * adc_filtered_q4 = adc_now << 4
     * f_last = 当前LLC频率命令
     */
        adc_filtered_q4 = ((uint32_t)adc_now << 4);
			/* 上一次输出频率 */
        f_last = s_llc.f_cmd;
    }
		/* alpha = 1/8: y += (x - y)/8 */
		/* 一阶IIR滤波, alpha = 1/8
     * x_q4 = adc_now * 16
     * adc_filtered_q4 = adc_filtered_q4 + (x_q4 - adc_filtered_q4)/8
     */
    {
        uint32_t x_q4 = ((uint32_t)adc_now << 4);
        adc_filtered_q4 += (x_q4 - adc_filtered_q4) >> 3;
    }
		uint16_t adc_filtered = (uint16_t)(adc_filtered_q4 >> 4);
    /* ADC过低，保持上一频率 */
    if (adc_filtered < adc_min_code) {
        return f_last;
    }
		 /* 定点计算 pctrl
     * pctrl = floor(adc_filtered * K_Q12 / 4096)
     *       = floor(adc_filtered * 1689 / 4096)
     *       ≈ adc_filtered * 0.412353515625
     */
    uint32_t pctrl = ((uint32_t)adc_filtered * (uint32_t)K_Q12) >> 12;
    if (pctrl < 2U) {
        pctrl = 2U;
    }
		 /* 频率计算
     * f_target = CTRL_CLK / pctrl
     *          = 120000000 / pctrl
     */
		float f_target = LLC_LOOP_SCAN_CTRL_CLK_HZ / (float)pctrl;
    f_target = f_clampf(f_target, s_llc.f_min, s_llc.f_max);
		
		
		//频率步进限制
		float df = f_target - f_last;
    if (df > f_step_max) {
        f_target = f_last + f_step_max;
    } else if (df < -f_step_max) {
        f_target = f_last - f_step_max;
    }

    f_last = f_target;
    return f_target;
}
#endif
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

static float llc_vbus_feedforward_hz(float vbus_v)
{
#if (LLC_VBUS_FF_EN)
    // 低通滤波截止频率约200Hz (alpha = 0.1 @ 10kHz更新)
    static float vbus_filtered = 400.0f;
    const float alpha = 0.1f;
    vbus_filtered = alpha * vbus_v + (1.0f - alpha) * vbus_filtered;
    
    float ff = (vbus_filtered - LLC_VBUS_FF_VNOM_V) * LLC_VBUS_FF_GAIN_HZ_PER_V;
    return f_clampf(ff, -LLC_VBUS_FF_MAX_HZ, LLC_VBUS_FF_MAX_HZ);
#else
    (void)vbus_v;
    return 0.0f;
#endif
}

static const char* llc_state_str(llc_state_t s)
{
    switch (s) {
    case ST_IDLE: return "IDLE";
    case ST_PRECHECK: return "PRECHECK";
    case ST_SOFTSTART: return "SOFTSTART";
    case ST_RUN_ENTRY_HOLD: return "RUN_ENTRY_HOLD";
    case ST_LLC_RUN: return "RUN";
    case ST_STOPPING: return "STOPPING";
    case ST_FAULT: return "FAULT";
    default: return "UNKNOWN";
    }
}

void llc_set_mode(llc_mode_t mode)
{
    s_llc_mode = mode;
	   /* reset controller */
    (void)llc_ctrl_step(0,false,false,s_llc.f_cmd,s_llc.f_cmd,0);
    (void)llc_current_limit_step(0,false);
}

/*********************************************************************************************************
* 函数名称：llc_ctrl_step
* 函数功能：
* 输入参数：e 电压误差：v_ref - v_meas; en 控制器使能标志; limit_active 电流限制激活标志
						f_init 初始频率（用于使能时的初始化）; f_track 跟踪频率（电流限制激活时使用）
* 输出参数：
* 返 回 值：
* 创建日期：2026年01月21日
* 注    意：
*********************************************************************************************************/
static float llc_ctrl_step(float e, bool en, bool limit_active, float f_init, float f_track, float f_nom_ff)
{
	  static bool en_z1 = false;  // 记录上一次的使能状态
    float kp = s_llc.kp;
    float ki = s_llc.ki;
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_slew = s_llc.f_slew;
	  float f_nom = s_llc.f_nom + f_nom_ff;  //额定频率（与软启动末拍对齐）
    float e_db = s_llc.e_db; 
    float f_q_step = s_llc.f_q_step;  //频率量化步进
	
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
		if (f_nom <= 0.0f) {
			f_nom = 0.5f * (f_min + f_max);
		}
		if(f_q_step<0.0f)
		{
			 f_q_step = 0.0f;
		}
		if (!en) {
			s_llc.integ = 0.0f; // 重置积分器
			s_llc.f_cmd = f_init; // 设置初始频率
			en_z1 = false;  // 清除使能记忆
			return s_llc.f_cmd;
		}
		
		//死区处理避免控制器对微小误差的过度反应，应用场景：减少噪声引起的振荡，提高系统稳定性
		float e_pi = (fabsf(e) < e_db) ? 0.0f : e;
		if (!en_z1) {
			s_llc.f_cmd = f_init;
			//s_llc.integ = (f_nom - f_init) - kp * e;
			//调用专用函数计算积分初始值，实现“无扰切换”（bumpless transfer）。
			s_llc.integ = llc_bumpless_integ(f_init, e_pi, kp, f_nom, f_min, f_max);
			en_z1 = true; // 记录上一次的使能状态为已经使能
    }

    float f_prev = s_llc.f_cmd; // 上一次的频率
		float f_sat = f_prev; // 限幅后的频率
		
		if (limit_active) {
			//优先级：电流保护 > 电压控制作用：当电流过大时，放弃电压环，跟踪电流环提供的频率
        float f_t = f_clampf(f_track, f_min, f_max); //	电流限制时，跟踪频率
        s_llc.integ = (f_nom - f_t) - kp * e_pi; // 重置积分器以防积分风up
        f_sat = f_t; // 设置限幅后的频率为跟踪频率
        //f_prev = f_t; //	更新上一次的频率
    } 
		else {
        float x_candidate = s_llc.integ + ki * e_pi; //候选积分值
			//u 是 PI 控制器根据电压误差计算出的频率调整量，决定了当前周期应当向 f_nom 增加或减少多少频率，以维持输出电压稳定。它是电压环的核心输出，
			//后续经过限幅、斜率限制等保护环节，最终生成安全的开关频率命令 f_cmd。
        float u = kp * e_pi + x_candidate; //电压控制作用
        float f_req = f_nom - u; //频率请求 = 额定频率 - 电压环的控制量
        f_sat = f_clampf(f_req, f_min, f_max); //频率限幅
			//f_sat 是 经过限幅（钳位）处理后的安全频率，确保输出频率在允许的硬件范围 [f_min, f_max] 内。
        if (fabsf(f_req - f_sat)>1e-6f) { 
            s_llc.integ = (f_nom - f_sat) - kp * e_pi; // 重置积分器以防积分风up
        } else {
            s_llc.integ = x_candidate; // 接受候选积分值
        }
    }
		
		float df = f_sat - f_prev; //频率增量
    if (df > f_slew) {
        s_llc.f_cmd = f_prev + f_slew; // 限制频率增量
    } else if (df < -f_slew) {
        s_llc.f_cmd = f_prev - f_slew; // 限制频率负增量
    } else {
        s_llc.f_cmd = f_sat;
    }
		
    if (f_q_step > 0.0f) {
        s_llc.f_cmd = roundf(s_llc.f_cmd / f_q_step) * f_q_step; // 量化频率
    }
    s_llc.f_cmd = f_clampf(s_llc.f_cmd, f_min, f_max); // 频率限幅
		
	  return s_llc.f_cmd;
}
/*********************************************************************************************************
* 函数名称：llc_current_limit_step
* 函数功能：电流限流PI控制器（带迟滞和抗饱和）
* 输入参数：i_meas 测量电流值（A）  使能控制（false时复位积分器并返回0）
* 输出参数：频率增量（Hz）
* 返 回 值：
* 创建日期：2026年01月22日
* 注    意：静态函数，返回频率增量（Hz）
*	算法概述：1. 迟滞切换：当测量电流 i_meas ≥ i_on 时激活限流模式，当 i_meas ≤ i_off 时退出
						2. PI控制：计算频率增量 u = kp*ierr + integ，其中 ierr = i_meas - iref（经限幅）
					  3. 抗饱和：当输出饱和时（u>df_max 或 u<0），采用反计算法调整积分器
						4. 斜率限制：限制输出变化率不超过 df_slew
						5. 输出钳位：最终输出限制在 [0, df_max] 范围内
*********************************************************************************************************/
static float llc_current_limit_step(float i_meas,bool en)
{
	 // 从全局结构体提取控制器参数（避免多次访问）
    float iref = s_llc_curr.iref;  // 电流参考值（A）
    float i_err_sat = s_llc_curr.i_err_sat;  // 误差限幅值
    float kp = s_llc_curr.kp; // 比例系数（Hz/A）
    float ki = s_llc_curr.ki; // 积分系数（Hz/(A·周期)）
    float df_max = s_llc_curr.df_max; // 最大频率增量（Hz）
    float df_slew = s_llc_curr.df_slew; // 频率变化率限制（Hz/周期）

    if (!en) {
        s_llc_curr.integ = 0.0f; // 清零积分器
        s_llc_curr.df_prev = 0.0f; // 清零上一周期输出
        s_llc_curr.limit_active = false; // 退出限流模式
        return 0.0f;
    }

		s_llc_curr.imeas = i_meas;  // 保存测量值供调试/监视
		// 迟滞切换逻辑：判断是否进入/退出限流模式
    if (!s_llc_curr.limit_active) { // 当前未处于限流模式
        if (i_meas >= s_llc_curr.i_on) { // 若电流超过开启阈值
            s_llc_curr.limit_active = true; // 激活限流模式
        }
    } else {
        if (i_meas <= s_llc_curr.i_off) { // 若电流低于关闭阈值
            s_llc_curr.limit_active = false; // 退出限流模式
        }
			}
    
		float df_sat = 0.0f;    // 饱和后的频率增量（未经过斜率限制）
		// 根据限流模式状态决定处理逻辑
    if (!s_llc_curr.limit_active) { // 非限流模式
        s_llc_curr.integ = 0.0f;    // 清零积分器（防止积分漂移）
        df_sat = 0.0f;              // 输出为0
    } else {												// 限流模式激活，执行PI控制
		//电压环采用 “标称频率减去控制量” 的结构（f_req = f_nom - u），因此误差定义为 v_ref - v_meas，使控制量 u 与频率变化 反向。
    //电流环采用 “最小频率加上增量” 的结构（f_cmd_i = f_min + df_i），因此误差定义为 i_meas - iref，使增量 df_i 与频率变化 同向。
        float ierr = i_meas - iref; // 计算电流误差（测量值 - 参考值）
        if (ierr > i_err_sat) {     // 误差限幅，防止积分器过度累积
            ierr = i_err_sat;       // 正向饱和
        } else if (ierr < -i_err_sat) {
            ierr = -i_err_sat;      // 负向饱和
        }
				// 积分器前向计算（候选值）
        float x_candidate = s_llc_curr.integ + ki * ierr;
				// PI控制器输出：比例项 + 积分候选值
        float u = kp * ierr + x_candidate;
				// 输出饱和处理（抗饱和逻辑的前半部分）
        if (u > df_max) {
            df_sat = df_max;  // 正向饱和至最大频率增量
        } else if (u < 0.0f) {
            df_sat = 0.0f;    // 负向饱和至0（频率不能为负）
        } else {
            df_sat = u;       // 未饱和，直接使用计算值
        }
				// 抗饱和积分器调整（反计算法）
        if (fabsf(u - df_sat) > 1e-6f) { // 如果输出发生了饱和（容差1e?6）
            s_llc_curr.integ = df_sat - kp * ierr;  // 重新计算积分器，使输出恰好等于饱和值
        } else {
            s_llc_curr.integ = x_candidate; // 未饱和，使用候选值更新积分器
        }
				// 积分器钳位，防止积分器溢出
        s_llc_curr.integ = f_clampf(s_llc_curr.integ, -df_max, df_max);
    }
		// 斜率（变化率）限制
		float ddf = df_sat - s_llc_curr.df_prev; // 计算本次饱和输出与上一周期最终输出的差值
    float df_i = df_sat;  // 初始值为饱和输出
    if (ddf > df_slew) {  // 如果正向变化超过允许斜率
        df_i = s_llc_curr.df_prev + df_slew; // 只增加允许的最大变化量
    } else if (ddf < -df_slew) {// 如果负向变化超过允许斜率
        df_i = s_llc_curr.df_prev - df_slew;// 只减少允许的最大变化量
    }    
	// 最终输出钳位
    df_i = f_clampf(df_i, 0.0f, df_max); // 确保输出在[0, df_max]范围内
    s_llc_curr.df_prev = df_i;  // 保存本次输出，供下一周期使用
    return df_i;
}


static uint32_t dbg_tick = 0;
static void llc_update_measurements(void)
{
	  s_llc_rt.meas.vbus_v = pfc_bus_voltage();
	
		if(!s_llc_adc_enable)
		{
			s_llc_rt.meas.vout_v = 0.0f;
			s_llc_rt.meas.iout_a = 0.0f;
			s_llc.vmeas = 0.0f;
			return;
		}
    s_llc_rt.meas.vout_v = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    s_llc_rt.meas.iout_a = conv_adc_to_i(g_adc_multi.isense_raw);
  
    s_llc.vmeas = s_llc_rt.meas.vout_v;
}

static bool llc_precheck_ok(void)
{
    return (s_llc_rt.meas.vbus_v >= LLC_VBUS_MIN_START_V);
}

static bool llc_pfc_ready_stable(bool enable_llc)
{
    if (!enable_llc) {
        s_llc_rt.pfc_ready_begin_ms = 0U; // 重置PFC准备开始时间
        return false;
    }

    if (s_llc_rt.pfc_ready_begin_ms == 0U) {
        s_llc_rt.pfc_ready_begin_ms = g_ms; // 记录PFC准备开始时间
    }

    return elapsed_reached(s_llc_rt.pfc_ready_begin_ms, PFC_READY_STABLE_BEFORE_LLC_MS); // 判断PFC是否准备稳定
}

static bool llc_faults_present(void)
{
    bool fault = false;
    
    /* 20us快环：只记录故障快照，不打印
     * 注意：只记录第一次故障值，避免被后续覆盖
     */
    /* IDLE / PRECHECK 不做输出故障判断 */
    if (s_llc_rt.app.state == ST_IDLE || s_llc_rt.app.state == ST_PRECHECK) {
        return false;
    }
		if (s_llc_adc_enable){
			if (s_llc_rt.meas.vout_v > LLC_VOUT_OVP_V) {
					if (!s_fault_snapshot.ovp_active) {  /* 首次触发才记录 */
							s_fault_snapshot.ovp_active = true;
							s_fault_snapshot.vout_snapshot = s_llc_rt.meas.vout_v;
					}
					fault = true;
			}

			if (s_llc_rt.meas.iout_a > LLC_IOUT_OCP_A) {
					if (!s_fault_snapshot.ocp_active) {  /* 首次触发才记录 */
							s_fault_snapshot.ocp_active = true;
							s_fault_snapshot.iout_snapshot = s_llc_rt.meas.iout_a;
					}
					fault = true;
			}
		}
		/* VBUS欠压建议只在 RUN / HOLD / SOFTSTART 阶段判断 */
		if (s_llc_rt.app.state == ST_SOFTSTART ||
        s_llc_rt.app.state == ST_RUN_ENTRY_HOLD ||
        s_llc_rt.app.state == ST_LLC_RUN) {
				if (s_llc_rt.meas.vbus_v < (LLC_VBUS_MIN_START_V - LLC_VOUT_HYST_V)) {
						if (!s_fault_snapshot.vbus_uv_active) {  /* 首次触发才记录 */
								s_fault_snapshot.vbus_uv_active = true;
								s_fault_snapshot.vbus_snapshot = s_llc_rt.meas.vbus_v;
						}
						fault = true;
				}
			}
    
    /* 标记有待打印的故障 */
    if (fault) {
        s_fault_snapshot.pending_print = true;
    }

    return fault;
}

/* 1ms慢环：统一打印故障和状态转换信息 */
void llc_fault_and_state_print(void)
{
    if (!s_fault_snapshot.pending_print) {
        return;
    }
    
    /* 打印状态转换 */
    if (s_fault_snapshot.state_changed) {
        debug_printf("LLC STATE: %s -> %s  t=%lu ms\r\n",
                     llc_state_str(s_fault_snapshot.prev_state),
                     llc_state_str(s_fault_snapshot.new_state),
                     s_fault_snapshot.state_change_ms);
        s_fault_snapshot.state_changed = false;
    }
    
    /* 打印故障信息 */
    if (s_fault_snapshot.ovp_active) {
        debug_printf("[FAULT] OVP! Vout=%.1fV > %.1fV\r\n", 
                     s_fault_snapshot.vout_snapshot, LLC_VOUT_OVP_V);
        s_fault_snapshot.ovp_active = false;
    }
    
    if (s_fault_snapshot.ocp_active) {
        debug_printf("[FAULT] OCP! Iout=%.2fA > %.2fA\r\n", 
                     s_fault_snapshot.iout_snapshot, LLC_IOUT_OCP_A);
        s_fault_snapshot.ocp_active = false;
    }
    
    if (s_fault_snapshot.vbus_uv_active) {
        debug_printf("[FAULT] VBUS_UV! Vbus=%.1fV\r\n", 
                     s_fault_snapshot.vbus_snapshot);
        s_fault_snapshot.vbus_uv_active = false;
    }
    
    s_fault_snapshot.pending_print = false;
}
static void llc_enter_fault(void)
{
    llc_state_enter(ST_FAULT);
}

/* ISR中调用：记录日志到缓冲区（非打印） */
static void llc_log_isr(const char* fmt, ...)
{
    uint8_t idx = s_log_write_idx;
    llc_log_entry_t* entry = &s_log_buf[idx];
    
    if (entry->valid) {
        return;  /* 缓冲区满，丢弃 */
    }
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry->msg, sizeof(entry->msg), fmt, args);
    va_end(args);
    
    entry->timestamp = g_ms;
    entry->valid = true;
    
    s_log_write_idx = (idx + 1) % LLC_LOG_BUF_SIZE;
}

/* 主循环调用：刷新并打印所有日志 */
void llc_log_flush(void)
{
    while (s_log_read_idx != s_log_write_idx) {
        llc_log_entry_t* entry = &s_log_buf[s_log_read_idx];
        
        if (entry->valid) {
            debug_printf("[%lu] %s\r\n", entry->timestamp, entry->msg);
            entry->valid = false;
        }
        
        s_log_read_idx = (s_log_read_idx + 1) % LLC_LOG_BUF_SIZE;
    }
}

static float llc_bumpless_integ(float f0, float e0, float kp, float f_nom, float f_min, float f_max)
{
    float u0    = f_nom - f0;              // 因为：f = f_nom - u
    float integ = u0 - kp * e0;            // u = kp*e + integ

    float i_lim = f_max - f_min;           // 积分限幅（单位：Hz 等价）
    return f_clampf(integ, -i_lim, i_lim); // 返回积分值，限幅在 -i_lim 和 i_lim 之间
}
/*********************************************************************************************************
*                                              状态机核心
*********************************************************************************************************/
static void llc_state_enter(llc_state_t next)
{
	// 更新 LLC 状态和进入时间
	if (s_llc_rt.app.state == next) 
	{
		return;
	}
	/* 状态转换记录到快照，1ms统一打印
	 * 只记录首次跳变，避免被后续覆盖
	 */
	if (!s_fault_snapshot.state_changed) {
		s_fault_snapshot.state_changed = true;
		s_fault_snapshot.prev_state = s_llc_rt.app.state;
		s_fault_snapshot.new_state = next;
		s_fault_snapshot.state_change_ms = g_ms;
	}
	s_fault_snapshot.pending_print = true;
	
	s_llc_rt.app.state = next; 
	s_llc_rt.app.entry_ms = g_ms; // 记录进入时间
	if (next != ST_LLC_RUN) {
		(void)llc_ctrl_step(0.0f, false, false, s_llc.f_cmd, s_llc.f_cmd,0.0f);
		(void)llc_current_limit_step(0.0f, false);
	}
	#if Bus_Adj
	//bus_vol_adj_reset();   //重置总线电压调整逻辑 百分之五十的占空比
	#endif
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
		case ST_IDLE:{
		  llc_softstart_on_fault();
		  llc_pwm_outputs_enable(0); // 禁用 PWM 输出
		  llc_driver_en_set(false); //disable llc
		  llc_set_freq(s_llc.f_max);

 		  s_llc_rt.softstart_begin_ms = 0U;  //软启动开始时间戳，用于控制软启动斜坡
 		  s_llc_rt.stopping_begin_ms = 0U;  //停机过程开始时间戳，用于控制停机时序
 		  s_llc_rt.hold_last_adjust_ms = 0U; //保持最后调整的时间戳，用于频率调整的去抖
		  s_llc_rt.pfc_ready_begin_ms = 0U; // PFC 准备开始时间戳，用于控制 PFC 准备稳定
		  s_llc_rt.run_entry_hold_begin_ms = 0U; //运行入口保持开始时间戳，用于控制运行入口保持时间
		  s_llc_rt.run_entry_stable_ticks = 0U; //运行入口稳定计数器，用于控制运行入口稳定时间
		  break;
		}
	 case ST_PRECHECK:  
	 {
	  	llc_driver_en_set(true); //	enable llc
    	llc_pwm_outputs_enable(0); // 禁用 PWM 输出
    	llc_set_freq(s_llc.f_max);
      break;
	 }
	 case ST_SOFTSTART:
	 {
			s_llc_rt.softstart_begin_ms = g_ms; //	记录软启动开始时间戳
			llc_driver_en_set(true);
	 		llc_softstart_start(LLC_SOFTSTART_TARGET_HZ);
			llc_pwm_outputs_enable(1);
			break;
	 }
	 case ST_RUN_ENTRY_HOLD:
	 {
			llc_driver_en_set(true);
			s_llc_rt.run_entry_hold_begin_ms = g_ms; // 记录运行入口保持开始时间戳
			s_llc_rt.run_entry_stable_ticks = 0U;  // 重置运行入口稳定计数器
			s_llc.f_cmd = llc_softstart_last_hz(); // 设置目标频率为软启动最后的频率
			llc_set_freq(s_llc.f_cmd);		  
			break;
	 }
	 case ST_LLC_RUN:
	 {
			llc_driver_en_set(true);
      s_llc_rt.hold_last_adjust_ms = g_ms; // 记录保持最后调整的时间戳
			s_llc_curr.df_prev = 0.0f;
			s_llc_curr.limit_active = false; // 重置限流器状态
		 
			/* 扫描模式特殊初始化 */
			if(s_llc_mode == LLC_MODE_LOOP_SCAN)
			{
					s_llc.f_cmd = llc_loop_scan_freq_from_adc();
					(void)llc_ctrl_step(0.0f, false, false, s_llc.f_cmd, s_llc.f_cmd, 0.0f);
					(void)llc_current_limit_step(0.0f, false);
			}
			break;
	 }
	 case ST_STOPPING:
	 {
      s_llc_rt.stopping_begin_ms = g_ms;
		  llc_driver_en_set(true);
      llc_set_freq(s_llc.f_max);
      break;
	 }
	 case ST_FAULT:
	 {
		 	llc_softstart_on_fault();
			llc_pwm_outputs_enable(0);
			llc_driver_en_set(false);
			llc_set_freq(s_llc.f_max);
			break;
	 }
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
	/* PC7 GPIO初始化：用于20us task执行时间测试 */
	rcu_periph_clock_enable(RCU_GPIOC);
	gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
	gpio_bit_reset(GPIOC, GPIO_PIN_7);
	
	llc_softstart_init();
	s_llc = (llc_t){
				.vref=LLC_VOUT_TARGET_V, .vmeas=0.0f,
				.kp=LLC_VCTRL_KP, .ki=LLC_VCTRL_KI, .integ=0.0f,
				.f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ,  .f_slew=LLC_F_SLEW_HZ,
				.f_nom=LLC_VCTRL_F_NOM_HZ, .e_db=LLC_VCTRL_E_DB_V, .f_q_step=LLC_VCTRL_F_Q_STEP_HZ
		};
	s_llc_curr = (llc_curr_t){
				.iref=LLC_IOUT_TARGET_A, .imeas=0.0f, .i_err_sat=LLC_IOUT_ERR_SAT_A,
				.kp=LLC_IOUT_CTRL_KP, .ki=LLC_IOUT_CTRL_KI * LLC_CTRL_TS_S, .integ=0.0f,
				.f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ,
				.df_max=LLC_IOUT_DF_MAX_HZ, .df_slew=LLC_IOUT_DF_SLEW_HZ_S * LLC_CTRL_TS_S, .df_prev=0.0f,
				.i_on=LLC_IOUT_TARGET_A + LLC_IOUT_ON_DELTA_A,
				.i_off=LLC_IOUT_TARGET_A - LLC_IOUT_OFF_DELTA_A,
				.limit_active=false
	};
	s_llc_rt.app.state = (llc_state_t)0xFF;
	llc_state_enter(ST_IDLE);
}
void llc_app_tick_adc_test(void)
{
	llc_update_measurements();
}
/* 20us 优化版本：分离1ms任务，简化运算 */
static volatile uint32_t s_tick_1ms_cnt = 0;  // 1ms分频计数器

/* 20us核心任务：极简版，仅保留SCAN模式必要功能
 * - 故障检测
 * - IDLE→PRECHECK 转换
 * - RUN状态：SCAN频率实时更新
 */
static void llc_app_tick_20us_core(void)
{
	llc_update_measurements();
	static uint8_t scan_div = 0U;
	/* 故障检测 */
	if (llc_faults_present()) {
		llc_state_enter(ST_FAULT);
		return;
	}
	bool enable_llc = pfc_is_ready();
	switch(s_llc_rt.app.state)
	{
    case ST_IDLE:
        if (llc_pfc_ready_stable(enable_llc) && llc_precheck_ok()) {
            llc_state_enter(ST_PRECHECK);
        }
        break;

    case ST_PRECHECK:
        if (!enable_llc || !llc_precheck_ok()) {
            llc_state_enter(ST_STOPPING);
        }
        break;

    case ST_SOFTSTART:
        if (!enable_llc || !llc_precheck_ok()) {
            llc_state_enter(ST_STOPPING);
        }
        break;

    case ST_RUN_ENTRY_HOLD:
        if (!enable_llc || !llc_precheck_ok()) {
            llc_state_enter(ST_STOPPING);
        }
        break;
			
		case ST_LLC_RUN:
        if (!enable_llc || (s_llc_rt.meas.vbus_v < (LLC_VBUS_MIN_START_V - LLC_VOUT_HYST_V))) {
            llc_state_enter(ST_STOPPING);
            break;
        }

        if (s_llc_mode == LLC_MODE_LOOP_SCAN) {
            scan_div++;
            if (scan_div >= 5U) {   /* 100us更新一次 */
                scan_div = 0U;
                s_llc.f_cmd = llc_loop_scan_freq_from_adc();
                llc_set_freq(s_llc.f_cmd);
            }
        }
        break;
			
		default:
			/* 其他状态由100us/1ms任务处理 */
			break;
	}
}

/* 软启流程统一任务：由主循环每1ms调用
 * 处理PRECHECK→SOFTSTART→RUN_ENTRY_HOLD→RUN完整流程
 */
void llc_app_tick_1ms_core(void)
{
	s_tick_1ms_cnt++;

  llc_fault_and_state_print();
	switch(s_llc_rt.app.state)
	{
		case ST_PRECHECK:
			/* 100ms后进入软启 */
			if (elapsed_reached(s_llc_rt.app.entry_ms, 100U)) {
				llc_state_enter(ST_SOFTSTART);
			}
			break;
			
		case ST_SOFTSTART:
			/* 软启tick */
			llc_softstart_tick_1khz();
			
			/* 软启完成进入RUN_ENTRY_HOLD */
			if(elapsed_reached(s_llc_rt.softstart_begin_ms, 
			                   LLC_SOFTSTART_DURATION_MS + LLC_SOFTSTART_STABILIZE_MS)) {
				llc_softstart_stop();
				s_llc.f_cmd = llc_softstart_last_hz();
				s_llc_adc_enable = true;
				llc_state_enter(ST_RUN_ENTRY_HOLD);
			}
			break;
			
		case ST_RUN_ENTRY_HOLD:
        if (s_llc_mode == LLC_MODE_LOOP_SCAN) {
            if (elapsed_reached(s_llc_rt.run_entry_hold_begin_ms, LLC_RUN_ENTRY_HOLD_MS)) {
                llc_state_enter(ST_LLC_RUN);
            }
        } else {
            if (elapsed_reached(s_llc_rt.run_entry_hold_begin_ms, LLC_RUN_ENTRY_HOLD_MS) &&
                s_llc_rt.run_entry_stable_ticks >= LLC_RUN_ENTRY_STABLE_TICKS) {
                llc_state_enter(ST_LLC_RUN);
            } else if (elapsed_reached(s_llc_rt.run_entry_hold_begin_ms, LLC_RUN_ENTRY_TIMEOUT_MS)) {
                llc_state_enter(ST_STOPPING);
            }
        }
        break;
			
		case ST_LLC_RUN:
        if (s_llc_mode == LLC_MODE_LOOP_SCAN) {
            (void)llc_ctrl_step(0.0f, false, false, s_llc.f_cmd, s_llc.f_cmd, 0.0f);

            if ((s_tick_1ms_cnt % 1000U) == 0U) {
                debug_printf("SCAN: adc=%u f=%.0f\r\n", g_adc_multi.vout_raw, s_llc.f_cmd);
            }
        } else {
            s_llc.vmeas = s_llc_rt.meas.vout_v;
            float err = s_llc.vref - s_llc.vmeas;
            float f_init = s_llc.f_cmd;

            (void)llc_current_limit_step(0.0f, false);

            float f_nom_ff = llc_vbus_feedforward_hz(s_llc_rt.meas.vbus_v);
            float f_cmd = llc_ctrl_step(err, true, false, f_init, f_init, f_nom_ff);
            llc_set_freq(f_cmd);

            if ((s_tick_1ms_cnt % 1000U) == 0U) {
                debug_printf("RUN: V=%.1f f=%.0f\r\n", s_llc.vmeas, s_llc.f_cmd);
            }
        }
        break;

			case ST_STOPPING:
        if (elapsed_reached(s_llc_rt.stopping_begin_ms, LLC_STOPPING_FREQ_HOLD_MS)) {
            llc_pwm_outputs_enable(0);
            llc_driver_en_set(false);
            llc_state_enter(ST_IDLE);
        }
        break;
		default:
			break;
	}
}

/* 20us周期入口：极简ISR，只执行高频任务
 * 软启流程已移至主循环1ms任务
 */
void llc_app_tick_20us(void)
{
	/* PC7 GPIO测试 */
	gpio_bit_set(GPIOC, GPIO_PIN_7);
	
	/* 20us高频任务（故障检测+SCAN频率更新） */
	llc_app_tick_20us_core();
	
	gpio_bit_reset(GPIOC, GPIO_PIN_7);
}

/* 兼容性保留：如果定时器仍是100us，用这个入口 */
void llc_app_tick_100us_compat(void)
{
	/* PC7 GPIO测试 */
	gpio_bit_set(GPIOC, GPIO_PIN_7);
	
	/* 20us高频任务（故障检测+SCAN频率更新） */
	llc_app_tick_20us_core();
	
	gpio_bit_reset(GPIOC, GPIO_PIN_7);
}

/* 向后兼容：llc_app_tick_100us 映射到兼容版本
 * 如果定时器仍是100us周期，调用此函数
 */
void llc_app_tick_100us(void)
{
	llc_app_tick_100us_compat();
}

llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}

