#include "pwm_llc.h"

static llc_pwm_cfg_t s_cfg; 

static uint32_t s_period=0; 

//其目的是将一个浮点数限制在指定的范围内
static inline float clampf(float x,float a,float b)
{ 
	return x<a?a:(x>b?b:x); 
}

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
	
	if(clk == 0)
	{
		return 0;
	}
	double t = 1e9 / (double)clk;
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
	
	gpio_init(LLC_H_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LLC_H_PIN);
	gpio_bit_reset(LLC_H_PORT, LLC_H_PIN);
	gpio_init(LLC_L_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LLC_L_PIN);
	gpio_bit_reset(LLC_L_PORT, LLC_L_PIN);
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
    uint32_t tclk = timer0_clk_hz();                // 如果你库里没有此宏，见文末备注
	s_period = (tclk/s_cfg.pwm_hz) - 1U;     //单边（边沿对齐）计数模式
/* TIMER0 configuration */
    t.prescaler         = 0;  // 预分频器设置为0（实际分频系数为0+1=1）
		t.alignedmode       = TIMER_COUNTER_EDGE;   //边缘对齐模式
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
		//死区时间与保护配置
    timer_break_parameter_struct bk;
    bk.runoffstate     = TIMER_ROS_STATE_ENABLE ; //设置定时器在运行状态下的断路行为为禁用。这意味着在正常运行期间，断路功能不会触发。
    bk.ideloffstate    = TIMER_IOS_STATE_ENABLE ; //设置定时器在空闲状态下的断路行为为禁用。与 runoffstate 类似，但在空闲状态下不启用断路功能。
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
		// 配置主输出触发源为通道0的比较输出,通过配置触发源，TIMER_TRI_OUT_SRC_O2CPRE
		
		//定时器可以在特定事件（如比较匹配）时生成触发信号，用于同步其他硬件模块（如ADC或其他定时器）。
		timer_master_output_trigger_source_select(TIMER0, TIMER_TRI_OUT_SRC_O2CPRE); //在通道 0 中发生了一次捕获或比较匹配事件，触发输出为 TRGO 。
		//启用定时器的自动重载影子寄存器功能。定时器的重载值会在下一个更新事件时生效，确保配置的平滑切换。
    timer_auto_reload_shadow_enable(TIMER0);
		//配置定时器的主输出功能。启用后，定时器可以输出信号到指定的引脚或模块
    //timer_primary_output_config(TIMER0, DISABLE);	
    timer_enable(TIMER0);
	llc_pwm_outputs_enable(0);  
}

/* 	频率在线更新：同时更新 ARR 和 CCR，保持占空比 ,ARR（Auto-Reload Register，自动重装载寄存器）
		CCR（Capture/Compare Register，捕获/比较寄存器）*/
//该函数的作用是将一个浮点数表示的占空比（ d ）转换为一个16位无符号整数（ uint16_t ），用于配置PWM（脉宽调制）的CCR（捕获/比较寄存器）值。
static inline uint16_t duty_to_ccr(float d)
{ 
	float duty = clampf(d,0.0f,0.99f); 
	uint32_t arr = TIMER_CAR(TIMER0);
	if(arr == 0)
	{
		return (duty > 0.0f) ? 1U : 0U;
	}
	uint32_t period_ticks = arr + 1U;
	uint32_t ccr = (uint32_t)((duty * (float)period_ticks) + 0.5f);
	if (ccr >= period_ticks) {
		ccr = period_ticks - 1U;
	}
	if (ccr > 0xFFFFU) {
		ccr = 0xFFFFU;
	}
	return ccr; 
}

void llc_pwm_set_duty(float d)
{ 
	float duty = clampf(d, 0.0f, 0.99f);
	s_cfg.duty = duty;
	uint16_t pwm_ccr = duty_to_ccr(duty);
	timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, pwm_ccr); 
}

/* 强制单路导通（用于自举预充）
high_side_on = true : 强制 CH0=1,  CH0N=0 => PA8(LLC_H)导通（高侧）
high_side_on = false: 强制 CH0=0,  CH0N=1 => PB13(LLC_L)导通（低侧）【自举预充推荐】
注意：在互补输出模式下，CH0N 会跟随 OCREF 取反，并插入死区（由 BDTR 配置）。
*/

static void llc_pwm_force_start(bool high_side_on)
{
	// 1. 禁用主输出 - 安全措施，防止意外输出
    timer_primary_output_config(TIMER0, DISABLE);
	// 2. 清零PWM脉冲值 - 确保输出为固定电平
    timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, 0U);
	// 3. 配置输出模式为固定电平模式
	timer_channel_output_mode_config(TIMER0, LLC_PWM_CH,high_side_on ? TIMER_OC_MODE_ACTIVE : TIMER_OC_MODE_INACTIVE);

    // 4. 强制生成更新事件，确保配置生效
    timer_flag_clear(TIMER0, TIMER_FLAG_UP);
    timer_event_software_generate(TIMER0, TIMER_EVENT_SRC_UPG);
    while (RESET == timer_flag_get(TIMER0, TIMER_FLAG_UP)) {}
	// 5. 等待更新事件完成
    timer_flag_clear(TIMER0, TIMER_FLAG_UP);

    TIMER_CNT(TIMER0) = 0U;
    timer_primary_output_config(TIMER0, ENABLE);
}

static void llc_pwm_restore_pwm(void)
{
    timer_primary_output_config(TIMER0, DISABLE);
	//恢复PWM模式配置
    timer_channel_output_mode_config(TIMER0, LLC_PWM_CH, TIMER_OC_MODE_PWM0);

    uint16_t pwm_ccr = duty_to_ccr(s_cfg.duty);
    timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, pwm_ccr);

    timer_flag_clear(TIMER0, TIMER_FLAG_UP);
    timer_event_software_generate(TIMER0, TIMER_EVENT_SRC_UPG);
    while (RESET == timer_flag_get(TIMER0, TIMER_FLAG_UP)) {}
    timer_flag_clear(TIMER0, TIMER_FLAG_UP);
    //确保PWM周期从0开始，保持波形完整性
    TIMER_CNT(TIMER0) = 0U;
    timer_primary_output_config(TIMER0, ENABLE);
}

void llc_pwm_outputs_enable(bool en)
{ 
     if (en) {
		//原理上只要这两行 等实际测看下
        llc_pwm_force_start(1);
		llc_pwm_restore_pwm();

		//先留着
		timer_primary_output_config(TIMER0, DISABLE);
		timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, 0);
		TIMER_CNT(TIMER0) = 0U;

		timer_primary_output_config(TIMER0, ENABLE);

		uint16_t pwm_ccr = duty_to_ccr(s_cfg.duty);
		timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, pwm_ccr);
	} else {
		timer_primary_output_config(TIMER0, DISABLE);
		timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, 0U);
	}
	
}

void llc_pwm_break(bool en){
    timer_primary_output_config(TIMER0, en ? DISABLE : ENABLE);  // en=1 相当于“刹停”
}
void llc_pwm_set_freq(uint32_t f_hz)
{ 
	if (f_hz == 0U) {
		return;
	}
	uint32_t tclk = timer0_clk_hz();
	if (tclk == 0U) {
		return;
	}
	uint32_t ticks = tclk / f_hz;
	if (ticks < 2U) {
		ticks = 2U;
	}
	if (ticks > 0x10000U) {
		ticks = 0x10000U;
	}
	s_period = ticks - 1U;
	TIMER_CAR(TIMER0) = s_period;
	s_cfg.pwm_hz = tclk / ticks;
	uint32_t deadtime = bdtr_deadtime_code_ns(s_cfg.deadtime_ns, tclk);
	// 读取定时器TIMER0的死区时间配置寄存器（TIMER_CCHP）
	uint32_t cchp = TIMER_CCHP(TIMER0); 
	// 清除原有的死区时间配置位
	cchp &= ~TIMER_CCHP_DTCFG;
	// 设置新的死区时间配置位
	cchp |= deadtime;
	// 将更新后的配置写回死区时间寄存器
	TIMER_CCHP(TIMER0) = cchp;
	// 根据占空比（s_cfg.duty）计算PWM的比较寄存器值（pwm_ccr）
	uint16_t pwm_ccr = duty_to_ccr(s_cfg.duty);
	// // 配置定时器TIMER0的通道（LLC_PWM_CH）的输出脉冲值
	timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, pwm_ccr);	
	timer_event_software_generate(TIMER0, TIMER_EVENT_SRC_UPG);

}
//周期(ns) = 1e9 × (ARR + 1) / tclk
uint32_t llc_pwm_get_period_ns(void)
{
	/* 获取定时器实际时钟频率 (Hz)，内部已考虑预分频与倍频 */
    uint32_t tclk = timer0_clk_hz();                 // TIM 内部时钟 Hz
    uint32_t arr  = TIMER_CAR(TIMER0);               // 当前 ARR
	
    // 你的 set_freq 用的是边沿计数：f = tclk/(ARR+1)
    // 周期(ns) = 1e9 * (ARR+1) / tclk
    if (tclk == 0U) 
			return 0U;
    uint64_t ns = (1000000000ULL * (uint64_t)(arr + 1U)) / (uint64_t)tclk;
		    if (ns > 0xFFFFFFFFULL)
        ns = 0xFFFFFFFFULL;
    return (uint32_t)ns;
}

uint32_t llc_pwm_get_deadtime_ns(void)
{
    return s_cfg.deadtime_ns; // 直接回传你配置进来的死区（ns）
}
