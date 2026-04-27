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
static llc_t s_llc; //llc上下文
static bool s_llc_run_request = false;
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
static bool llc_is_active_state(llc_state_t st);  /* 前向声明 */

//双环竞争版本的
static float llc_ctrl_step(float e, bool en, bool limit_active, float f_init, float f_track);
static float llc_bumpless_integ(float f0, float e0, float kp, float f_nom, float f_min, float f_max);
static float llc_current_limit_step(float i_meas,bool en);

static void llc_fan_tick(void); //风扇的task

static void llc_current_thresholds_update(void);
static void llc_derate_reset(void);
static void llc_derate_update(float i_meas, bool enable);
static bool llc_temp_derate_active(void);
static bool llc_overtemp_shutdown(void);
/*********************************************************************************************************
*                                              静态工具
*********************************************************************************************************/

void llc_set_run_request(bool en)
{
    s_llc_run_request = en;
}

bool llc_get_run_request(void)
{
    return s_llc_run_request;
}

void llc_set_vref(float vref)
{
    /* 给一点基础钳位，避免乱写 */
    s_llc.vref = f_clampf(vref, 0.0f, LLC_VOUT_OVP_V - 0.5f);
}

float llc_get_vref(void)
{
    return s_llc.vref;
}

/*********************************************************************************************************
* 函数名称：llc_set_iref
* 函数功能：设置LLC电流环参考值，实现电流限制值的平滑下调和动态阈值更新
* 输入参数：iref - 目标电流参考值（A）
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月08日
* 注    意：电流参考值只能平滑下调或首次设置，上调需通过降额恢复机制；同步更新电流保护阈值
*********************************************************************************************************/
void llc_set_iref(float iref)
{
    float i = f_clampf(iref, 0.0f, LLC_IOUT_TARGET_A);
    s_llc_curr.iref = i;
    if (s_llc_curr.iref_cmd > i || s_llc_curr.iref_cmd <= 0.0f) {
        s_llc_curr.iref_cmd = i;
    }
    llc_current_thresholds_update();
}

float llc_get_iref(void)
{
    return s_llc_curr.iref;
}

void llc_get_status(llc_status_t *st)
{
    if (st == NULL) {
        return;
    }

    st->vout_v = s_llc_rt.meas.vout_v;
    st->iout_a = s_llc_rt.meas.iout_a;
    st->vbus_v = s_llc_rt.meas.vbus_v;
    st->state  = s_llc_rt.app.state;
}

bool llc_is_fault_state(void)
{
    return (s_llc_rt.app.state == ST_FAULT);
}
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
#if (LLC_F_NOM_FOLLOW_MODE == LLC_F_NOM_FOLLOW_TARGET_FREQ)
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
                 (s_llc_rt.app.state == ST_LLC_RUN);

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

/*********************************************************************************************************
* 函数名称：llc_set_freq
* 函数功能：
* 输入参数：hz 目标频率
* 输出参数：
* 返 回 值：
* 创建日期：2026年04月08日
* 注    意：force_update 是否强制立即更新（见llc_pwm_set_freq说明）
*********************************************************************************************************/
static void llc_set_freq(float hz, bool force_update)
{
    float f = f_clampf(hz, s_llc.f_min, s_llc.f_max);
    s_llc.f_cmd = f;
    llc_pwm_set_freq((uint32_t)f, force_update);
}

/*********************************************************************************************************
* 函数名称：llc_ctrl_step
* 函数功能：
* 输入参数：e 电压误差：v_ref - v_meas; en 控制器使能标志; limit_active 电流限制激活标志
						f_init 初始频率（用于使能时的初始化）; f_track 跟踪频率（电流限制激活时使用）
* 输出参数：
* 返 回 值：
* 创建日期：2026年04月08日
* 注    意：
*********************************************************************************************************/
static float llc_ctrl_step(float e, bool en, bool limit_active, float f_init, float f_track)
{
    /* 使能记忆使用结构体字段，而非函数内 static，便于状态机切换时统一重置 */
	  float kp = s_llc.kp;
    float ki = s_llc.ki;
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_slew = s_llc.f_slew; 
	  float f_nom = s_llc.f_nom;  //额定频率（与软启动末拍对齐）
    float e_db = s_llc.e_db; 
    float f_q_step = s_llc.f_q_step;  //频率量化步进
	
    //1.f_min/f_max/kp/ki/f_nom/f_q_step 合法性检查
	  if (f_min <= 0.0f || f_min >= f_max) {
        f_min = 65000.0f;
        f_max = 250000.0f;
    }
    if (f_slew <= 0.0f) {
        f_slew = 3000.0f;
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
		//2.是否使能电压环
		if (!en) {
			s_llc.integ = 0.0f; // 重置积分器
			s_llc.f_cmd = f_init; // 设置电压环环路控制的初始频率
      s_llc.ctrl_en_z1 = false;  /* 清除使能记忆，下次进入时重新做 bumpless 初始化 */
			return s_llc.f_cmd;
		}
		if (e_db < 0.0f) 
			e_db = 0.0f;
		//死区处理避免控制器对微小误差的过度反应，应用场景：减少噪声引起的振荡，提高系统稳定性
		float e_pi = (fabsf(e) < e_db) ? 0.0f : e;
		//3.是否是首次执行
    if (!s_llc.ctrl_en_z1) {
			s_llc.f_cmd = f_init;
			//s_llc.integ = (f_nom - f_init) - kp * e;
			//调用专用函数计算积分初始值，实现“无扰切换”（bumpless transfer）。
			s_llc.integ = llc_bumpless_integ(f_init, e_pi, kp, f_nom, f_min, f_max); //u0 = f_nom -  f_init
      s_llc.ctrl_en_z1 = true;
    }
    float f_prev = s_llc.f_cmd; // 上一次的频率
		float f_sat = f_prev; // 限幅后的频率
		//优先级：电流保护 > 电压控制作用：当电流过大时，放弃电压环，跟踪电流环提供的频率
		if (limit_active) {
        float f_t = f_clampf(f_track, f_min, f_max); //	电流限制时，跟踪频率
        s_llc.integ = (f_nom - f_t) - kp * e_pi; // 重置积分器以防积分风up
        f_sat = f_t; // 设置限幅后的频率为跟踪频率
        //f_prev = f_t; //	更新上一次的频率
    } else {
      /* 电压 PI 控制：e>0(Vref>Vout) → 加功率 → 频率下降 */
			//u 是 PI 控制器根据电压误差计算出的频率调整量，决定了当前周期应当向 f_nom 增加或减少多少频率，以维持输出电压稳定。它是电压环的核心输出，
			//后续经过限幅、斜率限制等保护环节，最终生成安全的开关频率命令 f_cmd。
			float x_candidate = s_llc.integ + ki * e_pi;  //∫edt = s_llc.integ 积分的累加值  ki * e_pi 本次误差的积分贡献
			float u = kp * e_pi + x_candidate; //kp * e_pi 比例项即时响应
			 		
			/* LLC频率控制：
			 * e > 0 (Vref > Vout) => 需要加功率 => 频率下降
			 */
			float f_req = f_nom - u; //频率请求 = 额定频率 - 电压环的控制量
			f_sat = f_clampf(f_req, f_min, f_max);
			if (fabsf(f_req - f_sat) > 1e-6f) {
					/* 饱和：抗积分饱和 */
					s_llc.integ = (f_nom - f_sat) - kp * e_pi;
			} else {
					/* 未饱和：正常积分 */
					s_llc.integ = x_candidate;
			}
			
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
    if (f_q_step > 0.0f) {
        s_llc.f_cmd = roundf(s_llc.f_cmd / f_q_step) * f_q_step; // 量化频率
    }
		s_llc.f_cmd = f_clampf(s_llc.f_cmd, f_min, f_max);
	  return s_llc.f_cmd;
}
/*********************************************************************************************************
* 函数名称：llc_current_limit_step
* 函数功能：电流限流PI控制器（带迟滞和抗饱和）
* 输入参数：i_meas 测量电流值（A）  使能控制（false时复位积分器并返回0）
* 输出参数：频率增量（Hz）
* 返 回 值：
* 创建日期：2026年04月08日
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
    float iref = s_llc_curr.iref_cmd;  // 电流参考值（A）
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

/*********************************************************************************************************
* 函数名称：llc_current_thresholds_update
* 函数功能：根据电流参考值 iref_cmd 动态计算迟滞比较器的上下阈值，实现电流限流模式的平滑切换
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年04月09日
* 注    意：若单阈值：电流在阈值附近抖动 → 限流模式频繁开关 → 频率命令震荡
						双阈值滞环：必须穿越整个滞回带才能切换，抗噪声干扰
*********************************************************************************************************/
static void llc_current_thresholds_update(void)
{
    float i_on  = s_llc_curr.iref_cmd + LLC_IOUT_ON_DELTA_A;  // 进入限流的阈值（偏高）
    float i_off = s_llc_curr.iref_cmd - LLC_IOUT_OFF_DELTA_A; // 退出限流的阈值（偏低）

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
/**************************************************************************************** *****************
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
    /* BRK 硬件故障：BKIN 当前低电平（外部比较器仍在报警） */
    if (protect_fault_active_hw()) {
        return true;
    }
    /* BRK 软件锁存：BKIN 曾经低过但已恢复，s_fault 仍保持（需 protect_clear_fault 才清除）*/
    if (protect_fault_latched()) {
        return true;
    }

    /* PFC 底层故障 */
    if (pfc_is_fault()) {
        return true;
    }

    if (s_llc_rt.meas.vout_v > LLC_VOUT_OVP_V) {  //58
        return true;
    }

    if (s_llc_rt.meas.iout_a > LLC_IOUT_OCP_A) { //41
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
/*********************************************************************************************************
*                                              状态机核心
*********************************************************************************************************/
static float llc_bumpless_integ(float f0, float e0, float kp, float f_nom, float f_min, float f_max)
{
    float u0    = f_nom - f0;              // 因为：f = f_nom - u
    float integ = u0 - kp * e0;            // u = kp*e + integ

    float i_lim = f_max - f_min;           // 积分限幅（单位：Hz 等价）
    return f_clampf(integ, -i_lim, i_lim); // 返回积分值，限幅在 -i_lim 和 i_lim 之间
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

    llc_state_t prev = s_llc_rt.app.state;  /* 记录旧状态用于日志 */
	s_llc_rt.app.state = next;
  s_llc_rt.app.entry_ms = g_ms;

    /* ========== 状态切换日志（非阻塞DMA串口） ========== */
    debug_printf("[LLC] %d->%d @%lu ms, Vout=%.1fV, Iout=%.1fA, Vbus=%.1fV\r\n",
                 (int)prev, (int)next,
                 (unsigned long)g_ms,
                 (double)s_llc_rt.meas.vout_v,
                 (double)s_llc_rt.meas.iout_a,
                 (double)s_llc_rt.meas.vbus_v);

	if (next != ST_LLC_RUN) {
		(void)llc_ctrl_step(0.0f, false, false, s_llc.f_cmd, s_llc.f_cmd);
		(void)llc_current_limit_step(0.0f, false);
		llc_derate_reset();
	}
	//s_llc_app.entry_ms = g_ms;
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
	 case ST_IDLE:
		  llc_softstart_on_fault();
		  llc_softstop_reset();  /* 重置软关断状态 */
		  llc_pwm_outputs_enable(0); // 禁用 PWM 输出
		  llc_driver_en_set(false); //disable llc
		  llc_set_freq(s_llc.f_max, true);  /* 强制更新：状态切换确保立即生效 */		
	 	  s_llc_rt.softstart_begin_ms = 0U;  //软启动开始时间戳，用于控制软启动斜坡
	 	  s_llc_rt.stopping_begin_ms = 0U;  //停机过程开始时间戳，用于控制停机时序
	 	  s_llc_rt.hold_last_adjust_ms = 0U; //保持最后调整的时间戳，用于频率调整的去抖
		  s_llc_rt.pfc_ready_begin_ms = 0U; //pfc准备好的时间戳，用于pfc准备好的时间计数
		  s_llc_rt.run_entry_hold_begin_ms = 0U;
		  s_llc_rt.run_entry_stable_ticks = 0U;
		  s_llc_rt.cycle_stop_begin_ms = 0U;  /* 重置周期性软关断计时 */
		  break;
	 case ST_PRECHECK:  
	  	llc_driver_en_set(true);
    	llc_pwm_outputs_enable(0); // 禁用 PWM 输出
    	llc_set_freq(s_llc.f_max, true);  /* 强制更新 */
	 
      break;
	 case ST_SOFTSTART:
			s_llc_rt.softstart_begin_ms = g_ms; //维稳计数
			llc_driver_en_set(true);
			llc_pwm_outputs_enable(1);
			llc_softstart_start(LLC_SOFTSTART_TARGET_HZ);
			break;
	 case ST_RUN_ENTRY_HOLD:
			llc_driver_en_set(true);
			s_llc_rt.run_entry_hold_begin_ms = g_ms; //记录保持时间
			s_llc_rt.run_entry_stable_ticks = 0U; //记录维稳次数
	    s_llc.f_cmd = llc_softstart_last_hz(); //频率设置为软启结束值
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
	 		s_llc_curr.df_prev = 0.0f;
			s_llc_curr.limit_active = false; // 重置限流器状态
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
	s_llc_curr = (llc_curr_t){
				.iref=LLC_IOUT_TARGET_A, //原始目标电流
				.iref_cmd=LLC_IOUT_TARGET_A, //当前实际生效的目标电流
		    .imeas=0.0f,  //当前测量电流，初始化为 0
				.i_err_sat=LLC_IOUT_ERR_SAT_A, //电流误差限幅，防止 ierr = i_meas - iref_cmd 太大时把 PI 一下冲飞
				.kp=LLC_IOUT_CTRL_KP, 
				.ki=LLC_IOUT_CTRL_KI * LLC_CTRL_TS_S, 
				.integ=0.0f,
				.f_min=LLC_F_MIN_HZ, 
				.f_max=LLC_F_MAX_HZ,
				.df_max=LLC_IOUT_DF_MAX_HZ,  //电流环最多能把频率从 f_min 往上抬多少
				.df_slew=LLC_IOUT_DF_SLEW_HZ_S * LLC_CTRL_TS_S, 
				.df_prev=0.0f, //上一拍电流环输出，供 slew 限制使用
				.i_on=LLC_IOUT_TARGET_A + LLC_IOUT_ON_DELTA_A,
				.i_off=LLC_IOUT_TARGET_A - LLC_IOUT_OFF_DELTA_A,
		    .derate_step_ms=0U, //多久允许再往下降一步
				.derate_recover_ms=0U, //多久允许再恢复一步
				.limit_active=false
	};
		llc_current_thresholds_update();
    /* 初始化周期性软关断 */
    s_llc_rt.cycle_stop_enable = false;  /* 禁用周期性软关，使用Burst */
    s_llc_rt.cycle_stop_begin_ms = 0U;

    llc_state_enter(ST_IDLE);
}

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
*           //3. Burst 模式：在 Burst 模式下仅保留电压滤波，不执行电压 PI 控制
*           4. 电压闭环：每 5 个 tick（500μs）执行一次电压 PI 控制，计算频率指令并更新开关频率
*           5. 频率更新：通过 llc_set_freq 函数自然更新频率，实现闭环微调
*           设计要点：电压滤波采用静态变量 vout_filt_v 保存滤波状态，确保滤波连续性；
*                     vout_filt_inited 标志用于滤波器初始化，避免启动瞬态；
*                     vloop_div 计数器实现 500μs 的慢环控制周期，降低计算负载
*********************************************************************************************************/
void llc_app_tick_100us(void)
{
		static uint8_t vloop_div = 0;
		
	  static float vout_filt_v   = 0.0f;   /* 电压PI用滤波状态 */
    static uint8_t vout_filt_inited = 0U; /* 滤波器初始化标志，局部即可 */

    if (s_llc_rt.app.state != ST_LLC_RUN ) {
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
		float iout_now = conv_adc_to_i(g_adc_multi.isense_raw);
		if (!vout_filt_inited) {
				vout_filt_v = vout_now;
				vout_filt_inited = 1U;
		}
	  if (vout_filt_v < 0.001f) 
		{
				vout_filt_v = vout_now;
		}
    /* 100us快环滤波，alpha可后续再调 */
    vout_filt_v += LLC_VOUT_FILT_ALPHA * (vout_now - vout_filt_v);
    s_llc.vmeas = vout_filt_v;

#if LLC_FIXED_FREQ_LOAD_TEST_ENABLE
    vloop_div = 0U;
    llc_set_freq(LLC_FIXED_FREQ_LOAD_TEST_HZ, false);
    return;
#endif
		// 1. 电压环计算
    float err = s_llc.vref - s_llc.vmeas;
		float f_init  = s_llc.f_cmd;                           // 上一拍下发的频率（用于slew基准）
	

		
		if (++vloop_div >= 5U) 
		{   // 500us
			vloop_div = 0;
			// 2. 降额保护更新
			llc_derate_update(iout_now, llc_temp_derate_active());
			// 3. 电流限制环
			float df_i = llc_current_limit_step(iout_now, true);
			float f_cmd_i = s_llc.f_min + df_i;
			/* 关键：用 df_i 判断当前是否仍被电流环抬高（不要用 limit_active） */
			// 4. 电压控制环（考虑电流限制）
			bool lim_for_v = df_i > 1.0f;   // df_i > 0 ==> 10  
			float f_cmd_v = llc_ctrl_step(err, true, lim_for_v, f_init, f_cmd_i); 
			float f_cmd = (f_cmd_i > f_cmd_v) ? f_cmd_i : f_cmd_v; 
			llc_set_freq(f_cmd, false);  /* 自然更新：闭环微调 */
		}
}
/*********************************************************************************************************
* 函数名称：llc_is_active_state
* 函数功能：判断指定的LLC状态是否为活跃工作状态
* 输入参数：st - 待判断的LLC状态枚举值
* 输出参数：无
* 返 回 值：true-表示该状态为活跃状态；false-表示该状态为非活跃状态（IDLE或FAULT）
* 创建日期：2026年04月08日
* 注    意：活跃状态包括：PRECHECK、SOFTSTART、RUN_ENTRY_HOLD、LLC_RUN、CYCLE_STOPPING、STOPPING；
*           非活跃状态包括：IDLE（空闲）和FAULT（故障）；
*           该函数用于故障检测逻辑中，仅在活跃状态下才进行故障判断，避免待机时误报欠压故障
*********************************************************************************************************/		
static bool llc_is_active_state(llc_state_t st)
{
    return (st == ST_PRECHECK) ||
           (st == ST_SOFTSTART) ||
           (st == ST_RUN_ENTRY_HOLD) ||
           (st == ST_LLC_RUN) ||
           (st == ST_CYCLE_STOPPING) ||
           (st == ST_STOPPING);
}
/*********************************************************************************************************
* 函数名称：llc_app_tick_1khz
* 函数功能：实现 LLC 变换器的 1kHz 周期性主控制任务，包括状态机管理、测量更新、故障检测及模式切换
* 输入参数：void
* 输出参数：无
* 返 回 值：void
* 创建日期：2026年04月08日
* 注    意：该函数在 1kHz 定时器中断中调用，是 LLC 控制的主状态机入口；
*           1. 测量更新：调用 llc_update_measurements 更新电压、电流、温度等测量值；
*           2. 故障检测：在非空闲和非故障状态下检测故障，触发 llc_enter_fault；
*           3. 状态机切换：实现 9 个状态的转换逻辑；
*              - ST_IDLE：PFC 就绪且预检通过时进入 ST_PRECHECK；
*              - ST_PRECHECK：等待 100ms 稳定后进入 ST_SOFTSTART；
*              - ST_SOFTSTART：执行软启动，时间到后进入 ST_RUN_ENTRY_HOLD；
*              - ST_RUN_ENTRY_HOLD：等待输出电压稳定（误差窗口内持续一定周期）或超时；
*              - ST_LLC_RUN：正常运行状态，母线电压过低时进入 ST_STOPPING；
*              - ST_CYCLE_STOPPING：执行软关断，延时后重新启动；
*              - ST_STOPPING：执行软关断，完成后回到 ST_IDLE；
*              - ST_FAULT：故障状态，当前无恢复逻辑；
*           4. 使能控制：LLC_ENABLE 宏与 PFC 就绪状态共同决定 enable_llc；
*           5. 预检保护：各运行状态下持续检查 llc_precheck_ok，失败则进入 ST_STOPPING
*********************************************************************************************************/
void llc_app_tick_1khz(void)
{
	llc_update_measurements(); // 更新电压/电流/温度
	//bool enable_llc = (LLC_ENABLE != 0) && pfc_is_ready();// 使能条件
	bool enable_llc = (LLC_ENABLE != 0) && pfc_is_ready() && s_llc_run_request;
	bool pfc_ready_stable = llc_pfc_ready_stable(enable_llc);// 稳定延时
	
	if (s_llc_rt.app.state != ST_IDLE && s_llc_rt.app.state != ST_FAULT) {
		if (llc_faults_present()) {
			llc_enter_fault();
			return; // 立即退出，不再执行后续状态处理
		}
	}
	switch(s_llc_rt.app.state)
	{
		case ST_IDLE: //	PFC稳定+预检通过
		{
			if(pfc_ready_stable &&llc_precheck_ok())
			{
				llc_state_enter(ST_PRECHECK);
			}
			break;
		}
		case ST_PRECHECK: //	时间到+条件保持
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
		case ST_SOFTSTART: //300ms+50ms延时到
		{
			llc_softstart_tick_1khz();
			if(!enable_llc|| !llc_precheck_ok())
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			if(elapsed_reached(s_llc_rt.softstart_begin_ms,LLC_SOFTSTART_DURATION_MS + LLC_SOFTSTART_STABILIZE_MS))
			{
				llc_softstart_stop();
				s_llc.f_cmd = llc_softstart_last_hz();
				llc_state_enter(ST_RUN_ENTRY_HOLD);
				break;
			}
		
			break;
		}
    case ST_RUN_ENTRY_HOLD: //6V窗口内稳定2次+20ms
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
		case ST_LLC_RUN: // 母线低压/使能关闭时退出
		{
			if(!enable_llc||(s_llc_rt.meas.vbus_v<(LLC_VBUS_MIN_START_V-LLC_VOUT_HYST_V)))
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			break;
	 }
		
		case ST_CYCLE_STOPPING: //关断完成+500ms等待
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
		case ST_STOPPING: //关断完成→IDLE
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
	
    /* 风扇控制：状态机处理完毕后统一更新，根据当前状态和输出电流决定风扇开关 */
    //llc_fan_tick();
}

llc_state_t llc_app_state(void)
{
	return s_llc_rt.app.state;
}





