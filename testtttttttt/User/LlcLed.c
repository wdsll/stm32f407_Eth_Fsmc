/*******************************************************************************
 * LlcLed.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcMain.h"
extern Uint16 gCurve[21];
extern Uint16 USB_Curve_Flag;
extern Uint16 ReadedFlag;
extern Uint16 Read_Dataiic;
extern Uint16 DeBugTIMECUNT;
extern Uint16 DisplayOff;
extern Uint16 Charge_state;
extern Uint16 gACin_ok;
extern Uint16 gElectricQuantitydsplay;
/*******************************************************************************
**                               �궨��
*******************************************************************************/

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
typedef enum{
    PercentDisplay  = 0x00,
    VoltageDisplay  = 0x01,
    CurrentDisplay  = 0x02,
    BatteryVol      = 0x03,/*TimeDisplay*/
    FullDisplay     = 0x04,
    ACDisplay       = 0x05,
    SoftDisplay     = 0x06,
    CurveDisplay    = 0x07,
}DisplayState;
typedef enum{
    ErrHardware    = 0x00,  // Ӳ������
    ErrVBatLack    = 0x01,  // ���Ƿѹ,���Լ�״̬ʱ�����ص�ѹ����(���ͻ򷴽�)
    ErrACUvp       = 0x02,  // ACǷѹ
    ErrTempOver    = 0x03,  // �����¶ȹ���
    ErrTempOut     = 0x04,  // ����¶ȹ���
    ErrIOutOver    = 0x05,  // ����,��������,�����Ƕ�·
    ErrVBatOver    = 0x06,  // ��ع�ѹ
    ErrIShort      = 0x07,  // �����·
    ErrVOutOver    = 0x08,  // �����ѹ,�����ж�,�����ͻ�
    ErrPFCOvp      = 0x09,  // PFC��ѹ
    ErrPFCUvp      = 0x0a,  // PFCǷѹ
    ErrBattery     = 0x0b,  // �����
    ErrACOvp       = 0x0c,   //   ErrACUvp       = 0x02,  // ACǷѹ
    ErrLINErr      = 0x0d,   // LINͨѶ����
    ErrStorage     = 0x0e,   // LINͨѶ����
    ChargeEnd      = 0x0f,   // ������
}ErrDisplayState;
/*typedef enum{
    ErrHardware    = 0x00,  // Ӳ������
    ErrACOvp       = 0x01,  // AC��ѹ
    ErrACUvp       = 0x02,  // ACǷѹ
    ErrVBatOver    = 0x03,  // ��ع�ѹ
    ErrVBatLack    = 0x04,  // ���Ƿѹ,���Լ�״̬ʱ�����ص�ѹ����(���ͻ򷴽�)
    ErrBattery     = 0x05,
    ErrIOutOver    = 0x06,  // ����,��������,�����Ƕ�·
    ErrIShort      = 0x07,  // �����·
    ErrVOutOver    = 0x08,  // �����ѹ,�����ж�,�����ͻ�
    ErrPFCOvp      = 0x09,  // PFC��ѹ
    ErrPFCUvp      = 0x0a,  // PFCǷѹ
    ErrTempOver    = 0x0b,   // 2���¶��쳣
    ErrTempOut     = 0x0c,   // ��ع���
    ErrLINErr      = 0x0d,   // LINͨѶ����
    ErrStorage     = 0x0e,   // LINͨѶ����
    ChargeEnd      = 0x0f,   // ������
}ErrDisplayState;*/
/*typedef enum{
    ErrHardwareChar    = 0x00,  // Ӳ������
    ErrACOvpChar       = 0x01,  // AC��ѹ
    ErrACUvpChar       = 0x02,  // ACǷѹ
    ErrVBatOverChar    = 0x03,  // ��ع�ѹ
    ErrVBatLackChar    = 0x04,  // ���Ƿѹ,���Լ�״̬ʱ�����ص�ѹ����(���ͻ򷴽�)
    ErrBatteryChar     = 0x05,
    ErrIOutOverChar    = 0x06,  // ����,��������,�����Ƕ�·
    ErrIShortChar      = 0x0a,  // PFCǷѹ
    ErrTempOverCHar    = 0x0b,   // 2���¶��쳣
    ErrTempOutChar     = 0x0c,   // ��ع���
    ErrLINErrCHar      = 0x0d,   // LINͨѶ����
    ErrStorageChar     = 0x0e,   // LINͨѶ����
    ChargeEndCHar      = 0x0f,   // ������
}ErrDisplayChar;*/
static DisplayState gseDisplayState = PercentDisplay;
static ErrDisplayState gsErrDisplayState = ErrHardware;
/*static ErrDisplayChar  gsErrDisplayChar  = ErrHardwareChar;*/

/*******************************************************************************
**                               ��������
*******************************************************************************/
static Uint16 gsDigLedTimeCnt = 0;
static Uint16 gsLedTimeCnt = 0;
static Uint16 gsLedCnt = 0;
static Uint16 gsLedIndex = 0;
Uint16 DisplayStep = 0;
Uint16 DisplayCount = 0;
Uint16 DisplayBattery = 0;
/*0x3F,0*/
/*0x06,1*/
/*0x5B,2*/
/*0x4F,3*/
/*0x66,4*/
/*0x6D,5*/
/*0x7D,6*/
/*0x07,7*/
/*0x7F,8*/
/*0x6F,9*/
/*0x77,A*/
/*0x7C,b*/
/*0x39,C*/
/*0x58,c*/
/*0x5E,d*/
/*0x79,E*/
/*0x71,F*/
/*0x76,H*/
/*0x74,h*/
/*0x38,L*/
/*0x54,n*/
/*0x37,N*/
/*0x5C,o*/
/*0x73,P*/
/*0x50,r*/
/*0x78,t*/
/*0x3E,U*/
/*0x40,-*/
/*0x67,q*/

static const Uint16 gCodeDuan[]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
                                 /*0:0   1:1  2:2  3:3   4:4   5:5   6:6   7:7   8:8   9:9*/
                                 0xBF, 0x86, 0xDB, 0xCF, 0xE6, 0xED, 0xFD, 0x87, 0xFF, 0xEF,
                                 /*h*/
                                 0x79, 0x39, 0x74, 0x77, 0x3E, 0x38 ,0x00, 0x71, 0x31, 0x7C,
                               /*20E   21C   22h   23A   24V   25L   26��   27F    28r   29b  */
                                0x50,  0x5C, 0x76, 0x78, 0x67, 0x73, 0x54, 0x5E};     //74HC595����
                                /*30:r 31:o  32:H  33:t  34:q   35:p  36:n 37:d*/
static const Uint16 gCodeWei[]={0x0e, 0x0d, 0x0b, 0x07, 0xef, 0xdf, 0xbf, 0x7f};   //λ��
typedef enum{
    ACVolstate  = 0x00,
    VoltValue    = 0x01,
    SoftRight   = 0x02,
    Right       = 0x03,
    CurveCharge = 0x04,
    EQ_Mode     = 0x05
}startdisplay;
startdisplay StartDisplay;
Uint16 gSendCounter = 0;
Uint16 gDisplay[4] = {18,18,18,18};
static Uint16 gsGetLedIndex = 0;
extern Uint16 g10miuCounter;
extern Uint16 EQ_MFlag;
extern Uint32 Wait_Time;
extern Uint16 Bat_Voltage;
extern int32 SCONCURCHARGEENDV;
extern int32 CONVOLCHARGECMV;
/*extern Uint16 gsChargeOffFlag;*/
///extern void TestFailManage();
/*******************************************************************************/
static const Uint16 LedTeb[][5] =
{//addLED
    {1,1},                // 0 : ����, Ԥ��
    {15,15,15,15,15},/*{15, 7, 3, 1, 0}, */   // 1 : ����е�������25%, ���״̬(����&���&�ػ�)
    {10,10,10,10,10},       // 2 : BV
    {3, 1, 0},           // 3 : ����е�������50%, ����75%

    {1,0,},             // 4 : ����е�������75%, ����100%
    {0,0,0,0,0},             // 5 : ����״̬, δִ�г��Ĵ���״̬
    {0, 15,},            // 6 : �������ϣ���������
    {14,14,14,14,14},    //7

    {13,13,13,13,13},    //8
    {11,11,11,11,11},    //9
    {7,7,7,7,7},         //10
};
static const Uint16 Led_Count[] = {1,5,4,3,2,2,2};
/*******************************************************************************
**                               ��������
*******************************************************************************/
void LedManage(void);
static void LedNormalDisplay(void);
static void HCDataIn(Uint16 Data);
static void HCDataOut(void);
void Display(Uint16 DigNumOne, Uint16 DigNumTwo, Uint16 DigNumThree, Uint16 DigNumFour);
static void LedCorrectDisplay(void);
static void LedErrDisplay(void);
extern Uint16 Second ;
extern Uint16 Minute ;
extern Uint16 Hour   ;
extern Uint16 ChargeOffFlag;
extern Uint16 ChargeState;
extern Uint16 CHargeState1;
extern Uint16 restart;
extern Uint16 Curve_Max;
static Uint16 Hon_temp=0,Min_temp=0,Second_temp=0,Second5S_Flag=0,Hon_Flag=0;
Uint16 Delta_Min=0;
/*******************************************************************************
** ��������:    LedManage
** ��������:    Led��ʾ�ƿ��Ƴ���
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static Uint16 gsDisplay[4] = {18,18,18,18};
static Uint16 LEDtemp = 256;
Uint16 gCurveNumiic = 0;//iic��������߱����� B124�����ߡ�

void LedManage(void)
{
	static Uint16 temp=1;
    Uint16 Battery_percent=10;
    if(1 == ChargeOffFlag)   //������
    {
       // CLR_KPIN;
        PFC_OFF;             //��PFC�����̻�
    }
    else
    {
        //SET_KPIN;
        PFC_ON;

    }

  if(  gACin_ok == 0)
  {
	  temp= 1;
  }


    if(temp&&gACin_ok)
    {
      temp= 0;
      DisplayStep = 0;

    }
    /////stop/////
    DeBugTIMECUNT--;
   // if((DeBugTIMECUNT > 3)&& (!DisplayOff))//��LED����
    	  if(DeBugTIMECUNT > 3)//��LED����
    {

    	 gSelfState=7;//��LED
    }
    else
    {
    	DeBugTIMECUNT=2;

    }

//    if(ChargeOffFlag == 0)
//    {
//    	 gSelfState=7;//��LED
//    }



    switch(gSelfState)
    {
        case 0:
            LedNormalDisplay();//������ʾ
            DisplayCount = 0;
            break;
        case 1:
        	if((gCurveNum-1) < 22)//�������С��22��
        	{
					if(gCurve[gCurveNum-1] < 100 && gCurve[gCurveNum-1] > 0)           //��ʾ��λ��������
					{
						gsDisplay[0] = 26;    //����ʾ
						gsDisplay[1] = 29;    //��ʾB
						gsDisplay[2] = gCurve[gCurveNum-1]/10;
						gsDisplay[3] = gCurve[gCurveNum-1]%10;
					}
					else if(gCurve[gCurveNum-1] >= 100 && gCurve[gCurveNum-1] <= 254)   //��ʾ��λ��������
					{
						gsDisplay[0] = 29;     //��ʾB
						gsDisplay[1] = gCurve[gCurveNum-1]/100;      //��ʾ1
						gsDisplay[2] = (gCurve[gCurveNum-1]%100)/10;
						gsDisplay[3] = gCurve[gCurveNum-1]%10;
					}
        	}
        	else//����22����ֱ�Ӷ���
        	{
        		if(gCurveNumiic < 100 && gCurve[gCurveNumiic-1] > 0)           //��ʾ��λ��������
				{
					gsDisplay[0] = 26;    //����ʾ
					gsDisplay[1] = 29;    //��ʾB
					gsDisplay[2] = gCurveNumiic/10;
					gsDisplay[3] = gCurveNumiic%10;
				}
				else if(gCurveNumiic >= 100 && gCurveNumiic <= 254)   //��ʾ��λ��������
				{
					gsDisplay[0] =  29;                   //��ʾB
					gsDisplay[1] =  gCurveNumiic/100;      //��ʾ1
					gsDisplay[2] =  gCurveNumiic%100/10;
					gsDisplay[3] =  gCurveNumiic%10;
				}
        	}


                break;
        case 2:
            if(ADRESULT_USBON < 1000)//USB�Ѳ��룿����2
            {
                gsDisplay[0] = 18;
                gsDisplay[1] = 18;
                gsDisplay[2] = 18;
                gsDisplay[3] = 18;

            }
            else
            {
                gsDisplay[0] = 26;//��
                gsDisplay[1] = 26;//��
                gsDisplay[2] = 26;//��
                gsDisplay[3] = 26;//��
             }
            break;
        case 3:/*EQ_Mode*/
            DisplayCount ++;
            if(1 == EQ_MFlag)
            {
                if(DisplayCount%500 > 400)
                {
                    gsDisplay[0] = 26; //��
                    gsDisplay[1] = 26; //��
                    gsDisplay[2] = 20; //E
                    gsDisplay[3] = 34; //Q
                }
                else
                {
                    gsDisplay[0] = 26;//��
                    gsDisplay[1] = 26;//��
                    gsDisplay[2] = 26;//��
                    gsDisplay[3] = 26;//��
                 }
            }
            else if(0 == EQ_MFlag)
            {
                if(DisplayCount%500 > 400)
                {
                    gsDisplay[0] = 26;
                    gsDisplay[1] = 0;
                    gsDisplay[2] = 27;//F
                    gsDisplay[3] = 27;//F
                }

                else
                {
                    gsDisplay[0] = 26;
                    gsDisplay[1] = 26;
                    gsDisplay[2] = 26;
                    gsDisplay[3] = 26;
                }
            }
            if(DisplayCount >=4000)
            {
               DisplayCount = 0;
            }
             break;
        case 4:
        	//	G_CanSend[1] = DisplayStep;
            switch(DisplayStep)//  AC��ʾ
            {
            case ACVolstate:   //AC��ѹ
                gsDisplay[0] = 26;
                gsDisplay[1] = 26;
                gsDisplay[2] = 23; //A
                gsDisplay[3] = 21;  //C
                DisplayCount ++;
                if(DisplayCount > 1000)
                {
                    DisplayCount=0;
                    DisplayStep = VoltValue;
                }
                break;
            case VoltValue:     //ac��ѹ
                gsDisplay[0] = (gsPFCUploadData.PFCAcInVolt)/1000;
                gsDisplay[1] = (gsPFCUploadData.PFCAcInVolt%1000)/100;
                gsDisplay[2] = (gsPFCUploadData.PFCAcInVolt%100)/10 + 10;
                gsDisplay[3] = (gsPFCUploadData.PFCAcInVolt%10);
                DisplayCount ++;
                if(DisplayCount > 1000)
                {
                    DisplayCount=0;
                    DisplayStep = SoftRight;
                }
                break;
            case SoftRight:/*cpu*/
                gsDisplay[0] = 21;  //C
                gsDisplay[1] = 35;  //P
                gsDisplay[2] = 24;  //U
                gsDisplay[3] = YEAR;  //��
                DisplayCount ++;
                if(DisplayCount > 2000)
                {
                    DisplayCount=0;
                    DisplayStep = Right;
                }
                break;
            case Right:

                gsDisplay[0] = MONTH_H;//��ʮλ
                gsDisplay[1] = MONTH_L;//�¸�λ
                gsDisplay[2] = DATE_H; //��ʮλ
                gsDisplay[3] = DATE_L; //�ո�λ
                DisplayCount ++;
/*
                if(1 == DisplayCount)
                    Read_Dataiic = 0;
*/
                 if(DisplayCount > 2000)
                 {
                      DisplayCount=0;
                      DisplayStep = CurveCharge;
                  }
                break;
            case CurveCharge://�������
			if((gCurveNum-1) < 22)//�������С��22��
			 {
						if(gCurve[gCurveNum-1] < 100 && gCurve[gCurveNum-1] > 0)
						{
							gsDisplay[0] = 26;
							gsDisplay[1] = 29;
							gsDisplay[2] = gCurve[gCurveNum-1]/10;
							gsDisplay[3] = gCurve[gCurveNum-1]%10;
						}
						else if(gCurve[gCurveNum-1] >= 100 && gCurve[gCurveNum-1] <= 254)
						{
							gsDisplay[0] = 29;
							gsDisplay[1] = gCurve[gCurveNum-1]/100;
							gsDisplay[2] = (gCurve[gCurveNum-1]%100)/10;
							gsDisplay[3] = gCurve[gCurveNum-1]%10;
						}
			 }
			 else//����22����ֱ�Ӷ���
			 {
					  if(gCurveNumiic < 100 && gCurve[gCurveNumiic-1] > 0)           //��ʾ��λ��������
						{
						 gsDisplay[0] = 26;    //����ʾ
						 gsDisplay[1] = 29;    //��ʾB
						 gsDisplay[2] = gCurveNumiic/10;
						 gsDisplay[3] = gCurveNumiic%10;
						}
						else if(gCurveNumiic >= 100 && gCurveNumiic <= 254)   //��ʾ��λ��������
						{
						 gsDisplay[0] =  29;                   //��ʾB
						 gsDisplay[1] =  gCurveNumiic/100;      //��ʾ1
						 gsDisplay[2] =  gCurveNumiic%100/10;
						 gsDisplay[3] =  gCurveNumiic%10;
					   }
			  }

                DisplayCount ++;
                if(DisplayCount > 2000)
                {
                    DisplayCount=0;
                    ReadedFlag = 1;
                    gSelfState = 0;
                }
                break;
            default:
                break;
            }
            break;
        case 5:  //5???
            DisplayCount ++;/*
            if(1 == DisplayCount)
                Read_Dataiic = 0;*/
            if(DisplayCount%500 > 400)
            {
							if((gCurveNum-1) < 22)//�������С��22��
							 {

									if(gCurve[gCurveNum-1] < 100 && gCurve[gCurveNum-1] > 0)
									{
										gsDisplay[0] = 26; //��
										gsDisplay[1] = 29; //B
										gsDisplay[2] = gCurve[gCurveNum-1]/10;
										gsDisplay[3] = gCurve[gCurveNum-1]%10;
									}
									else if(gCurve[gCurveNum-1] >= 100 && gCurve[gCurveNum-1] <= 254)
									{
										gsDisplay[0] = 29;  //B
										gsDisplay[1] = gCurve[gCurveNum-1]/100;     //1
										gsDisplay[2] = (gCurve[gCurveNum-1]%100)/10;
										gsDisplay[3] = gCurve[gCurveNum-1]%10;
									}
							 }

							 else//����22����ֱ�Ӷ���
							 {
									  if(gCurveNumiic < 100 && gCurve[gCurveNumiic-1] > 0)           //��ʾ��λ��������
										{
										 gsDisplay[0] = 26;    //����ʾ
										 gsDisplay[1] = 29;    //��ʾB
										 gsDisplay[2] = gCurveNumiic/10;
										 gsDisplay[3] = gCurveNumiic%10;
										}
										else if(gCurveNumiic >= 100 && gCurveNumiic <= 254)   //��ʾ��λ��������
										{
										 gsDisplay[0] =  29;                   //��ʾB
										 gsDisplay[1] =  gCurveNumiic/100;      //��ʾ1
										 gsDisplay[2] =  gCurveNumiic%100/10;
										 gsDisplay[3] =  gCurveNumiic%10;
									   }
							  }
            }
            else
            {
                gsDisplay[0] = 26;
                gsDisplay[1] = 26;
                gsDisplay[2] = 26;
                gsDisplay[3] = 26;
            }
            if(DisplayCount > 10000)
            {
                DisplayCount = 0;
                gSelfState = 0;
            }
            break;
        case 6:
            if(Bat_Voltage < 233)
                Battery_percent = 10;
            else if(Bat_Voltage < 236)
                Battery_percent = 20;
            else if(Bat_Voltage < 239)
                Battery_percent = 30;
            else if(Bat_Voltage < 242)
                Battery_percent = 40;
            else if(Bat_Voltage < 245)
                Battery_percent = 50;
            else if(Bat_Voltage < 247)
                Battery_percent = 60;
            else if(Bat_Voltage < 250)
                Battery_percent = 70;
            else if(Bat_Voltage < 252)
                Battery_percent = 80;
            else if(Bat_Voltage < 255)
                Battery_percent = 90;
            else
                Battery_percent = 100;
            if(DisplayBattery == 1)
            {
                LEDtemp = Bat_Voltage;   //��ص�ѹ
            }
            if(1 == DisplayOff)
            {
					DisplayBattery ++;
					if(DisplayBattery < 3000)
				   {
						gsGetLedIndex = LedTeb[2][gsLedCnt];
						gsDisplay[0] = LEDtemp / 100;                 /*Bat_Voltage / 100;*/
						gsDisplay[1] = ((LEDtemp % 100) / 10 ) + 10;  /*(Bat_Voltage % 100 / 10) + 10;*/
						gsDisplay[2] = LEDtemp % 10;                  /*Bat_Voltage % 10;*/
						gsDisplay[3] = 24;                            //V
				   }
				   else if(DisplayBattery < 5000)
				   {
					   gsGetLedIndex = LedTeb[10][gsLedCnt];
					   if(1 == Battery_percent/100)
						   gsDisplay[0] = Battery_percent/100;
					   else
						   gsDisplay[0] = 26;
					   gsDisplay[1] = (Battery_percent%100)/10;
					   gsDisplay[2] = Battery_percent%10 + 10;
					   gsDisplay[3] = 0;
				   }
				   else
				   {
					   gsGetLedIndex = LedTeb[1][gsLedCnt];
					   gsDisplay[0] = 26;
					   gsDisplay[1] = 26;
					   gsDisplay[2] = 26;
					   gsDisplay[3] = 26;
				   }
					if(DisplayBattery > 5000)//5000
					{
						DisplayOff = 0;
						DisplayBattery = 0;
					}
            }
            else
            {
                gsGetLedIndex = LedTeb[1][gsLedCnt];//cyp
                gsDisplay[0] = 26;
                gsDisplay[1] = 26;
                gsDisplay[2] = 26;
                gsDisplay[3] = 26;
            }
            break;
        case 7:
           {
            gsGetLedIndex = LedTeb[1][gsLedCnt];//cyp
            gsDisplay[0] = 26;//��
            gsDisplay[1] = 26;//��
            gsDisplay[2] = 26;//��
            gsDisplay[3] = 26;//��
           }
            break;
        case 8://????
            DisplayCount ++;
            if((DisplayCount%2000 > 1000)&&(0 == USBON))//û��USB����ʾ����
            {
									if((gCurveNum-1) < 22)//�������С��22��
									 {
											if(gCurve[gCurveNum-1] < 100 && gCurve[gCurveNum-1] > 0)
											{
												gsDisplay[0] = 26;
												gsDisplay[1] = 29; //b
												gsDisplay[2] = gCurve[gCurveNum-1]/10;
												gsDisplay[3] = gCurve[gCurveNum-1]%10;
											}
											else if(gCurve[gCurveNum-1] >= 100 && gCurve[gCurveNum-1] <= 254)
											{
												gsDisplay[0] = 29; //b
												gsDisplay[1] = gCurve[gCurveNum-1]/100;    //1
												gsDisplay[2] = (gCurve[gCurveNum-1]%100)/10;
												gsDisplay[3] = gCurve[gCurveNum-1]%10;
											}
									}
									 else//����22����ֱ�Ӷ���
									 {
										   if(gCurveNumiic < 100 && gCurve[gCurveNumiic-1] > 0)           //��ʾ��λ��������
											{
											 gsDisplay[0] = 26;    //����ʾ
											 gsDisplay[1] = 29;    //��ʾB
											 gsDisplay[2] = gCurveNumiic/10;
											 gsDisplay[3] = gCurveNumiic%10;
											}
											else if(gCurveNumiic >= 100 && gCurveNumiic <= 254)   //��ʾ��λ��������
											{
											 gsDisplay[0] =  29;                   //��ʾB
											 gsDisplay[1] =  gCurveNumiic/100;      //��ʾ1
											 gsDisplay[2] =  gCurveNumiic%100/10;
											 gsDisplay[3] =  gCurveNumiic%10;
											}
									 }
            }
            else if(1 == USBON && 0 == USB_Curve_Flag) //USB��ʾdon
            {
                gsDisplay[0] = 26;//��
                gsDisplay[1] = 37;//d
                gsDisplay[2] = 31;//o;
                gsDisplay[3] = 36;//n;

             }
            else
            {
                gsDisplay[0] = 26;
                gsDisplay[1] = 26;
                gsDisplay[2] = 26;
                gsDisplay[3] = 26;
            }
            if(DisplayCount > 2000)
            {
                DisplayCount = 0;
                USB_Curve_Flag = 0;
                gSelfState = 0;
            }
            break;


        default:
            break;
    }
    Display(gsDisplay[0], gsDisplay[1], gsDisplay[2], gsDisplay[3]);
}
/*******************************************************************************
** ��������:    LedNormalDisplay
** ��������:    Led����������ʾ
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void LedNormalDisplay(void)
{
    Uint16 tmpErrNum = 0;
    gsDigLedTimeCnt++;
    gsLedTimeCnt++;
    if(gsLedTimeCnt > 500)
    {
        gsLedTimeCnt = 0;
        gsLedIndex = GetLedIndex();//�յ�����״̬���ٷ�����ô��ʾ
        ///////2020 0825 start//////5s shanyici
        if((1 == Second5S_Flag || Min_temp > 720) && 1 == ChargeOffFlag)
        {
            gsLedIndex = 5;
        }
        /////////stop////////////
        /*gsGetLedIndex = LedTeb[gsLedIndex][gsLedCnt];*/
        if(1 == Charge_state)
        {
            if(0 == gseDisplayState)
                gsGetLedIndex = LedTeb[10][gsLedCnt];
            if(1 == gseDisplayState)
                gsGetLedIndex = LedTeb[9][gsLedCnt];
            if(2 == gseDisplayState)
                gsGetLedIndex = LedTeb[8][gsLedCnt];
        }
        gsLedCnt ++;
        if(gsLedCnt >= Led_Count[gsLedIndex])
        {
            gsLedCnt = 0;
        }
    }
    if(gsDigLedTimeCnt  >  999)/*2000*/
    {
       if(gsDigLedTimeCnt > 2000)
        {
            if(gsLedIndex != 6)
            {
                gsErrDisplayState = ErrHardware;
                LedCorrectDisplay();   //������ʾ
            }
            gsDigLedTimeCnt = 0;
        }
            if(gsLedIndex != 6)
            {
/*                gsErrDisplayState = ErrHardware;
                LedCorrectDisplay();*/
            }
            else
            {
            
                gseDisplayState = PercentDisplay;
                tmpErrNum = (SysErr.bit.HardwareErr) + (SysErr.bit.ACOvp) + (SysErr.bit.ACUvp) + (SysErr.bit.VBatOver)
                        + (SysErr.bit.VBatLack) + (SysErr.bit.BatErr) + (SysErr.bit.IOutOver) + (SysErr.bit.IShort)
                        + (SysErr.bit.PFCOvp) + (SysErr.bit.PFCUvp) + (SysErr.bit.TempOver) + (SysErr.bit.TempOutOver)
                        + (SysErr.bit.LINErr) + (SysErr.bit.StorageErr);/* + ChargeOffFlag; *//*this is old right show she charge off*/
                if(1 != tmpErrNum && 1888 == gsDigLedTimeCnt)
                {
                    gsErrDisplayState ++;
                }
                LedErrDisplay();//������ȡ
                if(gsDigLedTimeCnt == 1000)
                {
                    gsGetLedIndex = LedTeb[5][gsLedCnt];
                    gsDisplay[0] = 26;
                    gsDisplay[1] = 20; //20E
                    if(gsErrDisplayState==12||gsErrDisplayState==10||gsErrDisplayState==9)gsErrDisplayState = 2;
                    if(gsErrDisplayState==8)gsErrDisplayState = 6;
                    if(gsErrDisplayState==14)gsErrDisplayState = 13;
                    if(gsErrDisplayState > 9)       //������
                    {
                       gsDisplay[2] = 1;
                       gsDisplay[3] = gsErrDisplayState - 10;
                    }
                    else
                    {
                        gsDisplay[2] = 0;
                        gsDisplay[3] = gsErrDisplayState;
                    }
                }
                else if(gsDigLedTimeCnt == 1599)
                {
                    gsGetLedIndex = LedTeb[1][gsLedCnt];
                    if(1 == gsErrDisplayState ||      //1���Ƿѹ,���Լ�״̬ʱ�����ص�ѹ����(���ͻ򷴽�)
                    		6 == gsErrDisplayState ||  // ��ع�ѹ
							4 == gsErrDisplayState)     // ����¶ȹ���
                    {

                        gsDisplay[0] = 26;  //��
                        gsDisplay[1] = 29;  //b
                        gsDisplay[2] = 23;  //A
                        gsDisplay[3] = 33;  //t
                    }
                    else if(2 == gsErrDisplayState || // ACǷѹ
                    		9 == gsErrDisplayState ||
							10 == gsErrDisplayState ||
							12 == gsErrDisplayState)  // AC��ѹ
                    {
                        gsDisplay[0] = 26;
                        gsDisplay[1] = 26;
                        gsDisplay[2] = 23; //A
                        gsDisplay[3] = 21; //C

                    }
                    else if( 3 == gsErrDisplayState )
                    {
                        gsDisplay[0] = 26;
                        gsDisplay[1] = 32; //H
                        gsDisplay[2] = 31; //O
                        gsDisplay[3] = 33; //t



                    }
                    else if(5 == gsErrDisplayState )
                    {
                        gsDisplay[0] = 26;
                        gsDisplay[1] = 20;//E
                        gsDisplay[2] = 30;//r
                        gsDisplay[3] = 30;//r


                    }
                    else if(13 == gsErrDisplayState)
                    {
                        gsDisplay[0] = 26;
                        gsDisplay[1] = 21;//C
                        gsDisplay[2] = 31;//o
                        gsDisplay[3] = 36;//n

                    }
                }
                if(gsDigLedTimeCnt > 2000)
                {
                    gsDigLedTimeCnt = 0;
                }
            }
    }
}

///////��ȷ�³����ʾ/////
static void LedCorrectDisplay(void)
{
    Uint16 tmpHour = 0;
    Uint16 Battery_percent=10;
    gseDisplayState++;
    if(gseDisplayState > CurrentDisplay)/*BatteryVol)*/
    {
        gseDisplayState = PercentDisplay;
    }


    ///////////////20200823 start///
    if(1 == ChargeOffFlag)//////C:21  E:20 113
    {
        PFC_OFF;
    }
    else
    {
        PFC_ON;
    }

    //PFC_OFF
    ////////////////20200823 stop///////////
    if(
    		(gElectricQuantitydsplay > 3 )//����׶�4������
			&&(gstElecPara.OutCur < 50)//����С��0.5Aʱ
			&&(SysErr.all == 0)//û�д���
			)
    {
 	   gseDisplayState = PercentDisplay;//ֻ��ʾ100%
    }

    switch(gseDisplayState)
    {

        case PercentDisplay:
            if(1 == ChargeOffFlag && Bat_Voltage > 254)
                Battery_percent = 100;
            else if(1 == Charge_state)   //���̬��ѹ�ٷֱ�
             {

							if(Bat_Voltage < 240)
								Battery_percent = 10;

							else if(Bat_Voltage < 250)
								Battery_percent = 20;
							else if(gElectricQuantitydsplay > 4)//���������ֱ����ʾ100%
								{
								Battery_percent = 100;
								}
							else if(Bat_Voltage < 260)
								Battery_percent = 30;
							else if(Bat_Voltage < 270)
								Battery_percent = 40;
							else if(Bat_Voltage < 275)
							{

									Battery_percent = 50;

							}
							else if(Bat_Voltage < 280)
							{

									Battery_percent = 60;

							}
							else if(Bat_Voltage < 283)
							{

									Battery_percent = 70;
							}
							else if(Bat_Voltage < 294)//300
							{

									 Battery_percent = 80;
							}
							else if(Bat_Voltage < 320) //320
							{
								 Battery_percent = 90;
							}
							else
								Battery_percent = 100;

			}

            else                          //�ǳ��̬��ѹ�ٷֱ�
            {
                if(Bat_Voltage < 233)
                    Battery_percent = 10;
                else if(Bat_Voltage < 236)
                    Battery_percent = 20;
                else if(Bat_Voltage < 239)
                    Battery_percent = 30;
                else if(Bat_Voltage < 242)
                    Battery_percent = 40;
                else if(Bat_Voltage < 245)
                    Battery_percent = 50;
                else if(Bat_Voltage < 247)
                    Battery_percent = 60;
                else if(Bat_Voltage < 250)
                    Battery_percent = 70;
                else if(Bat_Voltage < 252)
                    Battery_percent = 80;
                else if(Bat_Voltage < 255)
                    Battery_percent = 90;
                else
                    Battery_percent = 100;
            }
            ///�����껹Ҫ�ſ�
            if(1 == Battery_percent/100)
            gsDisplay[0] = Battery_percent/100;
            else
            gsDisplay[0] = 26;
            gsDisplay[1] = (Battery_percent%100)/10;
            gsDisplay[2] = Battery_percent%10 + 10;
            gsDisplay[3] = 0;

//            gsDisplay[0] = gsPFCUploadData.PFCAcInVolt / 1000;
//            gsDisplay[1] = gsPFCUploadData.PFCAcInVolt % 1000 / 100;
//            gsDisplay[2] = gsPFCUploadData.PFCAcInVolt % 100 / 10;
//            gsDisplay[3] = gsPFCUploadData.PFCAcInVolt % 10  ;



            break;
        case VoltageDisplay: //�����ѹ
            gsDisplay[0] = gstElecPara.OutVol / 100;
            gsDisplay[1] = (gstElecPara.OutVol % 100 / 10) + 10;
            gsDisplay[2] = gstElecPara.OutVol % 10;
            gsDisplay[3] = 24;  //V
            break;
        case CurrentDisplay:///�������

            if(gstElecPara.OutCur >= 1000)
            {
                gsDisplay[0] = gstElecPara.OutCur / 1000;
                gsDisplay[1] = (gstElecPara.OutCur % 1000 / 100) + 10;
                gsDisplay[2] = gstElecPara.OutCur % 100 / 10;
                gsDisplay[3] = 23; //A
            }
            else
            {
                gsDisplay[0] = gstElecPara.OutCur / 100 + 10;
                gsDisplay[1] = (gstElecPara.OutCur % 100 / 10);
                gsDisplay[2] = gstElecPara.OutCur % 10;
                gsDisplay[3] = 23; //A
            }
            /*if(SampleData.DataOutTemp > 0)
            {
                gsDisplay[0] = 26;
                gsDisplay[1] = (SampleData.DataOutTemp/ 10);
                gsDisplay[2] = SampleData.DataOutTemp % 10;
                gsDisplay[3] = 26;
            }
            else
            {
                gsDisplay[0] = 0;
                gsDisplay[1] = (0 - SampleData.DataOutTemp)/ 10;
                gsDisplay[2] = (0 - SampleData.DataOutTemp) % 10;
                gsDisplay[3] = 26;
            }*/
            break;
        case  FullDisplay:////��ʾOVER ����FULL
                //////////stop/////////////
                break;
        case ACDisplay:
            break;
        case SoftDisplay:
            break;
        case CurveDisplay:
            break;

        default:
            break;
    }
}


static void LedErrDisplay(void)
{
    switch(gsErrDisplayState)
    {
        case ErrHardware:
            if((SysErr.bit.HardwareErr))
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrVBatLack:
            if(SysErr.bit.VBatLack)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;//1
            }
        case ErrACUvp:
            if(SysErr.bit.ACUvp)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrTempOver:
            if(SysErr.bit.TempOver)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrTempOut:
            if(SysErr.bit.TempOutOver)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrIOutOver:
            if(SysErr.bit.IOutOver)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrVBatOver:
            if(SysErr.bit.VBatOver)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrIShort:
            if(SysErr.bit.IShort)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrVOutOver:
            if(SysErr.bit.VOutOver)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrPFCOvp:
            if(SysErr.bit.PFCOvp)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrPFCUvp:
            if(SysErr.bit.PFCUvp)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrBattery:
            if(SysErr.bit.BatErr)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrACOvp:
            if(SysErr.bit.ACOvp)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrLINErr:
            if(SysErr.bit.LINErr)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ErrStorage:
            if(SysErr.bit.StorageErr)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        case ChargeEnd:
            if(ChargeOffFlag)
            {
                break;
            }
            else
            {
                gsErrDisplayState ++;
            }
        default:
            gsErrDisplayState = ErrHardware;
            LedErrDisplay();
            break;
    }
}

/*******************************************************************************
** ��������:    HCDataIn
** ��������:    74HC595���������룬�ȷ�λ�루�ڼ�λ��ʾ�����ٷ�����
** ��ʽ����:    һ������
** ���ز���:    ��
*******************************************************************************/
static void HCDataIn(Uint16 Data)
{
    Uint16 tmpCount;
    for(tmpCount = 0; tmpCount < 8; tmpCount ++)
    {
        if(0x80 == (Data & 0x80))
        {
            SET_SER;
        }
        else
        {
            RST_SER;
        }
        Data =  Data << 1;
        RST_SRCK;
        SET_SRCK;
    }
}

/*******************************************************************************
** ��������:    HCDataOut
** ��������:    74HC595��������ʾ����HCDataIn���������ʾ
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void HCDataOut(void)
{
    RST_RCK;
    SET_RCK;
}

/*******************************************************************************
** ��������:    Display
** ��������:    �������ʾ
** ��ʽ����:    ��Ҫ��ʾ���ĸ�����
** ���ز���:    ��
*******************************************************************************/
void Display(Uint16 DigNumOne, Uint16 DigNumTwo, Uint16 DigNumThree, Uint16 DigNumFour)
{
    HCDataIn(( gsGetLedIndex << 4) + gCodeWei[gSendCounter]);
    HCDataIn(gCodeDuan[gDisplay[gSendCounter]]);
    HCDataOut();
    gSendCounter ++;
    if(gSendCounter > 3)
    {
        gSendCounter = 0;
        gDisplay[0] = DigNumOne;
        gDisplay[1] = DigNumTwo;
        gDisplay[2] = DigNumThree;
        gDisplay[3] = DigNumFour;
    }
}
