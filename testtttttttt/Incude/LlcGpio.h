/*******************************************************************************
 * LlcGpio.h
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#ifndef LLCGPIO_H_
#define LLCGPIO_H_

#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define LED_GRNON    GpioDataRegs.GPACLEAR.bit.GPIO4 = 1
#define LED_GRNOFF   GpioDataRegs.GPASET.bit.GPIO4   = 1
#define LED_REDON    GpioDataRegs.GPACLEAR.bit.GPIO5 = 1
#define LED_REDOFF   GpioDataRegs.GPASET.bit.GPIO5   = 1

#define DC_EN        GpioDataRegs.GPACLEAR.bit.GPIO11  = 1
#define DC_DIS       GpioDataRegs.GPASET.bit.GPIO11    = 1
//#define DC_DIS       GpioDataRegs.GPACLEAR.bit.GPIO11    = 1
#define LED_WORKON   GpioDataRegs.GPACLEAR.bit.GPIO19  = 1
#define LED_WORKOFF  GpioDataRegs.GPASET.bit.GPIO19    = 1
#define LED_WORKTOG  GpioDataRegs.GPATOGGLE.bit.GPIO19 = 1

#define FAN_ON       GpioDataRegs.GPASET.bit.GPIO10   = 1
#define FAN_OFF      GpioDataRegs.GPACLEAR.bit.GPIO10 = 1

#define SET_KPAW     GpioDataRegs.GPASET.bit.GPIO17   = 1  //�����߼̵�������
#define CLR_KPAW     GpioDataRegs.GPACLEAR.bit.GPIO17 = 1

//#define SET_KPIN     GpioDataRegs.GPBSET.bit.GPIO33   = 1//��PFC
//#define CLR_KPIN     GpioDataRegs.GPBCLEAR.bit.GPIO33 = 1//��PFC�������

#define SET_KPOUT    GpioDataRegs.GPASET.bit.GPIO18   = 1  //������̵�������
#define CLR_KPOUT    GpioDataRegs.GPACLEAR.bit.GPIO18 = 1  //�Ͽ�����̵���

#define LED_OGRNON   GpioDataRegs.GPASET.bit.GPIO12   = 1
#define LED_OGRNOFF  GpioDataRegs.GPACLEAR.bit.GPIO12 = 1
#define LED_OREDON   GpioDataRegs.GPASET.bit.GPIO16   = 1
#define LED_OREDOFF  GpioDataRegs.GPACLEAR.bit.GPIO16 = 1

#define SET_RCK      GpioDataRegs.GPASET.bit.GPIO20   = 1        //��ST-RCK
#define RST_RCK      GpioDataRegs.GPACLEAR.bit.GPIO20 = 1        //��ST-RCK
#define SET_SRCK     GpioDataRegs.GPASET.bit.GPIO21   = 1        //��CLK-SRCK
#define RST_SRCK     GpioDataRegs.GPACLEAR.bit.GPIO21 = 1        //��CLK-SRCK
#define SET_SER      GpioDataRegs.GPASET.bit.GPIO24   = 1        //��SER
#define RST_SER      GpioDataRegs.GPACLEAR.bit.GPIO24 = 1        //��SER
/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
extern void InitGpio(void);

#endif /* LLCGPIO_H_ */
