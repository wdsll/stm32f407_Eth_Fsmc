/*********************************************************************************************************
* 模块名称：adc_dma.h
* 摘    要：1.5kW OBC 主头文件
* 当前版本：0.1.0 (初始版本)
* 作    者：Rengar
* 完成日期：2026年07月30日
*********************************************************************************************************/
/* adc_dma.h - ADC 多通道 DMA 采样 (10 通道) */
#ifndef _ADC_DMA_H_
#define _ADC_DMA_H_

#include "main.h"

typedef struct {
    /* 原始 ADC 值 (索引对应扫描序列) */
    uint16_t ac_vol_raw;     /* PA1  AC_VOL_SENSE */
    uint16_t fan_cs_raw;     /* PA2  FAN_CS */
    uint16_t bus_vol_raw;    /* PA3  BUS_VOL_SENSE */
    uint16_t t_case_raw;     /* PA4  T_SENSE_CASE */
    uint16_t vout_raw;       /* PA5  VOUT_SENSE */
    uint16_t isense_raw;     /* PA6  I_SENSE */
    uint16_t t_pfc_raw;      /* PA7  T_SENSE_PFC_MOS */
    uint16_t t_tr_raw;       /* PC4  T_SENSE_TR */
    uint16_t vbt_raw;        /* PC5  VBT_SENSE */
    uint16_t t_llc_raw;      /* PB1  T_SENSE_LLC_MOS */

    /* 转换后的物理量 */
    float    ac_vol_v;
    float    fan_cs_a;
    float    bus_vol_v;
    float    t_case_c;
    float    vout_v;
    float    iout_a;
    float    t_pfc_c;
    float    t_tr_c;
    float    vbat_v;
    float    t_llc_c;
} adc_multi_t;

extern adc_multi_t g_adc_multi;

void adc_multi_init_dma(uint32_t exttrig);
void adc_multi_start(void);
void adc_multi_trigger_fast(void);
void adc_multi_copy(void);
void adc_multi_sample_aux_1khz(void);

/* 物理量转换 */
float adc_raw_to_voltage(uint16_t raw, float rtop, float rbot);
float adc_raw_to_current(uint16_t raw);
float adc_raw_to_fan_current(uint16_t raw);
float ntc_raw_to_temp_c(uint16_t raw);

#endif /* _ADC_DMA_H_ */
