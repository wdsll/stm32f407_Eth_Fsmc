/*
 * dmm_measurement.h
 *
 *  Created on: Sep 27, 2025
 *      Author: huyan
 */

#ifndef INC_DMM_MEASUREMENT_H_
#define INC_DMM_MEASUREMENT_H_

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZERO_MODE 0U

void DMM_Measurement_Init(void);
void DMM_Measurement_Start(void);
bool DMM_Measurement_TryAcquire(int8_t *channel);

void DMM_Measurement_SelectVoltagePath(void);
void DMM_Measurement_SelectCurrentPath(void);
void DMM_Measurement_SelectResistancePath(void);


void DMM_Measurement_SetBias(uint8_t channel, int32_t value);
void DMM_Measurement_SetBiasDefaults(void);
int32_t DMM_Measurement_GetBias(uint8_t channel);

int32_t AD7190_Filter(int channel, int32_t sample);

float MEAS_ReadVoltage_V(uint8_t ch, uint8_t use_filter);
float MEAS_ReadVoltage_V_Last(uint8_t use_filter);
float MEAS_ReadVoltage_V_Single(uint8_t ch, uint8_t use_filter);

void DMM_SetCurrentChannel(uint8_t ch);
uint8_t DMM_Measurement_GetResistanceVoltageChannel(void);
uint8_t DMM_Measurement_GetResistanceCurrentChannel(void);

float DMM_R_Read(uint8_t use_filter, float *r_out, float *i_out, float *v_out);
void MEAS_ReadResistance_Ohm(float *r, float *i, float *v);
void MEAS_R_Autorange_1kHz(void);

uint8_t DMM_Measurement_GetCurrentChannel(void);
void DMM_R_SetChannels(uint8_t v_ad_ch, uint8_t i_ad_ch);
float DMM_ReadCurrent_A(uint8_t use_filter);
#ifdef __cplusplus
}
#endif

#endif /* INC_DMM_MEASUREMENT_H_ */
