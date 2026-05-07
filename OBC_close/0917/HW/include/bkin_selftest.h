/*********************************************************************************************************
* 模块名称：bkin_selftest.h
* 摘    要：BKIN 刹车脚上电自检模块（不上高压）
* 当前版本：1.0.0
* 作    者：Re & 小姜
* 完成日期：2026-05-07
* 内    容：
*   在 PFC/LLC 上高压之前，验证 TIMER0 BKIN(PB12) 刹车通道功能完整性：
*     1. GPIO 空闲电平读取（IPU 上拉 → 应为高电平）
*     2. TIMER0 Break 中断触发验证（软件模拟 BKIN 低电平）
*     3. MOE 关断验证（Break 后 MOE 应自动清零）
*   自检通过才允许进入后续 PFC/LLC 启动流程。
*
* 硬件信号链路（4840 原理图）：
*   主板 OCP 比较器(LMV393) → HARD_PRO → 光耦(LTV-817S,OP2) → PB12(BKIN)
*   光耦输出低有效：OCP 触发 → 光耦导通 → PB12 被拉低 → BKIN 触发
*
* 注    意：
*   本模块仅在启动阶段运行一次，运行期间由 protect_exti 负责实时保护。
*********************************************************************************************************/
#ifndef BKIN_SELFTEST_H
#define BKIN_SELFTEST_H

#include "main.h"

/* ==== 自检结果码 ==== */
typedef enum {
    BKIN_TEST_PASS              = 0x00U,  /* 全部通过 */
    BKIN_TEST_FAIL_GPIO_LOW     = 0x01U,  /* GPIO 空闲电平为低（光耦短路/BKIN 对地短路） */
    BKIN_TEST_FAIL_BRK_IRQ      = 0x02U,  /* Break 中断未触发 */
    BKIN_TEST_FAIL_MOE_NOT_CLR  = 0x04U,  /* Break 后 MOE 未清零 */
    BKIN_TEST_FAIL_BRK_FLAG     = 0x08U,  /* Break 标志位未置位 */
    BKIN_TEST_FAIL_NO_RECOVER   = 0x10U,  /* BKIN 恢复后 GPIO 电平未回到高 */
} bkin_test_result_t;

/* ==== 自检超时配置 ==== */
#define BKIN_SELFTEST_BRK_TIMEOUT_MS    (50U)    /* 等待 Break 中断超时 */
#define BKIN_SELFTEST_RECOVER_MS        (10U)    /* 等待 BKIN 恢复高电平延时 */

/*********************************************************************************************************
*                                              API
*********************************************************************************************************/

/**
 * @brief BKIN 刹车脚上电自检
 *
 * 在 PFC/LLC 启动前调用。不上高压，仅验证 BKIN GPIO/TIMER0 Break 通道功能。
 *
 * 测试步骤：
 *   Step 1 - GPIO 空闲电平：读取 PB12，IPU 上拉应为高电平
 *   Step 2 - 软件触发 Break：清除 MOE + 置位 Break 标志，模拟 BKIN 低有效
 *   Step 3 - 验证 Break 标志和 MOE 状态
 *   Step 4 - 清除故障锁存，恢复初始状态
 *
 * @return bkin_test_result_t  位域组合，BKIN_TEST_PASS(0x00) 表示全部通过
 *
 * @note 前置条件：
 *   - llc_pwm_init() 已调用（TIMER0 已配置，BKIN GPIO 已初始化为 IPU）
 *   - protect_exti_init() 已调用（Break 中断已使能）
 *   - PFC/LLC 未启动（无高压）
 *
 * @warning 此函数内部会操作 TIMER0 的 MOE 和 Break 标志，
 *          调用结束后会恢复 MOE=0（安全状态）。
 */
bkin_test_result_t bkin_selftest_run(void);

/**
 * @brief 获取上次自检结果（用于运行时查询）
 * @return 上次 bkin_selftest_run() 的返回值，若未运行过返回 0xFF
 */
bkin_test_result_t bkin_selftest_last_result(void);

/**
 * @brief 自检结果转可读字符串
 * @param result 自检结果码
 * @return 结果描述字符串
 */
const char* bkin_selftest_result_str(bkin_test_result_t result);

#endif /* BKIN_SELFTEST_H */
