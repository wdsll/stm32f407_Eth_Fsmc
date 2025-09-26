/*
 * tcp_clinet.h
 *
 *  Created on: Aug 8, 2025
 *      Author: huyan
 */

#ifndef INC_ADC_MUX_H_
#define INC_ADC_MUX_H_

#include <stddef.h>
#include <stdint.h>
#ifndef CAL_AD7190_REF_V
#define CAL_AD7190_REF_V     (2.500f)
#endif
#ifndef CAL_AD7190_GAIN_V
#define CAL_AD7190_GAIN_V    (1.0f)
#endif
#ifndef CAL_AD7190_GAIN_I
#define CAL_AD7190_GAIN_I    (1.0f)
#endif
#ifndef CAL_AD7190_GAIN_RV
#define CAL_AD7190_GAIN_RV   (1.0f)
#endif
#ifndef CAL_AD7190_GAIN_RI
#define CAL_AD7190_GAIN_RI   (1.0f)
#endif

#ifndef CAL_I_RANGE_UP_TH
#define CAL_I_RANGE_UP_TH    (0.82f)
#endif
#ifndef CAL_I_RANGE_DN_TH
#define CAL_I_RANGE_DN_TH    (0.18f)
#endif

#ifndef CAL_R_V_LOW
#define CAL_R_V_LOW          (0.10f) /* V */
#endif
#ifndef CAL_R_V_HIGH
#define CAL_R_V_HIGH         (1.80f) /* V */
#endif
#define R_RANGE_COUNT (sizeof(g_r_ranges)/sizeof(g_r_ranges[0]))
#include "main.h"
typedef struct
{
    uint8_t u16_sel;
    uint8_t u20_sel;
    float Rstd_ohm;
    const char* label;
} RRangeCfg;
static uint8_t s_i_range=1;
static uint8_t s_r_idx=2;
size_t MEAS_Range_R_Count(void);
const RRangeCfg* MEAS_Range_R_Config(size_t index);

void MEAS_Init(void);
void MEAS_SetResistanceRangeIndex(uint8_t idx);
uint8_t MEAS_GetResistanceRangeIndex(void);
const char* MEAS_Range_R_Label(void);

float MEAS_CodeToVoltage(uint32_t code, float vref, float gain);

#endif /* INC_ADC_MUX_H_ */
