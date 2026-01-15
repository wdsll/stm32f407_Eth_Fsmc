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
typedef struct {
	float vref, vmeas;
	float kp, ki, integ;
	float f_min, f_max, f_cmd, f_slew;
} llc_t;

static llc_t s_llc;
/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/
void llc_app_init(void);
void llc_app_tick_1khz(void);
void llc_app_tick_1khz_withoutVbus(void);
llc_state_t llc_app_state(void);


void llc_app_tick_adc_test(void);



#endif /* LLC_CONTROL_H */