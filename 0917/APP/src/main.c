
/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "main.h"
#include <math.h>
#include "add_dma.h"
#include "pwm_llc.h"
#include "RCU.h"
#include "ICU.h"
#include "debug_printf.h"
#include "pwm.h"
#include "protect_exti.h"
#include "llc_open_loop.h"
#include "pfc_control.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

#define LLC_USE_OPEN_LOOP 0

#define Bus_Adj 0

/* ======================== Bring-up configuration ======================== */
/* Keep PFC control bypassed so that the firmware only exercises the LLC open-loop sweep for hardware validation. */
#define LLC_BRINGUP_OPEN_LOOP_ONLY   1

#if LLC_BRINGUP_OPEN_LOOP_ONLY
#define LLC_BYPASS_PFC_CONTROL      1
#else
#define LLC_BYPASS_PFC_CONTROL      0
#endif

/* 控制循环参数（1 kHz） */
#define CONTROL_LOOP_HZ            (1000U)
#define CONTROL_LOOP_DT_S          (1.0f / (float)CONTROL_LOOP_HZ)
/* 主循环一次最多处理的 tick，超过将计数为丢弃（避免主循环长时间占用） */
#define MAX_TICKS_PER_LOOP         (5U)
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/

enum{
	LLC_START_DELAY_MS = 10000
};

typedef struct
{
	llc_state_t state;
	uint32_t entry_ms;
}llc_app_ctx_t;

typedef struct
{
	pfc_state_t state;
	uint32_t entry_ms;
	uint32_t vbus_ok_since_ms; //表示总线电压自从变为ok后的时间点，如果未稳定通常约定为0
	uint32_t dropout_since_ms; //表示发生掉电/失稳时间的时间点，用来判断是否需要进入减载或者重试逻辑
	uint32_t startup_cmd_ms;   
	bool enable_cmd;
} pfc_app_ctx_t;

typedef struct
{
	bool active;  //是否激活软启动
	bool pause;  // 是否暂停软启动
	uint32_t paused_elapsed_ms;  // 暂停时已运行的毫秒数
	uint32_t start_ms;  //启动时间（毫秒）
	uint32_t duration_ms; // 软启动持续时间（毫秒）
	float start_duty; //起始占空比
	float target_duty; //目标占空比
	
// 运行时计算得到的安全上下限
	float    duty_min_safe;
	float    duty_max_safe;
	
	float last_duty;
} llc_softstart_ctx_t;

typedef struct
{
        bool active;
        uint16_t duration_ms;
        uint32_t start_ms;
} protect_clear_pulse_ctx_t;

static protect_clear_pulse_ctx_t s_protect_clear_pulse = { false, 0U, 0U };

#if AUX_POWER_MONITOR_ENABLE
static aux_power_monitor_t s_aux_power = {
	.v3v3_v = 0.0f,
	.vbat_v = 0.0f,
	.v3v3_min_v = AUX_V3V3_OK_MIN_V,
	.vbat_min_v = AUX_VBAT_OK_MIN_V,
	.last_update_ms = 0U,
	.drop_detected_ms = 0U,
	.restore_detected_ms = 0U,
	.power_ok = false,
};
#else
static aux_power_monitor_t s_aux_power = {
	.v3v3_v = 0.0f,
	.vbat_v = 0.0f,
	.v3v3_min_v = AUX_V3V3_OK_MIN_V,
	.vbat_min_v = AUX_VBAT_OK_MIN_V,
	.last_update_ms = 0U,
	.drop_detected_ms = 0U,
	.restore_detected_ms = 0U,
	.power_ok = true,
};
#endif
	

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/
static float s_pfc_bus_v = 0.0f;
static bool s_pfc_hw_enabled = false;
static bool s_pfc_hw_relay = false;


static llc_t s_llc;
static llc_app_ctx_t s_llc_app;
static pfc_app_ctx_t s_pfc_app;


#if LLC_SOFTSTART_ENABLE
static llc_softstart_ctx_t s_llc_softstart;
#endif

#if LLC_USE_OPEN_LOOP
static llc_open_loop_ctrl_t s_llc_open_loop;
static bool s_llc_open_loop_completed = false;
static float s_llc_open_loop_final_freq = LLC_F_INIT_HZ;
static const llc_open_loop_segment_t s_llc_open_loop_profile[] = {
	//{ .start_hz = LLC_F_MAX_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = LLC_F_SLEW_HZ, .hold_time_ms = 100U },
	{ .start_hz = LLC_F_INIT_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = 0, .hold_time_ms = 100U },
};
#endif

volatile uint32_t g_ms=0;
static volatile uint32_t s_control_tick_pending = 0U;


static volatile uint32_t s_tick_drop_count = 0U; /* 被丢弃的 tick 计数 */


void delay_ms(uint32_t duration_ms)
{
    if (duration_ms == 0U) {
        return;
    }

    uint32_t start_ms = g_ms;
    while ((uint32_t)(g_ms - start_ms) < duration_ms) {
        __NOP();
    }
}

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void aux_power_monitor_update(float v3v3_v, float vbat_v);
static void llc_softstart_reset(void);
static void llc_softstart_begin(float target_duty);
static void llc_softstart_tick(void);

static void llc_state_enter(llc_state_t next);
static void pfc_state_enter(pfc_state_t next);
static void protect_hw_clear_pulse_tick(void);
static void protect_clear_gpio_init(void); // ← 新增：清锁存脚初始化

static void pfc_hw_init(void);
static void pfc_hw_set_enable(bool en);
static void pfc_hw_set_relay(bool closed);
void systick_1ms_init(void);

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
void systick_config(void)
{
    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U)){
        /* capture error */
        while (1){
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}
void systick_1ms_init(void){
		SystemCoreClockUpdate();    
     uint32_t reload  = SystemCoreClock / 1000U;
	  if (reload == 0U || reload > SysTick_LOAD_RELOAD_Msk) {
                                           // 失败：频率异常或超出24位
    }
		reload -= 1U; //调整重载值，确保定时器行为符合预期。
		if (reload > SysTick_LOAD_RELOAD_Msk) {
			reload = SysTick_LOAD_RELOAD_Msk;
		}
		
		SysTick->CTRL = 0U;  //先禁用 SysTick。
		SysTick->LOAD = reload; //设置重载值。
		SysTick->VAL  = 0U; //清除当前计数值。
    //NVIC_SetPriority(SysTick_IRQn, 0x0F);
		NVIC_SetPriority(SysTick_IRQn, irq_priority_encode(IRQ_PRIO_SYSTICK_PREEMPT, IRQ_PRIO_SYSTICK_SUB));
		//设置 SysTick 的时钟源。如果该位被置 1，表示使用处理器时钟（HCLK）；如果为 0，表示使用 HCLK 的 8 分频。
		//控制 SysTick 中断的启用。如果该位被置 1，表示允许 SysTick 定时器在计数到 0 时触发中断。
		//控制 SysTick 定时器的启用。如果该位被置 1，表示启动定时器计数。
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  
								SysTick_CTRL_TICKINT_Msk   |
								SysTick_CTRL_ENABLE_Msk;
}

static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot){
    float v = (raw * VREF_ADC) / 4095.0f;
    return v * (rtop + rbot) / rbot;
}

//去偏置
//float v_net = v_adc - v_zero;               // 去偏置
//return v_net / (ISHUNT_OHM * IAMP_GAIN);    // 单位：安培
//v_zero≈0.17V
static inline float conv_adc_to_i(uint16_t raw){
    float v = (raw * VREF_ADC) / 4095.0f;
		float v1 = v - 0.17;              // 去偏置
    return v1 / (ISHUNT_OHM * IAMP_GAIN);
}


static inline float f_absf(float x){ return x < 0 ? -x : x; }
static inline float f_minf(float a,float b){ return a < b ? a : b; }
static inline float f_maxf(float a,float b){ return a > b ? a : b; }
static inline float f_clampf(float x,float lo,float hi)
{ return x<lo?lo:(x>hi?hi:x); }

static inline uint32_t elapsed_since(uint32_t start_ms)
{
	return (start_ms == 0U) ? 0U : (uint32_t)(g_ms - start_ms);
}

void protect_hw_clear_pulse(uint16_t pulse_ms)
{
    uint16_t duration = pulse_ms;
    if (duration == 0U) {
        duration = 10U;
    }

    gpio_bit_set(GPIOC, GPIO_PIN_11);
    s_protect_clear_pulse.active = true;
    s_protect_clear_pulse.duration_ms = duration;
    s_protect_clear_pulse.start_ms = g_ms;
}

static void protect_hw_clear_pulse_tick(void)
{
    if (!s_protect_clear_pulse.active) {
        return;
    }

    if ((uint32_t)(g_ms - s_protect_clear_pulse.start_ms) >= s_protect_clear_pulse.duration_ms) {
        gpio_bit_reset(GPIOC, GPIO_PIN_11);
        s_protect_clear_pulse.active = false;
        s_protect_clear_pulse.start_ms = 0U;
    }
}

//该函数用于判断从给定的起始时间（毫秒）开始，是否已经经过了指定的持续时间（毫秒）
static inline bool elapsed_reached(uint32_t start_ms, uint32_t duration_ms)
{
	if(duration_ms == 0)
	{
		return true;
	}
	if(start_ms == 0)
	{
		return false;
	}
	return elapsed_since(start_ms) >= duration_ms;
}

static void aux_power_monitor_update(float v3v3_v, float vbat_v)
{
#if AUX_POWER_MONITOR_ENABLE
	s_aux_power.v3v3_v = v3v3_v;
	s_aux_power.vbat_v = vbat_v;
	
	if(v3v3_v < s_aux_power.v3v3_min_v){
		s_aux_power.v3v3_min_v = v3v3_v;
	}
	
	if(vbat_v < s_aux_power.vbat_min_v)
	{
		s_aux_power.vbat_min_v = vbat_v;
	}
	
	bool aux_ok = (v3v3_v >= AUX_V3V3_OK_MIN_V) && (vbat_v >= AUX_VBAT_OK_MIN_V);
	
	if(aux_ok)
	{
		if(!s_aux_power.power_ok)
		{
			s_aux_power.restore_detected_ms = g_ms;
		}
	}
	else{
		if(s_aux_power.power_ok)
		{
			s_aux_power.drop_detected_ms = g_ms;
		}
	}
	s_aux_power.power_ok = aux_ok;
	s_aux_power.last_update_ms = g_ms;
#else
	(void)v3v3_v;
	(void)vbat_v;
#endif
}
static inline bool aux_power_ok_now(void)
{
	#if AUX_POWER_MONITOR_ENABLE
    return s_aux_power.power_ok;
	#else
		return true;
	#endif
}
/*********************************************************************************************************
* 函数名称：aux_power_ok_stable_since
* 函数功能：用于判断辅助电源是否在指定的时间范围内稳定恢复。
* 输入参数：ms
* 输出参数：void
* 返 回 值：bool
* 创建日期：2025年10月21日
* 注    意： 
*********************************************************************************************************/
static inline bool aux_power_ok_stable_since(uint32_t ms)
{
	#if AUX_POWER_MONITOR_ENABLE
    /* 恢复去抖：要求 power_ok=true 且恢复时间记过阈值 */
    if (!s_aux_power.power_ok) return false;
    if (s_aux_power.restore_detected_ms == 0U) return false;
    return (uint32_t)(g_ms - s_aux_power.restore_detected_ms) >= ms;
	#else
		(void)ms;
		return false;
	#endif
}
/*********************************************************************************************************
* 函数名称：aux_power_brownout_stable
* 函数功能：检查辅助电源掉电是否稳定
* 输入参数：ms
* 输出参数：void
* 返 回 值：bool
* 创建日期：2025年10月21日
* 注    意：返回true表示掉电稳定，false表示不稳定 
*********************************************************************************************************/
static inline bool aux_power_brownout_stable(uint32_t ms)
{
    /* 掉电去抖：要求 power_ok=false 且掉电时间记过阈值 */
    if (s_aux_power.power_ok) return false;
    if (s_aux_power.drop_detected_ms == 0U) return false;
    return (uint32_t)(g_ms - s_aux_power.drop_detected_ms) >= ms;
}


#if LLC_SOFTSTART_ENABLE

/* 根据当前周期/死区，计算“有效占空安全窗” */
static void ss_update_safe_window(void)
{
	  uint32_t per_ns = llc_pwm_get_period_ns();  //获取 PWM 周期时间（单位为纳秒）
    uint32_t dt_ns  = llc_pwm_get_deadtime_ns(); //获取 PWM 死区时间（单位为纳秒）
	  float guard = 0.0f;
    if (per_ns > 0U && dt_ns <= (UINT32_MAX / 2)) {
        /* 互补两沿都插死区：保守取 2*deadtime */
        guard = (2.0f * (float)dt_ns) / (float)per_ns;  // 0~1
    }
		else
		{
			guard = 0.0f;
		}

		guard += LLC_SOFTSTART_EXTRA_MARGIN; //  其中guard为死区时间保护带加上额外经验余量。

    /* 下限不超过 0.49，上限不低于 0.51，避免靠近 50% 附近偶发直通/无效脉宽 */
    s_llc_softstart.duty_min_safe = f_clampf(guard, 0.0f, 0.49f);
    s_llc_softstart.duty_max_safe = f_clampf(1.0f - guard,  0.51f, 0.99f);
}

/* 余弦 S 曲线：0→1 */
static inline float ease_cos(float t)
{
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    return 0.5f * (1.0f - cosf(3.1415926535f * t));
}

static void ss_apply(float duty)
{
    float d = f_clampf(duty, s_llc_softstart.duty_min_safe, s_llc_softstart.duty_max_safe);
		s_llc_softstart.last_duty = d;
    llc_pwm_set_duty(d);
}


/* 指数曲线：0→1 */
static inline float ease_exp(float t, float k)
{
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    float denom = 1.0f - expf(-k);
    if (denom < 1e-6f) return t;
    return (1.0f - expf(-k * t)) / denom;
}
static void llc_softstart_reset(void)
{
	/* ========== 1. 清除内部状态标志 ========== */
		s_llc_softstart.active = false;
		s_llc_softstart.pause = false;
		s_llc_softstart.paused_elapsed_ms = 0U;
		s_llc_softstart.start_ms = 0U;
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;
	/* ========== 2. 设置初始/目标占空比 ========== */
		s_llc_softstart.start_duty = f_clampf(LLC_SOFTSTART_START_DUTY, 0.0f, 0.99f);  // 0.1

		s_llc_softstart.target_duty =  f_clampf(LLC_SOFTSTART_TARGET_DUTY, 0.0f, 0.99f);  //0.5
	
	/* 根据当前周期/死区，计算“有效占空安全窗” */
	/* ========== 3. 计算安全占空窗口 ========== */
		ss_update_safe_window(); //更新了 s_llc_softstart.duty_min_safe和s_llc_softstart.duty_max_safe
	
	/* 确保初始占空在安全范围内 */
	    if (s_llc_softstart.start_duty < s_llc_softstart.duty_min_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_min_safe;
    else if (s_llc_softstart.start_duty > s_llc_softstart.duty_max_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_max_safe;
	
	/* ========== 4. 应用初始占空比 ========== */
		ss_apply(s_llc_softstart.start_duty); //0.1
		s_llc_softstart.last_duty = s_llc_softstart.start_duty;
	/* ========== 5. 调试日志（可选） ========== */
#if defined(DEBUG_PRINTF_LLCSOFTSTART)
    debug_printf("[LLC-OpenLoop-SS] reset: start=%.3f target=%.3f dur=%lu ms safe[%.3f, %.3f]\n",
                 s_llc_softstart.start_duty,
                 s_llc_softstart.target_duty,
                 (unsigned long)s_llc_softstart.duration_ms,
                 s_llc_softstart.duty_min_safe,
                 s_llc_softstart.duty_max_safe);
#endif
}
static void llc_softstart_begin(float target_duty)
{
	 /* ========== 1. 参数初始化 ========== */
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;  //软启动的总持续时间（毫秒）
		/* 起始与目标占空比安全钳位 */
		float start_duty = f_clampf(LLC_SOFTSTART_START_DUTY, 0.0f, 0.99f);
		float final_duty = f_clampf(target_duty, 0.0f, 0.99f);
	
		/* ========== 2. 清状态标志 ========== */
		s_llc_softstart.pause      = false;  //标志位，指示是否暂停软启动,这边是不暂停软启动
		s_llc_softstart.paused_elapsed_ms = 0U;
	
		s_llc_softstart.start_duty = start_duty;
		s_llc_softstart.target_duty = final_duty;
	/* ========== 3. 计算安全占空窗 ========== */
		ss_update_safe_window();
	/* 强制占空比落入安全范围 */
	  if (s_llc_softstart.start_duty < s_llc_softstart.duty_min_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_min_safe;
    else if (s_llc_softstart.start_duty > s_llc_softstart.duty_max_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_max_safe;

    if (s_llc_softstart.target_duty < s_llc_softstart.duty_min_safe)
        s_llc_softstart.target_duty = s_llc_softstart.duty_min_safe;
    else if (s_llc_softstart.target_duty > s_llc_softstart.duty_max_safe)
        s_llc_softstart.target_duty = s_llc_softstart.duty_max_safe;
	
	/* ========== 4. 判定是否跳过软启动 ========== */
		//如果目标占空比 target_duty 小于或等于初始占空比 start_duty ，或者软启动时间为 0，则直接跳过软启动：
		if(final_duty <= start_duty || s_llc_softstart.duration_ms == 0U)
		{
			s_llc_softstart.active = false; //标志位，指示软启动是否正在进行,这边是未在进行
			s_llc_softstart.start_duty = final_duty;
			s_llc_softstart.target_duty = final_duty;
			ss_apply(final_duty);
			s_llc_softstart.last_duty    = final_duty;
#if defined(DEBUG_PRINTF_LLCSOFTSTART)
        debug_printf("[LLC-SS] skipped: fixed duty=%.3f (no ramp)\n", final_duty);
#endif
			return;
		}
		else
		{
			s_llc_softstart.active = true; //软启动正在进行
			s_llc_softstart.start_ms = g_ms; //记录当前时间戳 start_ms 
			ss_apply(start_duty);
			s_llc_softstart.last_duty = start_duty;
#if defined(DEBUG_PRINTF_LLCSOFTSTART) && (DEBUG_PRINTF_LLCSOFTSTART)
    debug_printf("[LLC-SS] begin: start=%.3f, target=%.3f, dur=%lu ms, safe[%.3f, %.3f], mode=%s\n",
                 s_llc_softstart.start_duty,
                 s_llc_softstart.target_duty,
                 (unsigned long)s_llc_softstart.duration_ms,
                 s_llc_softstart.duty_min_safe,
                 s_llc_softstart.duty_max_safe,
#if defined(LLC_SOFTSTART_USE_COSINE_EASE) && (LLC_SOFTSTART_USE_COSINE_EASE)
                 "cosine");
#else
                 "exp");
#endif
#endif
		}
}
void llc_softstart_update_target(float new_target_0_1)
{
    s_llc_softstart.target_duty = f_clampf(new_target_0_1, 0.0f, 0.99f);
}

/*********************************************************************************************************
* 函数名称：llc_softstart_set_pause
* 函数功能：这段代码的主要功能是控制软启动（soft start）的暂停和恢复逻辑。
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月20
* 注    意：代码通过设置 pause 标志来控制软启动过程的暂停和恢复。
			暂停逻辑：当 pause 为 true 时，记录当前软启动已经运行的时间（ elapsed ），并将其保存到 paused_elapsed_ms 中，同时标记软启动为暂停状态。
			恢复逻辑：当 pause 为 false 时，根据之前保存的暂停时间（ paused_elapsed_ms ）重新计算软启动的起始时间（ start_ms ），并恢复软启动过程。
*********************************************************************************************************/
void llc_softstart_set_pause(bool pause)
{
    if(pause)
		{
			if (!s_llc_softstart.pause && s_llc_softstart.active)
			{
				//如果软启动未暂停且处于激活状态，计算从开始到当前的时间差
				uint32_t elapsed = (uint32_t)(g_ms - s_llc_softstart.start_ms);
				
				if (elapsed > s_llc_softstart.duration_ms)
				{
					elapsed = s_llc_softstart.duration_ms; //确保 elapsed 不超过总持续时间 duration_ms 。
				}
				s_llc_softstart.paused_elapsed_ms = elapsed; //保存 elapsed 到 paused_elapsed_ms ，并标记为暂停状态
			}
			s_llc_softstart.pause = true;
		}
		else
		{
			 if (s_llc_softstart.pause && s_llc_softstart.active)
			 {
				 //如果软启动处于暂停状态且激活，读取之前保存的 paused_elapsed_ms
				 uint32_t elapsed = s_llc_softstart.paused_elapsed_ms;  //paused_elapsed_ms ：记录暂停时已经运行的时间
					if (elapsed > s_llc_softstart.duration_ms)
					{
						//确保 elapsed 不超过总持续时间 duration_ms 。 当前软启动已经运行的时间（ elapsed ）
							elapsed = s_llc_softstart.duration_ms;
					}
					if (g_ms >= elapsed)
					{
						//根据当前时间 g_ms 和 elapsed 重新计算 start_ms ，确保时间逻辑正确。
							s_llc_softstart.start_ms = g_ms - elapsed;
					}
					else
					{
							s_llc_softstart.start_ms = 0U;
					}
			 }
			 s_llc_softstart.pause = false;
		}
}

void llc_softstart_abort(void)
{
    s_llc_softstart.active = false;
    s_llc_softstart.pause = false;
		s_llc_softstart.paused_elapsed_ms = 0U;
    ss_update_safe_window(); // 以防期间改过频率/死区 卡的是周期和死区时间吧
    ss_apply(f_clampf(LLC_SOFTSTART_FAILSAFE_DUTY, 0.0f, 0.99f)); //0.0
}
static void llc_softstart_tick(void)
{
    if (!s_llc_softstart.active) 
		{
			return;
		}
		
    /* 用你项目已有的故障判据 */
    if (protect_fault_latched() || protect_fault_active_hw()) {
        llc_softstart_abort();
        return;
    }
		 /* 更新安全窗口（频率变化时死区可能变化） */
		ss_update_safe_window();
		/* 若暂停，则保持当前占空比不变 */
		if (s_llc_softstart.pause) 
		{
			ss_apply(s_llc_softstart.last_duty);
			return;
		}
		 /* 计算已运行时间 */
		uint32_t elapsed = (uint32_t)(g_ms - s_llc_softstart.start_ms);
		
		 /* 若软启动时间到达或超时 → 锁定目标占空 */
		if(elapsed >= s_llc_softstart.duration_ms)  // 软启动持续时间（毫秒）
		{
			s_llc_softstart.active = false;
			ss_apply(s_llc_softstart.target_duty);
			s_llc_softstart.last_duty = s_llc_softstart.target_duty;
#if defined(DEBUG_PRINTF_LLCSOFTSTART) 
        debug_printf("[LLC-SS] done: duty=%.3f after %lu ms\n",
                     s_llc_softstart.target_duty, (unsigned long)elapsed);
#endif
			return;
		}
		 /* 计算进度 (0.0~1.0) */
		float progress = (s_llc_softstart.duration_ms > 0U) ? ((float)elapsed / (float)s_llc_softstart.duration_ms) : 1.0f;

		
		 /* 将线性换成 S 曲线（如需线性，把 ease 改成 progress） */
#if defined(LLC_SOFTSTART_USE_COSINE_EASE) && (LLC_SOFTSTART_USE_COSINE_EASE)
    float k = ease_cos(progress);
#else
    float k = ease_exp(progress, LLC_SOFTSTART_EXP_K);
#endif
		
		//使用线性插值公式计算当前占空比
		float duty = s_llc_softstart.start_duty +
								 (s_llc_softstart.target_duty - s_llc_softstart.start_duty) * k;
		duty = f_clampf(duty, s_llc_softstart.duty_min_safe, s_llc_softstart.duty_max_safe);
		ss_apply(duty);
		s_llc_softstart.last_duty = duty;
}

#else

static void llc_softstart_reset(void)
{
                //llc_pwm_set_duty(f_clampf(LLC_PWM_DUTY, 0.0f, 0.99f));
}
static void llc_softstart_begin(float target_duty)
{
                //llc_pwm_set_duty(f_clampf(target_duty, 0.0f, 0.99f));
}
static void llc_softstart_tick(void)
{
                /* Soft-start disabled. Nothing to do. */
}
#endif /* LLC_SOFTSTART_ENABLE */

static void pfc_hw_init(void)
{
	#if defined(PFC_EN_PORT)&&defined(PFC_EN_PIN)&&defined(PFC_EN_RCU)
		rcu_periph_clock_enable(PFC_EN_RCU);
	 	gpio_init(PFC_EN_PORT,GPIO_MODE_OUT_PP,GPIO_OSPEED_50MHZ,PFC_EN_PIN);
		gpio_bit_reset(PFC_EN_PORT, PFC_EN_PIN);
	#endif
	
	#if defined(PFC_MAIN_RELAY_PORT)&&defined(PFC_MAIN_RELAY_PIN)&&defined(PFC_MAIN_RELAY_RCU)
		rcu_periph_clock_enable(PFC_MAIN_RELAY_RCU);
		gpio_init(PFC_MAIN_RELAY_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PFC_MAIN_RELAY_PIN);
		gpio_bit_reset(PFC_MAIN_RELAY_PORT, PFC_MAIN_RELAY_PIN);
	#endif
	
	s_pfc_hw_enabled = false;
	s_pfc_hw_relay = false;
}

/* PFC READY 且已在 READY 态保持一小段时间，供 LLC 启动前做最后确认 */
static inline bool pfc_ready_for_llc(void)
{
    return (s_pfc_app.state == PFC_ST_READY) &&
           elapsed_reached(s_pfc_app.entry_ms, PFC_READY_STABLE_BEFORE_LLC_MS);
}
static void pfc_hw_set_enable(bool en)
{
	if(s_pfc_hw_enabled == en)
	{
		return;
	}
	s_pfc_hw_enabled = en;
	#if defined(PFC_EN_PORT)&&defined(PFC_EN_PIN)
		if(en)
		{
			gpio_bit_set(PFC_EN_PORT,PFC_EN_PIN);
		}
		else
		{
			gpio_bit_reset(PFC_EN_PORT,PFC_EN_PIN);
		}
	#else
		(void)en;
	#endif
}

static void pfc_hw_set_relay(bool closed)  //PA12
{
	if(s_pfc_hw_relay == closed)
	{
		return;
	}
	s_pfc_hw_relay = closed;
	#if defined(PFC_MAIN_RELAY_PORT)&&defined(PFC_MAIN_RELAY_PIN)
		if(closed)
		{
			gpio_bit_set(PFC_MAIN_RELAY_PORT, PFC_MAIN_RELAY_PIN);
		}
		else
		{
			gpio_bit_reset(PFC_MAIN_RELAY_PORT, PFC_MAIN_RELAY_PIN);
		}
	#else
		(void)closed;
	#endif
}
#if LLC_BYPASS_PFC_CONTROL
static void pfc_state_enter(pfc_state_t next)
{
	(void)next;
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = g_ms;
	s_pfc_app.enable_cmd = true;
	pfc_hw_set_enable(false);
}
#else
static void pfc_state_enter(pfc_state_t next)
{
	s_pfc_app.state = next;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = 0U;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = 0U;
	switch(next)
	{
		case PFC_ST_IDLE:
			pfc_hw_set_enable(false);

			break;
		case PFC_ST_READY:
			// 到这一步才合上继电器
			pfc_hw_set_enable(true);
			//pfc_hw_set_relay(true);
			s_pfc_app.vbus_ok_since_ms = g_ms;
			break;
		case PFC_ST_FAULT:
		default:
			pfc_hw_set_enable(false);
			//pfc_hw_set_relay(false);
			break;
	}
}
#endif

void pfc_app_init()
{
	pfc_hw_init();
	s_pfc_bus_v = 0.0f;
	
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = g_ms;
	s_pfc_app.enable_cmd = true;
	
#else
	s_pfc_app.state = PFC_ST_IDLE;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = 0U;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = 0U;
	s_pfc_app.enable_cmd = false;
#endif
	pfc_hw_set_enable(false);
}
//该函数的目的是在启动某种请求时，确保系统状态正确，并记录请求的起始时间（如果系统当前不处于空闲状态）。
void pfc_app_request_start(void)
{
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_app.enable_cmd = true;
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.startup_cmd_ms = g_ms;
	return;
#else
  if(!s_pfc_app.enable_cmd)
	{
		s_pfc_app.enable_cmd = true;
	}
	if(s_pfc_app.state == PFC_ST_IDLE)
	{
		s_pfc_app.startup_cmd_ms = 0U;
		s_pfc_app.entry_ms = g_ms;
	}
	else
	{
		s_pfc_app.entry_ms = g_ms;
	}
#endif
}

void pfc_app_force_off()
{
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_app.enable_cmd = true;
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.startup_cmd_ms = g_ms;
	return;
#else
	s_pfc_app.enable_cmd = false;
	if(s_pfc_app.state != PFC_ST_IDLE)
	{
		 pfc_state_enter(PFC_ST_IDLE);
	}
#endif
}
void pfc_app_tick_1khz(float vbus_v)
{
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_bus_v = vbus_v;
	s_pfc_app.enable_cmd = true;
	if(s_pfc_app.state != PFC_ST_READY)
	{
		s_pfc_app.state = PFC_ST_READY;
		s_pfc_app.entry_ms = g_ms;
		s_pfc_app.vbus_ok_since_ms = g_ms;
		s_pfc_app.startup_cmd_ms = g_ms;
	}
	return;
#else
	s_pfc_bus_v = vbus_v;
	static uint8_t s_pfc_clear_sent = 0;  // 防抖，只发一次
	// 检查是否存在故障（硬件故障或锁存故障）
	bool fault_active = protect_fault_latched()||protect_fault_active_hw();
	//排除故障状态
	if(fault_active && s_pfc_app.state != PFC_ST_FAULT)
	{
		pfc_state_enter(PFC_ST_FAULT);
		return;
	}
		/* 任何时刻辅源棕断稳定 → 直接退回 IDLE 并关断硬件 */
	if (aux_power_brownout_stable(AUX_DROP_DEBOUNCE_MS)){
			if (s_pfc_app.state != PFC_ST_IDLE) 
				pfc_state_enter(PFC_ST_IDLE);
			return;
	}

	switch(s_pfc_app.state)
	{
		case PFC_ST_IDLE:
			 /* 必须先保证辅源已经恢复且去抖通过 */
			if (!aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS)){
				s_pfc_app.startup_cmd_ms = 0U;
				// 如果硬件已启用，则禁用硬件
				if(s_pfc_hw_enabled)
				{
					pfc_hw_set_enable(false);
					s_pfc_hw_enabled = false; // 更新状态
				}
				s_pfc_app.vbus_ok_since_ms = 0U;
				break;
			}
			/* 未使能 → 保持硬件关闭并清计时 */
			if(!s_pfc_app.enable_cmd)
			{
				if(s_pfc_hw_enabled)
				{
					pfc_hw_set_enable(false);
					s_pfc_hw_enabled = false; // 更新状态
				}
				// 重置启动命令计时器和电压稳定计时器
				s_pfc_app.startup_cmd_ms = 0U;
				s_pfc_app.vbus_ok_since_ms = 0U;
				break;
			}
			// 如果启动命令计时器未初始化，则初始化
			if(s_pfc_app.startup_cmd_ms == 0U)
			{
				s_pfc_app.startup_cmd_ms = g_ms;
			}
			//PFC_STARTUP_DELAY_MS ：启动延时，确保 PFC 硬件在启用前等待足够时间。
    /* 达到启动延时后，允许上电驱动 */
    if (elapsed_reached(s_pfc_app.startup_cmd_ms, PFC_STARTUP_DELAY_MS) && !s_pfc_hw_enabled) {
        pfc_hw_set_enable(true);
        s_pfc_hw_enabled = true;
    }
		    /* 电压达到 READY 门限后开始计稳定时间；仅在“明显回落”时才清零 */
    if (vbus_v >= PFC_VBUS_READY_V) {
        if (s_pfc_app.vbus_ok_since_ms == 0U) {
            s_pfc_app.vbus_ok_since_ms = g_ms;
        } else if (elapsed_reached(s_pfc_app.vbus_ok_since_ms, PFC_READY_DELAY_MS)) {
            pfc_state_enter(PFC_ST_READY);
        }
    } else if (vbus_v < (PFC_VBUS_READY_V - PFC_VBUS_OK_RESET_MARGIN_V)) {
        /* 只有明显回落才清零，避免轻微抖动导致频繁清零 */
        s_pfc_app.vbus_ok_since_ms = 0U;
    }
			break;
		case PFC_ST_READY:
			// 检查是否收到禁用命令
			if(!s_pfc_app.enable_cmd)
			{
				pfc_state_enter(PFC_ST_IDLE);
				break;
			}
    /* 使用更低的阈值 + 更长的掉电延时，减少抖动引起的退出 */
    if (vbus_v >= PFC_VBUS_DROPOUT_THRESHOLD_V) {
        s_pfc_app.dropout_since_ms = 0U;
    } else {
        if (s_pfc_app.dropout_since_ms == 0U) {
            s_pfc_app.dropout_since_ms = g_ms;
        } else if (elapsed_reached(s_pfc_app.dropout_since_ms, PFC_VBUS_DROPOUT_MS_NEW)) {
            pfc_state_enter(PFC_ST_IDLE);
        }
    }
			break;
		case PFC_ST_FAULT:
			
		default:
			if(!fault_active)
			{
			 /* 用户撤销使能也允许退出 FAULT */ /* 条件 1：用户撤销 enable 且 BKIN 已经释放 → 直接回 IDLE */
				if (!s_pfc_app.enable_cmd && !protect_fault_active_hw()) {
						pfc_state_enter(PFC_ST_IDLE);
						s_pfc_clear_sent = 0;
				}
				
				/* 条件 2：允许自动重试：BKIN已高 + 辅源稳定 + 达到重试延时 */
				if(!protect_fault_active_hw()&&aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS)&&(uint32_t)(g_ms - s_pfc_app.entry_ms)>= PFC_RESTART_DELAY_MS)
				{
					 if (protect_fault_latched()) 
					 {
						 protect_clear_fault();
					 }
					 if (!s_pfc_clear_sent) {
							protect_hw_clear_pulse(10);
							s_pfc_clear_sent = 1;
           }
					pfc_state_enter(PFC_ST_IDLE);
					 s_pfc_clear_sent = 0;  // 复位节流
					 break;
				}

			}
			break;
		}
#endif
}

pfc_state_t pfc_app_state(void)
{
  return s_pfc_app.state;
}

bool pfc_app_ready(void)
{
#if LLC_BYPASS_PFC_CONTROL
        return true;
#else
	return s_pfc_app.state == PFC_ST_READY;
#endif
}

float pfc_bus_voltage(void)
{
	return s_pfc_bus_v;
}

/* ADC1通道14测试函数声明 */
uint16_t adc1_channel14_test(void);
uint16_t adc1_channel14_multiple_samples(uint16_t sample_count, uint16_t *samples);

void llc_step(llc_t* l){
	/* 1) 误差：目标电压 - 实测电压（单位V） */
    float e = l->vref - l->vmeas;
	/* 2) 积分累加：每个控制周期增加 ki*e（不考虑饱和） */
    l->integ += l->ki * e;
	/* 3) PI输出映射到频率请求：kp*e + I，再加基线 f_min（单位Hz） */
    float f_req = l->kp * e + l->integ + l->f_min;
	/* 4) 限幅 */
    if(f_req < l->f_min)   // 频率下限钳位：不可低于 f_min
			f_req = l->f_min;
    if(f_req > l->f_max)   // 频率上限钳位：不可高于 f_max
			f_req = l->f_max;
		
    float df = f_req - l->f_cmd;  // 期望频率与当前下发频率的差值 Δf
		
    if(f_absf(df) > l->f_slew)   // 斜率限幅：若 |Δf| 大于每周期最大步长 f_slew
			l->f_cmd += (df>0?l->f_slew:-l->f_slew); //就按正/负方向只移动 f_slew（限速变频）
    else 
			l->f_cmd = f_req; // 否则一步到位：直接把下发频率设为目标
}
static void llc_state_enter(llc_state_t next)
{
	// 更新 LLC 状态和进入时间
	s_llc_app.state = next;
	s_llc_app.entry_ms = g_ms;
	#if Bus_Adj
	bus_vol_adj_reset();   //重置总线电压调整逻辑 百分之五十的占空比
	#endif
	// 根据目标状态执行相应的初始化或清理操作
	switch(next)
	{
		case ST_IDLE:
#if LLC_USE_OPEN_LOOP
		llc_open_loop_stop(&s_llc_open_loop); // 停止开环控制（如果启用）
		s_llc_open_loop_completed = false;
#endif
			llc_softstart_reset(); // 重置软启动
			pfc_app_force_off();   // 强制关闭 PFC
			llc_pwm_outputs_enable(0); // 禁用 PWM 输出
			pfc_hw_set_relay(false); // 关闭继电器
			s_llc.integ = 0.0f; // 重置积分项
			s_llc.f_cmd = s_llc.f_max; // 设置频率为最大值
			break;
	  case ST_WAIT_AUX:  /* 新增：只在辅源稳定后才进入 WAIT_VBUS */
#if LLC_USE_OPEN_LOOP
        llc_open_loop_stop(&s_llc_open_loop);
        s_llc_open_loop_completed = false;
#endif
        llc_softstart_reset(); // 重置软启动
        pfc_app_force_off();   // 强制关闭 PFC
				pfc_hw_set_relay(false); // 关闭继电器
        llc_pwm_outputs_enable(0); // 禁用 PWM 输出
        
        s_llc.integ = 0.0f;  // 重置积分项
        s_llc.f_cmd = s_llc.f_max; // 设置频率为最大值
        break;
		case ST_WAIT_VBUS:
#if LLC_USE_OPEN_LOOP
			llc_open_loop_stop(&s_llc_open_loop);
			s_llc_open_loop_completed = false;
#endif
			llc_softstart_reset();
			pfc_app_request_start(); //记录请求的起始时间
			pfc_hw_set_relay(false);
			llc_pwm_outputs_enable(0);
			s_llc.integ = 0.0f;
			s_llc.f_cmd = s_llc.f_max;
			break;
		case ST_LLC_RUN:
			s_llc.integ = 0.0f;
#if LLC_USE_OPEN_LOOP
		if(!s_llc_open_loop_completed)
		{
			llc_open_loop_start(&s_llc_open_loop);
			s_llc.f_cmd = f_clampf(llc_open_loop_get_freq(&s_llc_open_loop), s_llc.f_min, s_llc.f_max);
		}
		else
		{
			s_llc.f_cmd = f_clampf(s_llc_open_loop_final_freq, s_llc.f_min, s_llc.f_max);
		}
		
#else
			s_llc.f_cmd = f_clampf(LLC_F_INIT_HZ, s_llc.f_min, s_llc.f_max);
#endif
			llc_softstart_begin(LLC_PWM_DUTY);
			llc_pwm_outputs_enable(1);
			// 添加调试信息输出
			debug_printf("[LLC] Starting open loop control. Initial frequency: %.1f Hz\n", s_llc.f_cmd);
			break;
		case ST_FAULT:
				
		default:	
#if LLC_USE_OPEN_LOOP
			llc_open_loop_stop(&s_llc_open_loop);
			s_llc_open_loop_completed = false;
#endif
			llc_softstart_reset();
			pfc_app_force_off();
			pfc_hw_set_relay(false);
			llc_pwm_outputs_enable(0);
			s_llc.f_cmd = s_llc.f_max;
			break;
	}
}

void llc_app_init()
{
	llc_state_enter(ST_WAIT_VBUS);
}
void llc_app_tick_1khz(void)
{
	 static uint8_t s_llc_clear_sent = 0;
	
	if(protect_fault_latched() || protect_fault_active_hw()||pfc_app_state() == PFC_ST_FAULT)
	{
		llc_state_enter(ST_FAULT);
		return;
	}
	
	if (aux_power_brownout_stable(AUX_DROP_DEBOUNCE_MS)){
			llc_state_enter(ST_WAIT_AUX);
			return;
	}
	
	switch(s_llc_app.state)
	{
		case ST_IDLE:
			llc_state_enter(ST_WAIT_VBUS);
			break;
		case ST_WAIT_AUX:
        if (aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS)){
            llc_state_enter(ST_WAIT_VBUS);
        }
        break;
		case ST_WAIT_VBUS:
#if LLC_BYPASS_PFC_CONTROL
			if(s_llc.vmeas >= LLC_ENTRY_V) 
#else
			if(pfc_app_ready() && (s_llc.vmeas >= LLC_ENTRY_V)) 
#endif
			{
				if (s_llc_app.entry_ms == 0U) 
				{
					s_llc_app.entry_ms = g_ms; // 可用于入门延时(若需要)
				}
				else if((uint32_t)(g_ms - s_llc_app.entry_ms) >= LLC_START_DELAY_MS)
				{
					pfc_hw_set_relay(true);
					llc_state_enter(ST_LLC_RUN);
				}
			}
			else{
				 s_llc_app.entry_ms = 0U;
				 pfc_hw_set_relay(false);
			}
			break;
		case ST_LLC_RUN:
				if(!aux_power_ok_now()){
					pfc_hw_set_relay(0);
					llc_state_enter(ST_WAIT_AUX);
					break;
        }
#if LLC_BYPASS_PFC_CONTROL
			if(s_llc.vmeas<(LLC_ENTRY_V-PFC_VBUS_READY_HYST_V))
#else
			if(!pfc_app_ready()||s_llc.vmeas<(LLC_ENTRY_V-PFC_VBUS_READY_HYST_V))
#endif
			{
				pfc_hw_set_relay(0);
				llc_state_enter(ST_WAIT_VBUS);
			}
			break;
		case ST_FAULT:
			
		default:
			{
				bool hw_active = protect_fault_active_hw();
				bool sw_latched = protect_fault_latched();
				bool aux_ok = aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS);

				/* 满足恢复条件再清一次硬件锁存 */
				if (!hw_active && aux_ok && elapsed_reached(s_llc_app.entry_ms, PFC_RESTART_DELAY_MS)) {
						if (sw_latched) 
							protect_clear_fault();
						if (!s_llc_clear_sent) {
								protect_hw_clear_pulse(10);
								s_llc_clear_sent = 1;
						}
						llc_state_enter(ST_WAIT_AUX);  // 先回到等辅源，再走 WAIT_VBUS → RUN
						s_llc_clear_sent = 0;
				}
			} break;
    }	
}

void llc_app_tick_1khz_withoutVbus(void)
{
	/* 只处理 LLC：不看母线电压、不看AUX，仅做故障保护 + 固定延时启动 + 开环/软启动推进 */
    static uint8_t s_llc_clear_sent = 0;
    static bool    ol_started = false;  /* 防止在 RUN 状态下重复 open-loop start */
	    if (protect_fault_latched() || protect_fault_active_hw()
#if !LLC_BYPASS_PFC_CONTROL
        || pfc_app_state() == PFC_ST_FAULT
#endif
        )
    {
        if (s_llc_app.state != ST_FAULT) {
            llc_state_enter(ST_FAULT);
            ol_started = false;
        }
        return;
    }
		
		adc_multi_copy(); 
		adc_multi_sample_aux_1khz();
	
    //float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
		
		//float v3v3 = (g_adc_multi.v3v3_raw * VREF_ADC) / 4095.0f;
		//float v3v3 = conv_adc_to_v_div(g_adc_multi.v3v3_raw,V3V3_RTOP_OHM,V3V3_RBOT_OHM);
		//float vbat = conv_adc_to_v_div(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
		//aux_power_monitor_update(v3v3, vbat);
		//s_llc.vmeas = vout;
		switch (s_llc_app.state)
		{
			  /* 这些状态统一当作“等待固定启动延时”，不做任何母线/AUX判断 */
        case ST_IDLE:
        case ST_WAIT_AUX:
        case ST_WAIT_VBUS:
				{
            if (s_llc_app.entry_ms == 0U) {
                s_llc_app.entry_ms = g_ms;
            }
            if (elapsed_reached(s_llc_app.entry_ms, LLC_START_DELAY_MS)) {
                //pfc_hw_set_relay(true);              /* 若未接硬件或宏未定义，此函数内部已做空操作保护 */
                llc_state_enter(ST_LLC_RUN);         /* 进入 RUN：在 llc_state_enter 中会做一次性初始化 */
                ol_started = false;
            }
        } break;

        case ST_LLC_RUN:
					llc_pwm_set_freq((uint32_t)s_llc.f_cmd);
						break;
						case ST_FAULT:
        default:
        {
            /* 自动重试：故障脚释放 + 到达重试延时 → 清软件/硬件锁存，回到 IDLE 等延时再起 */
            bool hw_active = protect_fault_active_hw();
            if (!hw_active && elapsed_reached(s_llc_app.entry_ms, PFC_RESTART_DELAY_MS)) {
                if (protect_fault_latched()) {
                    protect_clear_fault();
                }
                if (!s_llc_clear_sent) {
                    protect_hw_clear_pulse(10); /* 10 ms；按你锁存清除时序需要可调 */
                    s_llc_clear_sent = 1;
                }
                pfc_hw_set_relay(false);
                llc_state_enter(ST_IDLE);
                s_llc_clear_sent = 0;
                ol_started = false;
            }
        } 
				break;
		}
}
llc_state_t llc_app_state(void)
{
	return s_llc_app.state;
}
//volatile uint32_t g_s  = 0;   // 新增：秒计数
void SysTick_Handler(void){
    g_ms++;
    s_control_tick_pending++;
}
static void control_loop_tick_1khz(void){
    /* 1 kHz control */
    adc_multi_copy(); 
		//adc_multi_sample_aux_1khz();
	
    float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
		//float v3v3 = (g_adc_multi.v3v3_raw * VREF_ADC) / 4095.0f;
		//float v3v3 = conv_adc_to_v_div(g_adc_multi.v3v3_raw,V3V3_RTOP_OHM,V3V3_RBOT_OHM);
		//float vbat = conv_adc_to_v_div(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
	
		//aux_power_monitor_update(v3v3, vbat);
    s_llc.vmeas = vout;
		//pfc_app_tick_1khz(vout);
		//llc_app_tick_1khz();
	  //llc_state_enter(ST_LLC_RUN);
		llc_app_tick_1khz_withoutVbus();
		bool llc_running = (llc_app_state() == ST_LLC_RUN);
#if Bus_Adj
		bus_vol_adj_tick(vout, llc_running);
#endif
		if(llc_running)
		{
			llc_softstart_tick();
#if LLC_USE_OPEN_LOOP
		/* ---------- 开环扫频过程 ---------- */
		if(!s_llc_open_loop_completed)
		{
			llc_open_loop_tick(&s_llc_open_loop);
			float freq = f_clampf(llc_open_loop_get_freq(&s_llc_open_loop),s_llc.f_min,s_llc.f_max);
			s_llc.f_cmd = freq;
			 /* 判断是否已停止运行（即扫频完成） */
			if(!llc_open_loop_running(&s_llc_open_loop))
			{
				s_llc_open_loop_final_freq = freq;
				s_llc_open_loop_completed = true;
#if defined(DEBUG_PRINTF_LLC_OPENLOOP)
        debug_printf("[LLC-OpenLoop] completed: %.1f Hz\n", freq);
#endif
			}
		}
#else
			//llc_step(&s_llc);
#endif
		}
    //llc_pwm_set_freq((uint32_t)s_llc.f_cmd);
		protect_hw_clear_pulse_tick();
}
int main(void){

	  InitRCU();
		nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

	  debug_printf_init(DEBUG_PRINTF_DEFAULT_BAUDRATE);
	  debug_printf("Debug console initialized @%lu baud\n", (unsigned long)DEBUG_PRINTF_DEFAULT_BAUDRATE);
	
		systick_1ms_init();
    /* LLC complementary PWM 配置LLC的PWM频率 、死区时间和占空比，并初始化PWM模块*/
    llc_pwm_cfg_t lcfg = { .pwm_hz=LLC_PWM_BASE_HZ, .deadtime_ns=LLC_PWM_DEAD_NS, .duty=LLC_PWM_DUTY };
    llc_pwm_init(&lcfg);

    /* Aux PWM on PB0 */
		//pb0_pwm_init(PB0_PWM_BASE_HZ);
		
		#if Bus_Adj
		bus_vol_adj_init();
		#else
		//pb0_pwm_set_duty(0.5f);
		#endif
		
    /* ADC multi (PA3/PA1 removed) triggered by TIMER0 CH2 for coherence */
    adc_multi_init_dma(ADC0_1_EXTTRIG_REGULAR_T0_CH2); 
    adc_multi_start();

    /* PA0 & PA1 input capture */
    //cap_pa01_init();

    /* Protection EXTI PC11 */
    protect_exti_init();
		
		/* after protect_exti_init(); */
if (!protect_fault_active_hw() && protect_fault_latched()) {
    /* BKIN已高、电路无真故障，但软件还记着旧标志 → 清软件 + 清硬件锁存 */
    protect_clear_fault();
    protect_hw_clear_pulse(10);   // 10ms 够用；你的硬件若更慢可调到 20ms
}

    /* LLC control default。初始化LLC的控制参数，包括目标电压、PID参数、频率范围和初始频率。*/
    s_llc = (llc_t){ 
			.vref=VBUS_TARGET_V, .vmeas=0.0f, .kp=0.01f, .ki=0.0005f,
      .f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ, .f_slew=LLC_F_SLEW_HZ 
		};
#if LLC_USE_OPEN_LOOP
		llc_open_loop_init(&s_llc_open_loop, s_llc_open_loop_profile, sizeof(s_llc_open_loop_profile)/sizeof(s_llc_open_loop_profile[0]));
		s_llc_open_loop_completed = false;
		s_llc_open_loop_final_freq = f_clampf(LLC_F_INIT_HZ, s_llc.f_min, s_llc.f_max);
#endif
		pfc_app_init();
		llc_app_init();
	  pfc_hw_set_relay(true);
		
		
    while(1){
			uint32_t pending_ticks = 0U;
			float  duty0, duty1; //PA3 PA1捕获的值
			__disable_irq();
			if(s_control_tick_pending > 0U)
			{
					pending_ticks = s_control_tick_pending; //pending_ticks ：用于逐个处理待执行的控制任务。
					s_control_tick_pending = 0U;  //记录待处理的控制周期任务数量。
			}
			__enable_irq();
			while(pending_ticks-- > 0U)
			{
				//control_loop_tick_1khz();
				llc_app_tick_1khz_withoutVbus();
				// 防止单次主循环处理过多 tick
					if(pending_ticks > MAX_TICKS_PER_LOOP)
					{
						s_tick_drop_count += (pending_ticks - MAX_TICKS_PER_LOOP);
						pending_ticks = MAX_TICKS_PER_LOOP;
					}
			}
    }
}

