/*******************************************************************************
 * LlcEEPROM.h
 *
 *  Created on: 2020-01-14
 *      Author: Gavin.D
*******************************************************************************/
#ifndef LLCEEPROM_H_
#define LLCEEPROM_H_

#include "DSP28x_Project.h"    // Device Headerfile and Examples Include File

/*******************************************************************************
**                               �궨��
*******************************************************************************/

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
typedef struct {
    Uint16 Year;     // ��
    Uint16 Month;    // ��
    Uint16 Day;      // ��
    Uint16 Hour;     // ʱ
    Uint16 Minute;   // ��
    Uint16 CurveNo;  // ���߱��
    Uint16 Event;    //�¼���1����������2�����رգ�3����ػ�
    Uint16 DurationHour;     // ����ʱ
    Uint16 DurationMinute;   // ������
    Uint16 StartVH;          // ��ʼ��ѹ
    Uint16 StartVL;          // ��ʼ��ѹ
    Uint16 EndVH;          //�����ߵ�ѹ
    Uint16 EndVL;          //�����͵�ѹ
    Uint16 ChargeAHH;      //��������
    Uint16 ChargeAHL;      //��������
    Uint16 ACVoltage;      //����AC��ѹ
}stStoreData;
extern stStoreData StoreData;
extern Uint16 EQ_MFlag;
/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
extern void InitEEPPara(void);
extern Uint16 WriteEEPStart(void);
extern Uint16 WriteEEPStop(void);
extern Uint16 WriteEEPErr(void);

#endif /* LLCEEPROM_H_ */
