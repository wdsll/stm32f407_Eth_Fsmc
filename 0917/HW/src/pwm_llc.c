
#include "pwm_llc.h"
#include "main.h"

//GPIO：高臂、低臂、N 输出都用 AF_PP；BKIN=PB12 用 上拉输入。
//死区：确认 bdtr_deadtime_code_ns(350ns, 108MHz) 得到约 0x26（≈352ns）。
//BKIN：短接 PB12→GND，应当瞬时关断（MOE 清）；松开在一个更新周期后自动恢复（因 outputautostate=ENABLE）。
//变频：调用 llc_pwm_set_freq() 后，示波器看到占空不漂；若固定 50%，把 llc_pwm_set_freq() 里直接设 CCR = ARR/2。
//#include "gd32f30x_timer.h"
static llc_pwm_cfg_t s_cfg; 
static uint32_t s_period=0; 
//其目的是将一个浮点数限制在指定的范围内
static inline float clampf(float x,float a,float b)
{ 
	return x<a?a:(x>b?b:x); 
}
//static uint8_t dt_ticks(uint32_t ns, uint32_t clk)
//{ 
	//unsigned long long t=(unsigned long long)ns*clk/1000000000ULL; 
	//return (t>255)?255:(uint8_t)t;
//}
/*********************************************************************************************************
* 函数名称：bdtr_deadtime_code_ns
* 函数功能：用于配置硬件中的死区时间寄存器（BDTR）
* 输入参数：dead_ns:死区时间，单位为纳秒
						clk: 时钟频率，单位为赫兹
* 输出参数：
* 返 回 值： 返回一个 8 位的无符号整数
* 创建日期：2025年10月08日
* 注    意：尝试不同的分频因子（1x、2x、8x、16x），找到合适的计数值范围。
根据不同的分频因子，调整计数值并映射到特定的寄存器编码范围。
*********************************************************************************************************/
static uint8_t bdtr_deadtime_code_ns(uint32_t dead_ns, uint32_t clk) 
{
	double t = 1e9 / (double)clk;

	if(clk == 0)
	{
		return 0;
	}
	uint32_t c = (uint32_t)(dead_ns / t + 0.5);
	if (c <= 127U) 
	{
		return (uint8_t)c;
	}
	uint32_t c2 = (uint32_t)(dead_ns/(2.0*t)+0.5); 
	if (c2>=64U && c2<=127U) 
	{
		return 128U+(uint8_t)(c2-64U);
	}
	uint32_t c8 = (uint32_t)(dead_ns/(8.0*t)+0.5); 
	if (c8>=32U && c8<=63U)  
	{
		return 192U+(uint8_t)(c8-32U);
	}
	uint32_t c16= (uint32_t)(dead_ns/(16.0*t)+0.5);
	if (c16>=32U && c16<=63U) 
	{
		return 224U+(uint8_t)(c16-32U); 
	}
	return 0;
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
/*********************************************************************************************************
* 函数名称：llc_pwm_init
* 函数功能：该函数用于配置和初始化LLC PWM模块，包括定时器、输出比较通道、Break/Deadtime结构体等。
目的是初始化一个用于 LLC 谐振变换器的 PWM（脉宽调制）控制器。
* 输入参数：cfg
* 输出参数：void
* 返 回 值：void
* 创建日期：2025/09/29
* 注    意：调用此函数前，需确保相关外设时钟已使能。 初始化完成后，PWM模块会自动启动。
*********************************************************************************************************/
//初始化定时器基本参数
void llc_pwm_init(const llc_pwm_cfg_t* cfg){
    s_cfg=*cfg;
    pins_init();
	/* 启用TIMER0的时钟 */
    rcu_periph_clock_enable(RCU_TIMER0);
		/* 定义TIMER0的初始化参数结构体 */
    timer_parameter_struct t;
   // timer_struct_para_init(&t);
	
    uint32_t tclk = timer0_clk_hz();                // 如果你库里没有此宏，见文末备注
    s_period = (tclk/(2U*s_cfg.pwm_hz)) - 1U;      /* 中心对齐频率公式 */ //s_period 存储计算出的 PWM 周期值。
/* TIMER0 configuration */
    t.prescaler         = 0;  // 预分频器设置为0（实际分频系数为0+1=1）
    t.alignedmode       = TIMER_COUNTER_CENTER_BOTH; //居中对齐且向上/向下计数的断言模式
    t.counterdirection  = TIMER_COUNTER_UP; // 向上计数
    t.period            = s_period;   // 自动重载值  1
    t.clockdivision     = TIMER_CKDIV_DIV1; // 时钟分频因子为1
    t.repetitioncounter = 0; // 重复计数器值为0
    timer_init(TIMER0, &t);

    /* === 手动初始化 OC 结构体（无 para_init 版本）  OC 配置：一个通道 + 互补（半桥）  === */
    timer_oc_parameter_struct oc;
    oc.outputstate   = TIMER_CCX_ENABLE;  // 使能主输出通道
    oc.outputnstate  = TIMER_CCXN_ENABLE;
    oc.ocpolarity    = TIMER_OC_POLARITY_HIGH; // 通道0输出极性为高电平
    oc.ocnpolarity   = TIMER_OCN_POLARITY_HIGH;  // 通道0N输出极性为高电平
    oc.ocidlestate   = TIMER_OC_IDLE_STATE_LOW;   //空闲状态下主输出为低电平
    oc.ocnidlestate  = TIMER_OCN_IDLE_STATE_LOW;
		//oc.ocnidlestate  = TIMER_OCN_IDLE_STATE_LOW;
		// 应用配置到TIMER0的通道0,配置定时器通道的输出参数
    timer_channel_output_config(TIMER0, LLC_PWM_CH, &oc);
		// 设置TIMER0的LLC_PWM_CH通道的输出模式为标准PWM模式（TIMER_OC_MODE_PWM0)
    timer_channel_output_mode_config(TIMER0, LLC_PWM_CH, TIMER_OC_MODE_PWM0); /*TIMER_OC_MODE_PWM0 !< 通道互补输出状态 */
		
		// 配置TIMER0的LLC_PWM_CH通道的初始脉冲值（占空比）
		// 参数0表示初始占空比为0%（输出低电平），常用于软启动或安全初始化
    timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, 0);
		// 启用TIMER0的LLC_PWM_CH通道的影子寄存器（TIMER_OC_SHADOW_ENABLE）
		// 作用：确保配置在下一次更新事件时生效，避免运行时配置冲突
		// 关键点：影子寄存器用于保证配置的原子性和实时性
    timer_channel_output_shadow_config(TIMER0, LLC_PWM_CH, TIMER_OC_SHADOW_ENABLE); 

    /* === 手动初始化 Break/Deadtime 结构体（无 para_init 版本） === */
		//死区时间与保护配置
    timer_break_parameter_struct bk;
    bk.runoffstate     = TIMER_ROS_STATE_DISABLE ; //设置定时器在运行状态下的断路行为为禁用。这意味着在正常运行期间，断路功能不会触发。
    bk.ideloffstate    = TIMER_IOS_STATE_DISABLE ; //设置定时器在空闲状态下的断路行为为禁用。与 runoffstate 类似，但在空闲状态下不启用断路功能。
    bk.protectmode     = TIMER_CCHP_PROT_OFF; //设置保护模式为关闭。这表示定时器不会启用额外的保护机制。
		//计算并设置死区时间（Dead Time）。死区时间是PWM信号中高电平和低电平之间的间隔，用于防止上下桥臂同时导通导致的短路。
    bk.deadtime        = bdtr_deadtime_code_ns(s_cfg.deadtime_ns, tclk);
		//启用断路功能。当检测到异常信号（如BKIN引脚的低电平）时，定时器会进入断路状态。
    bk.breakstate      = TIMER_BREAK_ENABLE;
		//设置断路信号的极性为低电平有效。这意味着当BKIN引脚为低电平时，会触发断路。
    bk.breakpolarity   = TIMER_BREAK_POLARITY_LOW;     // BKIN 低有效
		//启用输出自动状态。在断路触发时，定时器的输出会自动切换到预定义的安全状态（通常是关闭输出）。
    bk.outputautostate = TIMER_OUTAUTO_ENABLE;				

    timer_break_config(TIMER0, &bk);
		// 配置主输出触发源为通道0的比较输出,通过配置触发源，
		//定时器可以在特定事件（如比较匹配）时生成触发信号，用于同步其他硬件模块（如ADC或其他定时器）。
		timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_CC0); //在通道 0 中发生了一次捕获或比较匹配事件，触发输出为 TRGO 。
		//启用定时器的自动重载影子寄存器功能。定时器的重载值会在下一个更新事件时生效，确保配置的平滑切换。
    timer_auto_reload_shadow_enable(TIMER0);
		//配置定时器的主输出功能。启用后，定时器可以输出信号到指定的引脚或模块
    timer_primary_output_config(TIMER0, ENABLE);
    timer_enable(TIMER0);

    llc_pwm_set_duty(s_cfg.duty); /* 如果你走 50% 固定，这里直接设 0.5f 即可 */
}

/* 	频率在线更新：同时更新 ARR 和 CCR，保持占空比 ,ARR（Auto-Reload Register，自动重装载寄存器）
		CCR（Capture/Compare Register，捕获/比较寄存器）*/
//该函数的作用是将一个浮点数表示的占空比（ d ）转换为一个16位无符号整数（ uint16_t ），用于配置PWM（脉宽调制）的CCR（捕获/比较寄存器）值。
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
