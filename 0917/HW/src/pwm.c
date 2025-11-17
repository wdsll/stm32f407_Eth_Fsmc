#include "pwm.h"
#include "main.h"
#include "math.h"
//BUS_VOL_ADJ 是 MCU 通过光耦去“修正/压低 PFC 的电压目标”的隔离调节口；MCU 输出 PWM → 
//光耦 LED 电流 → 一次侧通过 Q5 注入 PFC_FB → 目标电压被调小（大概率）。用它可以做轻载降压、软启动辅助、打嗝恢复限幅等功能。
static uint32_t s_period=0;
static bus_vol_adj_ctrl_t s_bus_adj;
static inline float clampf
(float x,float a,float b){ return x<a?a:(x>b?b:x); }


//APB1 是微控制器中用于低速外设的总线，而定时器的时钟频率可能受到总线预分频器的影响。


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
	if(pwm_hz == 0)
	{
		return;
	}
	rcu_periph_clock_enable(RCU_GPIOB);
	gpio_init(PB0_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, PB0_PIN);
	rcu_periph_clock_enable(RCU_TIMER2);
	timer_parameter_struct t;
	//timer_struct_para_init(&t);
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
	//配置定时器通道的输出模式和参数。
	timer_channel_output_mode_config(PB0_PWM_TIMER, PB0_PWM_CH, TIMER_OC_MODE_PWM0);
	//设置PWM的初始占空比（此处为0，表示初始无输出）。
	timer_channel_output_pulse_value_config(PB0_PWM_TIMER, PB0_PWM_CH, 0); 
	//启用自动重载影子寄存器，确保周期更新时无干扰。
	timer_auto_reload_shadow_enable(PB0_PWM_TIMER); 
	timer_enable(PB0_PWM_TIMER);
}
	
void pb0_pwm_set_duty(float d){
    d = clampf(d,0.0f,0.99f);
    timer_channel_output_pulse_value_config(PB0_PWM_TIMER, PB0_PWM_CH, (uint16_t)(d*s_period));
}

void bus_vol_adj_reset(void)
{
		s_bus_adj.integ = 0.0f;
		s_bus_adj.duty_cmd = clampf(s_bus_adj.neutral_duty, s_bus_adj.duty_min, s_bus_adj.duty_max);
		pb0_pwm_set_duty(s_bus_adj.duty_cmd);
}

void bus_vol_adj_init(void)
{
		s_bus_adj.target_v = VBUS_TARGET_V;
		s_bus_adj.kp = 0.0025f;
		s_bus_adj.ki = 0.0005f;
		s_bus_adj.neutral_duty = 0.5f;
		s_bus_adj.duty_min = 0.05f;
		s_bus_adj.duty_max = 0.95f;
		bus_vol_adj_reset();
}
//这段代码的主要目的是实现一个基于比例积分（PI）控制的电压调节器，用于动态调整 PWM（脉宽调制）的占空比，以维持目标总线电压（ target_v ）的稳定。
//当总线电压（ vbus ）偏离目标值时，代码通过 PI 控制算法计算新的占空比，并通过 pb0_pwm_set_duty 函数设置 PWM 输出。
void bus_vol_adj_tick(float vbus,bool enabled)
{
	if(!enabled)
	{
		s_bus_adj.integ = 0.0f;
		//neutral_duty ：中性占空比，即无误差时的默认值。
		float duty = clampf(s_bus_adj.neutral_duty,s_bus_adj.duty_min,s_bus_adj.duty_max);
		if(duty != s_bus_adj.duty_cmd)
		{
			s_bus_adj.duty_cmd = duty;
			pb0_pwm_set_duty(s_bus_adj.duty_cmd);
		}
		return;	
	}
	float error = s_bus_adj.target_v-vbus;
	float integ = s_bus_adj.integ + (s_bus_adj.ki*error);
	float duty_unclamped = s_bus_adj.neutral_duty + (s_bus_adj.kp * error) + integ;
  float duty = clampf(duty_unclamped, s_bus_adj.duty_min, s_bus_adj.duty_max);
	if(duty != duty_unclamped)	
	{
		integ = duty - s_bus_adj.neutral_duty - (s_bus_adj.kp * error);
	}
	float integ_min = s_bus_adj.duty_min - s_bus_adj.neutral_duty;
	float integ_max = s_bus_adj.duty_max - s_bus_adj.neutral_duty;
	s_bus_adj.integ = clampf(integ, integ_min, integ_max);
	
	if(duty != s_bus_adj.duty_cmd)
	{
		s_bus_adj.duty_cmd = duty;
		pb0_pwm_set_duty(s_bus_adj.duty_cmd);
	}
}


/* =========================================================================================
 *   PB0 占空比 ? 母线电压 线性模型 + 自动标定
 * ========================================================================================= */
/* 标定时读母线电压的回调函数类型：返回当前 Vbus（单位：V） */


/* 线性模型：Vbus ≈ a + b * duty */
typedef struct {
    float  a;      /* 截距：duty=0 时母线电压 */
    float  b;      /* 斜率：dVbus / dDuty （通常为负数） */
    uint8_t valid; /* 1=模型有效，0=模型未标定或失败 */
} bus_vol_adj_model_t;

static bus_vol_adj_model_t s_bus_adj_model = {0.0f, 0.0f, 0U};

/* 供其它模块判断：模型是否有效 */
bool bus_vol_adj_model_valid(void)
{
    return (s_bus_adj_model.valid != 0U);
}

/* 根据 PB0 占空比预测母线电压（需要模型有效，否则回退为 target_v） */
float bus_vol_adj_vbus_from_duty(float duty)
{
    duty = clampf(duty, 0.0f, 1.0f);
    if (!s_bus_adj_model.valid) {
        /* 未标定：返回当前目标电压当作“估计值” */
        return s_bus_adj.target_v;
    }
    return s_bus_adj_model.a + s_bus_adj_model.b * duty;
}

/* 根据期望母线电压，反推 PB0 占空比（若无模型则退回 neutral_duty） */
float bus_vol_adj_duty_from_vbus(float vbus_target)
{
    if (!s_bus_adj_model.valid || fabsf(s_bus_adj_model.b) < 1e-6f) {
        /* 未标定 / 斜率异常：复位到当前 PI 中性占空比 */
        return clampf(s_bus_adj.neutral_duty,
                      s_bus_adj.duty_min,
                      s_bus_adj.duty_max);
    }

    float duty = (vbus_target - s_bus_adj_model.a) / s_bus_adj_model.b;
    return clampf(duty, s_bus_adj.duty_min, s_bus_adj.duty_max);
}

/* 自动标定：
 *  - 只在实验/调试阶段调用一次即可（例如上电后先跑标定，再恢复正常控制逻辑）
 *  - read_vbus 回调负责返回当前母线电压（可以基于 PA3 捕获换算）
 * 过程：
 *  1) 依次输出若干个占空比点 duty_i 到 PB0
 *  2) 等待 PFC 稳定（settle_ms）
 *  3) 多次采样 Vbus 求平均
 *  4) 用最小二乘拟合 Vbus ≈ a + b*duty
 * 标定结束后：通过 bus_vol_adj_duty_from_vbus() / bus_vol_adj_vbus_from_duty()
 * 就可以在开环情况下估算 PB0 对母线目标的影响。
 */
void bus_vol_adj_run_calibration(bus_vol_adj_vbus_reader_t read_vbus)
{
    if (read_vbus == NULL) {
        s_bus_adj_model.valid = 0U;
        s_bus_adj_model.a = 0.0f;
        s_bus_adj_model.b = 0.0f;
        return;
    }

    /* 标定占空比点，可以根据需要改成 {0.1,0.5,0.9} 之类 */
    static const float duty_table[] = { 0.0f, 0.3f, 0.6f, 0.9f };
    const int NUM_POINTS = (int)(sizeof(duty_table) / sizeof(duty_table[0]));

    const uint32_t SETTLE_MS     = 500U; /* 每个点的稳定等待时间 */
    const uint32_t SAMPLE_INTERVAL_MS = 2U;
    const int      SAMPLES_PER_POINT = 32;

    float sumD  = 0.0f;
    float sumV  = 0.0f;
    float sumDD = 0.0f;
    float sumDV = 0.0f;

    for (int i = 0; i < NUM_POINTS; ++i) {
        float duty = duty_table[i];
        pb0_pwm_set_duty(duty);

        /* 等待 PFC 母线稳定到新的目标 */
        delay_ms(SETTLE_MS);

        /* 多次采样求平均，减少噪声 */
        float v_sum = 0.0f;
        for (int n = 0; n < SAMPLES_PER_POINT; ++n) {
            v_sum += read_vbus();
            delay_ms(SAMPLE_INTERVAL_MS);
        }
        float vbus = v_sum / (float)SAMPLES_PER_POINT;

        sumD  += duty;
        sumV  += vbus;
        sumDD += duty * duty;
        sumDV += duty * vbus;
    }

    float Np    = (float)NUM_POINTS;
    float denom = Np * sumDD - sumD * sumD;

    if (fabsf(denom) < 1e-6f) {
        /* 拟合失败：标记为无效 */
        s_bus_adj_model.valid = 0U;
        s_bus_adj_model.a     = 0.0f;
        s_bus_adj_model.b     = 0.0f;
    } else {
        /* 最小二乘拟合：V = a + bD */
        s_bus_adj_model.b = (Np * sumDV - sumD * sumV) / denom;
        s_bus_adj_model.a = (sumV - s_bus_adj_model.b * sumD) / Np;
        s_bus_adj_model.valid = 1U;
    }

    /* 标定完毕，恢复 BUS_VOL_ADJ 的默认状态 */
    bus_vol_adj_reset();
}
