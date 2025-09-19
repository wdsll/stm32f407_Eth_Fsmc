#include "add_dma.h"
#include "main.h"

volatile adc_multi_frame_t g_adc_multi;
static uint16_t s_buf[6];
static void analog_pins(void){
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
    gpio_init(GPIOB, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_init(GPIOC, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_4|GPIO_PIN_5);
}

static void dma_cfg(void){
    rcu_periph_clock_enable(RCU_DMA0);
    dma_parameter_struct d = {0}; 
		dma_deinit(DMA0, DMA_CH0);
    d.periph_addr=(uint32_t)&ADC_RDATA(ADC0);
    d.periph_inc=DMA_PERIPH_INCREASE_DISABLE;
    d.memory_addr=(uint32_t)s_buf;
    d.memory_inc=DMA_MEMORY_INCREASE_ENABLE;
    d.periph_width=DMA_PERIPHERAL_WIDTH_16BIT;
    d.memory_width=DMA_MEMORY_WIDTH_16BIT;
    d.number=6;
    d.priority=DMA_PRIORITY_HIGH;
    d.direction=DMA_PERIPHERAL_TO_MEMORY;
    dma_init(DMA0, DMA_CH0, d);
    dma_circulation_enable(DMA0, DMA_CH0);
    dma_channel_enable(DMA0, DMA_CH0);
}

void adc_multi_init_dma(uint32_t trig_src){
    analog_pins(); dma_cfg();
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);
    adc_deinit(ADC0);
    adc_mode_config(ADC_MODE_FREE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 6);

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
}

void adc_multi_start(void)
{ 
	adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL); 
}

void adc_multi_copy(void){
    g_adc_multi.vout_raw   = s_buf[0];
    g_adc_multi.isense_raw = s_buf[1];
    g_adc_multi.tsense_raw = s_buf[2];
    g_adc_multi.v3v3_raw   = s_buf[3];
    g_adc_multi.vbt_raw    = s_buf[4];
    g_adc_multi.t_llc_raw  = s_buf[5];
}