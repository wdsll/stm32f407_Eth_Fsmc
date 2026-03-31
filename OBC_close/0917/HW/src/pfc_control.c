#include "pfc_control.h"
#include <math.h>
#include "debug_printf.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))
/* VBUS有效性检查范围 */
#define PFC_VBUS_RATIO_IDLE_MIN     (1.3f)    /* 未使能时：VBUS/VAC 最小倍数 */
#define PFC_VBUS_RATIO_IDLE_MAX     (1.5f)    /* 未使能时：VBUS/VAC 最大倍数 */
#define PFC_VBUS_ENABLED_MIN_V      (380.0f)  /* 使能后：VBUS 最小电压 */
#define PFC_VBUS_ENABLED_MAX_V      (410.0f)  /* 使能后：VBUS 最大电压 */


/*********************************************************************************************************
*                                              量测/上下文
*********************************************************************************************************/
typedef struct {
		float vac_v;  /* 等效/瞬时（按你的采样换算） */
	  float vac_rms; /* 低通等效 RMS */
    float vbus_v;
	  uint16_t vac_raw;
    uint16_t vbus_raw;
} pfc_measure_t;

typedef struct {
    pfc_state_t state;  // 当前状态机状态
    uint32_t entry_ms;  // 进入当前状态的时刻

    bool enable_cmd;   // 外部使能命令
    bool fault_latched; // 故障锁存标志
	  
	  /* 启动/READY计时 */
	//防抖机制: 所有状态变化都需要持续一定时间才确认，避免瞬时干扰
  //分级验证: 启动→电压达标→就绪确认，层层递进
  //双向监控: 既监控启动过程，也监控运行中的掉电情况
	  uint32_t startup_cmd_ms;  // enable_cmd 起始时刻
	  uint32_t hw_enable_since_ms;  // hardware enable timestamp for VBUS ramp
    uint32_t vbus_ok_since_ms;  // VBUS 达标起始时刻（用于 READY 延时）
    uint32_t dropout_since_ms;  // READY 后掉电计时
	
	/* AC 去抖 
	智能去抖策略: 双向计时器设计：AC正常时累积 ac_ok_since_ms ，异常时累积 ac_loss_since_ms
								避免AC电压在阈值附近波动导致的频繁状态切换
							  提供电网质量的可观测性
	*/
	  uint32_t ac_ok_since_ms;  // AC正常计时
    uint32_t ac_loss_since_ms;  // AC掉电计时
	
	  /* 1.414 自检 
		1.414倍验证原理:
			物理基础：整流后的直流电压 ≈ 交流有效值 × √2 ≈ 1.414
			上电自检：验证整流桥是否正常工作
			安全保障：防止硬件故障导致的误操作s		
		*/
    uint32_t vac_ratio_since_ms;  // VAC/VBUS比例验证计时
    bool     bus_matches_ac;   // 比例匹配标志
	
	  /* 
		故障处理策略:
			fault_since_ms 实现故障的时序管理（如冷却时间）
			clear_sent 防止清除指令重复发送，避免硬件冲突
			支持故障恢复机制，而不是简单的硬复位	
		*/
    uint32_t fault_since_ms;    // 进入 FAULT 的时刻
    uint8_t  clear_sent;    // fault clear 防抖：protect_hw_clear_pulse 只打一遍

    bool hw_enabled;             // 当前是否已经拉起硬件使能（避免逻辑歧义）

    pfc_measure_t meas;   // 所有测量数据的集合
} pfc_ctx_t;

static pfc_ctx_t s_pfc;

/*********************************************************************************************************
*                                              小工具函数
*********************************************************************************************************/
static float adc_to_v_scaled(uint16_t raw, float gain)
{
    float v_adc = (raw * VREF_ADC) / 4095.0f;
    return v_adc * gain;
}

static float adc_to_v_div(uint16_t raw, float rtop, float rbot)
{
    float v_adc = (raw * VREF_ADC) / 4095.0f;
    return v_adc * (rtop + rbot) / rbot;
}

static float lpf(float prev, float sample, float alpha)
{
    return prev + alpha * (sample - prev);
}

static void pfc_reset_startup_timers(void)
{
    s_pfc.startup_cmd_ms = 0U;  //清除启动计时 !0会影响启动流程
    s_pfc.vbus_ok_since_ms = 0U; //清除电压就绪计时 !0会影响就绪判断
    s_pfc.hw_enable_since_ms = 0U;
}
static bool pfc_ac_ok(void)
{
    float vac = s_pfc.meas.vac_v;
    return (vac >= PFC_AC_VALID_MIN_VRMS) && (vac <= (PFC_AC_VALID_MAX_VRMS + 2.0f));
}

static bool pfc_ac_ok_debounced(void)
{
	// 阶段1: 检查AC电源是否正常
    if (!pfc_ac_ok()) {
        s_pfc.ac_ok_since_ms = 0U; // AC异常，立即重置计时器
        return false;
    }
  // 阶段2: AC刚恢复，开始计时
    if (s_pfc.ac_ok_since_ms == 0U) {
        s_pfc.ac_ok_since_ms = g_ms;
			//s_pfc.ac_ok_since_ms = (g_ms == 0U) ? 1U : g_ms;
        return false;
    }
    // 阶段3: 检查AC是否稳定持续了配置的去抖时间
     return  elapsed_reached(s_pfc.ac_ok_since_ms, PFC_AC_OK_DEBOUNCE_MS);
	
}

static bool pfc_ac_loss_debounced(void)
{
    if (pfc_ac_ok()) {
        s_pfc.ac_loss_since_ms = 0U;
        return false;
    }
 // AC掉电后的去抖逻辑
    if (s_pfc.ac_loss_since_ms == 0U) {
        s_pfc.ac_loss_since_ms = g_ms;
			//s_pfc.ac_loss_since_ms = (g_ms == 0U) ? 1U : g_ms;
        return false;
    }

    return elapsed_reached(s_pfc.ac_loss_since_ms, PFC_AC_LOSS_DEBOUNCE_MS);
}
/*********************************************************************************************************
*                                 硬件输出：合并“继电器+PFC使能”为一个脚
*********************************************************************************************************/
/* 继电器脚：优先用 PFC_MAIN_RELAY_*，若工程里没定义可退回 PFC_EN_*（兼容你旧工程） */
#if defined(PFC_MAIN_RELAY_PORT) && defined(PFC_MAIN_RELAY_PIN) && defined(PFC_MAIN_RELAY_RCU)
#define PFC_MAIN_OUT_PORT   PFC_MAIN_RELAY_PORT
#define PFC_MAIN_OUT_PIN    PFC_MAIN_RELAY_PIN
#define PFC_MAIN_OUT_RCU    PFC_MAIN_RELAY_RCU
#elif defined(PFC_EN_PORT) && defined(PFC_EN_PIN) && defined(PFC_EN_RCU)
#define PFC_RELAY_PORT   PFC_EN_PORT
#define PFC_RELAY_PIN    PFC_EN_PIN
#define PFC_RELAY_RCU    PFC_EN_RCU
#else
#error "No PFC MAIN/EN GPIO defined"
#endif

void pfc_hw_set_main(bool on)
{
		static bool s_gpio_initialized = false;
	  static bool s_last = false;
		bool first = !s_gpio_initialized;

		if (first)
		{
			  rcu_periph_clock_enable(PFC_MAIN_OUT_RCU);
				gpio_init(PFC_MAIN_OUT_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PFC_MAIN_OUT_PIN);
				s_gpio_initialized = true;
		}
    /* 首次调用一定要落一次电平；非首次且状态未变才直接返回 */
    if (!first && (s_last == on)) {
        return;
    }
    s_last = on;
    if (on) 
			gpio_bit_set(PFC_MAIN_OUT_PORT, PFC_MAIN_OUT_PIN);
    else    
			gpio_bit_reset(PFC_MAIN_OUT_PORT, PFC_MAIN_OUT_PIN);
}
/* BUS_ADJ PWM：三态机跑通阶段固定 0（交给 NCP1654 自己闭环） */
static void pfc_pwm_set(float duty)
{
    if (duty <= 0.0f) {
        duty = 0.0f;
    } 
    pb0_pwm_set_duty(duty);
}

/* 关输出：PWM=0 + MAIN=0 */
static void pfc_outputs_off(void)
{
  pfc_pwm_set(0.0f);
	pfc_hw_set_main(false);
	s_pfc.hw_enabled = false;
	s_pfc.hw_enable_since_ms = 0U;
}

/*********************************************************************************************************
							当前状态 → 状态进入函数 → 新状态初始化 → 运行tick处理
								 ↓              ↓              ↓           ↓
							状态保持    →   条件判断    →   状态切换   →   循环执行
*********************************************************************************************************/
static void pfc_state_enter(pfc_state_t next)
{
    if (s_pfc.state == next)   //幂等性保证：多次调用同一状态转换不会产生副作用
			return;

    s_pfc.state = next;
		//全局时钟依赖：使用 g_ms 确保系统时间的一致性
    s_pfc.entry_ms = g_ms;  //时间戳记录： entry_ms 是所有时序控制的基础时间锚点

	if(next == PFC_ST_IDLE)
	{
		s_pfc.startup_cmd_ms = 0U; //清除启动计时 !0会影响启动流程
		s_pfc.vbus_ok_since_ms = 0U; //清除电压就绪计时 !0会影响就绪判断
		s_pfc.dropout_since_ms =0U;  //清除掉电计时 !0会影响掉电检测
		s_pfc.ac_loss_since_ms = 0U;  //清除AC掉电计时 !0会影响 AC监控
		s_pfc.clear_sent = 0U; //重置故障清除标志  !0 会影响故障管理
		pfc_outputs_off();
	}
	else if(next == PFC_ST_RAMP)
	{
		s_pfc.vbus_ok_since_ms = 0U; // reset vbus stable timer
		s_pfc.dropout_since_ms = 0U; // reset dropout timer
		s_pfc.ac_loss_since_ms = 0U; // reset AC loss timer
	}
	/*
	差异化设计：
		选择性重置：只重置与运行监控相关的计时器
		保持连续性：不重置 startup_cmd_ms 和 vbus_ok_since_ms ，维护启动历史
		硬件策略：硬件状态由tick函数控制，而非状态进入函数
	*/
	else if(next == PFC_ST_READY)
	{
		s_pfc.dropout_since_ms = 0U; //清除掉电计时 !0会影响掉电检测
		s_pfc.ac_loss_since_ms = 0U; //清除AC掉电计时 !0 会影响AC监控
		/* READY 状态保持硬件开（由 tick 控制） */
	}
	else if(next == PFC_ST_FAULT)
	{
		s_pfc.fault_since_ms = g_ms;  //fault_since_ms 用于故障恢复的冷却时间
		s_pfc.fault_latched =true;  //fault_latched = true 需要手动清除，确保安全
		s_pfc.clear_sent = 0U;
		/*fault 必须关断硬件*/
		pfc_outputs_off();
	}
		
}
/*********************************************************************************************************
*                                              输入采样/判定
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：pfc_sample_inputs
* 函数功能：PFC系统的数据采集前端，负责从硬件获取原始ADC数据，转换为工程量，并执行关键的硬件自检逻辑。这是整个控制系统的"感官"部分。
* 输入参数：void			
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年12月24日
* 注    意：
*********************************************************************************************************/
static void pfc_sample_inputs(void)
{
		static uint32_t last_print_ms = 0U;
	  //采样时间选择： ADC_SAMPLETIME_71POINT5 提供约17.1μs的采样时间，
    s_pfc.meas.vbus_raw = adc1_aux_read_channel(BUS_VOL_SAMPLE,  ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vac_raw  = adc1_aux_read_channel(AC_VOL_SAMPLE,   ADC_SAMPLETIME_71POINT5);
	
    float vac  = adc_to_v_scaled(s_pfc.meas.vac_raw,  PFC_AC_ADC_GAIN_V_PER_VIN) + PFC_AC_OFFSET;
    float vbus = adc_to_v_scaled(s_pfc.meas.vbus_raw, PFC_VBUS_ADC_GAIN_V_PER_VIN);

    s_pfc.meas.vac_v   = vac;  // vac_v   瞬时值
    s_pfc.meas.vbus_v  = vbus; // vbus_v  瞬时值
	/*
		时间常数：τ = 1/α = 20个采样周期
		对于1kHz调用：约20ms达到63.2%的阶跃响应
		半周期数：ln(0.5)/ln(1-0.05) ≈ 13.5个采样周期
	*/  
	  //s_pfc.meas.vac_rms = lpf(s_pfc.meas.vac_rms, vac, 0.05f); //vac_rms 有效值 低通滤波(α=0.05)
	  /* 上电自检：VBUS >= 1.414*VAC（下限判定，允许容差放宽） */
    bool ratio_ok = false;
    float vac_r = s_pfc.meas.vac_v;
	  if (vac_r > 10.0f) {
	  	//容差设计：考虑二极管压降、测量误差等 给了个0.15的容忍度
       float vbus_min = vac_r * PFC_VBUS_VAC_RATIO * (1.0f - PFC_VBUS_VAC_RATIO_TOLERANCE);
       ratio_ok = (vbus >= vbus_min);
    } 
		if (ratio_ok) {
			if (s_pfc.vac_ratio_since_ms == 0U) {
					s_pfc.vac_ratio_since_ms = g_ms; //开始计时
			}
			if (elapsed_reached(s_pfc.vac_ratio_since_ms, PFC_VBUS_VAC_RATIO_STABLE_MS)) {
					s_pfc.bus_matches_ac = true; //确认稳定
			}
    } else {
			s_pfc.vac_ratio_since_ms = 0U;  //重置计时
			s_pfc.bus_matches_ac = false;
    }
#if 0		
		 if ((uint32_t)(g_ms - last_print_ms) >= 200U) {
        last_print_ms = g_ms;
			  int32_t vbus_mV = (int32_t)(vbus * 1000.0f + (vbus >= 0.0f ? 0.5f : -0.5f));
			  int32_t vac_mV  = (int32_t)(vac  * 1000.0f + (vac  >= 0.0f ? 0.5f : -0.5f));
			  int32_t vacf_mV = (int32_t)(s_pfc.meas.vac_rms * 1000.0f + (s_pfc.meas.vac_rms >= 0.0f ? 0.5f : -0.5f));
				debug_printf("[PFC] VBUS=%ldmV VAC=%ldmV VAC_F=%ldmV ""raw(vb=%u,va=%u) match=%u\r\n",
						 (long)vbus_mV, (long)vac_mV, (long)vacf_mV,
						 (unsigned)s_pfc.meas.vbus_raw, (unsigned)s_pfc.meas.vac_raw,
						 (unsigned)(s_pfc.bus_matches_ac ? 1U : 0U));

    }
#endif
}


static bool pfc_ac_overvoltage(void)
{
    return s_pfc.meas.vac_v > (PFC_AC_VALID_MAX_VRMS + PFC_AC_OVERVOLTAGE_MARGIN_V);
}

static void pfc_handle_fault(const char *reason)
{
    (void)reason;
    /* 故障：必须立即释放继电器 + BUS_ADJ=0 */
    pfc_outputs_off();
    pfc_state_enter(PFC_ST_FAULT);
}

/*********************************************************************************************************
*                                              对外 API
*********************************************************************************************************/
void pfc_init(void)
{
//复合字面量语法： (pfc_ctx_t){0} 是C99特性，比 memset 更安全
    s_pfc = (pfc_ctx_t){0};
    
	/* PB0 PWM 仍初始化，但三态机阶段保持 0 */
    pb0_pwm_init(PB0_PWM_BASE_HZ);
    bus_vol_adj_init();
		
    adc1_aux_init();
	  s_pfc.state = PFC_ST_IDLE;
    pfc_outputs_off();
}

void pfc_enable(void)
{
    s_pfc.enable_cmd = true;
    if (s_pfc.state == PFC_ST_IDLE) {
    /* 进入 IDLE 不变，由 tick 负责 startup delay 后拉起硬件 */
        s_pfc.startup_cmd_ms = 0U;
    }
}
void pfc_disable(void)
{
    s_pfc.enable_cmd = false;
	/*立即退回ILE并且关断硬件*/
	if(s_pfc.state != PFC_ST_IDLE)
	{
		pfc_state_enter(PFC_ST_IDLE);
	}
}

/*********************************************************************************************************
*                                              1kHz tick
*********************************************************************************************************/

void adc_test(void)
{
	pfc_sample_inputs();
}
/*********************************************************************************************************
* 函数名称：pfc_tick_1khz
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年12月29
* 注    意：PFC_ST_IDLE → PFC_ST_RAMP ->PFC_ST_READY → (故障检测) → PFC_ST_FAULT
*********************************************************************************************************/
uint32_t bkin_flag = 0;
void pfc_tick_1khz(void)
{
  pfc_sample_inputs();                                    // 采集ADC输入信号，更新测量数据
	
	float vbus_v = s_pfc.meas.vbus_v;                   // 获取当前VBUS电压值
	
    if (vbus_v >= PFC_VBUS_OVP_V) {                         // 检查VBUS是否过压
        pfc_handle_fault("VBUS_OV");                          // VBUS过压故障
        return;                                            // 立即退出
    }

    /* 正常状态机处理 */
    switch (s_pfc.state) {		
    case PFC_ST_IDLE:   // 空闲状态：这是最复杂的状态，包含三重检查机制：
        /* 第一重检查：未使能时保持关断 */
        if (!s_pfc.enable_cmd) {                             // 检查用户是否撤销使能
            pfc_outputs_off();                               // 关断所有输出
					 // 重置所有相关计时器
            s_pfc.startup_cmd_ms   = 0U;                     // 清除启动命令计时
            s_pfc.vbus_ok_since_ms = 0U;                     // 清除VBUS就绪计时
						s_pfc.ac_ok_since_ms   = 0U;                     // 清除AC正常计时
            break;                                          // 跳出状态处理
        }

				/* 第二重检查：AC电源去抖机制，使用时间窗口确保AC电源稳定 */
				if (!pfc_ac_ok_debounced()) {                                    // AC电源是否正常
            pfc_outputs_off();
            pfc_reset_startup_timers();
						break;
						}
				
				if(!s_pfc.hw_enabled && !s_pfc.bus_matches_ac){    // 第三重检查：上电自检
				/* 检查VBUS与VAC的比例是否匹配，只在硬件未使能前检查，避免PFC工作后误判 */
            pfc_outputs_off();
            pfc_reset_startup_timers();
            break;
				}
				/* 启动延时和使能序列 */
        if (s_pfc.startup_cmd_ms == 0U) {                   // 如果还没开始启动计时
            s_pfc.startup_cmd_ms = g_ms;                     // 记录启动命令时间
        }

				/* 启动时序控制：延时到点再拉起硬件 */
				if (!s_pfc.hw_enabled && elapsed_reached(s_pfc.startup_cmd_ms, PFC_STARTUP_DELAY_MS)) {
					pfc_hw_set_main(true);                             // 拉起主继电器
					s_pfc.hw_enabled = true;   
					s_pfc.hw_enable_since_ms = g_ms;					// 标记硬件已使能
					/* 状态机跑通阶段：BUS_ADJ PWM保持0，交给NCP1654自己闭环控制 */
					pfc_pwm_set(0.0f);                               // 设置PWM占空比为0
					pfc_state_enter(PFC_ST_RAMP);
				}
    break;                                              // 结束IDLE状态处理，进入升压爬坡状态
    case PFC_ST_RAMP:
			// 第一重检查：用户撤销使能检查
			  if (!s_pfc.enable_cmd) {
            pfc_state_enter(PFC_ST_IDLE);
            break;
        }
			 // 第二重检查：AC掉电去抖机制（使用双向计时器检测AC电源失效）	
        if (pfc_ac_loss_debounced()) {
					 pfc_state_enter(PFC_ST_IDLE);
					break;
				} 
			// 第三重检查：硬件使能保护机制（防止单点故障导致硬件意外关闭）				
				if (!s_pfc.hw_enabled) {
						pfc_hw_set_main(true);
						s_pfc.hw_enabled = true;
						s_pfc.hw_enable_since_ms = g_ms;
        }
        pfc_pwm_set(0.0f);
			// VBUS爬坡延时检查：等待硬件使能后的稳定时间
        if (!elapsed_reached(s_pfc.hw_enable_since_ms, PFC_VBUS_RAMP_DELAY_MS)) {
            s_pfc.vbus_ok_since_ms = 0U;
            break;
        }
				// VBUS电压范围检查：判断VBUS是否在目标范围内
        bool vbus_in_range = (vbus_v >= PFC_VBUS_ENABLED_MIN_V) && (vbus_v <= PFC_VBUS_ENABLED_MAX_V);
        if (vbus_in_range) {  // VBUS在有效范围内，开始就绪确认去抖
            if (s_pfc.vbus_ok_since_ms == 0U) {
                s_pfc.vbus_ok_since_ms = g_ms;
            } else if (elapsed_reached(s_pfc.vbus_ok_since_ms, PFC_READY_DELAY_MS)) {
                pfc_state_enter(PFC_ST_READY); // VBUS稳定达标持续足够时间，确认就绪
            }
        } else {
            s_pfc.vbus_ok_since_ms = 0U;
					// 爬坡超时故障检查：防止VBUS长时间无法爬升到位
            if (elapsed_reached(s_pfc.hw_enable_since_ms, PFC_VBUS_RAMP_TIMEOUT_MS)) {
                pfc_handle_fault("VBUS_RAMP");
            }
        }
        break;
				
		// READY状态：双向监控机制
    case PFC_ST_READY:
        /* 用户撤销使能检查：立即回IDLE关断 */
        if (!s_pfc.enable_cmd) {                             // 检查用户是否撤销使能
          pfc_state_enter(PFC_ST_IDLE);                      // 回到IDLE状态
          break;                                          // 跳出状态处理
        }

        /* AC掉电去抖机制：AC电源失效时回退到IDLE */
        if (pfc_ac_loss_debounced()) {                                    // AC电源是否正常
						pfc_state_enter(PFC_ST_IDLE);   					// 回到IDLE状态
					  break;
        }

        /* READY状态下确保硬件保持使能（防止单点故障） */
        if (!s_pfc.hw_enabled) {                              // 检查硬件是否意外关闭
            pfc_hw_set_main(true);                             // 重新拉起主继电器
            s_pfc.hw_enabled = true;                           // 标记硬件已使能
					  s_pfc.hw_enable_since_ms = g_ms;
					  s_pfc.vbus_ok_since_ms = 0U;                       // 重置VBUS监控计时器
					  s_pfc.dropout_since_ms = 0U;                       // 重置掉电监控计时器
        }
        pfc_pwm_set(0.0f);                                   // 保持PWM为0，让NCP1654闭环
		
				/* VBUS掉电去抖：VBUS低于阈值持续一段时间才退回IDLE */
				if (vbus_v >= PFC_VBUS_DROPOUT_THRESHOLD_V) {          // VBUS正常
					s_pfc.dropout_since_ms = 0U;                       // 重置掉电计时
				} else {                                                // VBUS低于阈值
				if (s_pfc.dropout_since_ms == 0U) {                // 首次检测到掉电
					s_pfc.dropout_since_ms = g_ms;                 // 开始掉电计时
				} else if (elapsed_reached(s_pfc.dropout_since_ms, PFC_VBUS_DROPOUT_MS)) { // 掉电时间达到
					pfc_state_enter(PFC_ST_IDLE);                  // 回到IDLE状态
					break;
			  }
		}
		break;                                              // 结束READY状态处理

    case PFC_ST_FAULT:                                      // 故障状态处理
				/*故障状态下确保所有输出保持关闭，这是安全的基本要求*/
				pfc_outputs_off();                                     // 关断所有输出

				/* 检查故障是否仍然存在，如果存在则继续停留在FAULT状态 */
				if (protect_fault_active_hw()) { // 硬件故障是否仍然存在
				break;                                          // 继续停留在FAULT状态
		}
		
		/* 故障已解除的处理：提供两种退出路径
		   路径1：用户主动撤销enable -> 直接回IDLE
		   路径2：自动重试机制：enable仍在 + 等待重启延时 -> 清锁存 + 清除脉冲 -> 回IDLE */
		if (!s_pfc.enable_cmd) {                             // 检查用户是否撤销使能
			pfc_state_enter(PFC_ST_IDLE);                      // 直接回到IDLE状态
		    /* 清理故障状态标志，为下次使用做准备 */
			s_pfc.clear_sent = 0U;                           // 重置清除发送标志
			break;                                          // 跳出状态处理
		}  

		/* 自动重试机制：故障解除后的冷却时间控制 */
		if (elapsed_reached(s_pfc.fault_since_ms, PFC_FAULT_RESTART_MS)) { // 冷却时间已到
				if (protect_fault_latched()) {                   // 如果还有软件锁存
						protect_clear_fault();                    // 清除故障锁存
				}
				if (!s_pfc.clear_sent) {                       // 防止重复发送清除指令
						s_pfc.clear_sent = 1U;                    // 标记已发送清除
				}
				pfc_state_enter(PFC_ST_IDLE);                      // 回到IDLE状态重新开始
		}
		break;                                              // 结束FAULT状态处理

    default:                                               // 未知状态处理
		/* 安全设计：任何未知状态都立即安全关断并进入故障状态 */
        pfc_outputs_off();                                   // 关断所有输出
        pfc_state_enter(PFC_ST_FAULT);                        // 进入故障状态
 		break;                                              // 结束处理
    }
}

/*********************************************************************************************************
*                                              getters
*********************************************************************************************************/
float pfc_get_vbus(void) { return s_pfc.meas.vbus_v; }

pfc_state_t pfc_state(void) { return s_pfc.state; }

float pfc_get_vac(void)  { return s_pfc.meas.vac_rms; }

float pfc_bus_voltage(void)
{
    return pfc_get_vbus();     // 或者直接 return s_pfc.meas.vbus_v;
}


bool pfc_is_ready(void)
{
/* READY 就放行 LLC；若你想更严，可以再加 vbus 门限 */
	return (s_pfc.state == PFC_ST_READY);
}

bool pfc_is_fault(void) { return s_pfc.state == PFC_ST_FAULT; }
bool pfc_is_fault_latched(void) { return s_pfc.fault_latched; }

/* 手动清故障接口：满足条件才允许退出 FAULT */

void pfc_clear_fault(void)
{
    if (s_pfc.state != PFC_ST_FAULT) return;

    if (protect_fault_active_hw() || protect_fault_latched()) 
			return;

    if (!elapsed_reached(s_pfc.fault_since_ms, PFC_FAULT_RESTART_MS)) 
			return;
    if (protect_fault_latched()) {
        protect_clear_fault();
    }
    s_pfc.fault_latched = false;
    pfc_state_enter(PFC_ST_IDLE);
}
