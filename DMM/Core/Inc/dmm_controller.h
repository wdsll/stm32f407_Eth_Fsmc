/*
 * dmm_controller.h
 *
 *  Created on: Sep 27, 2025
 *      Author: huyan
 */

#ifndef INC_DMM_CONTROLLER_H_
#define INC_DMM_CONTROLLER_H_

#include "main.h"
#include "dmm_measurement.h"
#include "lcd.h"
#include "keyled.h"
#ifdef __cplusplus
extern "C" {
#endif

void DMM_Controller_Init(void);

void DMM_Controller_SelectMode(DMM_Mode mode);

void DMM_Controller_CycleMode(int direction);

void DMM_Controller_HandleModeSwitch(void);

void DMM_Controller_HandleMeasurement(int8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* INC_DMM_CONTROLLER_H_ */
