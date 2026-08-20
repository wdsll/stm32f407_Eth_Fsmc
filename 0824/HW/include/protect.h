/* protect.h - 故障保护 (LLC_FAULT_CHECK 轮询 + 软件保护)
 * 原理图 HARD_PRO/BKIN 未接 MCU, 改用 LLC_FAULT_CHECK (D触发器输出, 低=故障)
 * + HARD_FAULT_CLR (MCU 清除锁存) */
#ifndef _PROTECT_H_
#define _PROTECT_H_

#include "main.h"

void protect_init(void);             /* 配置 LLC_FAULT_CHECK 输入 + HARD_FAULT_CLR 输出 */
bool protect_fault_active_hw(void);  /* LLC_FAULT_CHECK 当前状态 (低=故障) */
bool protect_fault_latched(void);    /* 软件锁存标志 */

void protect_clear_fault(void);      /* 清除外部锁存 + 软件标志 */
 
void protect_tick_1khz(void);        /* 软件保护检查 (轮询) */
void protect_set_fault(fault_type_t f);

#endif /* _PROTECT_H_ */
