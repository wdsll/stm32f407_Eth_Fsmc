/*******************************************************************************
 * LlcErr.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcMain.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define VolOutOvpEnt  360      //�����ѹ��ѹ������ 365
#define VolOutOvpRec  350      //�����ѹ��ѹ�ظ��� 350
#define VolOutOvpECnt 100      //�����ѹ��ѹ����ʱ��
#define VolOutOvpRCnt 2000     //�����ѹ��ѹ�ظ�ʱ��

#define BatOutUvpEnt  30      //��ص�ѹǷѹ������
#define BatOutUvpRec  68      //��ص�ѹǷѹ�ظ���
#define BatOutUvpECnt 100      //��ص�ѹǷѹ����ʱ��
#define BatOutUvpRCnt 2000     //��ص�ѹǷѹ�ظ�ʱ��

/*
#define BatOutUvpEnt  180      //��ص�ѹ��������ѹ18V
#define BatOutUvpRec  130      //��ص�ѹǷѹ�ظ���
#define BatOutUvpECnt 100      //��ص�ѹǷѹ����ʱ��
#define BatOutUvpRCnt 3000     //��ص�ѹǷѹ�ظ�ʱ��
*/

#define BatOutOvpEnt  345      //��ص�ѹ��ѹ������350
#define BatOutOvpRec  335      //��ص�ѹ��ѹ�ظ���340
#define BatOutOvpECnt 100      //��ص�ѹ��ѹ����ʱ��
#define BatOutOvpRCnt 2000     //��ص�ѹ��ѹ�ظ�ʱ��

#define CurPeakOver   4200     //������������������ػ�3900
#define CurOutOcpEnt  4200     //�����������������// 3900
#define CurOutOcpRec  100      //������������ظ���
#define CurOutOcpECnt 300       //���������������ʱ��//10ms
#define CurOutOcpRCnt 3000     //������������ظ�ʱ��

#define ACOvpEnt  2700         //AC��ѹ������
#define ACOvpRec  2650         //AC��ѹ�ظ���
#define ACOvpECnt 100          //AC��ѹ����ʱ��
#define ACOvpRCnt 3000         //AC��ѹ�ظ�ʱ��

#define ACUvpEnt  800          //ACǷѹ������80V
#define ACUvpRec  880          //ACǷѹ�ظ���90V
#define ACUvpECnt 2            //ACǷѹ����ʱ��//100   40ms
#define ACUvpRCnt 3000         //ACǷѹ�ظ�ʱ��
#define ACValley  500          //����Ƿѹ״̬

#define PFCOvpEnt  4150        //PFC_bus��ѹ������ 415v
#define PFCOvpRec  3850        //PFC_bus��ѹ�ظ��� 385v
#define PFCOvpECnt 100         //PFC��ѹ����ʱ��
#define PFCOvpRCnt 3000        //PFC��ѹ�ظ�ʱ��


#define PFCUvpEnt  3250         //PFC_bus��ѹ������ 325v
#define PFCUvpRec  3550         //PFC_bus��ѹ�ظ��� 355v
#define PFCUvpECnt 2            //PFC��ѹ����ʱ��100
#define PFCUvpRCnt 3000         //PFC��ѹ�ظ�ʱ��
#define PFCValleyVol  3000        //PFC_bus���ȵ�ѹ��
//#define TEMPOvpEnt  1302       //�¶ȹ��±�����85(����)
//#define TEMPOvpRec  1617       //�¶ȹ��»ظ���75

/*
#define TEMPOvpEnt  658       //�¶ȹ��±�����114(����)////QC
#define TEMPOvpRec  837       //�¶ȹ��»ظ���104
*/

#define TEMPOvpEnt  919       //�¶ȹ��±�����100(����)////QE
#define TEMPOvpRec  1162       //�¶ȹ��»ظ���90


#define TEMPOvpECnt 1000       //�¶ȹ��±�����
#define TEMPOvpRCnt 5000       //�¶ȹ��»ظ�ʱ��

#define OTEMPOvpEnt     811       //�¶ȹ��±�����60(����)
#define OTEMPOvpEnt1    1243       //�¶ȹ��±�����45(����)
#define OTEMPOvpRec     1422      //�¶ȹ��»ظ���40 (�ָ�)
#define OTEMPOvpECnt    1000      //�¶ȹ��±�����
#define OTEMPOvpRCnt    5000      //�¶ȹ��»ظ�ʱ��

#define PFCTEMPOvpEnt  85          //PFC���±�����
#define PFCTEMPOvpRec  50          //PFC���»ظ���
#define PFCTEMPOvpECnt 1000         //PFC���±���ʱ��
#define PFCTEMPOvpRCnt 5000        //PFC���»ظ�ʱ��

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
volatile union unERRSTAT SysErr;    // ϵͳ�쳣״̬

typedef enum{
    CorrectState = 0x00,
    ErrorState   = 0x01,
}CheckErrState;

typedef enum{
    UnderPro = 0x00,
    OverPro  = 0x01,
    PeakOver = 0x02,
    ValleyDown = 0x03,
}CheckErrType;

typedef struct {
    int16 ErrorValue;      //�趨ֵ
    int16 ErrorPeakValue;   ///����趨ֵ
    int16 ErrorValleyValue;////�ȵ��趨ֵ
    int16 RecoveryValue;   //�˳�����ظ�ֵ
    int16 EntryTime;       //�������ʱ���趨ֵ
    int16 RecoveryTime;    //�˳�����ʱ��ֵ
    int16 Counter;         //�жϽ������Ĵ���
    CheckErrState Result;  //0/1
}ErrorCheck;

static ErrorCheck sstVolOutOvp;
static ErrorCheck sstBatOutUvp;
static ErrorCheck sstBatOutOvp;
static ErrorCheck sstCurOutOcp;
static ErrorCheck sstACUvp;
static ErrorCheck sstACOvp;
static ErrorCheck sstPFCUvp;
static ErrorCheck sstPFCOvp;
static ErrorCheck sstTempOvp;
static ErrorCheck sstOTempOvp;
static ErrorCheck sstPFCTempOvp;

/*******************************************************************************
**                               ��������
*******************************************************************************/
extern Uint16 ChargeOffFlag;
extern Uint16 gsIShortCounter;
extern Uint16 gCurveNum;
extern Uint16 Temp_ADC[65];
/*******************************************************************************
**                               ��������
*******************************************************************************/
void InitErrPara(void);
void CheckErr(void);
static void CheckError(ErrorCheck *stErrorCheck, CheckErrType enCheckErrType, Uint16 ComVal);
void CheckOpenCircuit(void);
/*******************************************************************************
** ��������:    InitErrPara
** ��������:    ��ʼ������Err����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void InitErrPara(void)
{
    SysErr.all = 0;

    sstVolOutOvp.ErrorValue = (((int32)VolOutOvpEnt << K_EXP) - K_VOUT_B) / K_VOUT_A;
    sstVolOutOvp.RecoveryValue = (((int32)VolOutOvpRec << K_EXP) - K_VOUT_B) / K_VOUT_A;
    sstVolOutOvp.Counter = 0;
    sstVolOutOvp.EntryTime = VolOutOvpECnt;
    sstVolOutOvp.RecoveryTime = VolOutOvpRCnt;
    sstVolOutOvp.Result = CorrectState;

    sstBatOutUvp.ErrorValue = (((int32)BatOutUvpEnt << K_EXP) - K_VBAT_B) / K_VBAT_A;
    sstBatOutUvp.RecoveryValue = (((int32)BatOutUvpRec << K_EXP) - K_VBAT_B) / K_VBAT_A;
    sstBatOutUvp.Counter = 0;
    sstBatOutUvp.EntryTime = BatOutUvpECnt;
    sstBatOutUvp.RecoveryTime = BatOutUvpRCnt;
    sstBatOutUvp.Result = CorrectState;

    sstBatOutOvp.ErrorValue = (((int32)BatOutOvpEnt << K_EXP) - K_VBAT_B) / K_VBAT_A;
    sstBatOutOvp.RecoveryValue = (((int32)BatOutOvpRec << K_EXP) - K_VBAT_B) / K_VBAT_A;
    sstBatOutOvp.Counter = 0;
    sstBatOutOvp.EntryTime = BatOutOvpECnt;
    sstBatOutOvp.RecoveryTime = BatOutOvpRCnt;
    sstBatOutOvp.Result = CorrectState;

    sstCurOutOcp.ErrorValue = (((int32)CurOutOcpEnt << K_EXP) - K_IOUT_B) / K_IOUT_A;///((int32)IOUT_DATA * K_IOUT_A + K_IOUT_B) >> K_EXP
    sstCurOutOcp.ErrorPeakValue = (((int32)CurPeakOver << K_EXP) - K_IOUT_B) / K_IOUT_A;
    sstCurOutOcp.RecoveryValue = (((int32)CurOutOcpRec << K_EXP) - K_IOUT_B) / K_IOUT_A;
    sstCurOutOcp.Counter = 0;
    sstCurOutOcp.EntryTime = CurOutOcpECnt;
    sstCurOutOcp.RecoveryTime = CurOutOcpRCnt;
    sstCurOutOcp.Result = CorrectState;

    sstACOvp.ErrorValue = ACOvpEnt;
    sstACOvp.RecoveryValue = ACOvpRec;
    sstACOvp.Counter = 0;
    sstACOvp.EntryTime = ACOvpECnt;
    sstACOvp.RecoveryTime = ACOvpRCnt;
    sstACOvp.Result = CorrectState;

    sstACUvp.ErrorValue = ACUvpEnt;
    sstACUvp.ErrorValleyValue = ACValley;
    sstACUvp.RecoveryValue = ACUvpRec;
    sstACUvp.Counter = 0;
    sstACUvp.EntryTime = ACUvpECnt;
    sstACUvp.RecoveryTime = ACUvpRCnt;
    sstACUvp.Result = CorrectState;

    sstPFCOvp.ErrorValue = PFCOvpEnt;
    sstPFCOvp.RecoveryValue = PFCOvpRec;
    sstPFCOvp.Counter = 0;
    sstPFCOvp.EntryTime = PFCOvpECnt;
    sstPFCOvp.RecoveryTime = PFCOvpRCnt;
    sstPFCOvp.Result = CorrectState;

    sstPFCUvp.ErrorValue = PFCUvpEnt;
    sstPFCUvp.RecoveryValue = PFCUvpRec;
    sstPFCUvp.Counter = 0;
    sstPFCUvp.EntryTime = PFCUvpECnt;
    sstPFCUvp.RecoveryTime = PFCUvpRCnt;
    sstPFCUvp.Result = CorrectState;
    sstPFCUvp.ErrorValleyValue=PFCValleyVol;

    sstTempOvp.ErrorValue = TEMPOvpEnt;
    sstTempOvp.RecoveryValue = TEMPOvpRec;
    sstTempOvp.Counter = 0;
    sstTempOvp.EntryTime = TEMPOvpECnt;
    sstTempOvp.RecoveryTime = TEMPOvpRCnt;
    sstTempOvp.Result = CorrectState;

    sstOTempOvp.ErrorValue = OTEMPOvpEnt;
    sstOTempOvp.RecoveryValue = OTEMPOvpRec;
    sstOTempOvp.Counter = 0;
    sstOTempOvp.EntryTime = OTEMPOvpECnt;
    sstOTempOvp.RecoveryTime = OTEMPOvpRCnt;
    sstOTempOvp.Result = CorrectState;

    sstPFCTempOvp.ErrorValue = PFCTEMPOvpEnt;
    sstPFCTempOvp.RecoveryValue = PFCTEMPOvpRec;
    sstPFCTempOvp.Counter = 0;
    sstPFCTempOvp.EntryTime = PFCTEMPOvpECnt;
    sstPFCTempOvp.RecoveryTime = PFCTEMPOvpRCnt;
    sstPFCTempOvp.Result = CorrectState;
}


/*******************************************************************************
** ��������:    CheckErr
** ��������:    ��ʼ����������
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
Uint16 RecoverShortCount = 0;
void CheckErr(void)
{
    CheckError(&sstVolOutOvp, OverPro, ADRESULT_VFAST);
    SysErr.bit.VOutOver = sstVolOutOvp.Result;                    //�����ѹ
    CheckError(&sstBatOutOvp, OverPro, ADRESULT_VSLOW);
    SysErr.bit.VBatOver = sstBatOutOvp.Result;                    //��ع�ѹ
    CheckError(&sstBatOutUvp, UnderPro, ADRESULT_VSLOW);
    SysErr.bit.VBatLack = sstBatOutUvp.Result;                    //���Ƿѹ
    if(ADRESULT_VSLOW > sstBatOutUvp.ErrorValue)//*��ص�ѹ���������*//
    {
        CheckError(&sstCurOutOcp, OverPro, ADRESULT_IFAST);
        SysErr.bit.IOutOver = sstCurOutOcp.Result;                    //�������
        CheckError(&sstCurOutOcp, OverPro, ADRESULT_ISHORT);      ///ADRESULT_IFAST
        SysErr.bit.IOutOver = sstCurOutOcp.Result;                    //�������
  /*      CheckError(&sstCurOutOcp, PeakOver, ADRESULT_ISHORT);      ///ADRESULT_IFAST
        SysErr.bit.IOutOver = sstCurOutOcp.Result;                    //���������
  */  }
    if(1 == SysErr.bit.VBatLack)/*��·�Ժ��ص�ѹ���⵽�ͣ����޵�ѹ����Ϊ�Ƕ�·�Ѿ�����*/
    {
        if(1 == SysErr.bit.IShort && RecoverShortCount < 10)
        {
            RecoverShortCount ++;
            SysErr.bit.IShort = 0;
            SysErr.bit.IOutOver = 0;
            gsIShortCounter = 0;
        }
    }
    else
    {
        RecoverShortCount = 0;
    }
    CheckError(&sstACUvp, UnderPro, gsPFCUploadData.PFCAcInVolt);
    SysErr.bit.ACUvp = sstACUvp.Result;                           //ACǷѹ
    CheckError(&sstACUvp, ValleyDown, gsPFCUploadData.PFCAcInVolt);
    SysErr.bit.ACUvp = sstACUvp.Result;                            //����ѹ
    CheckError(&sstACOvp, OverPro, gsPFCUploadData.PFCAcInVolt);
    SysErr.bit.ACOvp = sstACOvp.Result;                           //AC��ѹ
    if(PfcOn == gPfcCmd && sstACUvp.ErrorValue > sstPFCUvp.ErrorValue > 3100)
    {
        CheckError(&sstPFCUvp, UnderPro, gsPFCUploadData.PFCBusVolt);
        SysErr.bit.PFCUvp = sstPFCUvp.Result;                         //PFCǷѹ
    }
   // CheckError(&sstPFCOvp, OverPro, gsPFCUploadData.PFCBusVolt);  //VBUS��ѹ
    SysErr.bit.PFCOvp = sstPFCOvp.Result;                         //PFC��ѹ
    CheckError(&sstTempOvp, UnderPro, ADRESULT_TEMP);           //LLC����
    //CheckError(&sstPFCTempOvp, OverPro, gsPFCUploadData.PFCTemp);  //PFC����
    SysErr.bit.TempOver = sstTempOvp.Result || sstPFCTempOvp.Result;                      //����
    CheckError(&sstOTempOvp, UnderPro, ADRESULT_OTEMP);
    SysErr.bit.TempOutOver = sstOTempOvp.Result;                  //�ⲿ����
    if((1 == gCurveNum || 2 == gCurveNum || 3 == gCurveNum || 4== gCurveNum ||
    		9 == gCurveNum|| 10 == gCurveNum|| 11 == gCurveNum|| 12 == gCurveNum||
			13 == gCurveNum) && (ADRESULT_OTEMP <= OTEMPOvpEnt1))///45��C
        SysErr.bit.TempOutOver = 1;
    else if((5 == gCurveNum || 6 == gCurveNum || 7 == gCurveNum || 8== gCurveNum ||
    		14 == gCurveNum|| 15 == gCurveNum||16 == gCurveNum || 17 == gCurveNum||
			18 == gCurveNum|| 19 == gCurveNum|| 20 == gCurveNum|| 21 == gCurveNum||
			22 == gCurveNum) && (ADRESULT_OTEMP <= OTEMPOvpEnt))///60�� COTEMPOvpEnt1
        SysErr.bit.TempOutOver = 1;


    CheckOpenCircuit();
}

/*******************************************************************************
** ��������:    CheckError
** ��������:    �����ѹ��ѹ���
** ��ʽ����:    Err�ṹ��ָ�룬�������ͣ��Ƚ�ֵ
** ���ز���:    ��
*******************************************************************************/
static void CheckError(ErrorCheck *stErrorCheck, CheckErrType enCheckErrType, Uint16 ComVal)
{
    switch(stErrorCheck->Result)
    {
        case CorrectState:
            switch(enCheckErrType)
            {
                case OverPro:
                    if(ComVal > stErrorCheck->ErrorValue)
                    {
                        stErrorCheck->Counter ++;
                        if(stErrorCheck->Counter > stErrorCheck->EntryTime)
                        {
                            stErrorCheck->Result = ErrorState;
                            stErrorCheck->Counter = 0;
                        }
                    }
                    else
                    {
                        stErrorCheck->Counter = 0;
                    }
                    break;
                case UnderPro:
                    if(ComVal < stErrorCheck->ErrorValue)
                    {
                        stErrorCheck->Counter ++;
                        if(stErrorCheck->Counter > stErrorCheck->EntryTime || ComVal > (((int32)3900 << K_EXP) - K_IOUT_B) / K_IOUT_A)
                        {
                            stErrorCheck->Result = ErrorState;
                            stErrorCheck->Counter = 0;
                        }
                    }
                    else
                    {
                        stErrorCheck->Counter = 0;
                    }
                    break;
                case PeakOver:
                    if(ComVal > stErrorCheck->ErrorPeakValue)
                        stErrorCheck->Result = ErrorState;
                    break;
                case ValleyDown:
                    if(ComVal < stErrorCheck->ErrorValleyValue)
                        stErrorCheck->Result = ErrorState;
                    break;
                default:
                    break;
            }
            break;
        case ErrorState:
            switch(enCheckErrType)
            {
                case OverPro:
                    if(ComVal < stErrorCheck->RecoveryValue)
                    {
                        stErrorCheck->Counter ++;
                        if(stErrorCheck->Counter > stErrorCheck->RecoveryTime)
                        {
                            stErrorCheck->Result = CorrectState;
                            stErrorCheck->Counter = 0;
                        }
                    }
                    else
                    {
                        stErrorCheck->Counter = 0;
                    }
                    break;
                case UnderPro:
                    if(ComVal > stErrorCheck->RecoveryValue)
                    {
                        stErrorCheck->Counter ++;
                        if(stErrorCheck->Counter > stErrorCheck->RecoveryTime)
                        {
                            stErrorCheck->Result = CorrectState;
                            stErrorCheck->Counter = 0;
                        }
                    }
                    else
                    {
                        stErrorCheck->Counter = 0;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/*******************************************************************************
** ��������:    CheckOpenCircuit
** ��������:    �����ѹ��ѹ���
** ��ʽ����:    Err�ṹ��ָ�룬�������ͣ��Ƚ�ֵ
** ���ز���:    ��
*******************************************************************************/
Uint16 gs10msCounter = 0;
int32 VBatBackUp = 0;
void CheckOpenCircuit(void)
{
    gs10msCounter++;
    if(gs10msCounter > 10)
    {
        gs10msCounter = 0;
        if(GpioDataRegs.GPADAT.bit.GPIO18)
        {
            if(((gstElecPara.OutVol - VBatBackUp) > 10)&&(gstElecPara.OutVol > 290)
                    &&(gstElecPara.OutCur < 100))
            {
                SysErr.bit.VBatLack = 1;
            }
        }
        VBatBackUp = gstElecPara.OutVol;
    }
}

