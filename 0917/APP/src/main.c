
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
	bool enable_cmd;
} pfc_app_ctx_t;

typedef struct
{
	bool active;
	bool pause;
	uint32_t start_ms;
	uint32_t duration_ms;
	float start_duty;
	float target_duty;
	
// 运行时计算得到的安全上下限
	float    duty_min_safe;
	float    duty_max_safe;
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
#if LLC_SOFTSTART_ENABLE
static llc_softstart_ctx_t s_llc_softstart;
#endif

#if LLC_USE_OPEN_LOOP
static llc_open_loop_ctrl_t s_llc_open_loop;
static bool s_llc_open_loop_completed = false;
static float s_llc_open_loop_final_freq = LLC_F_INIT_HZ;
static const llc_open_loop_segment_t s_llc_open_loop_profile[] = {
	{ .start_hz = LLC_F_MIN_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = LLC_F_SLEW_HZ, .hold_time_ms = 200U },
	{ .start_hz = LLC_F_INIT_HZ, .stop_hz = LLC_F_MAX_HZ, .slew_hz_per_ms = 500.0f, .hold_time_ms = 200U },
	{ .start_hz = LLC_F_MAX_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = 500.0f, .hold_time_ms = 0U },
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
static inline float f_absf(float x){ return x < 0 ? -x : x; }
static inline float f_minf(float a,float b){ return a < b ? a : b; }
static inline float f_maxf(float a,float b){ return a > b ? a : b; }
static inline float f_clampf(float x,float lo,float hi)
{ return x<lo?lo:(x>hi?hi:x); }

#if LLC_SOFTSTART_ENABLE


/* 根据当前周期/死区，计算“有效占空安全窗” */
static void ss_update_safe_window(void)
{
	  uint32_t per_ns = llc_pwm_get_period_ns();
    uint32_t dt_ns  = llc_pwm_get_deadtime_ns();
	  float guard = 0.0f;
    if (per_ns > 0U && dt_ns <= (UINT32_MAX / 2)) {
        /* 互补两沿都插死区：保守取 2*deadtime */
        guard = (2.0f * (float)dt_ns) / (float)per_ns;  // 0~1
    }
		else
		{
			guard = 0.0f;
		}
		
		guard += LLC_SOFTSTART_EXTRA_MARGIN; // 经验余量

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
		s_llc_softstart.active = false;
		s_llc_softstart.pause = false;
		s_llc_softstart.start_ms = 0U;
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;
		s_llc_softstart.start_duty = f_clampf(LLC_SOFTSTART_START_DUTY, 0.0f, 0.99f);

		s_llc_softstart.target_duty =  f_clampf(LLC_SOFTSTART_TARGET_DUTY, 0.0f, 0.99f);
		ss_update_safe_window();
		ss_apply(s_llc_softstart.start_duty);
}
static void llc_softstart_begin(float target_duty)
{
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;
		float start_duty = f_clampf(LLC_SOFTSTART_START_DUTY, 0.0f, 0.99f);
		float final_duty = f_clampf(target_duty, 0.0f, 0.99f);
		s_llc_softstart.pause      = false;
		
		s_llc_softstart.start_duty = start_duty;
		s_llc_softstart.target_duty = final_duty;
		ss_update_safe_window();
		if(final_duty <= start_duty || s_llc_softstart.duration_ms == 0U)
		{
			s_llc_softstart.active = false;
			s_llc_softstart.start_duty = final_duty;
			s_llc_softstart.target_duty = final_duty;
			ss_apply(final_duty);
		}
		else
		{
			s_llc_softstart.active = true;
			s_llc_softstart.start_ms = g_ms;
			ss_apply(start_duty);
		}
}
void llc_softstart_update_target(float new_target_0_1)
{
    s_llc_softstart.target_duty = f_clampf(new_target_0_1, 0.0f, 0.99f);
}
void llc_softstart_set_pause(bool pause)
{
    s_llc_softstart.pause = pause;
}

void llc_softstart_abort(void)
{
    s_llc_softstart.active = false;
    s_llc_softstart.pause = false;
    ss_update_safe_window(); // 以防期间改过频率/死区
    ss_apply(f_clampf(LLC_SOFTSTART_FAILSAFE_DUTY, 0.0f, 0.99f));
}

static void llc_softstart_tick(void)
{
    if (!s_llc_softstart.active) 
			return;

    /* 用你项目已有的故障判据 */
    if (protect_fault_latched() || protect_fault_active_hw()) {
        llc_softstart_abort();
        return;
    }
		if (s_llc_softstart.pause) 
			return;
		uint32_t elapsed = (uint32_t)(g_ms - s_llc_softstart.start_ms);
		if(elapsed >= s_llc_softstart.duration_ms)
		{
			s_llc_softstart.active = false;
			ss_apply(s_llc_softstart.target_duty);
			return;
		}
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
		ss_apply(duty);
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
	
	switch(next)
	{
		case PFC_ST_IDLE:
			pfc_hw_set_enable(s_pfc_app.enable_cmd);

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
	s_pfc_app.enable_cmd = false;
	pfc_hw_set_enable(false);
	//pfc_hw_set_relay(false);
}
//该函数的目的是在启动某种请求时，确保系统状态正确，并记录请求的起始时间（如果系统当前不处于空闲状态）。
void pfc_app_request_start(void)
{
	s_pfc_app.enable_cmd = true;
	if(s_pfc_app.state != PFC_ST_IDLE)
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
	bool fault_active = protect_fault_latched()||protect_fault_active_hw();
	//排除故障状态
	if(fault_active && s_pfc_app.state != PFC_ST_FAULT)
	{
		pfc_state_enter(PFC_ST_FAULT);
		 return;
	}
	
	switch(s_pfc_app.state)
	{
		case PFC_ST_IDLE:
			if(!s_pfc_app.enable_cmd)
			{
				if(s_pfc_hw_enabled)
				{
					pfc_hw_set_enable(false);
				}
				s_pfc_app.vbus_ok_since_ms = 0U;
				break;
			}
			if(!s_pfc_hw_enabled)
			{
				//用示波器测量从使能信号（ pfc_hw_set_enable(true) ）到输出电压稳定的实际时间,将延时设为实测时间的 1.2-1.5倍 以留余量。
				if((uint32_t)(g_ms - s_pfc_app.entry_ms)<PFC_STARTUP_DELAY_MS)
				{
					break;
				}
				pfc_hw_set_enable(true);
			}
			
			
			//输入电压 vbus_v 达到目标值（ PFC_VBUS_READY_V ），并保持一段时间（ PFC_READY_DELAY_MS ）进入ready状态
			if(vbus_v>=PFC_VBUS_READY_V)
			{
				if(s_pfc_app.vbus_ok_since_ms == 0U)
				{
					s_pfc_app.vbus_ok_since_ms = g_ms;
				}
				else if((uint32_t)(g_ms - s_pfc_app.vbus_ok_since_ms) >= PFC_READY_DELAY_MS)
				{
					pfc_state_enter(PFC_ST_READY);
				}
			}
			else
			{
				s_pfc_app.vbus_ok_since_ms = 0; // 重新计时
			}
			break;
		case PFC_ST_READY:
			if(!s_pfc_app.enable_cmd)
			{
				pfc_state_enter(PFC_ST_IDLE);
				break;
			}
	//输入电压低于阈值（ PFC_VBUS_READY_V - PFC_VBUS_READY_HYST_V ），并持续一定时间（ PFC_VBUS_DROPOUT_MS ）。
			if(vbus_v >= (PFC_VBUS_READY_V - PFC_VBUS_READY_HYST_V))
			{
				s_pfc_app.dropout_since_ms = 0U;
			}
			else
			{
				if(s_pfc_app.dropout_since_ms == 0U)
				{
					s_pfc_app.dropout_since_ms = g_ms;
				}
				else if((uint32_t)(g_ms - s_pfc_app.dropout_since_ms)>=PFC_VBUS_DROPOUT_MS)
				{
					pfc_state_enter(PFC_ST_IDLE);
				}
			}
			break;
		case PFC_ST_FAULT:
			
		default:
			if(!s_pfc_app.enable_cmd)
			{
				if(!fault_active)
				{
					pfc_state_enter(PFC_ST_IDLE);
				}
			}
			else if(!fault_active)
			{
				if((uint32_t)(g_ms - s_pfc_app.entry_ms)>= PFC_RESTART_DELAY_MS)
				{
					pfc_state_enter(PFC_ST_IDLE);
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
	s_llc_app.state = next;
	s_llc_app.entry_ms = g_ms;
	#if Bus_Adj
	bus_vol_adj_reset();   //重置总线电压调整逻辑 百分之五十的占空比
	#endif
	switch(next)
	{
		case ST_IDLE:
#if LLC_USE_OPEN_LOOP
			llc_open_loop_stop(&s_llc_open_loop);
#endif
			llc_softstart_reset();
			pfc_app_force_off();  
			llc_pwm_outputs_enable(0);
		
			s_llc.integ = 0.0f;
			s_llc.f_cmd = s_llc.f_min;
			break;
		case ST_WAIT_VBUS:
#if LLC_USE_OPEN_LOOP
			llc_open_loop_stop(&s_llc_open_loop);
#endif
			llc_softstart_reset();
			pfc_app_request_start(); //记录请求的起始时间
			llc_pwm_outputs_enable(0);
			s_llc.integ = 0.0f;
			s_llc.f_cmd = s_llc.f_min;
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
			break;
		case ST_FAULT:
				
		default:	
			llc_softstart_reset();
			pfc_app_force_off();
			llc_pwm_outputs_enable(0);
			s_llc.f_cmd = s_llc.f_min;
			break;
	}
}

void llc_app_init()
{
	llc_state_enter(ST_WAIT_VBUS);
}
/*********************************************************************************************************
* 函数名称：llc_state_enter
* 函数功能：状态机切换函数，用于控制 LLC（谐振变换器）的不同工作状态
* 输入参数：next
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月09日
* 注    意：当 LLC 需要从一个状态切换到另一个状态时，此函数会被调用，执行相应的初始化或清理操作。
*********************************************************************************************************/
void llc_app_tick_1khz(void)
{
	if(protect_fault_latched() || protect_fault_active_hw()||pfc_app_state() == PFC_ST_FAULT)
	{
		llc_state_enter(ST_FAULT);
		return;
	}
	switch(s_llc_app.state)
	{
		case ST_IDLE:
			llc_state_enter(ST_WAIT_VBUS);
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
			if(!pfc_app_ready()||s_llc.vmeas<(LLC_ENTRY_V-PFC_VBUS_READY_HYST_V))
			{
				pfc_hw_set_relay(0);
				llc_state_enter(ST_WAIT_VBUS);
			}
			break;
		case ST_FAULT:
		default:
			break;
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
    float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
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
		if(!s_llc_open_loop_completed)
		{
			llc_open_loop_tick(&s_llc_open_loop);
			float freq = f_clampf(llc_open_loop_get_freq(&s_llc_open_loop),s_llc.f_min,s_llc.f_max);
			s_llc.f_cmd = freq;
			if(!llc_open_loop_running(&s_llc_open_loop))
			{
				s_llc_open_loop_final_freq = freq;
				s_llc_open_loop_completed = true;
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
	  debug_printf_init(DEBUG_PRINTF_DEFAULT_BAUDRATE);
	  debug_printf("Debug console initialized @%lu baud\n", (unsigned long)DEBUG_PRINTF_DEFAULT_BAUDRATE);

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

    /* LLC control default。初始化LLC的控制参数，包括目标电压、PID参数、频率范围和初始频率。*/
    s_llc = (llc_t){ 
			.vref=VBUS_TARGET_V, .vmeas=0.0f, .kp=0.01f, .ki=0.0005f,
      .f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ, .f_slew=LLC_F_SLEW_HZ 
		};
#if LLC_USE_OPEN_LOOP
		llc_open_loop_init(&s_llc_open_loop, s_llc_open_loop_profile, sizeof(s_llc_open_loop_profile)/sizeof(s_llc_open_loop_profile[0]));
		s_llc_open_loop_completed = false;
		s_llc_open_loop_final_freq = f_clampf(llc_open_loop_get_freq(&s_llc_open_loop), s_llc.f_min, s_llc.f_max);
#endif
		pfc_app_init();
		llc_app_init();
		systick_1ms_init();
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

