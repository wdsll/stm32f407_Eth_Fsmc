/*
 * tcp_clinet.h
 *
 *  Created on: Aug 8, 2025
 *      Author: huyan
 */

#ifndef INC_ADC_MUX_H_
#define INC_ADC_MUX_H_

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


static const RRangeCfg g_r_ranges[]=
{
    {1,0,250.0f,"U16.S1 250Ω"},
    {2,0,2500.0f,"U16.S2 2.5kΩ"},
    {3,0,25000.0f,"U16.S3 25kΩ"},
    {4,0,250000.0f,"U16.S4 250kΩ"},
    {5,0,2500000.0f,"U16.S5 2.5MΩ"},
    {6,0,25000000.0f,"U16.S6 25MΩ"}
};
/* ---- Global calibration variables (runtime adjustable) ---- */
static float g_VRange_Scale   = 1.0f; /* V-mode DUT/ADC scale */
static float g_IPath_VperA    = 1.0f; /* current path V/A (ADC volts per 1A at amp output) */
static float g_IRange_Gain    = 1.0f; /* current range gain (updated by i_range_apply or UI) */
static float g_RV_Chain_Scale = 1.0f; /* resistance V chain recover (default = VRange_Scale) */
static float g_RI_Path_VperA  = 1.0f; /* resistance current V/A */
static float g_RI_Range_Gain  = 1.0f; /* resistance current range gain */

static uint8_t s_i_range=1;
#endif /* INC_ADC_MUX_H_ */
