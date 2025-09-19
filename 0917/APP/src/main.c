#include "main.h"
#include "add_dma.h"
#include "pwm_llc.h"
#include "RCU.h"
#include "ICU.h"
#include "pwm.h"
#include "protect_exti.h"

static llc_t s_llc;
volatile uint32_t g_ms=0;


void systick_1ms_init(void){
		SystemCoreClockUpdate();    
     uint32_t reload  = SystemCoreClock / 1000U;
	  if (reload == 0U || reload > SysTick_LOAD_RELOAD_Msk) {
                                           // ʧ�ܣ�Ƶ���쳣�򳬳�24λ
    }
		
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
void SysTick_Handler(void){
    g_ms++;
    /* 1 kHz control */
    adc_multi_copy();
    float vout = conv_adc_to_v_div(g_adc_multi.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    s_llc.vmeas = vout;
    llc_step(&s_llc);
    llc_pwm_set_freq((uint32_t)s_llc.f_cmd);
}

void llc_step(llc_t* l){
	/* 1) ��Ŀ���ѹ - ʵ���ѹ����λV�� */
    float e = l->vref - l->vmeas;
	/* 2) �����ۼӣ�ÿ�������������� ki*e�������Ǳ��ͣ� */
    l->integ += l->ki * e;
	/* 3) PI���ӳ�䵽Ƶ������kp*e + I���ټӻ��� f_min����λHz�� */
    float f_req = l->kp * e + l->integ + l->f_min;
	/* 4) �޷� */
    if(f_req < l->f_min)   // Ƶ������ǯλ�����ɵ��� f_min
			f_req = l->f_min;
    if(f_req > l->f_max)   // Ƶ������ǯλ�����ɸ��� f_max
			f_req = l->f_max;
		
    float df = f_req - l->f_cmd;  // ����Ƶ���뵱ǰ�·�Ƶ�ʵĲ�ֵ ��f
		
    if(f_absf(df) > l->f_slew)   // б���޷����� |��f| ����ÿ������󲽳� f_slew
			l->f_cmd += (df>0?l->f_slew:-l->f_slew); //�Ͱ���/������ֻ�ƶ� f_slew�����ٱ�Ƶ��
    else 
			l->f_cmd = f_req; // ����һ����λ��ֱ�Ӱ��·�Ƶ����ΪĿ��
}

int main(void){
	
	  InitRCU();
	//RCU_Config_72M();
    //systick_1ms_init();

    /* LLC complementary PWM ����LLC��PWMƵ�ʡ�����ʱ���ռ�ձȣ�����ʼ��PWMģ��*/
    llc_pwm_cfg_t lcfg = { .pwm_hz=LLC_PWM_BASE_HZ, .deadtime_ns=LLC_PWM_DEAD_NS, .duty=LLC_PWM_DUTY };
    llc_pwm_init(&lcfg);

    /* Aux PWM on PB0 */
      pb0_pwm_init(PB0_PWM_BASE_HZ);
      pb0_pwm_set_duty(0.5f);

    /* ADC multi (PA0/PA1 removed) triggered by TIMER0 CH0 for coherence */
    adc_multi_init_dma(ADC0_1_EXTTRIG_REGULAR_T0_CH0); 
    adc_multi_start();

    /* PA0 & PA1 input capture */
    cap_pa01_init();

    /* Protection EXTI PC11 */
    protect_exti_init();

    /* LLC control default����ʼ��LLC�Ŀ��Ʋ���������Ŀ���ѹ��PID������Ƶ�ʷ�Χ�ͳ�ʼƵ�ʡ�*/
    s_llc = (llc_t){ 
			.vref=VBUS_TARGET_V, .vmeas=0.0f, .kp=0.01f, .ki=0.0005f,
      .f_min=LLC_F_MIN_HZ, .f_max=LLC_F_MAX_HZ, .f_cmd=LLC_F_INIT_HZ, .f_slew=LLC_F_SLEW_HZ 
		};

    while(1){
				uint32_t t0, t1; //PA0 PA1�����ֵ
        if(cap_pa0_read_period(&t0)){
            (void)t0; /* TODO: convert ticks->Hz using TIMER1 clock if��Ҫ */
        }
        if(cap_pa1_read_period(&t1)){
            (void)t1;
        }
        if(protect_fault_latched()){
					//��⵽���ϣ�ͨ��PC11�жϣ�����ر�LLC��PWM������������Ҫ��һ��������ϡ�
            llc_pwm_outputs_enable(0);
            /* TODO: fault handling */
        }
        __NOP();
    }
}

