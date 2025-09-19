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
		dma_deinit(DMA0, DMA_CH0);  // 复位 DMA0 的通道 0，将其寄存器恢复到默认状态
    d.periph_addr=(uint32_t)&ADC_RDATA(ADC0);   // 设置外设地址，指向 ADC0 的规则数据寄存器
    d.periph_inc=DMA_PERIPH_INCREASE_DISABLE;  // 禁止外设地址自增，每次都从同一个 ADC 数据寄存器读取
    d.memory_addr=(uint32_t)s_buf;     // 设置内存地址，指向用于缓存转换结果的数组
    d.memory_inc=DMA_MEMORY_INCREASE_ENABLE;  // 允许内存地址自增，连续写入缓冲区的每个元素
    d.periph_width=DMA_PERIPHERAL_WIDTH_16BIT;  // 指定外设数据宽度为 16 位，对应 ADC 输出宽度
    d.memory_width=DMA_MEMORY_WIDTH_16BIT;    // 指定内存存储宽度为 16 位，与外设宽度保持一致
    //d.number=6;
		d.number=ADC_MULTI_CHANNEL_COUNT * 2U;  // 每次 DMA 循环要传输 6 个数据项，对应 6 个规则通道的结果,这边配置了双缓冲区
    d.priority=DMA_PRIORITY_HIGH;   // 将该通道优先级设为高，减少被其他 DMA 任务抢占的概率
    d.direction=DMA_PERIPHERAL_TO_MEMORY;  // 设置传输方向：从外设读数据写入内存
    dma_init(DMA0, DMA_CH0, d);   // 依据以上参数初始化 DMA0 通道 0
    dma_circulation_enable(DMA0, DMA_CH0);   // 使能循环模式，DMA 在完成一轮传输后自动重新开始
    //dma_channel_enable(DMA0, DMA_CH0);
		
		dma_interrupt_enable(DMA0, DMA_CH0, DMA_INT_FTF); // 开启 DMA0 通道0 的“全传输完成”中断，一旦本轮 DMA 传完所有配置的样本就会产生中断，便于在回调中处理完整数据。
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_INT_HTF);  // 开启 DMA0 通道0 的“半传输完成”中断，半缓冲区写满时提前响应，可用于双缓冲或流水处理。
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
