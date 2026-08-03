/*********************************************************************************************************
* 模块名称：protect.c
* 摘    要：
* 作    者：Rengar
* 内    容：故障保护 (LLC_FAULT_CHECK 轮询 + OCP/OVP/过温)
* 注    意：HARD_PRO/BKIN 未接 MCU, 改为轮询 LLC_FAULT_CHECK (D触发器锁存, 低=故障), HARD_FAULT_CLR 用于 MCU 清除该锁存。
*          
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "protect.h"
#include "adc_dma.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static volatile bool s_fault_latched = false;
/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/

/* ========== 初始化 ========== */
void protect_init(void)
{
    rcu_periph_clock_enable(LLC_FAULT_CHECK_RCU);
    rcu_periph_clock_enable(HARD_FAULT_CLR_RCU);

    /* LLC_FAULT_CHECK 输入 (上拉, 低=故障) */
    gpio_init(LLC_FAULT_CHECK_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, LLC_FAULT_CHECK_PIN);
    /* HARD_FAULT_CLR 输出 (初始低, 不清除锁存) */
    gpio_init(HARD_FAULT_CLR_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, HARD_FAULT_CLR_PIN);
    gpio_bit_reset(HARD_FAULT_CLR_PORT, HARD_FAULT_CLR_PIN);
}

bool protect_fault_active_hw(void)
{
    /* D触发器输出, 低电平=故障 */
    return (gpio_input_bit_get(LLC_FAULT_CHECK_PORT, LLC_FAULT_CHECK_PIN) == 0U);
}

bool protect_fault_latched(void) { return s_fault_latched; }

void protect_clear_fault(void)
{
    /* 脉冲清除外部锁存: 拉高 -> 延时 -> 拉低 (D触发器清零需保持一定脉宽) */
    gpio_bit_set(HARD_FAULT_CLR_PORT, HARD_FAULT_CLR_PIN);
    delay_ms(10U);
    gpio_bit_reset(HARD_FAULT_CLR_PORT, HARD_FAULT_CLR_PIN);
    s_fault_latched = false;
}

void protect_set_fault(fault_type_t f)
{
    s_fault_latched = true;
    g_fault = f;
    g_charger_state = MAIN_STEP_FAULT;
}

/* ========== 软件保护 (1kHz 轮询) ========== */
void protect_tick_1khz(void)
{
    extern adc_multi_t g_adc_multi;

    if (g_charger_state == MAIN_STEP_FAULT) return;

    /* LLC_FAULT_CHECK (D触发器输出, 低=故障) */
    if (protect_fault_active_hw()) {
        protect_set_fault(FAULT_LLC_CHECK);
        return;
    }

    /* OVP: 输出过压 */
    if (g_adc_multi.vout_v > VOUT_OVP_V) {
        protect_set_fault(FAULT_OVP);
        return;
    }

    /* OCP: 输出过流 (主限流由模拟硬件, 此为软件后备) */
    if (g_adc_multi.iout_a > IOUT_OCP_A) {
        protect_set_fault(FAULT_OCP);
        return;
    }

    /* 过温保护: PFC MOS / LLC MOS / 变压器 / 外壳 */
    if (g_adc_multi.t_pfc_c > TEMP_SHUTDOWN_C) { protect_set_fault(FAULT_OVER_TEMP_PFC);  return; }
    if (g_adc_multi.t_llc_c  > TEMP_SHUTDOWN_C) { protect_set_fault(FAULT_OVER_TEMP_LLC); return; }
    if (g_adc_multi.t_tr_c   > TEMP_SHUTDOWN_C) { protect_set_fault(FAULT_OVER_TEMP_TR);  return; }
    if (g_adc_multi.t_case_c > TEMP_SHUTDOWN_C) { protect_set_fault(FAULT_OVER_TEMP_CASE);return; }
}







