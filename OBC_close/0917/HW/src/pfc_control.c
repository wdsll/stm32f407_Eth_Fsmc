#include "pfc_control.h"
#include <math.h>

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))


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
	//采样时间选择： ADC_SAMPLETIME_71POINT5 提供约17.1μs的采样时间，
    s_pfc.meas.vbus_raw = adc1_aux_read_channel(BUS_VOL_SAMPLE,  ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vac_raw  = adc1_aux_read_channel(AC_VOL_SAMPLE,   ADC_SAMPLETIME_71POINT5);
	
    float vac  = adc_to_v_scaled(s_pfc.meas.vac_raw,  PFC_AC_ADC_GAIN_V_PER_VIN);
    float vbus = adc_to_v_scaled(s_pfc.meas.vbus_raw, PFC_VBUS_ADC_GAIN_V_PER_VIN);

    s_pfc.meas.vac_v   = vac;  //	vac_v	 瞬时值
    s_pfc.meas.vbus_v  = vbus; // vbus_v  瞬时值
		/*
			时间常数：τ = 1/α = 20个采样周期
			对于1kHz调用：约20ms达到63.2%的阶跃响应
			半周期数：ln(0.5)/ln(1-0.05) ≈ 13.5个采样周期
		*/  
	  s_pfc.meas.vac_rms = lpf(s_pfc.meas.vac_rms, vac, 0.05f); //vac_rms 有效值 低通滤波(α=0.05)
	  /* 上电自检：VBUS >= 1.414*VAC（下限判定，允许容差放宽） */
    bool ratio_ok = false;
    float vac_r = s_pfc.meas.vac_rms;
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
}

static bool pfc_ac_ok(void)
{
    float vac = s_pfc.meas.vac_rms;
    return (vac >= PFC_AC_VALID_MIN_VRMS) && (vac <= (PFC_AC_VALID_MAX_VRMS + 2.0f));
}

static bool pfc_ac_overvoltage(void)
{
    return s_pfc.meas.vac_rms > (PFC_AC_VALID_MAX_VRMS + PFC_AC_OVERVOLTAGE_MARGIN_V);
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
    pb0_pwm_set_duty(0.0f);
		
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
void pfc_tick_1khz(void)
{
    pfc_sample_inputs();
	float vbus_v = s_pfc.meas.vbus_v;

	/*采样数据有效性检查*/
	if(vbus_v < 0.0f || vbus_v > 500.0f)
	{
		pfc_handle_fault("SENSOR_INVALID");
		return;
	}

    /* 保护/故障 */
    if (protect_fault_active_hw() || protect_fault_latched())   //读刹车脚&判断有没有发生刹车脚中断
	{
        pfc_handle_fault("HARD_PRO");
        return;
    }
    /* AC overvoltage is treated as a fault */
    if (pfc_ac_overvoltage()) { //uv 380
        pfc_handle_fault("VAC_OV");
        return;
    }

    if (vbus_v >= PFC_VBUS_OVP_V) {
        pfc_handle_fault("VBUS_OV");
        return;
    }

    /* 正常状态机 */
    switch (s_pfc.state) {
    case PFC_ST_IDLE:
        /* 未使能：保持关断 */
        if (!s_pfc.enable_cmd) {
            pfc_outputs_off();
            s_pfc.startup_cmd_ms   = 0U;
            s_pfc.vbus_ok_since_ms = 0U;
			s_pfc.ac_ok_since_ms   = 0U;
            break;
        }
		bool should_shutdown = false;
		bool need_timer_reset = false;
/* 1) AC OK 去抖 */
		if (!pfc_ac_ok()) {
            s_pfc.ac_ok_since_ms   = 0U;
			need_timer_reset = true;
			should_shutdown = true;
        }
		else if (s_pfc.ac_ok_since_ms == 0U) {
            s_pfc.ac_ok_since_ms = g_ms;
			should_shutdown = true;
        }
		else if (!elapsed_reached(s_pfc.ac_ok_since_ms, PFC_AC_OK_DEBOUNCE_MS)) {
			should_shutdown = true;
			}
	   else if(!s_pfc.hw_enabled && !s_pfc.bus_matches_ac){
	   	 /* 2) 上电自检：只在 hw 未使能前检查，避免 PFC 调到 400V 后误判 */
	   		need_timer_reset = true;
		    should_shutdown = true;
	   	}
	   /* 统一处理关断和计时器重置 */
	   if (should_shutdown) {
            pfc_outputs_off();
            break;
        }

		if (need_timer_reset) {
            s_pfc.startup_cmd_ms   = 0U;
            s_pfc.vbus_ok_since_ms = 0U;
			s_pfc.ac_ok_since_ms   = 0U;   // 防止绕过去抖
		}
/* 3) EN 启动延时 */
        if (s_pfc.startup_cmd_ms == 0U) {
            s_pfc.startup_cmd_ms = g_ms;
        }
		/* EN 时序：到点再拉起硬件 */
		if (!s_pfc.hw_enabled && elapsed_reached(s_pfc.startup_cmd_ms, PFC_STARTUP_DELAY_MS)) {
			pfc_hw_set_main(true);
			s_pfc.hw_enabled = true;
			/* 三态机跑通阶段：BUS_ADJ 保持 0，交给 NCP1654 自己闭环 */
			pfc_pwm_set(0.0f);
		}
		/* 确保硬件状态与enable_cmd一致 */
		else if(s_pfc.hw_enabled && !s_pfc.enable_cmd)
		{
			pfc_hw_set_main(false);
			s_pfc.hw_enabled = false;
		}
        /* READY 判定：VBUS 达到门限并保持一定时间 */
        if (s_pfc.hw_enabled) {
            if (vbus_v >= PFC_VBUS_READY_V) {
                if (s_pfc.vbus_ok_since_ms == 0U) {
                    s_pfc.vbus_ok_since_ms = g_ms;
                } else if (elapsed_reached(s_pfc.vbus_ok_since_ms, PFC_READY_DELAY_MS)) {
                    pfc_state_enter(PFC_ST_READY);
                }
            } else if (vbus_v < (PFC_VBUS_READY_V - PFC_VBUS_OK_RESET_MARGIN_V)) {
                /* 只有明显回落才清零，避免抖动 */
                s_pfc.vbus_ok_since_ms = 0U;
            }
        } else {
            s_pfc.vbus_ok_since_ms = 0U;
        }

        break;
    case PFC_ST_READY:
        /* 用户撤销使能：回 IDLE 关断 */
        if (!s_pfc.enable_cmd) {
            pfc_state_enter(PFC_ST_IDLE);
			/* 确保退出READY时清理相关计时器 */
			s_pfc.vbus_ok_since_ms = 0U;
			s_pfc.dropout_since_ms = 0U;
			s_pfc.ac_loss_since_ms = 0U;
            break;
        }
        /* READY 下 AC 掉电去抖：掉电回 IDLE */
        if (!pfc_ac_ok()) {
            if (s_pfc.ac_loss_since_ms == 0U) {
                s_pfc.ac_loss_since_ms = g_ms;
            } else if (elapsed_reached(s_pfc.ac_loss_since_ms, PFC_AC_LOSS_DEBOUNCE_MS)) {
                pfc_state_enter(PFC_ST_IDLE);
                break;
            }
        } else {
            s_pfc.ac_loss_since_ms = 0U;
        }

        /* READY 状态确保硬件保持开启（若被外部关断，这里会重新拉起） */
        if (!s_pfc.hw_enabled) {
            pfc_hw_set_main(true);
            s_pfc.hw_enabled = true;
        }
        pfc_pwm_set(0.0f);
		
		/* 掉电去抖：VBUS 低于阈值持续一段时间才退回 IDLE */
		if (vbus_v >= PFC_VBUS_DROPOUT_THRESHOLD_V) {
			s_pfc.dropout_since_ms = 0U;
		} else {
			if (s_pfc.dropout_since_ms == 0U) {
				s_pfc.dropout_since_ms = g_ms;
			} else if (elapsed_reached(s_pfc.dropout_since_ms, PFC_VBUS_DROPOUT_MS)) {
				pfc_state_enter(PFC_ST_IDLE);
			}
		}
		break;


    case PFC_ST_FAULT: 
		/*故障状态下确保输出保持关闭*/
		pfc_outputs_off();
		 /* 若故障仍存在，继续停在 FAULT */
		if (protect_fault_active_hw() || protect_fault_latched()) {
			break;
		}
		
/* 故障已解除：两种退出路径
		   1) 用户撤销 enable -> 直接回 IDLE
		   2) 自动重试：enable 仍在 + 等待 restart 延时 -> 清锁存 + clear pulse -> 回 IDLE */
		if (!s_pfc.enable_cmd) {
			pfc_state_enter(PFC_ST_IDLE);
		    /* 清理故障状态标志 */
			s_pfc.clear_sent = 0U;
			break;
		}  
		/* 自动重试：冷却时间到 -> clear latch + clear pulse（防抖）-> 回 IDLE */
		if (elapsed_reached(s_pfc.fault_since_ms, PFC_FAULT_RESTART_MS)) {
				if (protect_fault_latched()) {
						protect_clear_fault();
				}
				if (!s_pfc.clear_sent) {
						s_pfc.clear_sent = 1U;
				}
				pfc_state_enter(PFC_ST_IDLE);
		}
		break;
    default:
		/* 未知状态：安全关断并进入故障状态 */
        pfc_outputs_off();
        pfc_state_enter(PFC_ST_FAULT);
 		break;
	
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
