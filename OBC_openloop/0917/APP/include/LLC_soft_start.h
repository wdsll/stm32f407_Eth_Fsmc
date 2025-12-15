#ifndef LLC_SOFT_START_H
#define LLC_SOFT_START_H

#include "main.h"

/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef struct
{
	bool initialized;
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


/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/

void llc_softstart_tick_1khz(void);
void llc_softstart_update_target(float new_target_0_1);
void llc_softstart_set_pause(bool pause);
void llc_softstart_abort(void);

void llc_softstart_on_fault(void);
void llc_softstart_start(float target_duty_0_1);
void llc_softstart_init(void);

#endif