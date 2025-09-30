
/*********************************************************************************************************
* 模块名称：main.c
* 摘    要：
* 当前版本：1.0.0
* 作    者：Rengar
* 完成日期：2025年09月24日  
* 内    容：
* 注    意：                                                                  
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "main.h"
#include "add_dma.h"
#include "pwm_llc.h"
#include "RCU.h"
#include "ICU.h"
#include "pwm.h"
#include "protect_exti.h"
#include "llc_open_loop.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

#define LLC_USE_OPEN_LOOP 1
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/

enum{
	LLC_START_DELAY_MS = 100
};

typedef struct
{
	llc_state_t state;
	uint32_t entry_ms;
}llc_app_ctx_t;
/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static llc_app_ctx_t s_llc_app;
static void llc_state_enter(llc_state_t next);
void systick_1ms_init(void);

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/

static llc_t s_llc;

#if LLC_USE_OPEN_LOOP
static llc_open_loop_ctrl_t s_llc_open_loop;
static const llc_open_loop_segment_t s_llc_open_loop_profile[] = {
	{ .start_hz = LLC_F_MIN_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = LLC_F_SLEW_HZ, .hold_time_ms = 200U },
	{ .start_hz = LLC_F_INIT_HZ, .stop_hz = LLC_F_MAX_HZ, .slew_hz_per_ms = 500.0f, .hold_time_ms = 200U },
	{ .start_hz = LLC_F_MAX_HZ, .stop_hz = LLC_F_INIT_HZ, .slew_hz_per_ms = 500.0f, .hold_time_ms = 0U },
};
#endif

volatile uint32_t g_ms=0;
void systick_1ms_init(void){
		SystemCoreClockUpdate();    
     uint32_t reload  = SystemCoreClock / 1000U;
	  if (reload == 0U || reload > SysTick_LOAD_RELOAD_Msk) {
                                           // 失败：频率异常或超出24位
    }
		reload -= 1U;
		if (reload > SysTick_LOAD_RELOAD_Msk) {
			reload = SysTick_LOAD_RELOAD_Msk;
		}
		
		SysTick->CTRL = 0U;
		SysTick->LOAD = reload;
		SysTick->VAL  = 0U;
    NVIC_SetPriority(SysTick_IRQn, 0x0F);
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
								SysTick_CTRL_TICKINT_Msk   |
								SysTick_CTRL_ENABLE_Msk;
}

static inline float conv_adc_to_v_div(uint16_t raw, float rtop, float rbot){
    float v = (raw * VREF_ADC) / 4095.0f;
    return v * (rtop + rbot) / rbot;
}
static inline float conv_adc_to_i(uint16_t raw){
    float v = (raw * VREF_ADC) / 4095.0f;
    return v / (ISHUNT_OHM * IAMP_GAIN);
}
static inline float f_absf(float x){ return x < 0 ? -x : x; }
static inline float f_minf(float a,float b){ return a < b ? a : b; }
static inline float f_maxf(float a,float b){ return a > b ? a : b; }
static inline float f_clampf(float x,float lo,float hi)
{ return x<lo?lo:(x>hi?hi:x); }


void llc_step(llc_t* l){
	/* 1) 误差：目标电压 - 实测电压（单位V） */
    float e = l->vref - l->vmeas;
	/* 2) 积分累加：每个控制周期增加 ki*e（不考虑饱和） */
    l->integ += l->ki * e;
	/* 3) PI输出映射到频率请求：kp*e + I，再加基线 f_min（单位Hz） */
    float f_req = l->kp * e + l->integ + l->f_min;
	/* 4) 限幅 */
    if(f_req < l->f_min)   // 频率下限钳位：不可低于 f_min
			f_req = l->f_min;
    if(f_req > l->f_max)   // 频率上限钳位：不可高于 f_max
			f_req = l->f_max;
		
    float df = f_req - l->f_cmd;  // 期望频率与当前下发频率的差值 Δf
		
    if(f_absf(df) > l->f_slew)   // 斜率限幅：若 |Δf| 大于每周期最大步长 f_slew
			l->f_cmd += (df>0?l->f_slew:-l->f_slew); //就按正/负方向只移动 f_slew（限速变频）
    else 
			l->f_cmd = f_req; // 否则一步到位：直接把下发频率设为目标
}




static void llc_state_enter(llc_state_t next)
{
	s_llc_app.state = next;
	s_llc_app.entry_ms = g_ms;
	
	switch(next)
	{
		case ST_IDLE:
		case ST_WAIT_VBUS:
#if LLC_USE_OPEN_LOOP
			llc_open_loop_stop(&s_llc_open_loop);
#endif
			llc_pwm_outputs_enable(0);
			s_llc.integ = 0.0f;
			s_llc.f_cmd = s_llc.f_min;
			break;
		case ST_LLC_RUN:
			s_llc.integ = 0.0f;
#if LLC_USE_OPEN_LOOP
			llc_open_loop_start(&s_llc_open_loop);
			s_llc.f_cmd = f_clampf(llc_open_loop_get_freq(&s_llc_open_loop), s_llc.f_min, s_llc.f_max);
#else
			s_llc.f_cmd = f_clampf(LLC_F_INIT_HZ, s_llc.f_min, s_llc.f_max);
#endif
			llc_pwm_outputs_enable(1);
			break;
		case ST_FAULT:
		default:
			llc_pwm_outputs_enable(0);
			s_llc.f_cmd = s_llc.f_min;
			break;
	}
}

void llc_app_init()
{
	llc_state_enter(ST_WAIT_VBUS);
}

void llc_app_tick_1khz(void)
{
	switch(s_llc_app.state)
	{
		case ST_IDLE:
			llc_state_enter(ST_WAIT_VBUS);
			break;
		case ST_WAIT_VBUS:
			if(protect_fault_latched() || protect_fault_active_hw())
			{
				llc_state_enter(ST_FAULT);
			}
			else if((uint32_t)(g_ms - s_llc_app.entry_ms)>=LLC_START_DELAY_MS)
			{
				llc_state_enter(ST_LLC_RUN);
			}
		case ST_LLC_RUN:
			if(protect_fault_latched() || protect_fault_active_hw())
			{
				llc_state_enter(ST_FAULT);
			}
			break;
		case ST_FAULT:
		default:
			break;
	}	
}

llc_state_t llc_app_state(void)
{
	return s_llc_app.state;
}

void SysTick_Handler(void){
    g_ms++;
    /* 1 kHz control */
    adc_multi_copy();
    float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    s_llc.vmeas = vout;
    //llc_step(&s_llc);
		llc_app_tick_1khz();
		if(llc_app_state() == ST_LLC_RUN)
		{
#if LLC_USE_OPEN_LOOP
		llc_open_loop_tick(&s_llc_open_loop);
		s_llc.f_cmd = f_clampf(llc_open_loop_get_freq(&s_llc_open_loop), s_llc.f_min, s_llc.f_max);
#else
			llc_step(&s_llc);
#endif
		}
    llc_pwm_set_freq((uint32_t)s_llc.f_cmd);
}
int main(void){
	
	  InitRCU();
	  //RCU_Config_72M();
    //systick_1ms_init();

    /* LLC complementary PWM 配置LLC的PWM频率 、死区时间和占空比，并初始化PWM模块*/
    llc_pwm_cfg_t lcfg = { .pwm_hz=LLC_PWM_BASE_HZ, .deadtime_ns=LLC_PWM_DEAD_NS, .duty=LLC_PWM_DUTY };
    llc_pwm_init(&lcfg);

    /* Aux PWM on PB0 */
		pb0_pwm_init(PB0_PWM_BASE_HZ);
		pb0_pwm_set_duty(0.5f);

    /* ADC multi (PA3/PA1 removed) triggered by TIMER0 CH0 for coherence */
    adc_multi_init_dma(ADC0_1_EXTTRIG_REGULAR_T0_CH0); 
    adc_multi_start();

    /* PA0 & PA1 input capture */
    cap_pa01_init();

    /* Protection EXTI PC11 */
    protect_exti_init();

    /* LLC control default。初始化LLC的控制参数，包括目标电压、PID参数、频率范围和初始频率。*/
    s_llc = (llc_t){ 
			.vref=VBUS_TARGET_V, .vmeas=0.0f, .kp=0.01f, .ki=0.0005f,
      .f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ, .f_slew=LLC_F_SLEW_HZ 
		};
#if LLC_USE_OPEN_LOOP
		llc_open_loop_init(&s_llc_open_loop, s_llc_open_loop_profile, sizeof(s_llc_open_loop_profile)/sizeof(s_llc_open_loop_profile[0]));
#endif
		llc_app_init();
		systick_1ms_init();
    while(1){
			
			
				float  duty0, duty1; //PA3 PA1捕获的值
        if(cap_pa0_read_duty(&duty0)){
            (void)duty0; /* TODO: convert ticks->Hz using TIMER1 clock if? */
        }
         if(cap_pa1_read_duty(&duty1)){
            (void)duty1;
        }
        //if(protect_fault_latched()){
					//检测到故障（通过PC11中断），则关闭LLC的PWM输出，并标记需要进一步处理故障。
          //  llc_pwm_outputs_enable(0);
            /* TODO: fault handling */
        //}
				if(llc_app_state() == ST_FAULT)
				{
					llc_pwm_outputs_enable(0);
				}
        __NOP();
    }
}

