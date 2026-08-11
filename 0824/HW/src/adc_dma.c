/* adc_dma.c - ADC 多通道 DMA 采样 (5 通道, ADC0 only) */
#include "adc_dma.h"

#define AC_DIV_RATIO \
    (5100.0f / (4.0f * 330000.0f + 5100.0f))
adc_multi_t g_adc_multi;

static uint16_t s_adc0_dma_buf[ADC_CHANNEL_QTY];

static float s_ac_raw_sum;
static uint64_t s_ac_raw_square_sum;
static uint32_t s_ac_sample_count;
static float s_ac_offset_raw = ADC_RESOLUTION * 0.5f;
/* ========== ADC0 多通道 DMA 初始化 ========== */
void adc_multi_init_dma(uint32_t exttrig)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_DMA0);

    /* ADC 引脚配置为模拟输入 (旧 SPL: GPIO_MODE_AIN, 无独立 PUPD 参数) */
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_MAX,
              GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_6);
    gpio_init(GPIOC, GPIO_MODE_AIN, GPIO_OSPEED_MAX, GPIO_PIN_5);
	
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8);

    adc_deinit(ADC0);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, exttrig);
	  adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* 通道排序 (rank 0..9, 与 adc_multi_t 字段顺序一致) */
    uint8_t ch_list[ADC_CHANNEL_QTY] = {
        AC_VOL_SAMPLE_CH, BUS_VOL_SAMPLE_CH, VOUT_SENSE_CH,
        I_SENSE_CH, VBT_SENSE_CH
    };
    for (uint8_t i = 0; i < ADC_CHANNEL_QTY; i++) {
        adc_regular_channel_config(ADC0, i, ch_list[i], ADC_SAMPLETIME_55POINT5);
    }
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, ADC_CHANNEL_QTY);

    /* DMA 配置 (循环模式) */
    dma_parameter_struct dma;
    dma_deinit(DMA0, DMA_CH0);
    /* 旧 SPL 无 dma_struct_para_init, dma_deinit 已复位, 下方逐字段赋值 */
    dma.periph_addr  = (uint32_t)&ADC_RDATA(ADC0);
    dma.memory_addr  = (uint32_t)s_adc0_dma_buf;
    dma.number       = ADC_CHANNEL_QTY;
    dma.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
    dma.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
    dma.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma.direction    = DMA_PERIPHERAL_TO_MEMORY;
    dma.priority     = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH0, dma);
    dma_circulation_enable(DMA0, DMA_CH0);
		dma_flag_clear(DMA0, DMA_CH0, DMA_FLAG_G);
		
    adc_enable(ADC0);
		adc_calibration_enable(ADC0);
    dma_channel_enable(DMA0, DMA_CH0); 
		adc_dma_mode_enable(ADC0);
		
		s_ac_raw_sum = 0.0f;
    s_ac_raw_square_sum = 0ULL;
    s_ac_sample_count = 0U;
		s_ac_offset_raw = ADC_RESOLUTION * 0.5f;
}

void adc_multi_start(void)
{
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}

void adc_multi_trigger_fast(void)
{
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}
/*
 * Copy only after DMA has completed all five channels.
 * Disabling CPU interrupts cannot stop DMA, so the DMA full-transfer flag is
 * the synchronization point between the peripheral and the CPU.
 */

bool adc_multi_copy_if_ready(void)
{
    if (dma_flag_get(DMA0, DMA_CH0, DMA_FLAG_FTF) == RESET) {
        return false;
    }

    g_adc_multi.ac_vol_raw  = s_adc0_dma_buf[0];
    g_adc_multi.bus_vol_raw = s_adc0_dma_buf[1];
    g_adc_multi.vout_raw    = s_adc0_dma_buf[2];
    g_adc_multi.isense_raw  = s_adc0_dma_buf[3];
    g_adc_multi.vbt_raw     = s_adc0_dma_buf[4];

    dma_flag_clear(DMA0, DMA_CH0, DMA_FLAG_FTF);
    return true;
}
void adc_multi_copy(void)
{
	(void)adc_multi_copy_if_ready();
}

/*
 * AC_VOL_SENSE is a bipolar mains waveform centered at REF_1V65.
 *
 * Sampling rate: 5 kHz
 * RMS window:    200 ms / 1000 samples
 *
 * The DC offset is removed using:
 * RMS = sqrt(E[x^2] - E[x]^2)
 */
void adc_ac_sample_fast(uint16_t raw)
{
    /* 累计原始码和原始码平方 */
    s_ac_raw_sum += (uint64_t)raw;
    s_ac_raw_square_sum +=
        (uint64_t)raw * (uint64_t)raw;
    s_ac_sample_count++;

	if (s_ac_sample_count >= AC_RMS_WINDOW_SAMPLES) {
		float count = (float)s_ac_sample_count;
		/* ADC直流中点，正常应接近2048码 */
		float mean_raw = (float)s_ac_raw_sum / count;

		float mean_square_raw = (float)s_ac_raw_square_sum / count;
/*
 * 去除REF_1V65直流偏置：
 * variance = E[x2] - E[x]2
 */
		float variance_raw = mean_square_raw - mean_raw * mean_raw;
		/*
		 * 浮点计算可能产生很小的负数，
		 * 因此开平方前进行保护。
		 */
		if (variance_raw < 0.0f) {
				variance_raw = 0.0f;
		}
		float rms_raw = sqrtf(variance_raw);
		/* ADC端交流有效值 */
		float adc_rms_v = rms_raw*VREF_ADC/(float)ADC_RESOLUTION;
		
		/* 换算到交流输入端 */
		g_adc_multi.ac_vol_v = adc_rms_v / AC_DIV_RATIO * AC_RMS_CALIBRATION;
		
		/* 保存实测直流中点，供瞬时值换算使用 */
		s_ac_offset_raw = mean_raw;
		
		/* 开始下一个RMS窗口 */
		s_ac_raw_sum = 0;
		s_ac_raw_square_sum = 0;
		s_ac_sample_count = 0;
	}
	/* 可选：保存带正负方向的瞬时交流电压 */
	g_adc_multi.ac_vol_inst_v = ((float)raw - s_ac_offset_raw)*VREF_ADC/(float)ADC_RESOLUTION/AC_DIV_RATIO;
}

void adc_multi_sample_aux_1khz(void)
{
    /* 1kHz 物理量转换 */
    g_adc_multi.bus_vol_v  = adc_raw_to_voltage(g_adc_multi.bus_vol_raw, VBUS_RTOP_OHM, VBUS_RBOT_OHM);
    //g_adc_multi.ac_vol_v   = adc_raw_to_voltage(g_adc_multi.ac_vol_raw,  AC_RTOP_OHM,   AC_RBOT_OHM);
    g_adc_multi.vout_v     = adc_raw_to_voltage(g_adc_multi.vout_raw,    VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    g_adc_multi.vbat_v     = adc_raw_to_voltage(g_adc_multi.vbt_raw,     VBT_RTOP_OHM,  VBT_RBOT_OHM);
    g_adc_multi.iout_a     = adc_raw_to_current(g_adc_multi.isense_raw);
}

/* ========== 物理量转换 ========== */
float adc_raw_to_voltage(uint16_t raw, float rtop, float rbot)
{
    float v_adc = ((float)raw * VREF_ADC) / (float)ADC_RESOLUTION;
    return v_adc * ((rtop + rbot) / rbot);
}

float adc_raw_to_current(uint16_t raw)
{
    /* NSI1312 隔离运放: V_out = I_shunt * R_shunt * Gain + V_ref
     * I = (V_adc - V_offset) / (R_shunt * Gain) */
    float v_adc = ((float)raw * VREF_ADC) / (float)ADC_RESOLUTION;
    float v_offset = VREF_ADC / 2.0f;  /* NSI1312 零电流基准 = VDD/2, 待标定 */
    float i = (v_adc - v_offset) / (ISHUNT_OHM * IAMP_GAIN);
    return (i < 0.0f) ? 0.0f : i;
}
#if 0
float adc_raw_to_fan_current(uint16_t raw)
{
    float v_adc = ((float)raw * VREF_ADC) / (float)ADC_RESOLUTION;
    float i = v_adc / (FAN_CS_SHUNT_OHM * FAN_CS_AMP_GAIN);
    return (i < 0.0f) ? 0.0f : i;
}

float ntc_raw_to_temp_c(uint16_t raw)
{
    /* NTC 分压: V_adc = VREF * R_ntc / (R_pullup + R_ntc)
     * R_ntc = R_pullup * V_adc / (VREF - V_adc)
     * T = 1 / (1/T0 + ln(R/R0)/B) - 273.15 */
    if (raw == 0U || raw >= ADC_RESOLUTION) return -273.15f;
    float v_adc = ((float)raw * VREF_ADC) / (float)ADC_RESOLUTION;
    float r_ntc = NTC_PULLUP_OHM * v_adc / (VREF_ADC - v_adc);
    if (r_ntc <= 0.0f) return -273.15f;
    float inv_t = (1.0f / NTC_T0_K) + (logf(r_ntc / NTC_R0_OHM) / NTC_BETA);
    return (1.0f / inv_t) - 273.15f;
}
#endif
