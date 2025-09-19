#include "add_dma.h"
#include "main.h"

volatile adc_multi_frame_t g_adc_multi;
//static uint16_t s_buf[6];

static void analog_pins(void){
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
    gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_init(GPIOC, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_4|GPIO_PIN_5);
}
static inline void adc_multi_store_frame(const uint16_t *src){
    s_latched.vout_raw   = src[0];
    s_latched.isense_raw = src[1];
    s_latched.tsense_raw = src[2];
    s_latched.v3v3_raw   = src[3];
    s_latched.vbt_raw    = src[4];
    s_latched.t_llc_raw  = src[5];
}
static void dma_cfg(void){
    rcu_periph_clock_enable(RCU_DMA0);
    dma_parameter_struct d = {0}; 
		dma_deinit(DMA0, DMA_CH0);  // ��λ DMA0 ��ͨ�� 0������Ĵ����ָ���Ĭ��״̬
    d.periph_addr=(uint32_t)&ADC_RDATA(ADC0);   // ���������ַ��ָ�� ADC0 �Ĺ������ݼĴ���
    d.periph_inc=DMA_PERIPH_INCREASE_DISABLE;  // ��ֹ�����ַ������ÿ�ζ���ͬһ�� ADC ���ݼĴ�����ȡ
    d.memory_addr=(uint32_t)s_buf;     // �����ڴ��ַ��ָ�����ڻ���ת�����������
    d.memory_inc=DMA_MEMORY_INCREASE_ENABLE;  // �����ڴ��ַ����������д�뻺������ÿ��Ԫ��
    d.periph_width=DMA_PERIPHERAL_WIDTH_16BIT;  // ָ���������ݿ��Ϊ 16 λ����Ӧ ADC ������
    d.memory_width=DMA_MEMORY_WIDTH_16BIT;    // ָ���ڴ�洢���Ϊ 16 λ���������ȱ���һ��
    //d.number=6;
		d.number=ADC_MULTI_CHANNEL_COUNT * 2U;  // ÿ�� DMA ѭ��Ҫ���� 6 ���������Ӧ 6 ������ͨ���Ľ��,���������˫������
    d.priority=DMA_PRIORITY_HIGH;   // ����ͨ�����ȼ���Ϊ�ߣ����ٱ����� DMA ������ռ�ĸ���
    d.direction=DMA_PERIPHERAL_TO_MEMORY;  // ���ô��䷽�򣺴����������д���ڴ�
    dma_init(DMA0, DMA_CH0, d);   // �������ϲ�����ʼ�� DMA0 ͨ�� 0
    dma_circulation_enable(DMA0, DMA_CH0);   // ʹ��ѭ��ģʽ��DMA �����һ�ִ�����Զ����¿�ʼ
    //dma_channel_enable(DMA0, DMA_CH0);
		
		dma_interrupt_enable(DMA0, DMA_CH0, DMA_INT_FTF); // ���� DMA0 ͨ��0 �ġ�ȫ������ɡ��жϣ�һ������ DMA �����������õ������ͻ�����жϣ������ڻص��д����������ݡ�
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_INT_HTF);  // ���� DMA0 ͨ��0 �ġ��봫����ɡ��жϣ��뻺����д��ʱ��ǰ��Ӧ��������˫�������ˮ����
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_G);
}

void adc_multi_init_dma(uint32_t trig_src){
    analog_pins(); 
	  dma_cfg();
		nvic_irq_enable(DMA0_Channel0_IRQn, 1U, 0U);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);
    adc_deinit(ADC0);
    adc_mode_config(ADC_MODE_FREE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, ADC_MULTI_CHANNEL_COUNT);

    adc_regular_channel_config(ADC0, 0, VOUT_SENSE_CH,     ADC_SAMPLETIME_55POINT5);
    adc_regular_channel_config(ADC0, 1, ADC_ISENSE_CH,     ADC_SAMPLETIME_55POINT5);
    adc_regular_channel_config(ADC0, 2, ADC_TSENSE_CH,     ADC_SAMPLETIME_55POINT5);
    adc_regular_channel_config(ADC0, 3, AD_3V3_CH,         ADC_SAMPLETIME_55POINT5);
    adc_regular_channel_config(ADC0, 4, VBT_SENSE_CH,      ADC_SAMPLETIME_55POINT5);
    adc_regular_channel_config(ADC0, 5, T_SENSE_LLCMOS_CH, ADC_SAMPLETIME_55POINT5);

    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, trig_src);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

    adc_enable(ADC0); adc_calibration_enable(ADC0);
    adc_dma_mode_enable(ADC0);
		dma_channel_enable(DMA0, DMA_CH0);
}

void adc_multi_start(void)
{ 
	//adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL); 
	timer_event_software_generate(LLC_PWM_TIMER, TIMER_EVENT_SRC_CH0G);
}

void adc_multi_copy(void){
	  uint32_t primask = __get_PRIMASK();
		__disable_irq();
    adc_multi_frame_t frame;
	  frame.vout_raw   = s_latched.vout_raw;
    frame.isense_raw = s_latched.isense_raw;
    frame.tsense_raw = s_latched.tsense_raw;
    frame.v3v3_raw   = s_latched.v3v3_raw;
    frame.vbt_raw    = s_latched.vbt_raw;
    frame.t_llc_raw  = s_latched.t_llc_raw;
    g_adc_multi = frame;
    __set_PRIMASK(primask);
}

void DMA0_Channel0_IRQHandler(void)
{
		if(dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_HTF)){
			dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_HTF);
			adc_multi_store_frame(&s_buf[0]);
    }
    if(dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF)){
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FTF);
        adc_multi_store_frame(&s_buf[ADC_MULTI_CHANNEL_COUNT]);
    }
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_G);
}
