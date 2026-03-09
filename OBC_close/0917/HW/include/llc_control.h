#ifndef LLC_CONTROL_H
#define LLC_CONTROL_H

#include "main.h"

typedef enum 
{ 
	ST_IDLE=0, 
	ST_PRECHECK,
	ST_SOFTSTART,
	ST_RUN_ENTRY_HOLD,
	ST_LLC_RUN,
	ST_STOPPING,
	ST_FAULT,
} llc_state_t;

typedef struct
{
	llc_state_t state;
	uint32_t entry_ms;
}llc_app_ctx_t;

//vmeas：实际测量到的电压 
//integ：积分器的当前累积值（积分状态），通常会在饱和或模式切换时清零或软限制以防风up。
//新增电流环pi参数
typedef struct {
	float vref, vmeas;
	float kp, ki, integ;
	float iref, imeas;
	float ikp, iki, i_integ;
	float f_min, f_max, f_cmd, f_slew;
	float f_nom, e_db, f_q_step;
	float f_cmd_v, f_cmd_i;
} llc_t;

static llc_t s_llc;

typedef enum
{
    LLC_MODE_NORMAL = 0,
    LLC_MODE_LOOP_SCAN
} llc_mode_t;


/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/
void llc_app_init(void);

/* 20us周期入口：三层任务架构（20us高频 + 100us中速 + 1ms低速）
 * 需在定时器中断中每20us调用一次
 */
void llc_app_tick_20us(void);

/* 100us兼容入口：当定时器配置为100us时使用
 * 自动分频为100us中速任务 + 1ms低速任务
 */
void llc_app_tick_100us_compat(void);

/* 保持向后兼容：实际调用llc_app_tick_20us()实现 */
void llc_app_tick_100us(void);

void llc_app_tick_1khz_withoutVbus(void);
llc_state_t llc_app_state(void);

void llc_set_mode(llc_mode_t mode);
void llc_app_tick_adc_test(void);



#endif /* LLC_CONTROL_H */