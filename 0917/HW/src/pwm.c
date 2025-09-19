#include "pwm.h"
#include "main.h"


static uint32_t s_period=0;

static inline float clampf(float x,float a,float b){ return x<a?a:(x>b?b:x); }

static inline uint32_t tim_apb1_clk_hz(void){
    uint32_t pclk1 = rcu_clock_freq_get(CK_APB1);
    /* APB1 预分频≠1 时，定时器时钟翻倍 */
    return (RCU_CFG0 & RCU_CFG0_APB1PSC) ? (pclk1 * 2U) : pclk1;
}

static inline uint32_t timer_clk_hz( ){

        return tim_apb1_clk_hz();
}

void pb0_pwm_init(uint32_t pwm_hz)
{
	rcu_periph_clock_enable(RCU_GPIOB);
	gpio_init(PB0_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PB0_PIN);
	rcu_periph_clock_enable(RCU_TIMER2);
	timer_parameter_struct t;
	timer_struct_para_init(&t);
	uint32_t tclk = timer_clk_hz();
	s_period = (tclk / pwm_hz) - 1U;
	t.prescaler=0;
	t.alignedmode=TIMER_COUNTER_EDGE;
	t.counterdirection=TIMER_COUNTER_UP;
	t.period=s_period;
	t.clockdivision=TIMER_CKDIV_DIV1;
	t.repetitioncounter=0;
	//配置PWM输出通道
	timer_init(PB0_PWM_TIMER, &t);
	//timer_channel_output_struct_para_init(&oc);
	timer_oc_parameter_struct ocpara;
	ocpara.outputstate = TIMER_CCX_ENABLE;          // 启用通道输出
	ocpara.ocpolarity = TIMER_OC_POLARITY_HIGH;     // 输出极性为高电平
	ocpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;   // 空闲状态为低电平
	
	
	timer_channel_output_config(PB0_PWM_TIMER, PB0_PWM_CH, &ocpara);
	timer_channel_output_mode_config(PB0_PWM_TIMER, PB0_PWM_CH, TIMER_OC_MODE_PWM0);
	timer_channel_output_pulse_value_config(PB0_PWM_TIMER, PB0_PWM_CH, 0);
	timer_auto_reload_shadow_enable(PB0_PWM_TIMER); 
	timer_enable(PB0_PWM_TIMER);
}
	
void pb0_pwm_set_duty(float d){
    d = clampf(d,0.0f,0.99f);
    timer_channel_output_pulse_value_config(PB0_PWM_TIMER, PB0_PWM_CH, (uint16_t)(d*s_period));
}