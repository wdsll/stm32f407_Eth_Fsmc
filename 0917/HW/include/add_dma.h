
#include "gd32f30x.h"

typedef struct {
    uint16_t vout_raw;    /* PA5 */
    uint16_t isense_raw;  /* PA6 */
    uint16_t tsense_raw;  /* PA7 */
    uint16_t v3v3_raw;    /* PC4 */
    uint16_t vbt_raw;     /* PC5 */
    uint16_t t_llc_raw;   /* PB1 */
} adc_multi_frame_t;

enum { 
	ADC_MULTI_CHANNEL_COUNT = 6U 
};
static uint16_t s_buf[ADC_MULTI_CHANNEL_COUNT * 2U];
static volatile adc_multi_frame_t s_latched;

extern volatile adc_multi_frame_t g_adc_multi;

void adc_multi_init_dma(uint32_t trig_src /* e.g. ADC_EXTTRIG_REGULAR_T0_CH0 */);
void adc_multi_start(void);
void adc_multi_copy(void);
