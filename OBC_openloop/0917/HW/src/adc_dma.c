#include "add_dma.h"
#include "main.h"
#include "systick.h"
#include "Debug_printf.h"
volatile adc_multi_frame_t g_adc_multi;

// ADC状态标志
static uint8_t adc0_initialized = 0;
static uint8_t adc1_initialized = 0;

/*********************************************************************************************************
* ADC0相关函数 - DMA/定时器触发模式（高速采集）
*********************************************************************************************************/

/* ADC0模拟引脚初始化 */
static void adc0_analog_pins_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    
    /* ADC0通道配置 */
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_5|GPIO_PIN_6);

}

/* ADC0 DMA配置 */
static void adc0_dma_cfg(void)
{
    rcu_periph_clock_enable(RCU_DMA0);
    dma_parameter_struct d = {0}; 
		dma_deinit(DMA0, DMA_CH0);
    d.periph_addr = (uint32_t)&ADC_RDATA(ADC0);
    d.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    d.memory_addr = (uint32_t)s_buf;
    d.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    d.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    d.memory_width = DMA_MEMORY_WIDTH_16BIT;
    d.number = ADC_TIM0_TRIGGERED_COUNT * 2U;
    d.priority = DMA_PRIORITY_HIGH;
		d.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init(DMA0, DMA_CH0, d);
    dma_circulation_enable(DMA0, DMA_CH0);
		
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_INT_FTF);
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_INT_HTF);
	
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_G);
}

/* ADC0初始化 - DMA/定时器触发模式 */
void adc0_dma_init(uint32_t trig_src)
{
    if(adc0_initialized) {
        return; // 避免重复初始化
    }
    
    adc0_analog_pins_init();
    adc0_dma_cfg();
    
    // 设置DMA中断优先级
    nvic_irq_enable(DMA0_Channel0_IRQn, 1U, 0U);
    
    // ADC0时钟和配置
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6); // 10MHz安全时钟
    adc_deinit(ADC0);
    adc_mode_config(ADC_MODE_FREE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, ADC_TIM0_TRIGGERED_COUNT);
    adc_regular_channel_config(ADC0, 0, VOUT_SENSE_CH, ADC_SAMPLETIME_41POINT5);
    adc_regular_channel_config(ADC0, 1, ADC_ISENSE_CH, ADC_SAMPLETIME_7POINT5);

    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, trig_src);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

    adc_enable(ADC0);
	  adc_calibration_enable(ADC0);
    adc_dma_mode_enable(ADC0);
	  dma_channel_enable(DMA0, DMA_CH0);
	
	adc0_initialized = 1;
}
/* ADC0启动DMA采集 */
void adc0_dma_start(void)
{
    if(!adc0_initialized) {
        return;
    }
    // 定时器触发ADC0采集
    timer_event_software_generate(LLC_PWM_TIMER, TIMER_EVENT_SRC_CH2G);
}

/* ADC0状态检查 */
uint8_t adc0_is_initialized(void)
{
    return adc0_initialized;
}

/*********************************************************************************************************
* ADC1相关函数 - 软件触发模式（低速采集）
*********************************************************************************************************/
/* ADC1模拟引脚初始化 */
static void adc1_analog_pins_init(void)
{   
		rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);
    /* ADC1通道配置 */
    gpio_init(GPIOC, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_4|GPIO_PIN_5);
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_3);
}

/* ADC1初始化 - 软件触发模式 */
void adc1_aux_init(void)
{
    if(adc1_initialized) {
        return; // 避免重复初始化
    }
    adc1_analog_pins_init();  
    rcu_periph_clock_enable(RCU_ADC1);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6); 
    adc_deinit(ADC1);
    adc_mode_config(ADC_MODE_FREE);
    adc_data_alignment_config(ADC1, ADC_DATAALIGN_RIGHT);
    adc_special_function_config(ADC1, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, DISABLE);
    adc_channel_length_config(ADC1, ADC_REGULAR_CHANNEL, 1U);
    adc_external_trigger_source_config(ADC1, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC1, ADC_REGULAR_CHANNEL, ENABLE);  // 关键修复：软件触发需要使能外部触发
    
    /* 关键修复：先使能ADC1，然后等待稳定，再进行校准 */ 
    /* 使能ADC1并检查状态 */
    adc_enable(ADC1);
    
    // 等待ADC1就绪（检查ADCON位）
    #define ADC_ENABLE_TIMEOUT 1000U
    uint32_t adc_enable_timeout = ADC_ENABLE_TIMEOUT;
    while(((ADC_CTL1(ADC1) & ADC_CTL1_ADCON) == 0) && (adc_enable_timeout > 0U)) {
        adc_enable_timeout--;
    }
    if(adc_enable_timeout == 0U) {
        return;
    }
    delay_ms(1);   
    /* 执行ADC1校准并检查状态 */
    adc_calibration_enable(ADC1);  
    // 等待校准完成（检查RSTCLB位清零）
    #define ADC_CALIBRATION_TIMEOUT 50000U  // 增加超时时间
    uint32_t cal_timeout = ADC_CALIBRATION_TIMEOUT;
    while(((ADC_CTL1(ADC1) & ADC_CTL1_RSTCLB) != 0) && (cal_timeout > 0U)) {
        cal_timeout--;
    }
    if(cal_timeout == 0U) {
        // 强制清除校准状态
        ADC_CTL1(ADC1) &= ~ADC_CTL1_RSTCLB;
    }   
    adc1_initialized = 1;
 }
 

/* ADC1单通道读取 */
uint16_t adc1_aux_read_channel(uint8_t channel, uint32_t sample_time)
{
    if(!adc1_initialized) {
        return 0xFFFF;
    }
    
    // 检查ADC1状态寄存器
    uint32_t ctl0 = ADC_CTL0(ADC1);
    uint32_t ctl1 = ADC_CTL1(ADC1);
    uint32_t stat = ADC_STAT(ADC1);
    
    adc_regular_channel_config(ADC1, 0U, channel, sample_time);
    adc_flag_clear(ADC1, ADC_FLAG_EOC);
    
    // 关键修复：检查并清除可能存在的EOC标志
    if(adc_flag_get(ADC1, ADC_FLAG_EOC)) {
        adc_flag_clear(ADC1, ADC_FLAG_EOC);
    }
    // 软件触发转换
    adc_software_trigger_enable(ADC1, ADC_REGULAR_CHANNEL);
    
    // 关键修复：检查SWRCST位是否被正确设置
    if((ctl1 & ADC_CTL1_SWRCST) == 0) {
        // 重新尝试触发
        adc_software_trigger_enable(ADC1, ADC_REGULAR_CHANNEL);
        ctl1 = ADC_CTL1(ADC1);
    }
    
    // 添加超时保护
    #define ADC_CONVERSION_TIMEOUT 100000U
    uint32_t timeout = ADC_CONVERSION_TIMEOUT;
    
    while((RESET == adc_flag_get(ADC1, ADC_FLAG_EOC)) && (timeout > 0U)){
        timeout--;
    }
    if(timeout == 0U) {
        return 0xFFFF;
    }
    uint16_t value = adc_regular_data_read(ADC1);
    adc_flag_clear(ADC1, ADC_FLAG_EOC); 
    return value;
}

/* ADC1定期采样函数 */
void adc1_sample_aux_1khz(void)
{
    if(!adc1_initialized) {
        return;
    }
    uint16_t v3v3 = adc1_aux_read_channel(AD_3V3_CH, ADC_SAMPLETIME_55POINT5);
    uint16_t vbt = adc1_aux_read_channel(VBT_SENSE_CH, ADC_SAMPLETIME_55POINT5);
		//uint16_t tpfc = adc1_aux_read_channel(T_SENSE_PFC_MOS, ADC_SAMPLETIME_55POINT5);
    s_latched.v3v3_raw = v3v3;
    s_latched.vbt_raw = vbt;
		//s_latched.tsense_raw = tpfc;
}

/* ADC1状态检查 */
uint8_t adc1_is_initialized(void)
{
    return adc1_initialized;
}

/*********************************************************************************************************
* 数据帧处理函数
*********************************************************************************************************/

static inline void adc_multi_store_frame(const uint16_t *src)
{
    s_latched.vout_raw   = src[0];
    s_latched.isense_raw = src[1];
/* 低速通道在 adc1_sample_aux_1khz() 中采样，这里只搬运定时器触发的高速结果 */
}

void adc_multi_copy(void)
{
    adc_multi_frame_t frame;
    frame.vout_raw   = s_latched.vout_raw;
    frame.isense_raw = s_latched.isense_raw;
	  frame.v3v3_raw   = s_latched.v3v3_raw;
    frame.vbt_raw    = s_latched.vbt_raw;
   // frame.tsense_raw = s_latched.tsense_raw;
    g_adc_multi = frame;

}

/*********************************************************************************************************
* 兼容性函数（保持原有接口）
*********************************************************************************************************/

void adc_multi_init_dma(uint32_t trig_src)
{
    adc0_dma_init(trig_src);
}

void adc_multi_start(void)
{
    adc0_dma_start();
    adc1_aux_init();
}

void adc_multi_sample_aux_1khz(void)
{
    adc1_sample_aux_1khz();
}

/*********************************************************************************************************
* ADC1通道14测试函数
*********************************************************************************************************/

uint16_t adc1_channel14_test(void)
{
    debug_printf("[ADC1-14] Starting channel 14 test\n");
    
    if(!adc1_initialized) {
        debug_printf("[ADC1-14] ADC1 not initialized, calling init...\n");
        adc1_aux_init();
    }
    
    debug_printf("[ADC1-14] Reading channel 14 (AD_3V3_CH = %d)\n", AD_3V3_CH);
    uint16_t adc_value = adc1_aux_read_channel(AD_3V3_CH, ADC_SAMPLETIME_55POINT5);
    
    if(adc_value == 0xFFFF) {
        debug_printf("[ADC1-14] ERROR: Channel 14 read failed\n");
    } else {
        debug_printf("[ADC1-14] SUCCESS: Channel 14 value = %d (0x%04X)\n", adc_value, adc_value);
    }
    
    return adc_value;
}

uint16_t adc1_channel14_multiple_samples(uint16_t sample_count, uint16_t *samples)
{
    if(samples == NULL || sample_count == 0) {
        return 0;
    }
    
    if(!adc1_initialized) {
        adc1_aux_init();
    }
    
    uint16_t successful_samples = 0;
    
    for(uint16_t i = 0; i < sample_count; i++) {
        uint16_t adc_value = adc1_aux_read_channel(AD_3V3_CH, ADC_SAMPLETIME_55POINT5);
        
        if(adc_value != 0xFFFF) {
            samples[successful_samples] = adc_value;
            successful_samples++;
        }
    }
    
    return successful_samples;
}

/*********************************************************************************************************
* DMA中断处理函数
*********************************************************************************************************/

void DMA0_Channel0_IRQHandler(void)
{
    if(dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_HTF)){
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_HTF);
        adc_multi_store_frame(&s_buf[0]);
    }
    if(dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF)){
		dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FTF);
        adc_multi_store_frame(&s_buf[ADC_TIM0_TRIGGERED_COUNT]);
    }
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_G);
}