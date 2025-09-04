/*******************************************************************************
 * LlcMain.h
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#ifndef LLCMAIN_H_
#define LLCMAIN_H_

#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File

#include "LlcAdc.h"
#include "LlcBoot.h"
#include "LlcCan.h"
#include "LlcErr.h"
#include "LlcGpio.h"
#include "LlcIIC.h"
#include "LlcInit.h"
#include "LlcLed.h"
#include "LlcLin.h"
#include "LlcLoop.h"
#include "LlcPwm.h"
#include "LlcSci.h"
#include "LlcTimer.h"
#include "LlcDS1338.h"
#include "LlcUSB.h"

#include "LlcCurve.h"
#include "LlcFlow.h"
#include "LlcSys.h"
#include "LlcEEPROM.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/
// =============================================================================
// �꺯��,max,min: �Ƚ��������������ؽ�Сֵ
// =============================================================================
#define max(a,b) (a)>(b)?(a):(b)
#define min(a,b) (a)<(b)?(a):(b)
/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
extern const Uint16 DeBug;          // debug��־,����ʹ��
extern Uint16 RammanualLoadStart;
extern Uint16 RammanualLoadEnd;
extern Uint16 RammanualRunStart;

extern int32 CONCURCHARGEENDV;
/*******************************************************************************
**                               ��������
*******************************************************************************/
// =============================================================================
// �꺯��,SERVICE_DOG: ι��
// =============================================================================
#define SERVICE_DOG EALLOW;\
    SysCtrlRegs.WDKEY = 0x0055;\
    SysCtrlRegs.WDKEY = 0x00AA;\
    EDIS

#endif /* LLCMAIN_H_ */
