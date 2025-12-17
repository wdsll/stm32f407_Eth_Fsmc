
/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "main.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

#define LLC_USE_OPEN_LOOP 0

#define Bus_Adj 0

/* ======================== Bring-up configuration ======================== */
/* Keep PFC control bypassed so that the firmware only exercises the LLC open-loop sweep for hardware validation. */
#define LLC_BRINGUP_OPEN_LOOP_ONLY   0

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


typedef struct
{
	bool active;
	uint16_t duration_ms;
	uint32_t start_ms;
} protect_clear_pulse_ctx_t;

static protect_clear_pulse_ctx_t s_protect_clear_pulse = { false, 0U, 0U };

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/



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
static void protect_hw_clear_pulse_tick(void);
static void protect_clear_gpio_init(void); // ← 新增：清锁存脚初始化

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





/* ADC1通道14测试函数声明 */
uint16_t adc1_channel14_test(void);
uint16_t adc1_channel14_multiple_samples(uint16_t sample_count, uint16_t *samples);



static void control_loop_tick_1khz(void){
    /* 1 kHz control */ 
		adc_multi_sample_aux_1khz();
		adc_multi_copy(); 
    float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
		float v3v3 = conv_adc_to_v_div(g_adc_multi.v3v3_raw,V3V3_RTOP_OHM,V3V3_RBOT_OHM);
		float vbat = conv_adc_to_v_div(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
	
		aux_power_monitor_update(v3v3, vbat);
    s_llc.vmeas = vout;
    pfc_tick_1khz();
    llc_app_tick_1khz();
		llc_app_tick_1khz_withoutVbus();
		bool llc_running = (llc_app_state() == ST_LLC_RUN);
#if Bus_Adj
		bus_vol_adj_tick(vout, llc_running);
#endif
		if(llc_running)
		{
			//llc_softstart_tick();
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


//volatile uint32_t g_s  = 0;   // 新增：秒计数
void SysTick_Handler(void){
    g_ms++;
    s_control_tick_pending++;
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
		pb0_pwm_set_duty(0.5f);
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
		pfc_disable();

		
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

