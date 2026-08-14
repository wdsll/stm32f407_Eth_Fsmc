/*********************************************************************************************************
* 模块名称：main.c
* 摘    要：1.5kW OBC 主程序 V1
* 作    者：Rengar
* 内    容：架构参考2.2KW OBC (GD32F303, 1kHz控制环 + 100us ADC) + CAN_LIMA48V10A (充电状态机)
						PFC(NCP1654外置) → LLC(专用IC) → 输出(继电器切换),MCU只负责监控、软件保护、功率时序和通信，不参与PFC/LLC闭环调节
* 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
*          
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "main.h"
#include "RCU.h"
#include "debug_printf.h"
#include "serial_console.h"
#include "pwm_llc.h"
#include "adc_dma.h"
#include "protect.h"
#include "can_comm.h"
#include "pfc_control.h"
#include "llc_control.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
/*
 * ADC仅用于输入/母线/输出监控和软件保护。
 * 2~5kHz均可；默认5kHz，即200us一个监控节拍。
 */
#ifndef ADC_MONITOR_SAMPLE_HZ
#define ADC_MONITOR_SAMPLE_HZ           5000U
#endif

#if ((ADC_MONITOR_SAMPLE_HZ < 2000U) || (ADC_MONITOR_SAMPLE_HZ > 5000U))
#error "ADC_MONITOR_SAMPLE_HZ must be between 2000Hz and 5000Hz"
#endif
/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
volatile uint32_t g_ms = 0U;     /* SysTick 1ms 节拍计数，全局时间基准 */
charger_state_t g_charger_state = MAIN_STEP_INIT;
fault_type_t    g_fault = FAULT_NONE;
/*
 * 以下变量是“待执行标志”，不是待补跑计数器：
 * 0 = 无任务；1 = 有一个最新任务待执行。
 * 主循环繁忙时不累计历史任务，只统计被丢弃的节拍。
 */
static volatile uint32_t s_control_tick_pending = 0U;
static volatile uint32_t s_adc_monitor_tick_pending  = 0U;
static volatile uint32_t s_tick_drop_count = 0U;
static volatile uint32_t s_adc_monitor_tick_drop_count  = 0U;
/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/
/* ========== 延时 ========== */
void delay_ms(uint32_t duration_ms)
{
    if (duration_ms == 0U) return;
    uint32_t start_ms = g_ms;
    while ((uint32_t)(g_ms - start_ms) < duration_ms) {
        __NOP();
    }
}
/* ========== SysTick 1ms 中断 ========== */
static void systick_1ms_init(void)
{
    SystemCoreClockUpdate();
    uint32_t reload = SystemCoreClock / 1000U;
    if (reload == 0U || reload > SysTick_LOAD_RELOAD_Msk) return;
    reload -= 1U;

    SysTick->CTRL = 0U;
    SysTick->LOAD = reload;
    SysTick->VAL  = 0U;
    NVIC_SetPriority(SysTick_IRQn,
        irq_priority_encode(IRQ_PRIO_SYSTICK_PREEMPT, IRQ_PRIO_SYSTICK_SUB));
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    g_ms++;
    /* 1ms任务尚未消费时，不累加、不补跑，只记录一次丢节拍。 */
    if (s_control_tick_pending == 0U) {
        s_control_tick_pending = 1U;
    } else {
        s_tick_drop_count++;
    }
}

/* ========== 2~5kHz ADC监控节拍定时器(TIMER3) ========== */
static uint32_t adc_monitor_timer_clk_hz(void)
{
    uint32_t apb1 = rcu_clock_freq_get(CK_APB1);
    return (RCU_CFG0 & RCU_CFG0_APB1PSC) ? (apb1 * 2U) : apb1;
}

static void adc_monitor_timer_init(void)
{
    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);

    uint32_t tclk = adc_monitor_timer_clk_hz();
    uint32_t period = tclk / ADC_MONITOR_SAMPLE_HZ;
    if (period == 0U) 
			period = 1U;
    period -= 1U;

    timer_parameter_struct t;
    timer_struct_para_init(&t);
    t.prescaler = 0U;
    t.counterdirection = TIMER_COUNTER_UP;
    t.period = period;
    t.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER3, &t);

    timer_interrupt_enable(TIMER3, TIMER_INT_UP);
    nvic_irq_enable(TIMER3_IRQn,
        irq_priority_encode(IRQ_PRIO_MEASURE_PREEMPT, IRQ_PRIO_MEASURE_SUB), 0U);
    timer_enable(TIMER3);
}
void TIMER3_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP) == SET) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
			  /*
         * 只保留最新一个采样任务。
         * 主循环没来得及处理时，旧节拍直接丢弃，禁止背靠背补采。
         */
        if (s_adc_monitor_tick_pending == 0U) {
            s_adc_monitor_tick_pending = 1U;
        } else {
            s_adc_monitor_tick_drop_count++;
        }
    }
}
/* ========== 1kHz监控、软件保护及功率时序任务 ========== */
static void monitor_protect_tick_1khz(void)
{
    /* ADC 多通道采样 (1kHz)，不得覆盖AC RMS结果 */
    adc_multi_sample_aux_1khz();

    /* 软件保护检查 (LLC_FAULT_CHECK 轮询 + OCP/OVP/过温) */
    protect_tick_1khz();

    /* PFC 状态机 (NCP1654 外置, MCU 监控+继电器控制) */
    pfc_tick_1khz();

    /*模拟IC闭环；MCU只更新参考值并管理功率时序. */
    power_supervisor_tick_1khz();
}
/* ========== 启动自检 ========== */
static bool adc_startup_check(void)
{
    /* 多次软件触发，使DMA缓冲区得到有效数据。TIMER3此时尚未启动。 */
    for (uint16_t i = 0U; i < ADC_STARTUP_SAMPLE_COUNT; i++) {
        adc_multi_trigger_fast();
        delay_ms(ADC_STARTUP_SAMPLE_DELAY_MS);
    }
    adc_multi_copy();
    adc_multi_sample_aux_1khz();

    /* 任意关键通道饱和 (满量程) 视为采样异常 */
		if (g_adc_multi.ac_vol_raw  >= ADC_RESOLUTION) return false;
    if (g_adc_multi.bus_vol_raw >= ADC_RESOLUTION) return false;
    if (g_adc_multi.vout_raw    >= ADC_RESOLUTION) return false;
		if (g_adc_multi.isense_raw  >= ADC_RESOLUTION) return false;
    if (g_adc_multi.vbt_raw     >= ADC_RESOLUTION) return false;
    return true;
}

static bool protect_startup_check(void)
{
    if (protect_fault_active_hw()) 
		{
			return false;
		}
    if (protect_fault_latched()) 
		{
			protect_clear_fault();
		}
    return !protect_fault_active_hw();
}
/* ========== 硬件初始化 ========== */
static void hw_gpio_init(void)
{
    /* PFC 继电器 */
    rcu_periph_clock_enable(PFC_RELAY_RCU);
    gpio_init(PFC_RELAY_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PFC_RELAY_PIN);
    gpio_bit_reset(PFC_RELAY_PORT, PFC_RELAY_PIN);

    /* LLC 使能 */
    rcu_periph_clock_enable(LLC_EN_RCU);
    gpio_init(LLC_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LLC_EN_PIN);
    gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);  /* 初始关闭 */

    /* 输出继电器 */
    rcu_periph_clock_enable(OUT_RELAY_RCU);
    gpio_init(OUT_RELAY_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, OUT_RELAY_PIN);
    gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);

    /* 风扇 */
    rcu_periph_clock_enable(FAN_CTL_RCU);
    gpio_init(FAN_CTL_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, FAN_CTL_PIN);
    gpio_bit_reset(FAN_CTL_PORT, FAN_CTL_PIN);

    /* HARD_FAULT_CLR 输出 (初始低) */
    rcu_periph_clock_enable(HARD_FAULT_CLR_RCU);
    gpio_init(HARD_FAULT_CLR_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, HARD_FAULT_CLR_PIN);
    gpio_bit_reset(HARD_FAULT_CLR_PORT, HARD_FAULT_CLR_PIN);

    /* LED 红/绿 */
    rcu_periph_clock_enable(LED_RED_RCU);
    gpio_init(LED_RED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED_RED_PIN | LED_GREEN_PIN);
    gpio_bit_set(LED_RED_PORT, LED_RED_PIN);      /* 初始红灯: 待机 */
    gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
}
/*********************************************************************************************************
*                                              主函数
*********************************************************************************************************/
int main(void)
{
    /* 1. 时钟系统 (120MHz) */
    InitRCU();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

    /* 2. 调试串口 (USART2) */
    debug_printf_init(DEBUG_USART_BAUDRATE);
    debug_printf("\r\n=== 1.5kW OBC monitor FW v0.2.2 ===\r\n");

    /* 3. GPIO */
    hw_gpio_init();

    /* 4. SysTick 1ms */
    systick_1ms_init();
	
    /* 5. PWM-to-DC references. Analog ICs close the CC/CV loops. */
    cv_pwm_init(PWM_BASE_HZ, CV_PWM_DUTY_INIT);
    cc_pwm_init(PWM_BASE_HZ, CC_PWM_DUTY_INIT);

    /* 6. ADC channels are used for monitoring/protection, never PWM correction. */
    adc_multi_init_dma(ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_multi_start();
    //adc_multi_trigger_fast();

    /* 7. 100us 快速采样定时器 */
    //adc_fast_timer_init_100us();

    /* 8. 保护 (LLC_FAULT_CHECK 输入 + HARD_FAULT_CLR 输出) */
    protect_init();

    /* 9. CAN 通信 (ISO1050, 500kbps, PA11/PA12) */
		/* CAN is intentionally left uninitialized during first bench integration. */
    //can_comm_init(CAN_BAUDRATE);
		
		    /* 10. 启动自检,此时不启动TIMER3，避免自检期间积累采样节拍 */
    bool protect_ok = protect_startup_check();
    bool adc_ok = adc_startup_check();

    if (!protect_ok || !adc_ok) {
        debug_printf("[STARTUP] Preflight FAIL: protect=%d adc=%d\r\n", protect_ok, adc_ok);
        g_charger_state = MAIN_STEP_FAULT;
        g_fault = FAULT_HARDWARE_PRO;
        gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
        while (1) { __NOP(); }  /* 安全停机 */
    }
    debug_printf("[STARTUP] Preflight OK\r\n");

    /* 11. Power-stage supervisory sequencing (no digital control loop). */
    pfc_init();
    power_supervisor_init();
		serial_console_init();
    /*
     *  清除初始化阶段的1ms节拍统计，预触发第一帧ADC，最后启动5kHz TIMER3监控节拍。
     */
    __disable_irq();
    s_control_tick_pending = 0U;
    s_adc_monitor_tick_pending = 0U;
    s_tick_drop_count = 0U;
    s_adc_monitor_tick_drop_count = 0U;
    __enable_irq();

    adc_multi_trigger_fast();
    adc_monitor_timer_init();

    debug_printf("[MAIN] Enter standby, ADC monitor=%luHz\r\n",
                 (unsigned long)ADC_MONITOR_SAMPLE_HZ);
    while (1) {
			  uint32_t run_control_tick = 0U;
        uint32_t run_adc_monitor_tick = 0U;
				/*
         * 原子地取走“最新任务”标志。
         * 每类任务本轮最多执行一次，不使用while补跑历史节拍。
         */
        __disable_irq();
        run_control_tick = s_control_tick_pending;
        s_control_tick_pending = 0U;
        run_adc_monitor_tick = s_adc_monitor_tick_pending;
        s_adc_monitor_tick_pending = 0U;
        __enable_irq();

				if(run_adc_monitor_tick != 0)
				{
				/*
				 * 读取上一节拍启动并已完成的DMA帧，累计AC RMS样本，
				 * 随后启动下一帧转换。遗漏节拍不会在这里补采。
				 */
					adc_multi_copy();
					adc_ac_sample_fast(g_adc_multi.ac_vol_raw);
					adc_multi_trigger_fast();
				}
				
				if (run_control_tick != 0U) {
            monitor_protect_tick_1khz();
        }
        /* 非阻塞串口发送 */
        if (debug_buffer_used() > 0U) {
            debug_tx_task();
        }

        /* CAN 接收处理 (非阻塞轮询) */
				/* can_comm_poll(); -- reserved for the later CAN integration phase. */
        //can_comm_poll();
		}
			
}
