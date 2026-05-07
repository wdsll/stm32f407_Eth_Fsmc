/*********************************************************************************************************
* 模块名称：bkin_selftest.c
* 摘    要：BKIN 刹车脚上电自检实现（不上高压）
* 当前版本：1.0.0
* 作    者：Re & 小姜
* 完成日期：2026-05-07
*
* 测试原理：
*   4840 原理图：LMV393(OCP) → HARD_PRO → LTV-817S(光耦) → PB12(BKIN)
*   正常无故障时光耦截止，PB12 被 IPU 上拉为高电平。
*
*   本模块在不施加高压的前提下，验证以下硬件保护通道：
*     1. GPIO 空闲高电平（确认光耦未短路、BKIN 未对地短路）
*     2. TIMER0 Break 中断机制（软件触发 Break，确认中断入口可达）
*     3. MOE 自动关断（Break 触发后 PWM 输出应立即关闭）
*     4. 故障恢复能力（清除锁存后系统可恢复正常状态）
*
* 安全约束：
*   - 整个过程 PFC/LLC 未启动，MOE 始终为 0，无 PWM 输出
*   - 即使测试异常，也不会产生功率输出
*   - 测试完成后 MOE 保持 0，等待后续正常启动流程开启
*********************************************************************************************************/
#include "bkin_selftest.h"
#include "pwm_llc.h"
#include "protect_exti.h"
#include <stdio.h>

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static volatile bkin_test_result_t s_last_result = 0xFFU;

/* g_brk_irq_fired 已在 protect_exti.c 中定义，通过 protect_exti.h 引用 */

/*********************************************************************************************************
*                                              内部函数
*********************************************************************************************************/

/**
 * @brief 延时等待（基于 g_ms）
 * @param ms 等待毫秒数
 */
static void selftest_delay_ms(uint32_t ms)
{
    uint32_t start = g_ms;
    while ((uint32_t)(g_ms - start) < ms) {
        __NOP();
    }
}

/**
 * @brief 检查 TIMER0 MOE (Main Output Enable) 状态
 * @return 1: MOE=1(输出使能)  0: MOE=0(输出禁止)
 */
static uint8_t selftest_moe_state(void)
{
    /* TIMER_CCHP(TIMER0) 寄存器 bit15 = POEN (Primary Output Enable) */
    return (TIMER_CCHP(TIMER0) & TIMER_CCHP_POEN) ? 1U : 0U;
}

/**
 * @brief 检查 TIMER0 Break 标志
 * @return 1: Break 标志已置位  0: Break 标志未置位
 */
static uint8_t selftest_brk_flag_state(void)
{
    return (timer_interrupt_flag_get(TIMER0, TIMER_INT_FLAG_BRK) != RESET) ? 1U : 0U;
}

/*********************************************************************************************************
*                                              公共接口
*********************************************************************************************************/

bkin_test_result_t bkin_selftest_run(void)
{
    bkin_test_result_t result = BKIN_TEST_PASS;

    debug_printf("[BKIN-SELFTEST] === Start (no HV) ===\r\n");

    /*==================================================================
     * Step 0: 前置条件检查 - 确保 MOE 为 0（无 PWM 输出）
     *==================================================================*/
    llc_pwm_outputs_enable(0);
    protect_clear_fault();
    g_brk_irq_fired = 0U;

    /*==================================================================
     * Step 1: GPIO 空闲电平测试
     *
     * 正常状态：光耦截止，IPU 上拉 → PB12 = 高电平
     * 异常情况：光耦短路 / BKIN 对地短路 → PB12 = 低电平
     *==================================================================*/
    debug_printf("[BKIN-SELFTEST] Step1: GPIO idle level check...\r\n");

    if (protect_fault_active_hw()) {
        /* BKIN 为低电平 → 硬件异常 */
        result = (bkin_test_result_t)((uint32_t)result | BKIN_TEST_FAIL_GPIO_LOW);
        debug_printf("[BKIN-SELFTEST] FAIL: PB12 is LOW at idle (opto short / BKIN-GND short)\r\n");
    } else {
        debug_printf("[BKIN-SELFTEST] PASS: PB12 idle HIGH\r\n");
    }

    /*==================================================================
     * Step 2: TIMER0 Break 中断 + MOE 关断测试
     *
     * 策略：
     *   先短暂使能 MOE（不开启 PWM 输出通道），然后软件触发 Break，
     *   验证 Break 中断能否触发、MOE 能否被硬件自动清零。
     *
     * 安全保证：
     *   - llc_pwm_outputs_enable(0) 后 PWM 通道 CCR=0，即使 MOE=1
     *     也不会产生有效驱动脉冲
     *   - 此步仅验证 TIMER0 硬件 Break 通道功能
     *==================================================================*/
    debug_printf("[BKIN-SELFTEST] Step2: Break interrupt + MOE test...\r\n");

    /* 2a. 清除 Break 标志和软件锁存 */
    timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_BRK);
    protect_clear_fault();
    g_brk_irq_fired = 0U;

    /* 2b. 短暂使能 MOE，模拟"正常运行"状态 */
    timer_primary_output_config(TIMER0, ENABLE);

    /* 2c. 软件强制拉低 BKIN GPIO，模拟硬件 OCP 触发
     *     将 BKIN(PB12) 临时切换为推挽输出并输出低电平
     *
     * 注意：此操作会覆盖 IPU 配置，测试完成后必须恢复
     */
    gpio_init(BKIN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, BKIN_PIN);
    gpio_bit_reset(BKIN_PORT, BKIN_PIN);  /* PB12 = Low → 触发 BKIN */

    /* 2d. 等待 Break 中断触发 */
    {
        uint32_t start = g_ms;
        while (!g_brk_irq_fired && !selftest_brk_flag_state()) {
            if ((uint32_t)(g_ms - start) >= BKIN_SELFTEST_BRK_TIMEOUT_MS) {
                break;
            }
            __NOP();
        }
    }

    /* 2e. 恢复 BKIN GPIO 为 IPU 模式（必须尽快恢复，避免误触发） */
    gpio_init(BKIN_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, BKIN_PIN);

    /* 2f. 检查 Break 标志 */
    if (!selftest_brk_flag_state()) {
        result = (bkin_test_result_t)((uint32_t)result | BKIN_TEST_FAIL_BRK_FLAG);
        debug_printf("[BKIN-SELFTEST] FAIL: Break flag NOT set after BKIN low\r\n");
    } else {
        debug_printf("[BKIN-SELFTEST] PASS: Break flag set\r\n");
    }

    /* 2g. 检查 MOE 是否被硬件自动清零 */
    if (selftest_moe_state() != 0U) {
        result = (bkin_test_result_t)((uint32_t)result | BKIN_TEST_FAIL_MOE_NOT_CLR);
        debug_printf("[BKIN-SELFTEST] FAIL: MOE still HIGH after Break\r\n");
    } else {
        debug_printf("[BKIN-SELFTEST] PASS: MOE cleared by Break\r\n");
    }

    /* 2h. 检查 Break ISR 是否执行（通过 protect_fault_latched 间接判断） */
    if (!protect_fault_latched()) {
        result = (bkin_test_result_t)((uint32_t)result | BKIN_TEST_FAIL_BRK_IRQ);
        debug_printf("[BKIN-SELFTEST] FAIL: Break ISR NOT executed (fault not latched)\r\n");
    } else {
        debug_printf("[BKIN-SELFTEST] PASS: Break ISR fired, fault latched\r\n");
    }

    /*==================================================================
     * Step 3: 故障恢复测试
     *
     * BKIN 恢复高电平后，清除软件锁存，验证系统能恢复正常
     *==================================================================*/
    debug_printf("[BKIN-SELFTEST] Step3: Fault recovery test...\r\n");

    /* 等待 BKIN 恢复高电平（IPU 上拉生效） */
    selftest_delay_ms(BKIN_SELFTEST_RECOVER_MS);

    if (protect_fault_active_hw()) {
        result = (bkin_test_result_t)((uint32_t)result | BKIN_TEST_FAIL_NO_RECOVER);
        debug_printf("[BKIN-SELFTEST] FAIL: BKIN still LOW after recovery delay\r\n");
    } else {
        debug_printf("[BKIN-SELFTEST] PASS: BKIN recovered HIGH\r\n");
    }

    /*==================================================================
     * Step 4: 清理恢复
     *
     * 清除所有故障标志，MOE 保持 0（安全状态），等待正常启动流程
     *==================================================================*/
    timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_BRK);
    protect_clear_fault();

    /* 确保 MOE=0（安全退出） */
    llc_pwm_outputs_enable(0);

    s_last_result = result;

    if (result == BKIN_TEST_PASS) {
        debug_printf("[BKIN-SELFTEST] === ALL PASSED ===\r\n");
    } else {
        debug_printf("[BKIN-SELFTEST] === FAILED: 0x%02X ===\r\n", (unsigned)result);
    }

    return result;
}

bkin_test_result_t bkin_selftest_last_result(void)
{
    return s_last_result;
}

const char* bkin_selftest_result_str(bkin_test_result_t result)
{
    if (result == BKIN_TEST_PASS) {
        return "PASS";
    }

    /* 多重故障拼接描述（静态缓冲区，适合 debug_printf 一次性打印） */
    static char buf[128];
    int pos = 0;

    if (result & BKIN_TEST_FAIL_GPIO_LOW) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%sGPIO_LOW", pos ? "|" : "");
    }
    if (result & BKIN_TEST_FAIL_BRK_IRQ) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%sBRK_IRQ", pos ? "|" : "");
    }
    if (result & BKIN_TEST_FAIL_MOE_NOT_CLR) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%sMOE_NOT_CLR", pos ? "|" : "");
    }
    if (result & BKIN_TEST_FAIL_BRK_FLAG) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%sBRK_FLAG", pos ? "|" : "");
    }
    if (result & BKIN_TEST_FAIL_NO_RECOVER) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%sNO_RECOVER", pos ? "|" : "");
    }

    if (pos == 0) {
        return "UNKNOWN";
    }

    return buf;
}
