/*
 * dmm_measuremnet.c
 *
 *  Created on: Sep 27, 2025
 *      Author: huyan
 */
#include "dmm_measurement.h"



#define AD7190_CHANNEL_COUNT 4U
#define AD7190_FILTER_DEPTH 8U
#define OPA_RES_R1              6800
#define OPA_RES_R2              2000
#define REFERENCE_VOLTAGE       3297
#define BIAS_VOLTAGE_IN1        0xFAB3E
#define BIAS_VOLTAGE_IN2        0xF9DCA
#define BIAS_VOLTAGE_IN3        0xFA8A4
#define BIAS_VOLTAGE_IN4        0xFA9EB
#define ADC20_FULL_SCALE        0xFFFFF

static __IO int32_t s_ad7190_data[AD7190_CHANNEL_COUNT];
static __IO int32_t s_bias_data[AD7190_CHANNEL_COUNT];

static int32_t s_filter_buf[AD7190_CHANNEL_COUNT][AD7190_FILTER_DEPTH];
static uint8_t s_filter_index[AD7190_CHANNEL_COUNT];

static volatile uint8_t s_samplingState = 0U;
static volatile uint8_t s_errorCounter = 0U;
static volatile int8_t s_lastChannel = -1;

static uint8_t s_i_ad_ch = 0U;
static uint8_t s_r_v_adch = 0U;
static uint8_t s_r_i_adch = 1U;

static float s_lastVoltage_V = 0.0f;


void DMM_Measurement_Init(void)
{
    memset((void *)s_ad7190_data, 0, sizeof(s_ad7190_data));
    memset((void *)s_bias_data, 0, sizeof(s_bias_data));
    memset(s_filter_buf, 0, sizeof(s_filter_buf));
    memset(s_filter_index, 0, sizeof(s_filter_index));

    s_samplingState = 0U;
    s_errorCounter = 0U;
    s_lastChannel = -1;
    s_i_ad_ch = 0U;
    s_r_v_adch = 0U;
    s_r_i_adch = 1U;
    s_lastVoltage_V = 0.0f;
}

void DMM_Measurement_Start(void)
{
    s_samplingState = 1U; //值为 1U 时，表示已经获取通道
    s_errorCounter = 0U;
}

bool DMM_Measurement_TryAcquire(int8_t *channel)
{
    if (s_samplingState == 2U) //表示当前的采样状态 值为 2U 时，表示可以获取通道。值为 1U 时，表示已经获取通道。
    {
        if (channel != NULL)
        {
            *channel = s_lastChannel; //s_lastChannel ：存储上一次使用的通道值。
        }
        s_samplingState = 1U; //将 s_samplingState 更新为 1U ，表示通道已被获取
        return true;
    }
    return false;
}

void DMM_Measurement_SelectVoltagePath(void)
{
    MUX_FUN_Select(0x01u);
}

void DMM_Measurement_SelectCurrentPath(void)
{
    MUX_FUN_Select(0x04u);
}

void DMM_Measurement_SelectResistancePath(void)
{
    MUX_FUN_Select(0x02u);
}

void DMM_Measurement_SetBias(uint8_t channel, int32_t value)
{
    if (channel < AD7190_CHANNEL_COUNT)
    {
        s_bias_data[channel] = value;
    }
}

void DMM_Measurement_SetBiasDefaults(void)
{
    s_bias_data[0] = BIAS_VOLTAGE_IN1;
    s_bias_data[1] = BIAS_VOLTAGE_IN2;
    s_bias_data[2] = BIAS_VOLTAGE_IN3;
    s_bias_data[3] = BIAS_VOLTAGE_IN4;
}

int32_t DMM_Measurement_GetBias(uint8_t channel)
{
    if (channel < AD7190_CHANNEL_COUNT)
    {
        return s_bias_data[channel];
    }
    return 0;
}

int32_t AD7190_Filter(int channel, int32_t sample)
{
    if ((channel < 0) || (channel >= (int)AD7190_CHANNEL_COUNT))
    {
        return sample;
    }

    s_filter_buf[channel][s_filter_index[channel]] = sample;
    s_filter_index[channel] = (uint8_t)((s_filter_index[channel] + 1U) % AD7190_FILTER_DEPTH);

    int64_t sum = 0;
    for (uint8_t i = 0U; i < AD7190_FILTER_DEPTH; ++i)
    {
        sum += s_filter_buf[channel][i];
    }

    return (int32_t)(sum / (int32_t)AD7190_FILTER_DEPTH);
}

static float code20_to_vinV(int32_t code20)
{
    double mv = (double)code20 * (double)REFERENCE_VOLTAGE * (double)OPA_RES_R1;
    mv /= (double)OPA_RES_R2;
    mv /= (double)ADC20_FULL_SCALE;
    return (float)(mv / 1000.0);
}


float MEAS_ReadVoltage_V(uint8_t ch, uint8_t use_filter)
{
    /* MUX 路径由 DMM_Controller_SelectMode() 统一管理，此处不重复切换 */
    int32_t code20 = (s_ad7190_data[ch] >> 4);
    if (use_filter != 0U)
    {
        code20 = AD7190_Filter((int)ch, code20);
    }

    s_lastVoltage_V = code20_to_vinV(code20);
    return s_lastVoltage_V;
}
float MEAS_ReadVoltage_V_Last(uint8_t use_filter)
{
    int8_t channel = s_lastChannel;
    if (channel < 0)
    {
        return 0.0f;
    }
    return MEAS_ReadVoltage_V_Single((uint8_t)channel, use_filter);
}

float MEAS_ReadVoltage_V_Single(uint8_t ch, uint8_t use_filter)
{
    if (ch >= AD7190_CHANNEL_COUNT)
    {
        return 0.0f;
    }

    int32_t code24 = s_ad7190_data[ch];
    int32_t code20 = (code24 >> 4);
    if (use_filter != 0U)
    {
        code20 = AD7190_Filter((int)ch, code20);
    }
    return code20_to_vinV(code20);
}

void DMM_SetCurrentChannel(uint8_t ad_ch)
{
    s_i_ad_ch = (ad_ch < AD7190_CHANNEL_COUNT) ? ad_ch : 0U;
}

uint8_t DMM_Measurement_GetCurrentChannel(void)
{
    return s_i_ad_ch;
}

static float code20_to_v_adc(int32_t code20)
{
    double mv = (double)code20 * (double)REFERENCE_VOLTAGE;
    mv /= (double)ADC20_FULL_SCALE;
    return (float)(mv / 1000.0);
}


float DMM_ReadCurrent_FSratio(uint8_t use_filter)
{
    int32_t code24 = s_ad7190_data[s_i_ad_ch];
    int32_t code20 = (code24 >> 4);
    if (use_filter != 0U)
    {
        code20 = AD7190_Filter((int)s_i_ad_ch, code20);
    }

    float fs = fabsf((float)code20) / (float)ADC20_FULL_SCALE;
    if (fs > 1.0f)
    {
        fs = 1.0f;
    }
    return fs;
}

__attribute__((weak)) float IRange_Gain(void)
{
    return 1.0f;
}

__attribute__((weak)) float IPath_VperA(void)
{
    return 1.0f;
}

float DMM_ReadCurrent_A(uint8_t use_filter)
{
    int32_t code24 = s_ad7190_data[s_i_ad_ch];
    int32_t code20 = (code24 >> 4);
    if (use_filter != 0U)
    {
        code20 = AD7190_Filter((int)s_i_ad_ch, code20);
    }

    float v_adc = code20_to_v_adc(code20);
    float gain = IPath_VperA() * IRange_Gain();
    if (gain == 0.0f)
    {
        return 0.0f;
    }
    return v_adc / gain;
}

/* MEAS_I_Autorange_1kHz 已移至 adc_mux.c（依赖 s_i_range / i_range_apply） */


__attribute__((weak)) float VRange_Scale(void)
{
    return 1.0f;
}

__attribute__((weak)) float RV_Chain_Scale(void)
{
    return VRange_Scale();
}

__attribute__((weak)) float RI_Path_VperA(void)
{
    return 1.0f;
}

__attribute__((weak)) float RI_Range_Gain(void)
{
    return 1.0f;
}

void DMM_R_SetChannels(uint8_t v_ad_ch, uint8_t i_ad_ch)
{
	//v_ad_ch：电阻测量时，电压采样用哪一路 AD7190 通道
    s_r_v_adch = (v_ad_ch < AD7190_CHANNEL_COUNT) ? v_ad_ch : 0U;
    //i_ad_ch：电阻测量时，电流采样用哪一路 AD7190 通道
    s_r_i_adch = (i_ad_ch < AD7190_CHANNEL_COUNT) ? i_ad_ch : 1U;
}

uint8_t DMM_Measurement_GetResistanceVoltageChannel(void)
{
    return s_r_v_adch;
}

uint8_t DMM_Measurement_GetResistanceCurrentChannel(void)
{
    return s_r_i_adch;
}


float DMM_R_Read(uint8_t use_filter, float *r_out, float *i_out, float *v_out)
{
    /* MUX 已由 DMM_Controller_SelectMode(DMM_MODE_RESISTANCE) 切到 AIN_R，
       此处不再重复切换，避免覆盖电阻路径 */
    int32_t code20_v = (s_ad7190_data[s_r_v_adch] >> 4);
    if (use_filter != 0U)
    {
        code20_v = AD7190_Filter((int)s_r_v_adch, code20_v);
    }
    float Vadc = code20_to_v_adc(code20_v);
    float Vdut = Vadc * RV_Chain_Scale();

    int32_t code20_i = (s_ad7190_data[s_r_i_adch] >> 4);
    if (use_filter != 0U)
    {
        code20_i = AD7190_Filter((int)s_r_i_adch, code20_i);
    }
    float Vadc_I = code20_to_v_adc(code20_i);
    float Idut = 0.0f;
    float gain = RI_Path_VperA() * RI_Range_Gain();
    if (gain != 0.0f)
    {
        Idut = Vadc_I / gain;
    }

    float Rdut = (fabsf(Idut) > 1e-12f) ? (Vdut / Idut) : 0.0f;

    if (r_out != NULL)
    {
        *r_out = Rdut;
    }
    if (i_out != NULL)
    {
        *i_out = Idut;
    }
    if (v_out != NULL)
    {
        *v_out = Vdut;
    }

    return Rdut;
}

void MEAS_ReadResistance_Ohm(float *r, float *i, float *v)
{
    (void)DMM_R_Read(1U, r, i, v);
}

void MEAS_R_Autorange_1kHz(void)
{
    /* Placeholder for future implementation */
}

void HAL_SYSTICK_Callback(void)
{
    uint8_t sample[4];

    if ((s_samplingState != 0U) && (AD7190_RDY_STATE() == 0))
    {
        HAL_SPI_Receive(&hspi_AD7190, sample, 4, 0xFF);

        if ((sample[3] & 0x80U) == 0U)
        {
            /* 单一电压模式：只接受 AIN1-AINCOM (CHD=4=0b100) */
            if ((sample[3] & 0x07U) == 4U)
            {
#if ZERO_MODE == 1U
                s_bias_data[0] = ((int32_t)sample[0] << 16) |
                                 ((int32_t)sample[1] << 8)  |
                                 ((int32_t)sample[2]);
#else
                int32_t raw = ((int32_t)sample[0] << 16) |
                              ((int32_t)sample[1] << 8)  |
                              ((int32_t)sample[2]);
                s_ad7190_data[0] = raw - s_bias_data[0];
#endif
                s_lastChannel    = 0;
                s_samplingState  = 2U;
                s_errorCounter   = 0U;
            }
            else
            {
                /* 收到非预期通道帧，计入错误 */
                if (s_errorCounter < 0xFFU) { ++s_errorCounter; }
                if (s_errorCounter >= 10U)  { s_samplingState = 0U; s_errorCounter = 0U; }
            }
        }
        else
        {
            if (s_errorCounter < 0xFFU) { ++s_errorCounter; }
            if (s_errorCounter >= 10U)  { s_samplingState = 0U; s_errorCounter = 0U; }
        }
    }
}

















































