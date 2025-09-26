/*
 * tcp_client.c
 *
 *  Created on: Aug 8, 2025
 *      Author: huyan
 */
#include "adc_mux.h"
#include "mux_hw.h"

#define R_RANGE_COUNT (sizeof(g_r_ranges)/sizeof(g_r_ranges[0]))

#if defined(__GNUC__)
#define STATIC_UNUSED __attribute__((unused))
#else
#define STATIC_UNUSED
#endif

static const RRangeCfg g_r_ranges[]=
{
    {1,0,250.0f,"U16.S1 250Ω"},
    {2,0,2500.0f,"U16.S2 2.5kΩ"},
    {3,0,25000.0f,"U16.S3 25kΩ"},
    {4,0,250000.0f,"U16.S4 250kΩ"},
    {5,0,2500000.0f,"U16.S5 2.5MΩ"},
    {6,0,25000000.0f,"U16.S6 25MΩ"}
};

static float g_VRange_Scale   = 1.0f; /* V-mode DUT/ADC scale */
static float g_IPath_VperA    STATIC_UNUSED = 1.0f; /* current path V/A (ADC volts per 1A at amp output) */
static float g_IRange_Gain    STATIC_UNUSED = 1.0f; /* current range gain (updated by i_range_apply or UI) */
static float g_RV_Chain_Scale = 1.0f; /* resistance V chain recover (default = VRange_Scale) */
static float g_RI_Path_VperA  STATIC_UNUSED = 1.0f; /* resistance current V/A */
static float g_RI_Range_Gain  STATIC_UNUSED = 1.0f; /* resistance current range gain */

static inline float code_to_V(uint32_t code,float vref,float gain)
{
    const float scale=vref/16777215.0f;
    return (code*scale)/(gain>0?gain:1.0f);
}


static void i_range_apply(uint8_t r)
{
    s_i_range=r&0x7;
    MUX_I_Select(s_i_range);
}

static void r_apply_index(uint8_t idx)
{
    if(idx>=R_RANGE_COUNT)
    	idx=R_RANGE_COUNT-1;
    s_r_idx=idx;
    //const RangeConfig *current_range = &g_r_ranges[idx];
    uint8_t u16=(g_r_ranges[idx].u16_sel-1)&0x7;
    uint8_t u20=(g_r_ranges[idx].u20_sel)&0x7;
    uint8_t addr=(u16)|(u20<<3);
    MUX_R_Select(addr);
}

size_t MEAS_Range_R_Count(void)
{
    return R_RANGE_COUNT;
}

const RRangeCfg* MEAS_Range_R_Config(size_t index)
{
    if(index>=R_RANGE_COUNT)
    {
        return NULL;
    }
    return &g_r_ranges[index];
}

uint8_t MEAS_GetResistanceRangeIndex(void)
{
    return s_r_idx;
}

float MEAS_CodeToVoltage(uint32_t code, float vref, float gain)
{
    return code_to_V(code, vref, gain);
}

void MEAS_SetResistanceRangeIndex(uint8_t idx)
{
    r_apply_index(idx);
}

const char* MEAS_Range_R_Label(void)
{
    return g_r_ranges[s_r_idx].label;
}

void MEAS_Init(void)
{
    //AD7190_Init_Default();z
    //RELAY_AllOff();
    i_range_apply(1);
    r_apply_index(s_r_idx);
    g_RV_Chain_Scale = g_VRange_Scale;
}
//自动量程切换
void MEAS_V_Autorange_1kHz(void){}









