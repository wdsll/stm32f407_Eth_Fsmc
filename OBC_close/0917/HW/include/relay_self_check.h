#ifndef RELAY_SELF_CHECK_H
#define RELAY_SELF_CHECK_H

#include <stdint.h>
/* 不包含 <stdbool.h>：GD32 固件已 typedef bool，ARMCLANG 下会冲突 */

/**
 * relay_self_check.h - 主继电器独立自检模块
 * 
 * 在 PFC/LLC 未工作的安全低压状态下，验证两个继电器能否正常软控：
 *   1. PFC 主继电器 (PC10) - 交流输入侧预充/继电器使能
 *   2. 直流输出继电器 (PB14) - 充电输出侧
 *
 * 自检方法：软件 GPIO 回读（写→切浮空输入→读回，验证一致性）
 * 适用场景：无硬件辅助触点反馈时
 *
 * 调用时机：main.c 中 pfc_init() 之后、pfc_enable() 之前
 */

/* ---------- 自检结果枚举 ---------- */
typedef enum {
    RELAY_CHECK_PASS     = 0,   /* 全部通过 */
    RELAY_CHECK_PFC_FAIL = 1,   /* PFC主继电器异常 */
    RELAY_CHECK_OUT_FAIL = 2,   /* 输出继电器异常 */
} relay_check_result_t;

/* ---------- 初始化 + 执行 ---------- */

/**
 * @brief  初始化两个继电器GPIO为输出模式（断开状态）
 * @note   必须在自检前调用一次，确保GPIO已初始化
 */
void relay_sc_init(void);

/**
 * @brief  执行双继电器完整自检流程
 * @return 自检结果码
 * @note   内部会执行：吸合→读回验证→释放→读回验证
 *         每个继电器各两轮（写高+写低），总耗时约50us
 *         完成后自动恢复两个继电器为断开(Off)状态
 */
relay_check_result_t relay_sc_run(void);

/**
 * @brief  获取上次自检结果的人类可读描述
 * @param  result: relay_sc_run() 的返回值
 * @return 静态字符串指针（无需free）
 */
const char* relay_sc_result_str(relay_check_result_t result);

#endif /* RELAY_SELF_CHECK_H */
