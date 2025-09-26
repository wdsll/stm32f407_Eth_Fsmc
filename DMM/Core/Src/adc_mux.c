/*
 * tcp_client.c
 *
 *  Created on: Aug 8, 2025
 *      Author: huyan
 */
#include "adc_mux.h"


static inline float code_to_V(uint32_t code,float vref,float gain)
{
    const float scale=vref/16777215.0f;
    return (code*scale)/(gain>0?gain:1.0f);
}
static void fun_select_v(void)
{
    MUX_FUN_Select(0);
}
static void fun_select_i(void)
{
    MUX_FUN_Select(1);
}
static void fun_select_r(void)
{
    MUX_FUN_Select(2);
}


static void i_range_apply(uint8_t r)
{
    s_i_range=r&0x7;
    MUX_I_Select(s_i_range);
}

static uint8_t s_r_idx=2;
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

const char* MEAS_Range_R_Label(void)
{
    return g_r_ranges[s_r_idx].label;
}

void MEAS_Init(void)
{
    //AD7190_Init_Default();
    //RELAY_AllOff();
    i_range_apply(1);
    r_apply_index(s_r_idx);
    g_RV_Chain_Scale = g_VRange_Scale;
}
//自动量程切换
void MEAS_V_Autorange_1kHz(void){}











