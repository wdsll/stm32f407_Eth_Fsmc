
/*********************************************************************************************************
* 模块名称：main.c
* 摘    要：
* 当前版本：1.0.0
* 作    者：Rengar
* 完成日期：2025年09月24日  
* 内    容：
* 注    意：                                                                  
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/

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
*                                              模块测试配置
*********************************************************************************************************/
#ifndef ENABLE_ADC_TEST
#define ENABLE_ADC_TEST              0
#endif

#ifndef ENABLE_PWM_TEST
#define ENABLE_PWM_TEST              0
#endif

#ifndef ENABLE_ICU_TEST
#define ENABLE_ICU_TEST              0
#endif

#ifndef ENABLE_PFC_TEST
#define ENABLE_PFC_TEST              0
#endif

#ifndef ENABLE_LLC_TEST
#define ENABLE_LLC_TEST              0
#endif

#if (ENABLE_ADC_TEST || ENABLE_PWM_TEST || ENABLE_ICU_TEST || ENABLE_PFC_TEST || ENABLE_LLC_TEST)
#define MODULE_TESTS_ACTIVE          1
#else
#define MODULE_TESTS_ACTIVE          0
#endif

#if ENABLE_PWM_TEST
#define PWM_TEST_SWEEP_PERIOD_MS      1000U
#endif

#if ENABLE_PFC_TEST
#define PFC_TEST_TOGGLE_PERIOD_MS     5000U
#endif

#if ENABLE_LLC_TEST
#define LLC_TEST_TOGGLE_PERIOD_MS     5000U
#endif
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

#define LLC_USE_OPEN_LOOP 1

#define Bus_Adj 0
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/

enum{
	LLC_START_DELAY_MS = 100
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

#if MODULE_TESTS_ACTIVE
typedef struct
{
#if ENABLE_ADC_TEST
    struct {
        float vout_v;
        float isense_a;
        float v3v3_v;
        float vbat_v;
        float t_llc_v;
        uint16_t raw_vout;
        uint16_t raw_isense;
        uint16_t raw_tsense;
        uint16_t raw_v3v3;
        uint16_t raw_vbat;
        uint16_t raw_t_llc;
    } adc;
#endif
#if ENABLE_PWM_TEST
    struct {
        float pb0_duty;
        float llc_freq_hz;
        uint32_t sweep_phase;
    } pwm;
#endif
#if ENABLE_ICU_TEST
    struct {
        float pa0_duty;
        float pa1_duty;
        uint32_t last_update_ms;
    } icu;
#endif
#if ENABLE_PFC_TEST
    struct {
        pfc_state_t state;
        bool enable_cmd;
        bool hw_fault;
        float bus_voltage;
        uint32_t last_toggle_ms;
    } pfc;
#endif
#if ENABLE_LLC_TEST
    struct {
        llc_state_t state;
        float freq_cmd;
        float vref;
        float vmeas;
        uint32_t last_toggle_ms;
    } llc;
#endif
}module_tests_ctx_t;

static volatile module_tests_ctx_t s_module_tests;
static void module_tests_init(void);
static void module_tests_tick_1khz(float vbus_v);
#else
static inline void module_tests_init(void) { }
static inline void module_tests_tick_1khz(float vbus_v) { (void)vbus_v; }
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
#if LLC_SOFTSTART_ENABLE
static llc_softstart_ctx_t s_llc_softstart;
#endif

#if LLC_USE_OPEN_LOOP
static llc_open_loop_ctrl_t s_llc_open_loop;
static bool s_llc_open_loop_completed = false;
static float s_llc_open_loop_final_freq = LLC_F_INIT_HZ;
/* ---------- LLC open-loop soft-start profile ---------- */
/* 实际参数：
 * 启动频率：130 kHz
 * 稳态频率： 90 kHz
 * 频率变化： Δf = 40 kHz
 * 变化斜率： 1 kHz/ms → 总耗时约 40 ms
 * 保持时间： 100 ms
 */
static const llc_open_loop_segment_t s_llc_open_loop_profile[] = {
	{ .start_hz = LLC_F_MAX_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = LLC_F_SLEW_HZ, .hold_time_ms = 100U },
	{ .start_hz = LLC_F_INIT_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = 0, .hold_time_ms = 0U },
};
#endif

volatile uint32_t g_ms=0;
static volatile uint32_t s_control_tick_pending = 0U;

/* 控制循环参数（1 kHz） */
#define CONTROL_LOOP_HZ            (1000U)
#define CONTROL_LOOP_DT_S          (1.0f / (float)CONTROL_LOOP_HZ)
/* 主循环一次最多处理的 tick，超过将计数为丢弃（避免主循环长时间占用） */
#define MAX_TICKS_PER_LOOP         (5U)
static volatile uint32_t s_tick_drop_count = 0U; /* 被丢弃的 tick 计数 */
/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void aux_power_monitor_update(float v3v3_v, float vbat_v);
static void llc_softstart_reset(void);
static void llc_softstart_begin(float target_duty);
static void llc_softstart_tick(void);

static void llc_state_enter(llc_state_t next);
static void pfc_state_enter(pfc_state_t next);

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
    NVIC_SetPriority(SysTick_IRQn, 0x0F);
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

static void aux_power_monitor_update(float v3v3_v, float vbat_v)
{
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

static inline void delay_ms_block(uint16_t ms)
{
    uint32_t start = g_ms;
    while ((uint32_t)(g_ms - start) < ms) { __NOP(); }
}

/* 发送一个清除脉冲给外部锁存电路 */
void protect_hw_clear_pulse(uint16_t pulse_ms)
{
    gpio_bit_set(GPIOC, GPIO_PIN_12);
    delay_ms_block(pulse_ms ? pulse_ms : 10);
    gpio_bit_reset(GPIOC, GPIO_PIN_12);
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

static inline bool aux_power_ok_now(void)
{
    return s_aux_power.power_ok;
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
    /* 恢复去抖：要求 power_ok=true 且恢复时间记过阈值 */
    if (!s_aux_power.power_ok) return false;
    if (s_aux_power.restore_detected_ms == 0U) return false;
    return (uint32_t)(g_ms - s_aux_power.restore_detected_ms) >= ms;
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

/*********************************************************************************************************
* 函数名称：ss_update_safe_window
* 函数功能：更新软启动的安全窗口范围
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月20
* 注    意：根据当前PWM周期和死区时间计算软启动的安全占空比范围，避免直通或无效脉宽
*********************************************************************************************************/
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
/*********************************************************************************************************
* 函数名称：llc_softstart_reset 开环版本
* 函数功能：复位软启动上下文：清除状态、暂停标志、时间戳。
						重设软启动参数：起始占空比、目标占空比、持续时间。
						计算当前安全占空窗：防止直通。
						立即应用初始占空比（通常为低占空起步）
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月20
* 注    意：开环模式下（LLC_USE_OPEN_LOOP=1）
*********************************************************************************************************/
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

/*********************************************************************************************************
* 函数名称：llc_softstart_begin
* 函数功能：目的是实现一个软启动（soft start）功能，用于平滑地将占空比（duty cycle）从初始值逐步调整到目标值。
* 输入参数：target_duty —— 目标占空比（0.0~1.0）
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月22
* 注    意：LLC_USE_OPEN_LOOP 模式下
*********************************************************************************************************/
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
/*********************************************************************************************************
* 函数名称：llc_softstart_tick
* 函数功能：该函数用于处理LLC软启动过程中的定时逻辑，包括故障检测、暂停处理、进度计算和占空比调整。
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月20
* 注    意：如果软启动未激活或处于暂停状态，函数将直接返回；如果检测到故障（已锁存或硬件触发），将中止软启动
*********************************************************************************************************/
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
                llc_pwm_set_duty(f_clampf(LLC_PWM_DUTY, 0.0f, 0.99f));
}
static void llc_softstart_begin(float target_duty)
{
                llc_pwm_set_duty(f_clampf(target_duty, 0.0f, 0.99f));
}
static void llc_softstart_tick(void)
{
                /* Soft-start disabled. Nothing to do. */
}
#endif /* LLC_SOFTSTART_ENABLE */
#if MODULE_TESTS_ACTIVE

static void module_tests_init(void)
{
#if ENABLE_PWM_TEST
    s_module_tests.pwm.pb0_duty = 0.0f;
    s_module_tests.pwm.llc_freq_hz = s_llc.f_cmd;
    s_module_tests.pwm.sweep_phase = 0U;
    pb0_pwm_set_duty(0.0f);
#endif
#if ENABLE_PFC_TEST
    s_module_tests.pfc.state = s_pfc_app.state;
    s_module_tests.pfc.enable_cmd = s_pfc_app.enable_cmd;
    s_module_tests.pfc.hw_fault = false;
    s_module_tests.pfc.bus_voltage = 0.0f;
    s_module_tests.pfc.last_toggle_ms = g_ms;
#endif
#if ENABLE_LLC_TEST
    s_module_tests.llc.state = s_llc_app.state;
    s_module_tests.llc.freq_cmd = s_llc.f_cmd;
    s_module_tests.llc.vref = s_llc.vref;
    s_module_tests.llc.vmeas = s_llc.vmeas;
    s_module_tests.llc.last_toggle_ms = g_ms;
#endif

#if ENABLE_ICU_TEST
    s_module_tests.icu.pa0_duty = 0.0f;
    s_module_tests.icu.pa1_duty = 0.0f;
    s_module_tests.icu.last_update_ms = g_ms;
#endif
}
static void module_tests_tick_1khz(float vbus_v)
{
#if ENABLE_ADC_TEST
    s_module_tests.adc.raw_vout = g_adc_multi.vout_raw;
    s_module_tests.adc.raw_isense = g_adc_multi.isense_raw;
    s_module_tests.adc.raw_tsense = g_adc_multi.tsense_raw;
    s_module_tests.adc.raw_v3v3 = g_adc_multi.v3v3_raw;
    s_module_tests.adc.raw_vbat = g_adc_multi.vbt_raw;
    s_module_tests.adc.raw_t_llc = g_adc_multi.t_llc_raw;
    s_module_tests.adc.vout_v = vbus_v;
    s_module_tests.adc.isense_a = conv_adc_to_i(g_adc_multi.isense_raw);
    s_module_tests.adc.v3v3_v = (g_adc_multi.v3v3_raw * VREF_ADC) / 4095.0f;
    s_module_tests.adc.vbat_v = conv_adc_to_v_div(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
    s_module_tests.adc.t_llc_v = (g_adc_multi.t_llc_raw * VREF_ADC) / 4095.0f;
#endif

#if ENABLE_PWM_TEST
//通过相位递增和分段计算，实现了占空比的平滑变化。
    uint32_t phase = (s_module_tests.pwm.sweep_phase + 1U) % PWM_TEST_SWEEP_PERIOD_MS;
    s_module_tests.pwm.sweep_phase = phase;
    uint32_t half = PWM_TEST_SWEEP_PERIOD_MS / 2U;
    float duty;
    if(half == 0U)
    {
        duty = 0.5f;
    }
    else if(phase < half)
    {
        duty = (float)phase / (float)half;
    }
    else
    {
        duty = (float)(PWM_TEST_SWEEP_PERIOD_MS - phase) / (float)half;
    }
    duty = f_clampf(duty, 0.0f, 1.0f);
    pb0_pwm_set_duty(duty);
    s_module_tests.pwm.pb0_duty = duty;
    s_module_tests.pwm.llc_freq_hz = s_llc.f_cmd;
#endif

#if ENABLE_PFC_TEST
    bool hw_fault = protect_fault_latched() || protect_fault_active_hw();
    s_module_tests.pfc.hw_fault = hw_fault;
    s_module_tests.pfc.state = s_pfc_app.state;
    s_module_tests.pfc.enable_cmd = s_pfc_app.enable_cmd;
    s_module_tests.pfc.bus_voltage = vbus_v;
    if(!hw_fault && (uint32_t)(g_ms - s_module_tests.pfc.last_toggle_ms) >= PFC_TEST_TOGGLE_PERIOD_MS)
    {
        if(s_pfc_app.enable_cmd)
        {
            pfc_app_force_off();
        }
        else
        {
            pfc_app_request_start();
        }
        s_module_tests.pfc.last_toggle_ms = g_ms;
    }
#endif
#if ENABLE_LLC_TEST
    s_module_tests.llc.state = s_llc_app.state;
    s_module_tests.llc.freq_cmd = s_llc.f_cmd;
    s_module_tests.llc.vref = s_llc.vref;
    s_module_tests.llc.vmeas = s_llc.vmeas;
    bool faults_active = protect_fault_latched() || protect_fault_active_hw();
    if(!faults_active && (uint32_t)(g_ms - s_module_tests.llc.last_toggle_ms) >= LLC_TEST_TOGGLE_PERIOD_MS)
    {
        if(s_llc_app.state == ST_LLC_RUN)
        {
            llc_state_enter(ST_WAIT_VBUS);
        }
        else if(s_llc_app.state == ST_WAIT_VBUS)
        {
            llc_state_enter(ST_LLC_RUN);
        }
        s_module_tests.llc.last_toggle_ms = g_ms;
    }
    if(faults_active)
    {
        s_module_tests.llc.last_toggle_ms = g_ms;
    }
#endif
    (void)vbus_v;
}
#endif

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

void pfc_app_init()
{
	pfc_hw_init();
	s_pfc_bus_v = 0.0f;
	s_pfc_app.state = PFC_ST_IDLE;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = 0U;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = 0U;
	s_pfc_app.enable_cmd = false;
	pfc_hw_set_enable(false);
	//pfc_hw_set_relay(false);
}
//该函数的目的是在启动某种请求时，确保系统状态正确，并记录请求的起始时间（如果系统当前不处于空闲状态）。
void pfc_app_request_start(void)
{
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
}

void pfc_app_force_off()
{
	s_pfc_app.enable_cmd = false;
	if(s_pfc_app.state != PFC_ST_IDLE)
	{
		 pfc_state_enter(PFC_ST_IDLE);
	}
}
/*********************************************************************************************************
* 函数名称：pfc_app_tick_1khz
* 函数功能：其主要目的是根据输入电压（ vbus_v ）和系统状态（如故障、使能命令等）动态调整 PFC 的工作状态，
	确保系统在安全、高效的状态下运行。
* 输入参数：vbus_v
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月09日
* 注    意：有启动延时、达标保持、迟滞、READY 消抖和充电超时五道保障，现场表现会更“稳且可预期
*********************************************************************************************************/
void pfc_app_tick_1khz(float vbus_v)
{
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
			// 检查是否收到禁用命令
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
			// 检查是否达到启动延时
			if(!elapsed_reached(s_pfc_app.startup_cmd_ms, PFC_STARTUP_DELAY_MS))
			{
				break;
			}
			if(!s_pfc_hw_enabled)
			{
				pfc_hw_set_enable(true);
				s_pfc_hw_enabled = true; // 更新状态
			}
			//输入电压 vbus_v 达到目标值（ PFC_VBUS_READY_V ），并保持一段时间（ PFC_READY_DELAY_MS ）进入ready状态
			if(vbus_v>=PFC_VBUS_READY_V)
			{
				// 如果电压稳定计时器未初始化，则初始化
				if(s_pfc_app.vbus_ok_since_ms == 0U)
				{
					s_pfc_app.vbus_ok_since_ms = g_ms;
				}
				// 检查是否达到电压稳定延时 
				else if((uint32_t)(g_ms - s_pfc_app.vbus_ok_since_ms) >= PFC_READY_DELAY_MS)
				{
					pfc_state_enter(PFC_ST_READY);
				}
			}
			else
			{
				s_pfc_app.vbus_ok_since_ms = 0; // 如果电压未达标，则重置电压稳定计时器
			}
			break;
		case PFC_ST_READY:
			// 检查是否收到禁用命令
			if(!s_pfc_app.enable_cmd)
			{
				pfc_state_enter(PFC_ST_IDLE);
				break;
			}
	//输入电压低于阈值（ PFC_VBUS_READY_V - PFC_VBUS_READY_HYST_V ），并持续一定时间（ PFC_VBUS_DROPOUT_MS ）。
			if(vbus_v >= (PFC_VBUS_READY_V - PFC_VBUS_READY_HYST_V))
			{
				// 如果电压达标，则重置掉电计时器
				s_pfc_app.dropout_since_ms = 0U;
			}
			else
			{
				//// 如果掉电计时器未初始化，则初始化
				if(s_pfc_app.dropout_since_ms == 0U)
				{
					s_pfc_app.dropout_since_ms = g_ms;
				}
				// 检查是否达到掉电延时
				else if((uint32_t)(g_ms - s_pfc_app.dropout_since_ms)>=PFC_VBUS_DROPOUT_MS)
				{
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
}

pfc_state_t pfc_app_state(void)
{
  return s_pfc_app.state;
}

bool pfc_app_ready(void)
{
	return s_pfc_app.state == PFC_ST_READY;
}

float pfc_bus_voltage(void)
{
	return s_pfc_bus_v;
}

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

/*********************************************************************************************************
* 函数名称：llc_state_enter
* 函数功能：状态机切换函数，用于控制 LLC（谐振变换器）的不同工作状态
* 输入参数：next
* 输出参数：void
* 返 回 值：void
* 创建日期：202年10月09日
* 注    意：当 LLC 需要从一个状态切换到另一个状态时，此函数会被调用，执行相应的初始化或清理操作。
*********************************************************************************************************/
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
			s_llc.f_cmd = f_clampf(LLC_F_MAX_HZ, s_llc.f_min, s_llc.f_max);
#endif
			llc_softstart_begin(LLC_PWM_DUTY);
			llc_pwm_outputs_enable(1);
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
	llc_state_enter(ST_WAIT_AUX);
}
/*********************************************************************************************************
* 函数名称：llc_app_tick_1khz
* 函数功能：状态机切换函数，用于控制 LLC（谐振变换器）的不同工作状态
* 输入参数：next
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月09日
* 注    意：当 LLC 需要从一个状态切换到另一个状态时，此函数会被调用，执行相应的初始化或清理操作。
*********************************************************************************************************/
void llc_app_tick_1khz(void)
{
	 static uint8_t s_llc_clear_sent = 0;
	
	if(protect_fault_latched() || protect_fault_active_hw()||pfc_app_state() == PFC_ST_FAULT)
	{
		llc_state_enter(ST_FAULT);
		return;
	}
	

	/* 统一的辅源棕断保护：任意态只要辅源掉线稳定，就回 WAIT_AUX */
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
        /* 辅源恢复去抖后，才允许去等 PFC/母线 */
        if (aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS)){
            llc_state_enter(ST_WAIT_VBUS);
        }
        break;
		case ST_WAIT_VBUS:
			if(pfc_app_ready() && (s_llc.vmeas >= LLC_ENTRY_V)) 
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
			 // 运行中：不就绪或电压跌破“进入阈值-迟滞”→ 退回等待
			//如果任一条件成立（PFC未就绪 或 电压过低），则调用 llc_state_enter(ST_WAIT_VBUS) ，切换至等待VBUS状态。
				if(!aux_power_ok_now()){
					pfc_hw_set_relay(0);
					llc_state_enter(ST_WAIT_AUX);
					break;
        }
			if(!pfc_app_ready()||s_llc.vmeas<(LLC_ENTRY_V-PFC_VBUS_READY_HYST_V))
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

llc_state_t llc_app_state(void)
{
	return s_llc_app.state;
}

void SysTick_Handler(void){
    g_ms++;
    s_control_tick_pending++;
}

/*********************************************************************************************************
* 函数名称：control_loop_tick_1khz
* 函数功能：主要用于实时控制电力电子系统中的功率转换模块
* 输入参数：next
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月09日
* 注    意：数据采集与转换：通过ADC读取输出电压的原始数据，并将其转换为实际的电压值。
						根据系统状态（如LLC是否运行）执行不同的控制逻辑
						动态调整LLC的工作频率，确保系统稳定运行。
*********************************************************************************************************/
static void control_loop_tick_1khz(void){
    /* 1 kHz control */
    adc_multi_copy(); 
		adc_multi_sample_aux_1khz();
	
    float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
		//float v3v3 = (g_adc_multi.v3v3_raw * VREF_ADC) / 4095.0f;
		float v3v3 = conv_adc_to_v_div(g_adc_multi.v3v3_raw,V3V3_RTOP_OHM,V3V3_RBOT_OHM);
		float vbat = conv_adc_to_v_div(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
	
		aux_power_monitor_update(v3v3, vbat);
    s_llc.vmeas = vout;
		pfc_app_tick_1khz(vout);
		llc_app_tick_1khz();
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
			llc_step(&s_llc);
#endif
		}
#if MODULE_TESTS_ACTIVE
    module_tests_tick_1khz(vout);
#endif
    llc_pwm_set_freq((uint32_t)s_llc.f_cmd);
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
		pb0_pwm_init(PB0_PWM_BASE_HZ);
		
		#if Bus_Adj
		bus_vol_adj_init();
		#else
		pb0_pwm_set_duty(0.5f);
		#endif
		
    /* ADC multi (PA3/PA1 removed) triggered by TIMER0 CH2 for coherence */
    adc_multi_init_dma(ADC0_1_EXTTRIG_REGULAR_T0_CH2); 
    adc_multi_start();

    /* PA0 & PA1 input capture */
    cap_pa01_init();

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
      .f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_MAX_HZ, .f_slew=LLC_F_SLEW_HZ 
		};
#if LLC_USE_OPEN_LOOP
		llc_open_loop_init(&s_llc_open_loop, s_llc_open_loop_profile, sizeof(s_llc_open_loop_profile)/sizeof(s_llc_open_loop_profile[0]));
		s_llc_open_loop_completed = false;
		s_llc_open_loop_final_freq = f_clampf(LLC_F_INIT_HZ, s_llc.f_min, s_llc.f_max);
#endif
		pfc_app_init();
		llc_app_init();
	
#if MODULE_TESTS_ACTIVE
		module_tests_init();
#endif
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
					control_loop_tick_1khz();
				// 防止单次主循环处理过多 tick
					if(pending_ticks > MAX_TICKS_PER_LOOP)
					{
						s_tick_drop_count += (pending_ticks - MAX_TICKS_PER_LOOP);
						pending_ticks = MAX_TICKS_PER_LOOP;
					}
			}
			if(cap_pa0_read_duty(&duty0)){
					(void)duty0; /* TODO: convert ticks->Hz using TIMER1 clock if? */
#if MODULE_TESTS_ACTIVE && ENABLE_ICU_TEST
					s_module_tests.icu.pa0_duty = duty0;
					s_module_tests.icu.last_update_ms = g_ms;
#endif
			}
			 if(cap_pa1_read_duty(&duty1)){
					(void)duty1;
#if MODULE_TESTS_ACTIVE && ENABLE_ICU_TEST
					s_module_tests.icu.pa1_duty = duty1;
					s_module_tests.icu.last_update_ms = g_ms;
#endif
			}
			//if(protect_fault_latched()){
				//检测到故障（通过PC11中断），则关闭LLC的PWM输出，并标记需要进一步处理故障。
				//  llc_pwm_outputs_enable(0);
					/* TODO: fault handling */
			//}
			if(llc_app_state() == ST_FAULT)
			{
				llc_pwm_outputs_enable(0);
			}
			__NOP();
    }
}

