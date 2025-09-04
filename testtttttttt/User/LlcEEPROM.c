/*******************************************************************************
 * LlcEEPROM.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcMain.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
stStoreData StoreData;
/*******************************************************************************
**                               ��������
*******************************************************************************/
static Uint16 WriteEEPStartStep = 0;
static Uint16 ReadCalibrationStep = 0;
static Uint16 WriteEEPStopStep = 0;
static Uint16 WriteEEPErrStep = 0;

static Uint16 SelfCount = 0;
static int16 CountStore = 1;/*0*/
static Uint16 H_ADDR = 0;
static Uint16 L_ADDR = 1;//0
static Uint16 gWriteEEPROMStep = 0;
static Uint16 AddrH = 0;
static Uint16 AddrL = 0;
static Uint32 WriteAddr =0;
/*******************************************************************************
**                               ��������
*******************************************************************************/
extern Uint16 gCurve[22];
void InitEEPPara(void);
Uint16 WriteEEPStart(void);
Uint16 WriteEEPStop(void);
Uint16 WriteEEPErr(void);
static Uint16 CheckErrToEEP(void);

static Uint16 WriteEEPROM(void);
/*******************************************************************************
** ��������:    InitEEPPara
** ��������:    EEPRomд�������ʼ��    256K=32768�ֽ�
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void InitEEPPara(void)
{
    StoreData.Year    = 0;
    StoreData.Month   = 0;
    StoreData.Day     = 0;
    StoreData.Hour    = 0;
    StoreData.Minute  = 0;
    StoreData.CurveNo = 0;
    StoreData.Event   = 0;  //1��������
    StoreData.DurationHour   = 0;
    StoreData.DurationMinute = 0;
    StoreData.StartVH = 0;
    StoreData.StartVL = 0;
    StoreData.EndVH   = 0;
    StoreData.EndVL   = 0;
    StoreData.ChargeAHH = 0;
    StoreData.ChargeAHL = 0;
    StoreData.ACVoltage = 0;
}

/*******************************************************************************
** ��������:    WriteEEPStart
** ��������:    д�뿪��ʱEEPRom��ֵ
** ��ʽ����:    ��
** ���ز���:    �Ƿ���д��,1������д�꣬0����δд��
*******************************************************************************/
Uint16 WriteEEPStart(void)
{
    ServiceDog();
    switch(WriteEEPStartStep)
    {
        case 0:
            SelfCount ++;
            if(DS1338_ReadTime())
            {
                StoreData.Year  = Year;
                StoreData.Month = Month;
                StoreData.Day   = Date;
                StoreData.Hour  = Hour;
                StoreData.Minute  = Minute;
                StoreData.CurveNo = gCurve[gCurveNum - 1];
                StoreData.Event   = 1;          //1��������
                StoreData.DurationHour   = 0;
                StoreData.DurationMinute = 0;
                StoreData.StartVH = gstElecPara.BatVol / 10;///100
                StoreData.StartVL = gstElecPara.BatVol % 10;///100
                StoreData.EndVH   = 0;
                StoreData.EndVL   = 0;
                StoreData.ChargeAHH = 0;
                StoreData.ChargeAHL = 0;
                StoreData.ACVoltage = gsPFCUploadData.PFCAcInVolt / 10;
                WriteEEPStartStep = 1;
            }
            if(SelfCount > 1000)
            {
                SelfCount = 0;
                WriteEEPStartStep = 1;
            }
            break;
        case 1:
            SelfCount ++;
            H_ADDR = ReadByte(1);       //�ӵ�1�ж���һ������λ��ַ
            if(H_ADDR != 256)
            {
                WriteEEPStartStep = 2;
            }
            if(SelfCount > 1000)
            {
              SelfCount = 0;
              WriteEEPStartStep = 2;
            }
            break;
        case 2:
            SelfCount ++;
            L_ADDR = ReadByte(2);     //�ӵ�1�ж���һ������λ��ַ
            if(L_ADDR != 256)
            {
                CountStore = H_ADDR * 256 + L_ADDR;  //һ�����ݵ��׵�ַ
                CountStore ++ ;                      //һ�����ݼ�1Ϊ��ŵ�ǰ�����Һõ�ַ
                gWriteEEPROMStep = 0;
                WriteEEPStartStep = 3;
            }
            if(SelfCount > 1000)
            {
                SelfCount = 0;
                gWriteEEPROMStep = 0;
              WriteEEPStartStep = 3;
            }
            break;
        case 3:
            SelfCount ++;
            if(WriteEEPROM())         //�ѵ�ǰ��һ�����ݴ�ź�
            {
                WriteEEPStartStep = 4;
            }
            if(SelfCount > 1000)
            {
              SelfCount = 0;
              WriteEEPStartStep = 4;
            }
            break;
        case 4:
            break;
        default:
            break;
    }
    if(4 == WriteEEPStartStep)
    {
        SelfCount = 0;
        WriteEEPStartStep = 0;
        return(1);
    }
    else
    {
        return(0);
    }
}

/*******************************************************************************
** ��������:    WriteEEPStop
** ��������:    д��ػ�ʱEEPRom��ֵ
** ��ʽ����:    ��
** ���ز���:    �Ƿ���д��,1������д�꣬0����δд��
*******************************************************************************/
Uint16 WriteEEPStop(void)
{
    Uint16 tmpChargeAH = 0;
    switch(WriteEEPStopStep)
    {
        case 0:
            SelfCount ++;
            if(DS1338_ReadTime())
            {
                StoreData.Year  = Year;
                StoreData.Month = Month;
                StoreData.Day   = Date;
                StoreData.Hour  = Hour;
                StoreData.Minute  = Minute;
                StoreData.CurveNo = gCurve[gCurveNum - 1];
                StoreData.Event   = 2;  //2���������ر�
                StoreData.DurationHour   = (gAllChargeTime / 3600);  //���ʱ��
                StoreData.DurationMinute = (gAllChargeTime - StoreData.DurationHour * 3600)/60;  //���ʱ��
                StoreData.StartVH = 0;
                StoreData.StartVL = 0;
                StoreData.EndVH   = gstElecPara.BatVol / 10;////100
                StoreData.EndVL   = gstElecPara.BatVol % 10;///100
                tmpChargeAH = gAllChargeAH / 360000;
                StoreData.ChargeAHH = tmpChargeAH / 256;
                StoreData.ChargeAHL = tmpChargeAH % 256;
                StoreData.ACVoltage = gsPFCUploadData.PFCAcInVolt / 10;
                WriteEEPStopStep = 1;
                SelfCount = 0;
            }
            if( SelfCount > 1000)
             {
                WriteEEPStopStep = 1;
                SelfCount = 0;
             }
            break;
        case 1:
            SelfCount ++;
            H_ADDR = ReadByte(1);
            if(H_ADDR != 256)
            {
                WriteEEPStopStep = 2;
                SelfCount = 0;
            }
            if( SelfCount > 1000)
             {
                WriteEEPStopStep = 2;
                SelfCount = 0;
             }
            break;
        case 2:
            SelfCount ++;
            L_ADDR = ReadByte(2);
            if(L_ADDR != 256)
            {
                CountStore = H_ADDR * 256 + L_ADDR;
                CountStore ++;
                gWriteEEPROMStep = 0;
                WriteEEPStopStep = 3;
                SelfCount = 0;
            }
            if( SelfCount > 1000)
             {
                gWriteEEPROMStep = 0;
                WriteEEPStopStep = 3;
                SelfCount = 0;
             }
            break;
        case 3:
            SelfCount ++;
            if(WriteEEPROM())
            {
                WriteEEPStopStep = 4;
                SelfCount = 0;
            }
            if( SelfCount > 2000)
            {
                WriteEEPStopStep = 4;
                SelfCount = 0;
            }
            break;
        case 4:
            break;
        default:
            break;
    }
    if(4 == WriteEEPStopStep)
    {
        SelfCount = 0;
        WriteEEPStopStep = 0;
        return(1);
    }
    else
    {
        return(0);
    }
}

/*******************************************************************************
** ��������:    WriteEEPErr
** ��������:    д�����ʱEEPRom��ֵ
** ��ʽ����:    ��
** ���ز���:    �Ƿ���д��,1������д�꣬0����δд��
*******************************************************************************/
Uint16 WriteEEPErr(void)
{
    Uint16 tmpChargeAH = 0;
    switch(WriteEEPErrStep)
    {
        case 0:
            SelfCount ++;
            if(DS1338_ReadTime())
            {
            StoreData.Year  = Year;
            StoreData.Month = Month;
            StoreData.Day   = Date;
            StoreData.Hour  = Hour;
            StoreData.Minute  = Minute;
            StoreData.CurveNo = gCurve[gCurveNum - 1];
            StoreData.Event   = 3;  //3�������
            StoreData.DurationHour   = (gAllChargeTime / 3600);  //���ʱ��
            StoreData.DurationMinute = (gAllChargeTime - StoreData.DurationHour * 3600)/60;  //���ʱ��
            StoreData.StartVH = 0;
            StoreData.StartVL = CheckErrToEEP();
            StoreData.EndVH   = gstElecPara.BatVol / 10;//100
            StoreData.EndVL   = gstElecPara.BatVol % 10;//100
            tmpChargeAH = gAllChargeAH / 360000;
            StoreData.ChargeAHH = tmpChargeAH / 256;
            StoreData.ChargeAHL = tmpChargeAH % 256;
            StoreData.ACVoltage = gsPFCUploadData.PFCAcInVolt / 10;
            WriteEEPErrStep = 1;
            SelfCount = 0;
            }
            if( SelfCount > 1000)
            {
                SelfCount = 0;
                WriteEEPErrStep = 1;
            }
        break;
        case 1:
            H_ADDR = ReadByte(1);
            if(H_ADDR != 256)
            {
                SelfCount = 0;
                WriteEEPErrStep = 2;
            }
            if( SelfCount > 1000)
            {
                SelfCount = 0;
                WriteEEPErrStep = 1;
            }
            break;
        case 2:
            SelfCount ++;
            L_ADDR = ReadByte(2);
            if(L_ADDR != 256)
            {
                CountStore = H_ADDR * 256 + L_ADDR;
                CountStore ++;
                gWriteEEPROMStep = 0;
                WriteEEPErrStep = 3;
                SelfCount = 0;
            }
            if( SelfCount > 1000)
            {
                SelfCount = 0;
                gWriteEEPROMStep = 0;
                WriteEEPErrStep = 3;
            }
            break;
        case 3:
            SelfCount ++;
            if(WriteEEPROM())
            {
                WriteEEPErrStep = 4;
                SelfCount = 0;
            }
            if( SelfCount > 1000)
            {
                SelfCount = 0;
                WriteEEPErrStep = 4;
            }
            break;
        case 4:
            SelfCount ++;
            if(WriteByte(12 , StoreData.Month))
                break;
            else
            {
                if(SelfCount > 1000)/*12#��ַ��������ʹ���·�*/
                    break;
             }
            break;
        default:
            break;
    }
    if(4 == WriteEEPErrStep)
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

static Uint16 CheckErrToEEP(void)
{
    Uint16 tmpReturn = 0;
    if(SysErr.bit.HardwareErr)
    {
        tmpReturn = 0;
    }
    else if(SysErr.bit.ACOvp)
    {
        tmpReturn = 1;
    }
    else if(SysErr.bit.ACUvp)
    {
        tmpReturn = 2;
    }
    else if(SysErr.bit.VBatOver)
    {
        tmpReturn = 3;
    }
    else if(SysErr.bit.VBatLack)
    {
        tmpReturn = 4;
    }
    else if(SysErr.bit.BatErr)
    {
        tmpReturn = 5;
    }
    else if(SysErr.bit.IOutOver)
    {
        tmpReturn = 6;
    }
    else if(SysErr.bit.IShort)
    {
        tmpReturn = 7;
    }
    else if(SysErr.bit.VOutOver)
    {
        tmpReturn = 8;
    }
    else if(SysErr.bit.PFCOvp)
    {
        tmpReturn = 9;
    }
    else if(SysErr.bit.PFCUvp)
    {
        tmpReturn = 10;
    }
    else if(SysErr.bit.TempOver)
    {
        tmpReturn = 11;
    }
    else if(SysErr.bit.TempOutOver)
    {
        tmpReturn = 12;
    }
    else if(SysErr.bit.LINErr)
    {
        tmpReturn = 13;
    }
    else if(SysErr.bit.StorageErr)
    {
        tmpReturn = 14;
    }
    return(tmpReturn);
}

/*******************************************************************************
** ��������:    WriteEEPROM
** ��������:    ��һ������д��EEPRom
** ��ʽ����:    ��
** ���ز���:    �Ƿ���д��,1������д�꣬0����δд��
*******************************************************************************/
static Uint16 WriteEEPROM(void)
{
    Uint16 TempAddr = 0,TempData = 256;
    switch(gWriteEEPROMStep)
    {
        case 0:
            if(CountStore >= 2020)//ֻ
            {
                CountStore = 1;
                WriteByte(3,1);      //����������2048ʱ����ʾ�Ѿ�д�����ڵ�ַ3�Ϸ�һ��1��Ϊд����־
            }
            WriteAddr = CountStore * 16;//�����32,768���ֽ� �������256K��λ���ܵ�ַ����  ����һ�����ݾ�Ҫ16���ֽ�
            AddrH = CountStore >>8;    	//���浱ǰһ�����ݵĸ�8λ
            AddrL = CountStore % 256;   //���浱ǰһ�����ݵĵ�8λ
            gWriteEEPROMStep ++;
            //WriteAddr++;
            break;
        case 1:
            if(WriteByte(WriteAddr,StoreData.Year))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 2:
            if(WriteByte(WriteAddr,StoreData.Month))
            {
                TempAddr = WriteAddr;
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 3:
            if(WriteByte(WriteAddr,StoreData.Day))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 4:
            if(WriteByte(WriteAddr,StoreData.Hour))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 5:
            if(WriteByte(WriteAddr,StoreData.Minute))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 6:
            if(WriteByte(WriteAddr,StoreData.CurveNo))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 7:
            if(WriteByte(WriteAddr,StoreData.Event))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 8:
            if(WriteByte(WriteAddr,StoreData.DurationHour))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 9:
            if(WriteByte(WriteAddr,StoreData.DurationMinute))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 10:
            if(WriteByte(WriteAddr,StoreData.StartVH))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 11:
            if(WriteByte(WriteAddr,StoreData.StartVL))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 12:
            if(WriteByte(WriteAddr,StoreData.EndVH))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 13:
            if(WriteByte(WriteAddr,StoreData.EndVL))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 14:
            if(WriteByte(WriteAddr,StoreData.ChargeAHH))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 15:
            if(WriteByte(WriteAddr,StoreData.ChargeAHL))
            {
                WriteAddr++;
                gWriteEEPROMStep ++;
            }
            break;
        case 16:
            if(WriteByte(WriteAddr,StoreData.ACVoltage))
            {
                gWriteEEPROMStep ++;
            }
            break;
        case 17:
            if(WriteByte(1,AddrH))
            {
                gWriteEEPROMStep ++;
            }
            break;
        case 18:
            if(WriteByte(2,AddrL))
            {
                gWriteEEPROMStep ++;
            }
            break;
        case 19:
            if(WriteEEPStartStep > 0)/*this state is startwrite */
            {
                TempData = ReadByte(12);
                if(TempData != 256 &&(13 > TempData)&&(TempData > 0) && (13 > StoreData.Month)&&(StoreData.Month > 0))
                {
                    if(TempData != StoreData.Month)/*this start is new month*/
                        EQ_MFlag = 1;
                }
            }
            if(WriteEEPStopStep > 0)/*this state is ErrWrite*/
            {
                if(WriteByte(12,TempAddr))/*store the month of stop*/
                {
                     gWriteEEPROMStep ++;
                }
            }
            break;
        default:
            break;
    }
    if(gWriteEEPROMStep > 19)
    {
        gWriteEEPROMStep = 0;
        return(1);
    }
    else
    {
        return(0);
    }
}
