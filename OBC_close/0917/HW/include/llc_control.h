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
	ST_CYCLE_STOPPING,  /* 周期性软关断状态 */
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
	
	/* 电压环 PI 参数 */
	float kp, ki, integ;
	float kp_raw, ki_raw;  // 备用RAW_PI
	/* 电流环（先保留） */
	float iref, imeas;
	float ikp, iki, i_integ;
	
	 /* 频率边界 */
	float f_min, f_max, f_cmd,f_nom, f_slew;
	float e_db, f_q_step;
	float f_cmd_v, f_cmd_i;
	
	bool ctrl_en_z1;
} llc_t;
//static llc_t s_llc;

/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/
void llc_app_init(void);
void llc_app_tick_1khz(void);
void llc_app_tick_100us(void);
void llc_app_tick_1khz_withoutVbus(void);
llc_state_t llc_app_state(void);


void llc_app_tick_adc_test(void);



#endif /* LLC_CONTROL_H */