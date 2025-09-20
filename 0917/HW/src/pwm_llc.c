
#include "pwm_llc.h"
#include "main.h"

//GPIO：高臂、低臂、N 输出都用 AF_PP；BKIN=PB12 用 上拉输入。

//死区：确认 bdtr_deadtime_code_ns(350ns, 108MHz) 得到约 0x26（≈352ns）。

//BKIN：短接 PB12→GND，应当瞬时关断（MOE 清）；松开在一个更新周期后自动恢复（因 outputautostate=ENABLE）。

//变频：调用 llc_pwm_set_freq() 后，示波器看到占空不漂；若固定 50%，把 llc_pwm_set_freq() 里直接设 CCR = ARR/2。
//#include "gd32f30x_timer.h"
static llc_pwm_cfg_t s_cfg; 
static uint32_t s_period=0; 
static inline float clampf(float x,float a,float b)
{ 
	return x<a?a:(x>b?b:x); 
}
static uint8_t dt_ticks(uint32_t ns, uint32_t clk)
{ 
	//unsigned long long t=(unsigned long long)ns*clk/1000000000ULL; 
	//return (t>255)?255:(uint8_t)t;
}

static uint8_t bdtr_deadtime_code_ns(uint32_t dead_ns, uint32_t clk)
{
	double t = 1e9 / (double)clk;

	uint32_t c = (uint32_t)(dead_ns / t + 0.5);
	if (c <= 127U) 
		return (uint8_t)c;
	
	uint32_t c2 = (uint32_t)(dead_ns/(2.0*t)+0.5); 
	if (c2>=64U && c2<=127U) 
		return 128U+(uint8_t)(c2-64U);
	
	uint32_t c8 = (uint32_t)(dead_ns/(8.0*t)+0.5); 
	if (c8>=32U && c8<=63U)  
		return 192U+(uint8_t)(c8-32U);
	
	uint32_t c16= (uint32_t)(dead_ns/(16.0*t)+0.5);
	if (c16>=32U && c16<=63U) 
		return 224U+(uint8_t)(c16-32U); 

}
static void pins_init(void){
    rcu_periph_clock_enable(RCU_GPIOA); 
		rcu_periph_clock_enable(RCU_GPIOB);
    gpio_init(LLC_H_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, LLC_H_PIN);
    gpio_init(LLC_L_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, LLC_L_PIN);
    gpio_init(BKIN_PORT,  GPIO_MODE_IPU,  GPIO_OSPEED_50MHZ, BKIN_PIN);
}

static uint32_t timer0_clk_hz(void){
    uint32_t apb2 = rcu_clock_freq_get(CK_APB2);
    return (RCU_CFG0 & RCU_CFG0_APB2PSC) ? (apb2 * 2U) : apb2;  // APB2分频≠1 时 TIMx=APB×2
}

void llc_pwm_init(const llc_pwm_cfg_t* cfg){
    s_cfg=*cfg;
    pins_init();
    rcu_periph_clock_enable(RCU_TIMER0);

    timer_parameter_struct t;
    timer_struct_para_init(&t);
	
    uint32_t tclk = timer0_clk_hz();                // 如果你库里没有此宏，见文末备注
    s_period = (tclk/(2U*s_cfg.pwm_hz)) - 1U;      /* 中心对齐频率公式 */

    t.prescaler         = 0;
    t.alignedmode       = TIMER_COUNTER_CENTER_BOTH;
    t.counterdirection  = TIMER_COUNTER_UP;
    t.period            = s_period;
    t.clockdivision     = TIMER_CKDIV_DIV1;
    t.repetitioncounter = 0;
    timer_init(TIMER0, &t);

    /* === 手动初始化 OC 结构体（无 para_init 版本）  OC 配置：一个通道 + 互补（半桥）  === */
    timer_oc_parameter_struct oc;
    oc.outputstate   = TIMER_CCX_ENABLE;
    oc.outputnstate  = TIMER_CCXN_ENABLE;
    oc.ocpolarity    = TIMER_OC_POLARITY_HIGH;
    oc.ocnpolarity   = TIMER_OCN_POLARITY_HIGH;
    oc.ocidlestate   = TIMER_OC_IDLE_STATE_LOW;
    oc.ocnidlestate  = TIMER_OCN_IDLE_STATE_LOW;
		//oc.ocnidlestate  = TIMER_OCN_IDLE_STATE_LOW;
		
    timer_channel_output_config(TIMER0, LLC_PWM_CH, &oc);
    timer_channel_output_mode_config(TIMER0, LLC_PWM_CH, TIMER_OC_MODE_PWM0);
    timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, 0);
    timer_channel_output_shadow_config(TIMER0, LLC_PWM_CH, TIMER_OC_SHADOW_ENABLE);

    /* === 手动初始化 Break/Deadtime 结构体（无 para_init 版本） === */
    timer_break_parameter_struct bk;
    bk.runoffstate     = TIMER_ROS_STATE_ENABLE;
    bk.ideloffstate    = TIMER_IOS_STATE_ENABLE;
    bk.protectmode     = TIMER_CCHP_PROT_OFF;
    bk.deadtime        = bdtr_deadtime_code_ns(s_cfg.deadtime_ns, tclk);
    bk.breakstate      = TIMER_BREAK_ENABLE;
    bk.breakpolarity   = TIMER_BREAK_POLARITY_LOW;     // BKIN 低有效
    bk.outputautostate = TIMER_OUTAUTO_ENABLE;

    timer_break_config(TIMER0, &bk);

		timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_CC0);
		
    timer_auto_reload_shadow_enable(TIMER0);
    timer_primary_output_config(TIMER0, ENABLE);
    timer_enable(TIMER0);

    llc_pwm_set_duty(s_cfg.duty); /* 如果你走 50% 固定，这里直接设 0.5f 即可 */
}
/*
void llc_pwm_init(const llc_pwm_cfg_t* cfg){
    s_cfg=*cfg; 
		pins_init(); 
		rcu_periph_clock_enable(RCU_TIMER0);
    timer_parameter_struct t;
		timer_struct_para_init(&t);
    uint32_t tclk=RCU_TIMER0CLK; 
		s_period=(tclk/(2U*s_cfg.pwm_hz))-1U;
    t.prescaler=0; 
		t.alignedmode=TIMER_COUNTER_CENTER_BOTH; 
		t.counterdirection=TIMER_COUNTER_UP;
    t.period=s_period; 
		t.clockdivision=TIMER_CKDIV_DIV1; 
		t.repetitioncounter=0; 
		timer_init(TIMER0,&t);
    timer_oc_parameter_struct oc; 
		timer_channel_output_struct_para_init(&oc);
    oc.outputstate=TIMER_CCX_ENABLE; 
		oc.outputnstate=TIMER_CCXN_ENABLE;
    oc.ocpolarity=TIMER_OC_POLARITY_HIGH; 
		oc.ocnpolarity=TIMER_OCN_POLARITY_HIGH;
    oc.ocidlestate=TIMER_OC_IDLE_STATE_LOW; 
		oc.ocnidlestate=TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER0, LLC_PWM_CH, &oc);
    timer_channel_output_mode_config(TIMER0, LLC_PWM_CH, TIMER_OC_MODE_PWM0);
    timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, 0);
    timer_channel_output_shadow_config(TIMER0, LLC_PWM_CH, TIMER_OC_SHADOW_ENABLE);
    timer_break_parameter_struct bk; 
		//timer_break_struct_para_init(&bk);
    bk.runoffstate=TIMER_ROS_STATE_ENABLE; bk.ideloffstate=TIMER_IOS_STATE_ENABLE;
    bk.protectmode=TIMER_CCHP_PROT_OFF; 
		bk.deadtime=dt_ticks(s_cfg.deadtime_ns,tclk);
    bk.breakstate=TIMER_BREAK_ENABLE; 
		bk.breakpolarity=TIMER_BREAK_POLARITY_LOW; 
		bk.outputautostate=TIMER_OUTAUTO_ENABLE;
    timer_break_config(TIMER0,&bk);
    timer_auto_reload_shadow_enable(TIMER0); 
		timer_primary_output_config(TIMER0, ENABLE); 
		timer_enable(TIMER0);
    llc_pwm_set_duty(s_cfg.duty);
}
*/

/* ---- 频率在线更新：同时更新 ARR 和 CCR，保持占空比 ---- */
static inline uint16_t duty_to_ccr(float d)
{ 
	d=clampf(d,0.0f,0.99f); 
	return (uint16_t)(d*s_period); 
}
void llc_pwm_set_duty(float d)
{ 
	timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, duty_to_ccr(d)); 
}
void llc_pwm_outputs_enable(bool en)
{ 
	timer_primary_output_config(TIMER0, en?ENABLE:DISABLE); 
}

void llc_pwm_break(bool en){
    timer_primary_output_config(TIMER0, en ? DISABLE : ENABLE);  // en=1 相当于“刹停”
}
void llc_pwm_set_freq(uint32_t f_hz)
{ 
	if(!f_hz) 
			return;
	s_cfg.pwm_hz=f_hz; 
	uint32_t tclk=timer0_clk_hz(); 
	s_period=(tclk/(2U*f_hz))-1U; 
	TIMER_CAR(TIMER0)=s_period; 
	
	/* 保持当前占空比（或固定 50%：直接用 s_period/2） */
	TIMER_CH0CV(TIMER0) = duty_to_ccr(s_cfg.duty);
	/* 若 LLC_PWM_CH 不是 CH0，请改成对应的 TIMER_CHxCV 宏 */
}
