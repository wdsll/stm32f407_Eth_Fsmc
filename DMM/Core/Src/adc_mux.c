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

/* ---- 量程状态：只在本文件有唯一实例 ---- */
static uint8_t s_i_range = 1U; /* 当前电流档位索引，0~7 */
static uint8_t s_r_idx   = 2U; /* 当前电阻量程索引 */

/* 前向声明（供 MEAS_Init 使用） */
static void i_range_apply(uint8_t r);
static void r_apply_index(uint8_t idx);

static const RRangeCfg g_r_ranges[]=
{
    {1,0,250.0f,"U16.S1 250Ω"},
    {2,0,2500.0f,"U16.S2 2.5kΩ"},
    {3,0,25000.0f,"U16.S3 25kΩ"},
    {4,0,250000.0f,"U16.S4 250kΩ"},
    {5,0,2500000.0f,"U16.S5 2.5MΩ"},
    {6,0,25000000.0f,"U16.S6 25MΩ"}
};
//用于表示电压模式下的DUT（被测设备）与ADC之间的比例系数。默认值为 1.0f ，表示无缩放
static float g_VRange_Scale   = 1.0f; /* V-mode DUT/ADC scale */
//g_IPath_VperA表示电流路径的电压与电流的比例关系（单位：伏特/安培）。标记为 STATIC_UNUSED ，表示当前可能未使用，但保留以备将来扩展
static float g_IPath_VperA    STATIC_UNUSED = 1.0f; /* current path V/A (ADC volts per 1A at amp output) */
//表示电流范围的增益系数，通常由 i_range_apply 函数或用户界面（UI）更新。标记为 STATIC_UNUSED ，可能暂时未使用。
static float g_IRange_Gain    STATIC_UNUSED = 1.0f; /* current range gain (updated by i_range_apply or UI) */
//用于电阻测量时电压链路的恢复比例系数，默认值与 g_VRange_Scale 相同
static float g_RV_Chain_Scale = 1.0f; /* resistance V chain recover (default = VRange_Scale) */
//表示电阻测量时电流路径的电压与电流比例关系（单位：伏特/安培）
static float g_RI_Path_VperA  STATIC_UNUSED = 1.0f; /* resistance current V/A */
//表示电阻测量时电流范围的增益系数。标记为 STATIC_UNUSED
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
//这段代码的主要目的是根据输入的索引值 idx 配置电阻测量范围（Range），并通过多路复用器（MUX）选择对应的通道。
//它是电阻测量功能的一部分，确保在测量不同范围的电阻时，硬件能够正确切换到对应的配置。
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
#if 1
    r_apply_index(s_r_idx); /* 恢复默认电阻档 */
    i_range_apply(1);  /* 设置默认电流档 */
#endif

    g_RV_Chain_Scale = g_VRange_Scale;
}
/*自动量程切换*/
void MEAS_V_Autorange_1kHz(void);

/* MEAS_I_Autorange_1kHz: 依赖 s_i_range / i_range_apply，必须在本文件 */
void MEAS_I_Autorange_1kHz(void)
{
    DMM_Measurement_SelectCurrentPath();

    float fs_ratio = DMM_ReadCurrent_FSratio(0U);
    if ((fs_ratio > CAL_I_RANGE_UP_TH) && (s_i_range < 7U))
    {
        i_range_apply((uint8_t)(s_i_range + 1U));
    }
    else if ((fs_ratio < CAL_I_RANGE_DN_TH) && (s_i_range > 0U))
    {
        i_range_apply((uint8_t)(s_i_range - 1U));
    }
}









