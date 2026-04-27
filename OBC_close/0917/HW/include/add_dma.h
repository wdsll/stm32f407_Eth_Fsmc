#ifndef ADD_DMA_H
#define ADD_DMA_H
#include "gd32f30x.h"

typedef struct {
    uint16_t vout_raw;    /* PA5 */ 
    uint16_t isense_raw;  /* PA6 */
   // uint16_t tsense_raw;  /* PA7 */
    uint16_t v3v3_raw;    /* PC4 */
    uint16_t vbt_raw;     /* PC5 */
  // uint16_t t_llc_raw;   /* PB1 */
} adc_multi_frame_t;

enum { 
	ADC_MULTI_CHANNEL_COUNT = 4U,
	ADC_TIM0_TRIGGERED_COUNT = 2U
};


extern volatile adc_multi_frame_t g_adc_multi;

/* ADC0 DMA 初始化，始终使用 TIMER0 Ch1 硬件触发（见 adc_dma.c） */
void adc_multi_init_dma(void);
void adc_multi_copy(void);
void adc_multi_start(void);
/* ADC0 软件触发（已废弃，保留兼容性） */
void adc_multi_trigger_fast(void) __attribute__((deprecated));
void adc_multi_sample_aux_1khz(void);
void adc1_aux_init(void);
uint16_t adc1_aux_read_channel(uint8_t channel, uint32_t sample_time);

/* ADC1通道14测试函数 */
uint16_t adc1_channel14_test(void);
uint16_t adc1_channel14_multiple_samples(uint16_t sample_count, uint16_t *samples);
#endif

