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
static float llc_ctrl_step(float e, bool en, bool limit_active, float f_init, float f_track);
static float llc_bumpless_integ(float f0, float e0, float kp, float f_nom, float f_min, float f_max);
static float llc_current_limit_step(float i_meas,bool en);
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
static float llc_ctrl_step(float e, bool en, bool limit_active, float f_init, float f_track)
{
	  static bool en_z1 = false;  // 记录上一次的使能状态
    float kp = s_llc.kp;
    float ki = s_llc.ki;
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_slew = s_llc.f_slew;
	  float f_nom = s_llc.f_nom;  //额定频率（与软启动末拍对齐）
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



static void llc_update_measurements(void)
{
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
	s_llc_rt.app.state = next; 
	s_llc_rt.app.entry_ms = g_ms; // 记录进入时间
	if (next != ST_LLC_RUN) {
		(void)llc_ctrl_step(0.0f, false, false, s_llc.f_cmd, s_llc.f_cmd);
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
			// 软启动完成
			{
				llc_softstart_stop(); // 停止软启动
				s_llc.f_cmd = llc_softstart_last_hz();	// 设置目标频率为软启动最后的频率
				llc_state_enter(ST_RUN_ENTRY_HOLD); // 进入运行入口保持状态
				break;
			}
			break;
		}
    case ST_RUN_ENTRY_HOLD:
		{
			if(!enable_llc|| !llc_precheck_ok())
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			float hold_err = s_llc_rt.meas.vout_v - LLC_VOUT_TARGET_V; // з
			if (fabsf(hold_err) <= LLC_RUN_ENTRY_STABLE_WINDOW_V) { // 在稳定范围内
				if (s_llc_rt.run_entry_stable_ticks < LLC_RUN_ENTRY_STABLE_TICKS) { // 保持稳定
					s_llc_rt.run_entry_stable_ticks++;
				}
			} 
			else 
			{
				s_llc_rt.run_entry_stable_ticks = 0U;
			}
			if (elapsed_reached(s_llc_rt.run_entry_hold_begin_ms, LLC_RUN_ENTRY_HOLD_MS) && s_llc_rt.run_entry_stable_ticks >= LLC_RUN_ENTRY_STABLE_TICKS) 
			// 达到保持时间且稳定
			{
				llc_state_enter(ST_LLC_RUN);
				break;
			}

			if (elapsed_reached(s_llc_rt.run_entry_hold_begin_ms, LLC_RUN_ENTRY_TIMEOUT_MS)){ // 超时
				llc_state_enter(ST_STOPPING);
				break;
			}
				break;		
		}
		case ST_LLC_RUN:
		{
			if(!enable_llc||(s_llc_rt.meas.vbus_v<(LLC_VBUS_MIN_START_V-LLC_VOUT_HYST_V))) // 电压过低
			{
				llc_state_enter(ST_STOPPING);
				break;
			}
			s_llc.vmeas = s_llc_rt.meas.vout_v;
			float err = s_llc.vref - s_llc.vmeas;
			float f_init  = s_llc.f_cmd;                           // 上一拍下发的频率（用于slew基准）
			float df_i = llc_current_limit_step(s_llc_rt.meas.iout_a, true);
			float f_cmd_i = s_llc.f_min + df_i;
			//float f_cmd_i = f_init + df_i;
			/* 关键：用 df_i 判断当前是否仍被电流环抬高（不要用 limit_active） */
			bool lim_for_v = df_i > 1.0f;   // df_i > 0 ==> 10                   
			float f_cmd_v = llc_ctrl_step(err, true, lim_for_v, f_init, f_cmd_i); 
			float f_cmd = (f_cmd_i > f_cmd_v) ? f_cmd_i : f_cmd_v; 
			llc_set_freq(f_cmd);			
			break;
		}
		case ST_STOPPING:
		{
			if(elapsed_reached(s_llc_rt.stopping_begin_ms,LLC_STOPPING_FREQ_HOLD_MS))
			{
				llc_pwm_outputs_enable(0);
				llc_driver_en_set(false);
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

