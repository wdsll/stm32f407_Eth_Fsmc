
/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "main.h"
#include "temp_control.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define Bus_Adj 0
/* 控制循环参数（1 kHz） */
#define CONTROL_LOOP_HZ            (1000U)
#define CONTROL_LOOP_DT_S          (1.0f / (float)CONTROL_LOOP_HZ)
/* 主循环一次最多处理的 tick，超过将计数为丢弃（避免主循环长时间占用） */
#define MAX_TICKS_PER_LOOP         (5U)
#define FAST_ADC_MAX_TICKS_PER_LOOP (20U)
#define FAULT_RECOVER_DELAY_MS     (200U)
/* 串口任务调用频率控制（每10次主循环调用1次） */
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/

static uint32_t s_fault_latched_ms = 0U;
/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/
volatile uint32_t g_ms=0;
static volatile uint32_t s_control_tick_pending = 0U;
static volatile uint32_t s_adc_fast_tick_pending = 0U;

static volatile uint32_t s_tick_drop_count = 0U; /* 被丢弃的 tick 计数 */
static volatile uint32_t s_adc_fast_tick_drop_count = 0U; /*  tick  */
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


/* ADC1通道14测试函数声明 */
uint16_t adc1_channel14_test(void);
uint16_t adc1_channel14_multiple_samples(uint16_t sample_count, uint16_t *samples);
static void adc_fast_task_100us(void);
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

static uint32_t adc_fast_timer_clk_hz(void)
{
    uint32_t apb1 = rcu_clock_freq_get(CK_APB1);
    return (RCU_CFG0 & RCU_CFG0_APB1PSC) ? (apb1 * 2U) : apb1;
}
static void adc_fast_timer_init_100us(void)
{
    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);

    uint32_t tclk = adc_fast_timer_clk_hz();
    uint32_t period = (tclk / ADC_FAST_SAMPLE_HZ);
    if (period == 0U) {
        period = 1U;
    }
    if (period > 0U) {
        period -= 1U;
    }

    timer_parameter_struct t;
    timer_struct_para_init(&t);
    t.prescaler = 0U;
    t.counterdirection = TIMER_COUNTER_UP;
    t.period = period;
    t.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER3, &t);

    timer_interrupt_enable(TIMER3, TIMER_INT_UP);
    nvic_irq_enable(TIMER3_IRQn, 1U, 0U);
    timer_enable(TIMER3);
}
void TIMER3_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP) == SET) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
        //adc_fast_task_100us(); // 直接触发ADC0软件采集
			  s_adc_fast_tick_pending++;
			#if 0
			  if (s_adc_fast_tick_pending < 1000U) {
            s_adc_fast_tick_pending++;
        } else {
            s_adc_fast_tick_drop_count++;
        }
			#endif
    }
}

static void adc_fast_task_100us(void)
{
    adc_multi_trigger_fast();
}

static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot){
    float v = ((float)raw * VREF_ADC) / 4095.0f;  
    return v * ((rtop + rbot) / rbot);       
}
static inline float conv_adc_to_v_test(uint16_t raw, float rtop, float rbot){
    float v = ((float)raw * VREF_ADC) / 4095.0f;  
    return v * 1.50f;       
}
static bool adc_startup_check(void)
{
    uint32_t sum = 0U;
    uint16_t valid = 0U;

    for (uint16_t i = 0U; i < ADC_STARTUP_SAMPLE_COUNT; i++) {
        uint16_t raw = adc1_aux_read_channel(AD_3V3_CH, ADC_SAMPLETIME_55POINT5);
        if (raw != 0xFFFFU) {
            sum += raw;
            valid++;
        }
        delay_ms(ADC_STARTUP_SAMPLE_DELAY_MS);
    }

    if (valid == 0U) {

        return false;
    }

     float avg_raw = (float)sum / (float)valid;
     float v3v3 = conv_adc_to_v_test((uint16_t)(avg_raw + 0.5f), V3V3_RTOP_OHM, V3V3_RBOT_OHM);

    if ((v3v3 < ADC_STARTUP_V3V3_MIN_V) || (v3v3 > ADC_STARTUP_V3V3_MAX_V)) {
 
        return false;
    }

    for (uint8_t i = 0U; i < 3U; i++) {
        adc_multi_sample_aux_1khz();
        adc_multi_copy();
        delay_ms(1U);
    }

    if ((g_adc_multi.vout_raw == 0xFFFFU) || (g_adc_multi.isense_raw == 0xFFFFU)) {

        return false;
    }

    return true;
}

static bool protect_startup_check(void)
{
    if (protect_fault_active_hw()) {

        return false;
    }

    if (protect_fault_latched()) {

        protect_clear_fault();
    }

    return !protect_fault_active_hw();
}

static void control_loop_tick_1khz(void){
    /* 1 kHz control */ 
		adc_multi_sample_aux_1khz();
		
    pfc_tick_1khz();
    llc_app_tick_1khz();
		//llc_app_tick_adc_test();

}

void SysTick_Handler(void){
    g_ms++;
    s_control_tick_pending++;
}

int main(void){

	  InitRCU();
		nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

	  debug_printf_init(DEBUG_PRINTF_DEFAULT_BAUDRATE);

			//debug_printf("System Clock: %lu Hz\n", SystemCoreClock);
		systick_1ms_init();
    /* LLC complementary PWM 配置LLC的PWM频率 、死区时间和占空比，并初始化PWM模块*/
    llc_pwm_cfg_t lcfg = { .pwm_hz=LLC_PWM_BASE_HZ, .deadtime_ns=LLC_PWM_DEAD_NS, .duty=LLC_PWM_DUTY };//130
    llc_pwm_init(&lcfg);
		
    /* ADC multi (PA3/PA1 removed) triggered by TIMER3 interrupt @100us (software trigger) */
    adc_multi_init_dma(ADC0_1_2_EXTTRIG_REGULAR_NONE); 
    adc_multi_start();
		adc_multi_trigger_fast();
    adc_fast_timer_init_100us(); // 启动100us定时器中断用于ADC0触发
		adc1_aux_init();
		temp_control_init();
    /* Protection EXTI PC11 */
    protect_exti_init();
		
		/* after protect_exti_init(); */
 if (!protect_fault_active_hw() && protect_fault_latched()) {
    /* BKIN已高、电路无真故障，但软件还记着旧标志 → 清软件 + 清硬件锁存 */
     protect_clear_fault();
		}
    bool protect_ok = protect_startup_check();
    bool adc_ok = adc_startup_check();
		//bool adc_ok = adc_test();
    if (!protect_ok || !adc_ok) {
       debug_printf("[STARTUP] Preflight checks failed, PFC/LLC hold\n");
       while (1) {
           __NOP();
        }
    }

		pfc_init();
		llc_app_init();
		delay_ms(1000);
		pfc_enable();

		
    while(1){
			uint32_t pending_ticks = 0U;
			uint32_t pending_adc_fast_ticks = 0U;
			__disable_irq();
			if(s_control_tick_pending > 0U)
			{
					pending_ticks = s_control_tick_pending; //pending_ticks ：用于逐个处理待执行的控制任务。
					s_control_tick_pending = 0U;  //记录待处理的控制周期任务数量。
			}
			
			if (s_adc_fast_tick_pending > 0U)
			{
				pending_adc_fast_ticks = s_adc_fast_tick_pending;
				s_adc_fast_tick_pending = 0U;
			}
			__enable_irq();
			
			while (pending_adc_fast_ticks-- > 0U)
			{
				
				adc_multi_copy(); 
				llc_app_tick_100us();
				adc_multi_trigger_fast();
				if (pending_adc_fast_ticks > FAST_ADC_MAX_TICKS_PER_LOOP)
				{
					s_adc_fast_tick_drop_count += (pending_adc_fast_ticks - FAST_ADC_MAX_TICKS_PER_LOOP);
					pending_adc_fast_ticks = FAST_ADC_MAX_TICKS_PER_LOOP;
				}
			}
			while(pending_ticks-- > 0U)
			{
				 control_loop_tick_1khz();
			
				//防止单次主循环处理过多 tick
					if(pending_ticks > MAX_TICKS_PER_LOOP)
					{
						s_tick_drop_count += (pending_ticks - MAX_TICKS_PER_LOOP);
						pending_ticks = MAX_TICKS_PER_LOOP;
					}
			}
#if 0
			static uint32_t last_ms = 0;
if ((g_ms - last_ms) >= 1000U) {
    last_ms = g_ms;
    debug_printf("[MAIN] g_ms=%lu\r\n", (unsigned long)g_ms);
}
#endif	
			/* 非阻塞串口发送任务 - 频率控制 */
	
				if ((debug_buffer_used() > 0U)) {
					debug_tx_task();
				}
			
    }
}

