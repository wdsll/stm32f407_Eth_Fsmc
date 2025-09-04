/*******************************************************************************
 * LlcLin.h
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#ifndef LLCLIN_H_
#define LLCLIN_H_

#include "DSP28x_Project.h"    // Device Headerfile and Examples Include File

/*******************************************************************************
**                               �궨��
*******************************************************************************/
typedef enum
{
    PfcOff = 0,
    PfcOn
}ePfcCmd_t;
extern ePfcCmd_t gPfcCmd;

#define PFC_ON       gPfcCmd = PfcOn;
#define PFC_OFF      gPfcCmd = PfcOff;

#define YEAR           4; //�� ��23��д3��24��д��4
#define MONTH_H        0; //��ʮλ
#define MONTH_L        3; //�¸�λ
#define DATE_H         2; //��ʮλ
#define DATE_L         6; //�ո�λ

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
typedef struct
{
    Uint16  PFCTemp;           // PFC�¶�
    Uint16  PFCAcInFrq;        // PFC AC����Ƶ��
    Uint16  PFCAcInVolt;       // PFC AC�����ѹ
    Uint16  PFCAcInCrt;        // PFC AC�������
    int16   PFCBusVolt;        // PFC VBUS���������ѹ
    Uint16  PFCWorkSts;
    Uint16  PFCCmdSts;
    Uint16  PFCSubSts;
    Uint16  PFCErr;

}sPFCUPLOADDATA;
extern sPFCUPLOADDATA gsPFCUploadData;
/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
extern void LinInit(void);
extern void LinManage(void);
extern void PfcConfi(void);

#endif /* LLCLIN_H_ */
