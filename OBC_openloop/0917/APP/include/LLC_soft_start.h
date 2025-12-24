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
	float start_hz; //起始频率
	float target_hz; //目标频率
	float last_hz;  //最终频率
} llc_softstart_ctx_t;


/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/

void llc_softstart_tick_1khz(void);
void llc_softstart_update_target(float new_target_hz);
void llc_softstart_set_pause(bool pause);
void llc_softstart_abort(void);

void llc_softstart_on_fault(void);
void llc_softstart_start(float target_freq_hz);
void llc_softstart_init(void);

#endif