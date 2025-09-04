/*******************************************************************************
 * LlcSys.c
 * Dedicated
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcMain.h"

#include "eeprom_24xx.h"
/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define POWER_MAX  900000        ///900000   //ϵͳ��������,
#define IOUT_MAX   3600          //3000      36A
#define VOUT_MAX   350

#define INIT_CHARGE_DATA {0, 0, 0, 0, 0, 0, 0, StepOne, eLlcSysCmdOff}

//#define AC_DERATING_VOL 1000
#define AC_PROTECT_VOL  850  //CYP������ѹ 85V
#define AC_DERATING_VOL 1150 //CYP�����ѹ 100  115  900W/0.83/115v=9.4A

#define TEMP_DERATING   2446//70   2689   //80;  2932//CYP 90��
#define TEMP_PROTECT    2689//80   2932   //90;  3175//CYP100��
extern Uint16 USB_Curve_Flag;
extern Uint16 Read_Dataiic;
Uint16 gElectricQuantitydsplay;
Uint16 gACin_ok = 1;
Uint16 ACinputFlag = 1;
Uint16 g_16data1[24]={0,0,0,0,0,0,0,0,
					  0,0,0,0,0,0,0,0,
					  0,0,0,0,0,0,0,0};
extern Uint16 ee_ReadWord(Uint16 *_pReadBuf, Uint16 _usAddress, Uint16 _usSize);
//#define TEMP_DERATING   2478
//#define TEMP_PROTECT    2792

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
typedef enum{
    SelfTest_State   = 0x00,
    StandBy_State    = 0x01,
    PulseOn_State    = 0x02,
    ChargeOn_State   = 0x03,
    Protection_State = 0x04,//����
    ShutDown_State   = 0x05,
    USBShutDown_State = 0x06,
}SystemState;
static SystemState gseSystemState;

typedef enum{
    StepOne   = 0x00,
    StepTwo   = 0x01,
    StepThree = 0x02,
    StepFour  = 0x03,
    StepFive  = 0x04,
}StateStep;
static StateStep gseStateStep = StepOne;

Uint16 Switch_Step = StepOne;
static Uint16 gsUSBShutDownFlag = 0;

struct stCHARGESTATPARAM {
    Uint16 ChargeMode;            // ���������׶�ģʽ
    int16  ChargeCmdV;            // ����Ŀ���ѹֵ���ɳ�����߾���
    int16  ChargeCmdI;            // ����Ŀ�����ֵ���ɳ�����߾���
    int16  ChargeSetI;            // ����Ŀ�����ֵ���ɳ�����߾�����min(�������ƣ�CMDI)
    Uint16 ChargeStartFlag;       // ���׶γ��ͣ����־��׼�����븡��
    Uint16 ChargeOffFlag;         // ���׶γ��ͣ����־��׼�����븡��
    Uint32 ChargeCnt;
    StateStep ChargeStat;
    eLlcSysCmd_t ChargeCmdOn;
};
static struct stCHARGESTATPARAM ChargeStateParam = INIT_CHARGE_DATA;

struct sElecPara gstElecPara;
stSetLlc_PARAM gstSetLlcPARAM;
/*******************************************************************************
**                               ��������
*******************************************************************************/
static Uint16 gsStateDelay = 0;
static Uint16 gsStateOver = 1;//0
static Uint16 gsStateCnt = 0;
static Uint16 gsChargeCnt = 0;
static Uint16 gsVolBatCompVal = 0;
static Uint16 gsChargeOffFlag = 0;
static Uint16 Chargerestart = 0;
Uint16 ChargeOffFlag=0;
Uint16 ChargeState = 0;
Uint16 CHargeState1 =0;
Uint16 EQ_MFlag = 0;  /*testmode EQ_MFlag = 1*/               //EQ ģʽ����dc
Uint16 ReadedFlag = 0;
Uint16 Key1s_Flag = 0;
Uint16 Key5s_Flag = 0;
Uint16 gCorrectionDate[8];
Uint16 u16Vout_K = 10216;     //��ѹϵ��K981;965
Uint16 s16Vout_B = 37725;     //��ѹϵ��B

Uint16 u16BatVol_K = 10232;    //���ϵ��K981;225  10132
Uint16 s16BatVol_B = 36219;     //���ϵ��B

Uint16 u16IOUT_K;     //����ϵ��K
Uint16 s16IOUT_B;     //����ϵ��B


static int16 gsSelfCounter = 0;
static Uint32 gsSwitchCounter = 0;

//Uint16 gCurve[22] = {121,71,156,199/* �񽡵��*/
//                     ,100,101,102,103/*��ʿ���*/
//                     ,104/*�񽡵��*/
//                     ,94,56,132,129,214,11/*Ʒ�ƣ��ɱ��� 214*/
//                     ,202,105,106,107,108,109/*Ʒ�ƣ�FULLRIVER  */
//					 ,110};/*����һ��*/
Uint16 gCurve[22] = {121,71,156,199/* �񽡵��*/
                     ,02,07/* �񽡵��*/,102,103/*��ʿ���*/
                     ,104/*�񽡵��*/
                     ,94,56,132,129,214,11/*Ʒ�ƣ��ɱ��� 214*/
                     ,202,105,106,201,100/*�������¼�*/,109/*Ʒ�ƣ�FULLRIVER  */
					 ,110};/*����һ��*/
Uint16 gCurveNum = 1;
Uint16 Curve_Max = 23;
Uint16 gSelfState = 0;
Uint16 DisplayOff = 0;

extern Uint16 restart;
extern Uint16 Read_Dataiic;
extern Uint16 Bat_Voltage;
//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��  0
int32 PRECHARGECMI = 0;//PRECHARGECMI_1;                                 //ָ�����0.05C
int32 PRECHARGECMV = 0;//PRECHARGECMV_1;                                   //ָ���ѹ����ܵ�ѹ
int32 PRECHARGEENDV = 0;//PRECHARGEENDV_1 ;                                //Ԥ���������ܵ�ѹ
int32 PRECHARGEENDCNT =0;// PRECHARGEENDCNT_1;                                 //Ԥ�����ʱ��2H
//�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������1
int32 CONCURCHARGECMI =0;// CONCURCHARGECMI_1;                                //ָ�����0.167C
int32 CONCURCHARGECMV =0;// CONCURCHARGECMV_1;                                //ָ���ѹ����ܵ�ѹ
int32 CONCURCHARGEENDI = 0;//CONCURCHARGEENDI_1;
int32 CONCURCHARGEENDV = 0;//CONCURCHARGEENDV_1 ;                       //������������ܵ�ѹ
int32 CONCURCHARGEENDCNT =0;// CONCURCHARGEENDCNT_1;
//���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��2
int32 SCONCURCHARGECMI =0;//SCONCURCHARGECMI_1 ;                                //ָ�����0.1C
int32 SCONCURCHARGECMV =0;//SCONCURCHARGECMV_1 ;                               //ָ���ѹ����ܵ�ѹ
int32 SCONCURCHARGEENDV = 0;//SCONCURCHARGEENDV_1;                         //������������ܵ�ѹ
int32 SCONCURCHARGEENDCNT =0;//SCONCURCHARGEENDCNT_1;                      //��������ʱ��2H
//��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��3
int32 CONVOLCHARGECMI = 0;//CONVOLCHARGECMI_1;                                 //ָ�����0.06C
int32 CONVOLCHARGECMV = 0;//CONVOLCHARGECMV_1;                               //ָ���ѹ����ܵ�ѹ
int32 CONVOLCHARGEENDI = 0;//CONVOLCHARGEENDI_1;                                    //��ѹ��������ܵ���
int32 CONVOLCHARGEENDCNT = 0;//CONVOLCHARGEENDCNT_1;                        //��ѹ����ʱ��2H
//������׶γ����Ϣ��������ѹָ���תʱ��4
int32 BCONVOLCHARGECMI =0;//BCONVOLCHARGECMI_1 ;                                   //ָ�����0.01C
int32 BCONVOLCHARGECMV = 0;//BCONVOLCHARGECMV_1;                               //ָ���ѹ����ܵ�ѹ
int32 BCONVOLCHARGEENDV =0;//BCONVOLCHARGEENDV_1 ;                          //������������ܵ�ѹ
int32 BCONVOLCHARGEENDI =0;
int32 BCONVOLCHARGEENDCNT = 0;//BCONVOLCHARGEENDCNT_1;            //��ѹ����ʱ��2H
//����׶γ����Ϣ��������ѹָ���תʱ��5
int32 FLOATCHARGECMI =0;// FLOATCHARGECMI_1;                                   //ָ�����0.01C
int32 FLOATCHARGECMV =0;//FLOATCHARGECMV_1 ;                                   //ָ���ѹ����ܵ�ѹ
int32 FLOATCHARGEENDCNT = 0;//FLOATCHARGECMV_1;                                        //Ԥ�����ʱ��4H
/*EQ�׶�*/
int32 EQCHARGECMI  = 0;//EQCurrCMD_1;
int32 EQCHARGECMV  = 0;//EQVoltCMD_1;
int32 EQCHARGETIME = 0;//EQTimeCMD_1;

//���ߵ�ַ 32321 - 32601
///////////////U���������߱���
Uint16  gPreChargeComi1;                                  //Ԥ��ָ�����0.05C  ��Ϊ��0.01A
Uint16  gPreChargeComv1;                                  //ָ���ѹ����ܵ�ѹ
Uint16  gPreChargeEndv1;                                  //Ԥ���������ܵ�ѹ
Uint16  gPreChargeEndcnt1;                                 //Ԥ�����ʱ��2H
		   //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
Uint16  gConstantChargeComi1;                             //ָ�����0.167C
Uint16  gConstantChargeComv1;                                 //ָ���ѹ����ܵ�ѹ
Uint16  gConstantChargeEndi1;
Uint16  gConstantChargeEndv1;                       //������������ܵ�ѹ
Uint16  gConstantChargeEndcnt1;
		   //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
Uint16  gConstant2ChargeComi1;                                //ָ�����0.1C
Uint16  gConstant2ChargeComv1;                               //ָ���ѹ����ܵ�ѹ
Uint16  gConstant2ChargeEndv1;                          //������������ܵ�ѹ
Uint16  gConstant2ChargeEndcnt1;                                        //��������ʱ��2H
		   //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
Uint16  gConstantVoltageChargeComi1;                                 //ָ�����0.06C
Uint16  gConstantVoltageChargeComv1;                               //ָ���ѹ����ܵ�ѹ
Uint16  gConstantVoltageChargeEndi1;                                   //��ѹ��������ܵ���
Uint16  gConstantVoltageChargeEndcnt1;                                       //��ѹ����ʱ��2H
		   //������׶γ����Ϣ��������ѹָ���תʱ��
Uint16  gTrickleChargeComi1;                                    //ָ�����0.01C
Uint16  gTrickleChargeComv1;                              //ָ���ѹ����ܵ�ѹ
Uint16  gTrickleChargeEndv1;             //������������ܵ�ѹ
Uint16  gTrickleChargeEndcnt1;                                         //��ѹ����ʱ��2H
		   //����׶γ����Ϣ��������ѹָ���תʱ��
Uint16  gFloatChargeComi1;                                  //ָ�����0.01C
Uint16  gFloatChargeComv1;                                   //ָ���ѹ����ܵ�ѹ
Uint16  gFloatChargeEndcnt1;                               //Ԥ�����ʱ��4H
		   ///EQ���׶���Ϣ������ʱ���ѹ����
Uint16  gEQCurrCMD1;
Uint16  gEQVoltCMD1;
Uint16  gEQTimeCMD1;







/*******************************************************************************
**                               ��������
*******************************************************************************/
void InitSysPara(void);
void ElecParaCal(void);
void SystemManage(void);

static void SysTimer(void);
static void ReFreshCurve(void);
static void SelfTestState(void);
static void StandByState(void);
static void PulseOnState(void);
static void ChargeOnState(void);
static void ProtectionState(void);
static void ShutDownState(void);
static void USBShutDownState(void);

static void TestFlashManage(void);
static void TestFailManage(void);

static void SelectCurve(void);
static void ChargeStartStateTest(void);
extern Uint16 USBReadByteS(Uint16 addr, Uint16 length);
extern Uint16 USBReadByteD(Uint16 addr, Uint16 *g_16data1, Uint16 length);
extern Uint16 ee_ReadWord(Uint16 *_pReadBuf, Uint16 _usAddress, Uint16 _usSize);
extern Uint16 gCurveNumiic;//iic��������߱����� B124������
static void PreChargeManage(void);
static void ConCurChargeManage(void);
static void SConCurChargeManage(void);
static void ConVolChargeManage(void);
static void BConVolChargeManage(void);
static void FloatChargeManage(void);
static void EQChargeManage(void);
static void PAUSEChargeManage(void);
static int32 InputPowerLimiting(int16 InputFeed);
static int16 TempPowerLimiting(int16 TempFeed);
static int32 PowerLimiting(void);
static int16 RenewCurrentLmiting();
//static Uint8 stempdata[56];
static Uint16 stempdata[56];
Uint16 GetLedIndex(void);

//��������ߵ�������

void ReadExteCurve(void)
{
	static  Uint16   temp;
	static  Uint16   saddress;
	static  Uint16   ShutDownFlag = 1;
	static  Uint16   gCurveNumper = 1;


//	if ((gCurveNum >= 23)&&(ShutDownFlag)&&(gCurveNumper != gCurveNum))//���ߴ���22���Һ��ϴ�������һ����
//	if ((gCurveNum >= 23)&&(gCurveNumper != gCurveNum))//���ߴ���22���Һ��ϴ�������һ����
//	{
//		GoToUSBShutDown();//��LLC�ص�
//	}
//	G_CanSend[0] = gCurveNum;
	//G_CanSend[6] = gCurveNumiic;
	//gCurveNumper = gCurveNum; //�����ϴ�����
//	if(gCurveNum==23)
//		{
//		temp = CURVEEXTE1;
//		}
//	else if(gCurveNum==24)
//		{
//		temp = CURVEEXTE2;
//		}
//	else if(gCurveNum==25)
//		{
//		temp = CURVEEXTE3;
//		}
//	else if(gCurveNum==26)
//		{
//		temp = CURVEEXTE4;
//		}
//	else if(gCurveNum==27)
//		{
//		temp = CURVEEXTE5;
//		}
	//USBReadByteS(temp, 54);  //������������

//	if(ee_ReadByte(stempdata, CURVEEXTE5, 56))//���ɹ�56
//	{

		saddress = (stempdata[2]-48)*78 + stempdata[3]-48+32000;   // 32000��ƫ����
//	   if(saddress==temp)//��ַ��ͬ��������
//	   {
//	    GoToUSBChargeOn();    //����󿪻�����
//	    ShutDownFlag = 0;     //0��Ҫ�ػ���
		gCurveNumiic = (stempdata[0]-48)*78 + stempdata[1]-48;    //��������߱�����
		gPreChargeComi1 = (stempdata[4]-48)*78 + stempdata[5]-48;     //Ԥ��ָ�����0.05C  ��Ϊ��0.01A
		gPreChargeComv1 = (stempdata[6]-48)*78 + stempdata[7]-48;        //ָ���ѹ����ܵ�ѹ
		gPreChargeEndv1 = (stempdata[8]-48)*78 + stempdata[9]-48;                                   //Ԥ���������ܵ�ѹ
		gPreChargeEndcnt1 = (stempdata[10]-48)*78 + stempdata[11]-48;                                  //Ԥ�����ʱ��2H
				   //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
		 gConstantChargeComi1 = (stempdata[12]-48)*78 + stempdata[13]-48;                             //ָ�����0.167C
		 gConstantChargeComv1 = (stempdata[14]-48)*78 + stempdata[15]-48;              //ָ���ѹ����ܵ�ѹ
		// gConstantChargeEndi1 = (stempdata[12]-48)*78 + stempdata[13]-48;
		 gConstantChargeEndv1 = (stempdata[16]-48)*78 + stempdata[17]-48;                        //������������ܵ�ѹ
		 gConstantChargeEndcnt1 = (stempdata[18]-48)*78 + stempdata[19]-48;
				   //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
		 gConstant2ChargeComi1 = (stempdata[20]-48)*78 + stempdata[21]-48;                                  //ָ�����0.1C
		 gConstant2ChargeComv1 = (stempdata[22]-48)*78 + stempdata[23]-48;                                //ָ���ѹ����ܵ�ѹ
		 gConstant2ChargeEndv1 = (stempdata[24]-48)*78 + stempdata[25]-48;                           //������������ܵ�ѹ
		 gConstant2ChargeEndcnt1 = (stempdata[26]-48)*78 + stempdata[27]-48;                                          //��������ʱ��2H
				   //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
		 gConstantVoltageChargeComi1  = (stempdata[28]-48)*78 + stempdata[29]-48;                                   //ָ�����0.06C
		 gConstantVoltageChargeComv1  = (stempdata[30]-48)*78 + stempdata[31]-48;                               //ָ���ѹ����ܵ�ѹ
		 gConstantVoltageChargeEndi1  = (stempdata[32]-48)*78 + stempdata[33]-48;                                  //��ѹ��������ܵ���
		 gConstantVoltageChargeEndcnt1  = (stempdata[34]-48)*78 + stempdata[35]-48;                                       //��ѹ����ʱ��2H
				   //������׶γ����Ϣ��������ѹָ���תʱ��
		  gTrickleChargeComi1  = (stempdata[36]-48)*78 + stempdata[37]-48;                                   //ָ�����0.01C
		  gTrickleChargeComv1  = (stempdata[38]-48)*78 + stempdata[39]-48;                               //ָ���ѹ����ܵ�ѹ
		  gTrickleChargeEndv1  = (stempdata[40]-48)*78 + stempdata[41]-48;              //������������ܵ�ѹ
		  gTrickleChargeEndcnt1  = (stempdata[42]-48)*78 + stempdata[43]-48;                                          //��ѹ����ʱ��2H
				   //����׶γ����Ϣ��������ѹָ���תʱ��
		 gFloatChargeComi1   = (stempdata[44]-48)*78 + stempdata[45]-48;                                   //ָ�����0.01C
		 gFloatChargeComv1   = (stempdata[46]-48)*78 + stempdata[47]-48;                                   //ָ���ѹ����ܵ�ѹ
		 gFloatChargeEndcnt1 = (stempdata[48]-48)*78 + stempdata[49]-48;                               //Ԥ�����ʱ��4H
				   //EQ���׶���Ϣ������ʱ���ѹ����
		 gEQCurrCMD1  = (stempdata[50]-48)*78 + stempdata[51]-48;
		 gEQVoltCMD1  = (stempdata[52]-48)*78 + stempdata[53]-48;
		 gEQTimeCMD1  = (stempdata[54]-48)*78 + stempdata[55]-48;
//		}
//	   else//�����ַ���Է���1������
//	   {
//		   gCurveNum = 1;
//	   }

//	}
}

/*******************************************************************************
** ��������:    AdcDataInit
** ��������:    ��ʼ��ADC�����������
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void InitSysPara(void)
{
	static Uint16 TempData;
	// ReadExteCurve();
    gstElecPara.OutVol = 0;
    gstElecPara.BatVol = 0;
    gstElecPara.OutCur = 0;
    gstElecPara.PfcVoltSet = 0;
    gstElecPara.ACInCur = 0;
    gstElecPara.ACInVol = 0;

    gstSetLlcPARAM.LlcTargetCur = 0;
    gstSetLlcPARAM.LlcTargetVol = 0;
    gstSetLlcPARAM.llcOnCommand = eLlcSysCmdOff;

    gseSystemState = SelfTest_State;

    gsStateDelay = 0;

    gsPFCUploadData.PFCAcInCrt = 0;
    gsPFCUploadData.PFCAcInFrq = 0;
    gsPFCUploadData.PFCAcInVolt = 0;
    gsPFCUploadData.PFCBusVolt = 0;
    gsPFCUploadData.PFCCmdSts = 0;
    gsPFCUploadData.PFCErr = 0;
    gsPFCUploadData.PFCSubSts = 0;
    gsPFCUploadData.PFCTemp = 0;
    gsPFCUploadData.PFCWorkSts = 0;
  //////////�����ѹKֵУ��
//    TempData = ReadByte(VOUT_ADDRESS_K);//�����ֽ�
//    if( TempData != 256)
//    {
//    	u16Vout_K = TempData<<8;
//    }
//    TempData = ReadByte(VOUT_ADDRESS_K+1);//�����ֽ�
//    if( TempData != 256)
//    {
//	   u16Vout_K = u16Vout_K+TempData;
//    }
//
//    if(u16Vout_K >  1177) //981*1.2
//    {
//    	u16Vout_K = 1177;
//    }
//    if(u16Vout_K < 784 ) //981*0.8
//    {
//    	u16Vout_K = 784;
//    }
// //////////////�����ѹBֵУ��
//
//	TempData = ReadByte(VOUT_ADDRESS_B);//�����ֽ�
//	if( TempData != 256)
//	{
//		s16Vout_B = TempData<<8;
//	}
//	TempData = ReadByte(VOUT_ADDRESS_B+1);//�����ֽ�
//	if( TempData != 256)
//	{
//		s16Vout_B = s16Vout_B+TempData;
//	}
//
//	if(s16Vout_B >  2000) //1599*1.2
//	{
//		s16Vout_B =2000;
//	}
//	if(s16Vout_B < 1270 ) //1599*0.8
//	{
//		s16Vout_B = 1270;
//	}
//
//
//	  //////////////��ص�ѹKֵУ��
//	    TempData = ReadByte(BATVOL_ADDRESS_K);//�����ֽ�
//	    if( TempData != 256)
//	    {
//	    	u16BatVol_K = TempData<<8;
//	    }
//	    TempData = ReadByte(VOUT_ADDRESS_K+1);//�����ֽ�
//	    if( TempData != 256)
//	    {
//	    	u16BatVol_K = u16BatVol_K+TempData;
//	    }
//
//	    if(u16BatVol_K >  1177) //981*1.2
//	    {
//	    	u16BatVol_K =1177;
//	    }
//	    if(u16BatVol_K < 784 ) //981*0.8
//	    {
//	    	u16BatVol_K = 784;
//	    }
//	 ////////////////��ص�ѹBֵУ��
//
//		TempData = ReadByte(BATVOL_ADDRESS_B);//�����ֽ�
//		if( TempData != 256)
//		{
//			s16BatVol_B = TempData<<8;
//		}
//		TempData = ReadByte(BATVOL_ADDRESS_B+1);//�����ֽ�
//		if( TempData != 256)
//		{
//			s16BatVol_B = s16BatVol_B+TempData;
//		}
//
//		if(s16BatVol_B >  2000) //1599*1.2
//		{
//			s16BatVol_B =2000;
//		}
//		if(s16BatVol_B < 1270 ) //1599*0.8
//		{
//			s16BatVol_B = 1270;
//		}

}





Uint16   KVBat_A;
int32    KVBat_B;
Uint16  kIoutData_A;
int32	kIoutData_B;
Uint16  kIoutDataSET_A;
int32	kIoutDataSET_B;
Uint16  gACDsplay;

/*******************************************************************************
** ��������:    ASCCharge16System
** ��������:    ASC��ת16���ƺ���   ���78*78
** �������:    ASC��ת
** ���ز���:    16���ƺ���
*******************************************************************************/
//Uint16 ASCCharge16System(Uint16 ASCDtata)
//{
//	static Uint16 temp=0;
//	temp = ((ASCDtata>>8)-48)*78;//��ɸ�8λASC����ת��   ���78*78
//	temp = temp+(ASCDtata-48); //��λ��48���ϸ�λ�����10����
//	return(temp);
//}

//У����������
void WriteCorrectionDate(viod)
{
	static Uint16 temp = 0;
	Uint16 CorrectionDate[2];

      if(G_CanReceive[0]==0xA0)
          {
//    	     greadflag = 1;//���Զ�һ��EPPROM
         	 switch(G_CanReceive[1])
         	    {
				case 0XA0:
					 CorrectionDate[0] = (G_CanReceive[4]<<8)+G_CanReceive[5];//��ѹϵ��K981;
					 CorrectionDate[1] = (G_CanReceive[6]<<8)+G_CanReceive[7];//��ѹϵ��B
					// ee_WriteWord(&temp2, VOUT_ADDRESS_B, 2);
					 ee_WriteWord(CorrectionDate, VOUT_ADDRESS_K, 4);

					break;
				case 0XA1:
					CorrectionDate[0]  = (G_CanReceive[4]<<8)+G_CanReceive[5]; //���ϵ��K981;
					CorrectionDate[1] = (G_CanReceive[6]<<8)+G_CanReceive[7]; //���ϵ��B
					 ee_WriteWord(CorrectionDate, BATVOL_ADDRESS_K, 4);

					break;
				case 0XA2:
					KVBat_A = (G_CanReceive[2]<<8)+G_CanReceive[3]; //Ŀ���ѹϵ��K981;
					temp = G_CanReceive[4];                         //Ŀ���ѹϵ��B
					temp = (temp<<8) + G_CanReceive[5];
					temp = (temp<<8) + G_CanReceive[6];
					temp = (temp<<8) + G_CanReceive[7];
					KVBat_B = temp - 500000 ;                       //500000ƫ����

					break;
				case 0XA3:

					kIoutData_A = (G_CanReceive[2]<<8)+G_CanReceive[3]; //��ʾ����ϵ��K981;
					temp = G_CanReceive[4];                             //��ʾ����ϵ��B
					temp = (temp<<8) + G_CanReceive[5];
					temp = (temp<<8) + G_CanReceive[6];
					temp = (temp<<8) + G_CanReceive[7];
					kIoutData_B = temp - 5000000 ;                         //5000000ƫ����

				case 0XA4:
					kIoutDataSET_A = (G_CanReceive[2]<<8)+G_CanReceive[3]; //Ŀ�����ϵ��K981;
					temp = G_CanReceive[4];                                //Ŀ�����ϵ��B
					temp = (temp<<8) + G_CanReceive[5];
					temp = (temp<<8) + G_CanReceive[6];
					temp = (temp<<8) + G_CanReceive[7];
					kIoutDataSET_B = temp - 5000000 ;                       //5000000ƫ����
					break;

				case 0XA5:
					gACDsplay = (G_CanReceive[2]<<8)+G_CanReceive[3];       //Ŀ�����ϵ��K981;
					break;

				case 0XA6: //10
					 DS1338_WriteByte(0,G_CanReceive[7]);////second
					DS1338_WriteByte(0x1,G_CanReceive[6]);/////mintu
					DS1338_WriteByte(0x2,G_CanReceive[5]);///Hour
					DS1338_WriteByte(0x4,G_CanReceive[4]);////data 1-31
					DS1338_WriteByte(0x5,G_CanReceive[3]);/// Month
					DS1338_WriteByte(0x6,G_CanReceive[2]);////YEAR//0-99
					break;
				default:
					break;
         	    }


         }
		  else
		  {
//			  greadflag = 0;//���ܶ�����
		  }

}

//����֤����
void ReadCorrectionDate(void)
{
//    G_CanSend[0] = u16Vout_K&0X00FF;
//	G_CanSend[1] = (u16Vout_K>>8)&0X00FF;
//	G_CanSend[2] = s16Vout_B&0X00FF;
//	G_CanSend[3] = (s16Vout_B>>8)&0X00FF;
//	G_CanSend[4] = g_16data1[1];
//   G_CanSend[5] = g_16data1[0];
//
//	G_CanSend[6] = g_16data1[3];
//	G_CanSend[7] = g_16data1[2];

	    //////////�����ѹKֵУ
	     u16Vout_K =  (g_16data1[1]<<8 )+g_16data1[0]; //VOUT_ADDRESS_K  32602 + 32603
	    if(u16Vout_K >  12000) //981*1.2
	    {
	    	u16Vout_K = 1200;
	    }
	    if(u16Vout_K < 8000 ) //981*0.8
	    {
	    	u16Vout_K = 8000;
	    }
	 //////////////�����ѹBֵУ��
	    s16Vout_B =  (g_16data1[3]<<8 )+g_16data1[2]; //VOUT_ADDRESS_B  32604 + 32605
//	    ee_ReadWord(&s16Vout_B,VOUT_ADDRESS_B,1);//�ֽ�
		if(s16Vout_B >  40000) //1599*1.2
		{
			s16Vout_B =40000;
		}
		if(s16Vout_B < 1 ) //1599*0.8
		{
			s16Vout_B = 0;
		}
		  //////////////��ص�ѹKֵУ��
		 //ee_ReadWord(&u16BatVol_K,BATVOL_ADDRESS_K,1);//�ֽ�
		   u16BatVol_K= (g_16data1[5]<<8 )+g_16data1[4]; //BATVOL_ADDRESS_K  32606 + 32607
		    if(u16BatVol_K >  12000) //981*1.2
		    {
		    	u16BatVol_K =12000;
		    }
		    if(u16BatVol_K < 8000 ) //981*0.8
		    {
		    	u16BatVol_K = 8000;
		    }
		 ////////////////��ص�ѹBֵУ��
		  //  ee_ReadWord(&s16BatVol_B,BATVOL_ADDRESS_B,1);//�ֽ�
		    s16BatVol_B = (g_16data1[7]<<8 )+g_16data1[6]; //BATVOL_ADDRESS_B  32608 + 32609
			if(s16BatVol_B >  40000) //1599*1.2
			{
				s16BatVol_B = 40000;
			}
			if(s16BatVol_B < 1 ) //1599*0.8
			{
				s16BatVol_B = 0;
			}







}



/*******************************************************************************
** ��������:    ElecParaCal
** ��������:    �������е����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
//int32 gonutgeee;
Uint32 g_datenum = 3220;
void ElecParaCal(void)
{
	static Uint32  ulTemp4 = 0;
	    static Uint32  ulTemp = 0;
	    static Uint32  ulTemp2 = 0;
	    static Uint32  u16Vout_Value_temp = 0;


//	    ReadCorrectionDate();//������������
	//////////////////////////////�����ѹ����///////////////////////////////////////////////////

//	    ulTemp = (Uint32)VOUT_DATA;
//		ulTemp = ulTemp*u16Vout_K;      //0.098114==>981
//		ulTemp = ulTemp/10000;
//		if(s16Vout_B >= 2000)          //-4.0164==>-401+2000=1599
//		{
//			ulTemp2 = (s16Vout_B-2000)/100;
//			ulTemp = ulTemp+ulTemp2;
//		}
//		else if(s16Vout_B < 2000)
//		{
//			ulTemp2 = (2000-s16Vout_B)/100;
//			ulTemp = ulTemp-ulTemp2;
//		}
//		u16Vout_Value_temp = ulTemp*12675;
//		u16Vout_Value_temp = (u16Vout_Value_temp>>17);
//		gstElecPara.OutVol = (int32)u16Vout_Value_temp;//17.1


//	    G_CanSend[0] = gsPFCUploadData.PFCAcInVolt&0X00FF;
//	  	G_CanSend[1] = (gsPFCUploadData.PFCAcInVolt>>8)&0X00FF;
//	  	G_CanSend[2] = gsPFCUploadData.PFCBusVolt&0X00FF;
//	  	G_CanSend[3] = (gsPFCUploadData.PFCBusVolt>>8)&0X00FF;


//		ulTemp = 12675*(Uint32)VOUT_DATA;
//		ulTemp = (ulTemp>>17);
//		ulTemp = ulTemp/10;
//	    G_CanSend[0] = ulTemp&0X00FF;
//		G_CanSend[1] = (ulTemp>>8)&0X00FF;
//		ulTemp = ulTemp*u16Vout_K;
//		ulTemp = ulTemp/100;
//		if(s16Vout_B >= 2000)          //-4.0164==>-401+2000=1599
//		{
//			ulTemp2 = (s16Vout_B-2000);
//			ulTemp = ulTemp+ulTemp2;
//		}
//		else if(s16Vout_B < 2000)
//		{
//			ulTemp2 = (2000-s16Vout_B);
//			ulTemp = ulTemp-ulTemp2;
//		}
//		gstElecPara.OutVol = (int32)ulTemp/10;//17.1
//
//	  	G_CanSend[2] = gstElecPara.OutVol&0X00FF;
//	  	G_CanSend[3] = (gstElecPara.OutVol>>8)&0X00FF;

		ulTemp = 12675*(Uint32)VOUT_DATA;//12675����ϵ��
		ulTemp = (ulTemp>>17);//�����ѹֵ 0.1V

		ulTemp = ulTemp*u16Vout_K;
		ulTemp = ulTemp/10;///10000
		if(s16Vout_B >= 40000)          //-4.0164==>-401+2000=1599
		{
			ulTemp2 = (s16Vout_B-40000);
			ulTemp = ulTemp+ulTemp2;
		}
		else if(s16Vout_B < 40000)
		{
			ulTemp2 = (40000-s16Vout_B);///1000
			ulTemp = ulTemp-ulTemp2;
		}
		gstElecPara.OutVol = (int32)ulTemp/1000;//����0.1V



		ulTemp = 12675*(Uint32)BAT_DATA;//12675����ϵ��
		ulTemp = (ulTemp>>17);//�����ѹֵ 0.1V
		ulTemp = ulTemp*u16BatVol_K;
		ulTemp = ulTemp/10;///10000
		if(s16BatVol_B >= 40000)          //-4.0164==>-401+2000=1599
		{
			ulTemp2 = (s16BatVol_B-40000);
			ulTemp = ulTemp+ulTemp2;
		}
		else if(s16BatVol_B < 40000)
		{
			ulTemp2 = (40000-s16BatVol_B);///1000
			ulTemp = ulTemp-ulTemp2;
		}
		gstElecPara.BatVol  = (int32)ulTemp/1000;//����0.1V






       // gstElecPara.BatVol = ((int32)BAT_DATA * K_VBAT_A + K_VBAT_B) >> K_EXP;

    gsVolBatCompVal = gstElecPara.BatVol;
    if(Line_Loss_Vol < 6)
    {
        gstElecPara.OutVol -= Line_Loss_Vol;
        gstElecPara.BatVol -= Line_Loss_Vol;
    }
    else
    {
        gstElecPara.OutVol -= 6;
        gstElecPara.BatVol -= 6;
    }

    kIoutData_B = K_IOUT_B ;                      //5000000ƫ����
   kIoutData_A = K_IOUT_A ;                      //36962

    if(kIoutData_A > 44354)//1.2
    {
    	kIoutData_A = 44354;
    }
    if(kIoutData_A < 29569)//1.2
    {
    	kIoutData_A = 44354;
    }
///////////////////////////////////////////////////////////////////////////
    if(kIoutData_B > (-489947))//1.2
   {
	kIoutData_B = (-489947);
   }
   if(kIoutData_B < (-734920))//1.2
   {
	   kIoutData_B = (-734920);
   }

    gstElecPara.OutCur = ((int32)IOUT_DATA * kIoutData_A + kIoutData_B) >> K_EXP;
    //gstElecPara.OutCur = ((int32)IOUT_DATA * K_IOUT_A + K_IOUT_B) >> K_EXP;
    if(gstElecPara.OutVol < 0)
    {
        gstElecPara.OutVol = 0;
    }
    if(gstElecPara.BatVol < 0)
    {
        gstElecPara.BatVol = 0;
    }
    if(gstElecPara.OutCur < 0)
    {
        gstElecPara.OutCur = 0;
    }
}

/*******************************************************************************
** ��������:    SystemManage
** ��������:    ϵͳ����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
Uint16 keycount = 0;
void SystemManage(void)
{


//		G_CanSend[1] = gACin_ok;
//		G_CanSend[2] = 0x12;
//		G_CanSend[3] = ACinputFlag;
//
//		G_CanSend[4] = gsPFCUploadData.PFCAcInVolt&0X00FF;
//		G_CanSend[5] = (gsPFCUploadData.PFCAcInVolt>>8)&0X00FF;
//
//		G_CanSend[6] = gsPFCUploadData.PFCBusVolt&0X00FF;
//		G_CanSend[7] = (gsPFCUploadData.PFCBusVolt>>8)&0X00FF;
	static Uint32 keycount=0;
    if(0 == USB_Curve_Flag)
        ReFreshCurve();
    gsChargeOffFlag = ChargeOffFlag;
    Bat_Voltage = gstElecPara.BatVol;
    SysTimer();
    if((SelfTest_State != gseSystemState)&&(ShutDown_State != gseSystemState)
            &&(USBShutDown_State != gseSystemState)&&(!DeBug))
    {
        CheckErr();
        if(SysErr.all)
        {
            gseSystemState = Protection_State;
        }
    }
//    keycount++;
//    if(keycount>187654321)  //53Сʱ=144000000
//    {
//    	gseSystemState = USBShutDown_State;
//    }

//	G_CanSend[7] = (gCurveNum>>8)&0X00FF;

    switch(gseSystemState)
    {
        case SelfTest_State:
            SelfTestState();//�Լ�
          //  G_CanSend[6]=0x01;
            break;
        case StandBy_State:
            StandByState();//��������PFC��ѹ������ֵ����صļ̵����պϣ�������һ��Ҫ��������
          //  G_CanSend[6]=0x02;
            break;
        case PulseOn_State:
            PulseOnState();//����״̬ ��������ֵ����
           // G_CanSend[6]=0x04;
            break;
        case ChargeOn_State:
            ChargeOnState();//�����
           // G_CanSend[6]=0x08;
            break;
        case Protection_State:
             ProtectionState(); //����״̬
            // G_CanSend[6]=0x10;
            break;
        case ShutDown_State:
            ShutDownState();//�ػ�״̬
           // G_CanSend[6]=0x20;
            break;
        case USBShutDown_State:
            USBShutDownState();//USB�ػ�״̬
//            G_CanSend[6]=0x40;
            break;
        default:
            break;
    }
    ChargeState = gseSystemState;
    CHargeState1 = gseStateStep;

    KVBat_B = K_VBAT_B;                       //500000ƫ����
    KVBat_A = K_VBAT_A;
    gstSetLlcPARAM.llcOnCommand = ChargeStateParam.ChargeCmdOn;
    gstSetLlcPARAM.LlcTargetCur = (((int32)ChargeStateParam.ChargeSetI << K_EXP) - K_IOUT_B) / K_IOUT_A; //�����ĵ�λ��0.01A,����28.00A������������2800
    gstSetLlcPARAM.LlcTargetVol = (((int32)ChargeStateParam.ChargeCmdV << K_EXP) - KVBat_B) / KVBat_A;//�����ĵ�λ��0.1V,����28.4V������������284
}
Uint16 Cleseflag=1;

//�������
void restHadeat (void)
{

	static  Uint16 AcInVoltData = 0;
	static  Uint16 BusVoltData = 0;
	static  Uint16 flagdataup = 1; //1ΪҪ����
	static  Uint16 timecuntData = 0;

	  if((AcInVoltData == gsPFCUploadData.PFCAcInVolt)
					&&(BusVoltData == gsPFCUploadData.PFCBusVolt)
					)//�Ƚ��ϴε�ֵ�Ƿ�һ�������5000�ζ���һ��˵������Ҫ�ؿ���
			{


				timecuntData++;
				if(timecuntData==4101)
				{

				gACin_ok = 1;	    //AC�����쳣
				timecuntData = 0;	//��0
				ACinputFlag = 1;    //ac������

				}

			}
			else //��һ�β�һ�������¼���
			{
				 ACinputFlag = 0; //����
				timecuntData=0;	//��0
				gACin_ok = 0;	 //AC����

			}

			AcInVoltData = gsPFCUploadData.PFCAcInVolt;
			BusVoltData = gsPFCUploadData.PFCBusVolt;

//			G_CanSend[4] = TempData1;
//			G_CanSend[5] = Cleseflag;
//
//			G_CanSend[6] = flagdataup&0x00ff;
//			G_CanSend[7] = timecuntData;

}

/*******************************************************************************
 *������: ReFreshCurve();
 *��������:����״̬�¸��³������:�Ҳ�Ӱ�쵱ǰ����״̬
 *��������:����״̬�¸��³������:�Ҳ�Ӱ�쵱ǰ����״̬
 * *****************************************************************************/

void ReFreshCurve()
{
	static  Uint16 gCurveNumpreor=0;
	static  Uint16 gRestFlag = 1;
	static  Uint16 gRestFlag1 = 1;


	 restHadeat ();//�������
//	 G_CanSend[0] = gRestFlag;

    if((gsPFCUploadData.PFCAcInVolt >= 750)&&(gsPFCUploadData.PFCBusVolt >= 3000)&&(ACinputFlag==0))     //AC>=75V��BUS>=300V
    {

    	gACin_ok = 0;	 //AC��������
    	gRestFlag = 1;
    	gRestFlag = 0;
    	// G_CanSend[5] = 0x08;

    }
    else if(((gsPFCUploadData.PFCAcInVolt <= 700)&&(gsPFCUploadData.PFCBusVolt <= 2800))||(ACinputFlag==1))//AC>=65V��BUS>=280V
    {

    	gACin_ok = 1;	 //AC�����쳣
    	gRestFlag++;
    	if(( gRestFlag==1000 )&&(gRestFlag1==1))
    	{

    	   USBReadByteD(VOUT_ADDRESS_K,g_16data1,24 );
//    	  USBReadByteD(VOUT_ADDRESS_K,g_16data1,8 );
//    	    WriteByte(VOUT_ADDRESS_K+16,17);

    	//	gRestFlag1=0;
    	}
    	if(( gRestFlag==1300 )&&(gRestFlag1==1))
    	{

    	//    ee_ReadByte(stempdata, CURVEEXTE4, 56);
    		 USBReadByteD(CURVEEXTE4,stempdata,56);
//    		USBReadByteD(CURVEEXTE4,stempdata,8);
//    		WriteByte(CURVEEXTE4+16,17);

    		gRestFlag1=0;
    	}

    	//gseSystemState = SelfTest_State;
    	ChargeStateParam.ChargeStat = StepOne;
//    	 GlobalVarInit();
//    	    DeviceInit();
//    	    InitAdcPara();


//    	       InitSysPara();

    	// G_CanSend[6]=0x80;
    	// G_CanSend[5] = 0x80;


    }
    else
    {
    	gACin_ok = 1;	 //AC�����쳣
    }

//    G_CanSend[4] = gACin_ok;

    if(SelfTest_State != gseSystemState && 0 == gACin_ok)//
    {
    	if((gCurveNum>22)&&(gCurveNumpreor != gCurveNum))//������������
		{
		// ReadExteCurve();
		}
		gCurveNumpreor = gCurveNum;

    }



    //�Ѱ��¿���
    if(GpioDataRegs.GPBDAT.bit.GPIO32 == 0 && SelfTest_State != gseSystemState && 0 == gACin_ok)//�����ѹ����
    {

        keycount ++;
        gsSwitchCounter = 0;
        if(keycount > 3000 && keycount <= 7000)
        {
            if(Key5s_Flag)
                gSelfState = 5;
            else
            {
                gSelfState = 5;
            }
        }
        if(keycount > 8000 && keycount < 15000)
        {
            gSelfState = 3;
            if(0 == EQ_MFlag)
            {
                EQ_MFlag = 1;
            }
            else
            {
               EQ_MFlag = 0;
            }
            keycount = 15000;
            gSelfState = 3;
        }
        if(keycount >= 15000)
        {
           gSelfState = 3;
        }
        if(keycount > 20000)
            keycount = 0;
    }
    else if(keycount > 200 && keycount < 3000 && SelfTest_State != gseSystemState && 0 == gACin_ok)//AC�����ѹ����
    {
    	//G_CanSend[4] = 0x02;
        if(1 == Key5s_Flag)
        {
            Key1s_Flag = 1;
        }
        if(1 == Key1s_Flag)
        {
            Key1s_Flag = 0;
            gCurveNum ++;
      //  G_CanSend[4] = 0x01;
            if(gCurveNum > Curve_Max)
             {
               gCurveNum = 1;
             }
            if(gCurveNum > 22)//����22��ʱ���ж�EEPROM������
			 {
			//   ReadExteCurve();
			 }

             gSelfState = 1;
        }
        keycount = 0;
    }
    else if(keycount > 3000 && keycount < 7000 && SelfTest_State != gseSystemState && 0 == gACin_ok) //AC�����ѹ����
    {
    	//G_CanSend[4] = 0x03;
            if(0 == Key5s_Flag)
            {
                Key5s_Flag = 1;
                keycount = 0;
            }
            else
            {
                Key5s_Flag = 2;

						if(WriteByte(0 , gCurveNum))  //0��ַ�ŵ������ߺ�
						{
							gSelfState = 0;
							Key1s_Flag = 0;
							Key5s_Flag = 0;
							keycount = 0;
							gsSwitchCounter = 0;
							gseStateStep = StepOne;       //������Ҫ��һ�»�
							gseSystemState = ShutDown_State;
							restart = 1;
						}


            }
    }
    else if(1 == gACin_ok)  //ACû�е�
    {
    //	G_CanSend[4] = 0x04;
        gsSwitchCounter = 0;

		if(USBON == 0)//���û��USB ��ʾ6������
		{
		  gSelfState = 6;
		}
        ChargeOffFlag = 0;
        gsChargeOffFlag = 0;
        restart = 1;
        if(0 == GpioDataRegs.GPBDAT.bit.GPIO32)  //����������
        {
            keycount ++;
            if(keycount > 1000)
            {
                keycount = 1000;
                DisplayOff = 1;        //acû�絫�а�������
            }
        }
        else
        {
            if(keycount < 1)
            {
                keycount = 0;
            }
            else
            {
                keycount --;
            }
        }
    }
            //AC�����ѹ����
    else if(0 == gACin_ok && 6 == gSelfState && gsPFCUploadData.PFCAcInVolt > 700)//AC��ѹ>70V
    {
    //	G_CanSend[4] = 0x10;
        gSelfState = 4;
        gseStateStep = StepFour;
        gseSystemState = ShutDown_State;
        restart = 1;
    }
    else
    {
    	//G_CanSend[4] = 0x20;
        gsSwitchCounter++;
        if(gsSwitchCounter > 10000)
        {
            gsSwitchCounter = 0;
            gSelfState = 0;
            Key1s_Flag = 0;
            Key5s_Flag = 0;
        }
          keycount = 0;
    }
    /*����*/
/*    if(1 == ChargeOffFlag && 0 == GpioDataRegs.GPADAT.bit.GPIO19)
        gSelfState = 7;*/
}
/*******************************************************************************
** ��������:    SelfTestState
** ��������:    �Լ�״̬����,��������⣬ǰ5�����Ǽ�⵽����������1�룬��ѡ�����ߣ�����ܹ�����ʾ��������1�룬��ȷ�����߱��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void SelfTestState(void)
{
    Uint16 temp = 256;
    if(gCurveNum > Curve_Max && ADRESULT_USBON < 1000 && 1 == USBON)
        gCurveNum = 1;
    gstSetLlcPARAM.llcOnCommand = eLlcSysCmdOff;
   // SysErr.bit.StorageErr;
    switch(gseStateStep)
    {
        case StepOne:
            if(0 == gACin_ok || 0 == ReadedFlag)//AC�����ѹ����
                gSelfState = 4;

            gsSelfCounter ++;
            if(gsSelfCounter > 5000)
            {
                gsSelfCounter = 0;
                gseStateStep = StepTwo;
            }
            if(GpioDataRegs.GPBDAT.bit.GPIO32 == 0)
            {
                gsSwitchCounter ++;
                if(gsSwitchCounter > 1000)
                {
                    gseStateStep = StepThree;
                    gsSwitchCounter = 0;
                    gsSelfCounter = 0;
//                    gSelfState = 3;
                }
            }
            else
            {
                gsSwitchCounter = 0;
            }
            break;
        case StepTwo:
            TestFlashManage();
            if(0 == gsStateDelay)
            {
                gSelfState = 4;
                gseStateStep = StepOne;
                gseSystemState = StandBy_State;
                Key5s_Flag = 0;
            }
            else
            {
                if(Key5s_Flag)
                {
                    if(WriteByte(0 , gCurveNum)) //0��ַ�ŵ������ߺ�
                    {
                        Key5s_Flag = 0;
                        gsStateDelay = 0;
                    }
                }
            }
            break;
        case StepThree:
            if(GpioDataRegs.GPBDAT.bit.GPIO32 == 0)
            {
                gsSelfCounter ++;
                if(gsSelfCounter > 2000 && gsSelfCounter < 7000)
                {
                    if(1 == Key5s_Flag)
                    {
                        if(WriteByte(0 , gCurveNum)) //0��ַ�ŵ������ߺ�
                        {
                            gSelfState = 5;
                        }
                    }
                    if(0 == Key5s_Flag)
                    {
                        gSelfState = 5;
                    }
                }
                if(gsSelfCounter > 70000)
                {
                    gSelfState = 3;
                    Key1s_Flag = 0;
                        Key5s_Flag = 0;
                        if(0 == EQ_MFlag)
                        {
                           EQ_MFlag = 1;
                        }
                        else
                        {
                            EQ_MFlag = 0;
                        }
                        gseStateStep = StepTwo;
                        gsStateDelay = 2000;
                        gsSelfCounter = 0;
                        gSelfState = 3;
                }
                else if(gsSelfCounter > 13000)
                {
                   gseStateStep = StepTwo;
                   gsStateDelay = 2000;
                   gsSelfCounter = 0;
                }
                gsSwitchCounter = 0;
            }
            else
            {/*�ɿ�*/
                gsSwitchCounter ++;
                if(gsSelfCounter >= 100 && gsSelfCounter <= 2000)
                {
                    if(0 == Key5s_Flag || 2 == Key5s_Flag)
                    {
                        Key1s_Flag = 0;
                    }
                    else if(1 == Key5s_Flag )
                    {
                        Key1s_Flag = 1;
                        gsSelfCounter = 0;
                    }
                }
                else if(gsSelfCounter > 2000 && gsSelfCounter <= 10000)
                {
                    if(0 == Key5s_Flag )
                    {
   ///                      Read_Dataiic = 0;
                            gSelfState = 1;
                            Key1s_Flag = 0;
                            Key5s_Flag = 1;
                            gseStateStep = StepOne;
                            gsSelfCounter = 0;
                     }
                    else if(1 == Key5s_Flag)
                    {
                         Key5s_Flag = 2;
                    }
                }
                else if(gsSelfCounter > 10000)
                {
                    Key1s_Flag = 0;
                    if(0 == EQ_MFlag)
                           EQ_MFlag = 1;
                    else
                           EQ_MFlag = 0;
                    gseStateStep = StepTwo;
                    gsStateDelay = 2000;
                    gsSelfCounter = 0;
                    gSelfState = 3;
                }
                else
                {
                    gsSelfCounter = 0;
                }
            }
            if(1 == Key1s_Flag)
            {
                gseStateStep = StepFour;
                gsSelfCounter = 0;
                gSelfState = 1;
            }
            else if(2 == Key5s_Flag)
            {
                if(WriteByte(0 , gCurveNum))//0��ַ�ŵ������ߺ�
                {
                  gseStateStep = StepTwo;
                  gsStateDelay = 2000;
                  gsSelfCounter = 0;
                  gSelfState = 4;
                  Key5s_Flag = 0;
                }
            }
            if(gsSwitchCounter > 8000)
            {
                if(Key5s_Flag)
                {
                    if(WriteByte(0 , gCurveNum)) //0��ַ�ŵ������ߺ�
                    {
                        gsSwitchCounter = 0;
                        gseStateStep = StepTwo;
                        gsStateDelay = 2000;
                        gsSelfCounter = 0;
                        gSelfState = 3;
                        Key1s_Flag = 0;
                        Key5s_Flag = 0;
                    }
                }
            }
            break;
        case StepFour:/*�ɿ�*/
                if(1 == Key1s_Flag)
                {
                	//G_CanSend[2] = 0x01;
                    Key1s_Flag = 0;
                    gsSelfCounter = 0;
                    gCurveNum ++;
                    if(gCurveNum > Curve_Max)//4
                    {
                        gCurveNum = 1;
                    }
//                    if(gCurveNum > 22)//����22��ʱ���ж�EEPROM������
//					 {
//					   ReadExteCurve();
//					 }

                    gSelfState = 1;
                    gseStateStep = StepThree;
                }
            break;
        default:
     ///       TestFailManage();
            break;
    }
}

/*******************************************************************************
** ��������:    StandByState
** ��������:    ����״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void StandByState(void)
{
    gstSetLlcPARAM.llcOnCommand = eLlcSysCmdOff;
    switch(gseStateStep)
    {
        case StepOne:
            SelectCurve();
            CLR_KPOUT;
            if(0 == ChargeOffFlag)
              //  SET_KPIN;
/*            else
                CLR_KPIN;*/
            PFC_ON;
            gsStateDelay = 100;
            gseStateStep = StepTwo;
            break;
        case StepTwo:
            if(0 == gsStateDelay)
            {
             /*   if(labs(gsPFCUploadData.PFCBusVolt - gstElecPara.PfcVoltSet) < 100 || gCurveNum == 5 || gCurveNum == 7 || gCurveNum == 8 || gCurveNum == 9)
                {                                                                            //// || gCurveNum == 2 || gCurveNum == 9 ||gCurveNum == 10 ||gCurveNum == 12 || gCurveNum == 11|| gCurveNum == 13
                    gsStateCnt ++;
                    if(gsStateCnt > 5000)
                    {
                        gsStateCnt = 0;
                        /////2020 0824 test
                        if(gstElecPara.BatVol < 180 || 1 == restart)
                         {
                            gsChargeOffFlag = 0;
                            Chargerestart = 0;
                            ChargeOffFlag = 0;
                            ///restart = 0;
                         }
                            gseStateStep = StepOne;
                            gseSystemState = PulseOn_State;
                            SET_KPAW;
                    }
                }
                ////////test////
                else*/ if(((gstElecPara.BatVol < 200 && 1 == gsChargeOffFlag)|| (0 == gsChargeOffFlag) || 1 == restart)&&(gstElecPara.BatVol > 35))
                {
                    gsStateCnt ++;
                    if(gsStateCnt > 3000)
                    {
                        gsChargeOffFlag = 0;
                        Chargerestart = 0;
                        ChargeOffFlag = 0;
                        gseStateStep = StepOne;
                        gseSystemState = PulseOn_State;
//                        SET_KPAW;
                    }
                }//////////2020 0824
                else
                {
                    gsStateCnt = 0;
                }
            }
            break;
        case StepThree:
            if((gstElecPara.BatVol < FLOATCHARGECMV)&&(gstElecPara.BatVol > 35))/*120*/
            {
                gstElecPara.PfcVoltSet = 3850;
                CLR_KPOUT;
                if(0 == ChargeOffFlag)
                   // SET_KPIN;
/*                else
                    CLR_KPIN;*/
                PFC_ON;
                gsStateDelay = 100;
                gseStateStep = StepTwo;
             }
            break;
        case StepFour:///////restart
            if((gstElecPara.BatVol < 190 || 1 == restart)&&(gstElecPara.BatVol > 35))
            {
                gsStateCnt ++;
                if(gsStateCnt > 1000)
                {
                    SelectCurve();
                    gsChargeOffFlag = 0;
                    Chargerestart = 0;
                    ChargeOffFlag = 0;
                    gstElecPara.PfcVoltSet = 3850;
                    CLR_KPOUT;
                    if(0 == ChargeOffFlag)
                     //   SET_KPIN;
/*                    else
                        CLR_KPIN;*/
                    PFC_ON;
                    if(gACin_ok == 0) //AC�����ѹ����
                    {
                        gseSystemState =  SelfTest_State;
                        gseStateStep = StepTwo;
                    }
                }
            }
            else
                gsStateCnt = 0;
            break;
        default:
            break;
    }
}

/*******************************************************************************
** ��������:    PulseOnState
** ��������:    ����״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/

static void PulseOnState(void)
{


    switch(gseStateStep)
    {
        case StepOne:
            if(/*gsChargeOffFlag != */1)
            {
                if((WriteEEPStart())||(gsStateOver>10000))   //��翪ʼǰ��¼��翪ʼ�����ݣ��ȴ�
                {
                    gseStateStep = StepTwo;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                //    PauseOnFlag = 1;
                }


            }
            else
            {
                gseStateStep = StepTwo;
            }


            break;
        case StepTwo:
            gsStateDelay = 100;
            gsStateOver  = 60000;  //60S
            ChargeStateParam.ChargeCmdV = gstElecPara.BatVol;//240;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
            ChargeStateParam.ChargeCmdI = 5;
            ChargeStateParam.ChargeCmdOn = eLlcSysCmdOn;
            gseStateStep = StepThree;
            break;
        case StepThree:
            ChargeStateParam.ChargeCmdOn = eLlcSysCmdOn;

            if(0 == gsStateOver)
            {
                //SysErr.bit.HardwareErr = 1;
                ChargeStateParam.ChargeCmdOn = eLlcSysCmdOff;
            }
            //if((gstElecPara.OutVol > (gstElecPara.BatVol - 20))&&(gstElecPara.OutVol <= 350))//�����ѹС��35V
           	//if (gstElecPara.OutVol <= 350)//�������Ժ�Ҫ�޸Ļ���������CYPERR  35V
            //{
               // PauseOnFlag = 0;

                gseStateStep = StepFour;
               // gsStateCnt = 0;
           // }
           // else
           // {
              //  gsStateCnt = 0;
           // }
            break;
        case StepFour:
//            if(1 == Chargerestart) //��ص�ѹ����35��С��20V,������븡��׶�
//            {
//                ChargeStateParam.ChargeMode = CHGSTAT_FLOATCHARGE;
//                ChargeStateParam.ChargeCmdV = FLOATCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
//                ChargeStateParam.ChargeCmdI = FLOATCHARGECMI;
//            }
           if(gsChargeOffFlag != 1)//��ص�ѹС��35V������20Vʱ gsChargeOffFlag=0
            {

                ChargeStartStateTest();
            }
            SET_KPOUT;         //����̵�����
            gseStateStep = StepOne;
            gseSystemState = ChargeOn_State;
            break;
        default:
            break;
    }
}

/*******************************************************************************
** ��������:    ChargeOnState
** ��������:    ���״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
int16 tmpCurLimit = 0;
int16 tmpCurLimitTest = 0;
static void ChargeOnState(void)
{
//	    G_CanSend[0] = 0X34;
//		G_CanSend[1] = 0X12;
	static	Uint16 gCurveNumCUNT = 1;
	gCurveNumCUNT++;


//		G_CanSend[4] = ChargeStateParam.ChargeMode&0X00FF;
//		G_CanSend[5] = (ChargeStateParam.ChargeMode>>8)&0X00FF;

//		G_CanSend[6] = gCurveNum&0X00FF;
//		G_CanSend[7] = (gCurveNum>>8)&0X00FF;
//		G_CanSend[7] = gCurveNum&0X00FF;

//		   G_CanSend[6] = gstElecPara.OutVol&0X00FF;
//			    G_CanSend[7] = (gstElecPara.OutVol>>8)&0X00FF;

	    gElectricQuantitydsplay = ChargeStateParam.ChargeMode;//�����״̬����LED
    ChargeStateParam.ChargeCmdOn = eLlcSysCmdOn;

    switch(ChargeStateParam.ChargeMode)
    {


    //  Chargerestart
        case CHGSTAT_PRECHARGE://// Ԥ��״̬
             PreChargeManage();
//         	G_CanSend[6] = gCurveNumCUNT&0X00FF;
//         	G_CanSend[7] = (gCurveNumCUNT>>8)&0X00FF;
             break;
        case CHGSTAT_CONCURCHARGE: // �������һ״̬
        	G_CanSend[1]=0X02;
//        	         	G_CanSend[2] = gCurveNumCUNT&0X00FF;
//        	         	G_CanSend[3] = (gCurveNumCUNT>>8)&0X00FF;
             ChargeStartStateTest();//???
             ConCurChargeManage();
             break;
        case CHGSTAT_SCONCURCHARGE: // ��������״̬
             SConCurChargeManage();
             break;
        case CHGSTAT_CONVOLCHARGE: // ��ѹ���һ״̬
             ConVolChargeManage();
             break;
        case CHGSTAT_BCONVOLCHARGE: // ������״̬
             BConVolChargeManage();  //0K
             break;
        case CHGSTAT_FLOATCHARGE: // ����״̬
             FloatChargeManage();  //OK
             break;
        case CHGSTAT_EQMODECHARGE:
             EQChargeManage();
             break;
        case CHGSTAT_PAUSEMODE:
             PAUSEChargeManage();
             break;
        default:
             break;
    }
    tmpCurLimit = RenewCurrentLmiting();
//    G_CanSend[0] = tmpCurLimit&0X00FF;
//   		G_CanSend[1] = (tmpCurLimit>>8)&0X00FF;
    //tmpCurLimit = IOUT_MAX;
    ChargeStateParam.ChargeSetI = min(ChargeStateParam.ChargeCmdI,tmpCurLimit);
//    G_CanSend[4] = ChargeStateParam.ChargeSetI&0X00FF;
//    G_CanSend[5] = (ChargeStateParam.ChargeSetI>>8)&0X00FF;
}

/*******************************************************************************
** ��������:    ProtectionState
** ��������:    ����״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void ProtectionState(void)
{
	static Uint16 ErrFlag = 1;//�д��־

    ChargeStateParam.ChargeCmdOn = eLlcSysCmdShutDown;
    if((gstElecPara.OutCur < 100)||(0 == gsStateDelay))//�������С��1Aʱ����ػ�
    {
        CLR_KPOUT;   //�Ͽ�����̵���
        PFC_OFF;      //��PFC�����̻�
    }
    CheckErr();
    if(0 == SysErr.all)
    {
        gseStateStep = StepOne;
        gseSystemState = StandBy_State;
        ErrFlag = 1; //Ϊ�´δ�����1.
    }

    if(ErrFlag)
    {
    WriteEEPErr();
    ErrFlag = 0;     //�д�ֻдһ��
    }
}

/*******************************************************************************
** ��������:    ShutDownState
** ��������:    �ػ�״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void ShutDownState(void)
{
    ChargeStateParam.ChargeCmdOn = eLlcSysCmdOff;
    switch(gseStateStep)
    {
        case StepOne:
//            CLR_KPAW;
            gsStateDelay = 1000;
            ChargeStateParam.ChargeCmdV = 0;
            ChargeStateParam.ChargeCmdI = 0;
            gseStateStep = StepTwo;
            break;
        case StepTwo:
            if((gstElecPara.OutCur < 100)||(0 == gsStateDelay))//�������С��1Aʱ����ػ�
            {
                CLR_KPOUT;
                PFC_OFF;
                gseStateStep = StepThree;
            }
            break;
        case StepThree:
            if(WriteEEPStop())
            {
                gseStateStep = StepOne;//StepThree
                gseSystemState = StandBy_State;
/*                if(gCurveNum == 5 || gCurveNum == 7 || gCurveNum == 8 || gCurveNum == 9)
                    Chargerestart = 1;
                else
                {
                    gsChargeOffFlag = 1;
                    ChargeOffFlag = 1;
                    DC_EN;
                }*/
            }
            break;
        case StepFour:///////restart
            if(((gstElecPara.BatVol < 180) || 1 == restart)&&(gstElecPara.BatVol > 35))  //��ص�ѹ��3.5-18Vʱ���ÿ���
            {
                SelectCurve();
                gsChargeOffFlag = 0;
                Chargerestart = 0;
                ChargeOffFlag = 0;
                gstElecPara.PfcVoltSet = 3850;
                CLR_KPOUT;
                if(0 == ChargeOffFlag)
              //      SET_KPIN;
                PFC_ON;
                gseSystemState =  StandBy_State;
                gseStateStep = StepOne;//StepTwo
            }

            break;
        default:
            break;
    }
}

/*******************************************************************************
** ��������:    ShutDownState
** ��������:    �ػ�״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
Uint16 StepUSBDown = 0;
Uint16 GoToUSBShutDown(void)
{
    if(!StepUSBDown)
    {
        gseSystemState = USBShutDown_State;
        gseStateStep = StepOne;
        StepUSBDown = 1;
    }
    return(gsUSBShutDownFlag);
}
void GoToUSBChargeOn(void)
{
    gseSystemState = USBShutDown_State;
    gseStateStep = StepFour;
    StepUSBDown = 0;
}

static void USBShutDownState(void)
{
    ChargeStateParam.ChargeCmdOn = eLlcSysCmdOff;
    switch(gseStateStep)
    {
        case StepOne:
            gsUSBShutDownFlag = 0;
            gsStateDelay = 1000;
            ChargeStateParam.ChargeCmdV = 0;
            ChargeStateParam.ChargeCmdI = 0;
            gseStateStep = StepTwo;
            break;
        case StepTwo:
            gsUSBShutDownFlag = 0;
            if((gstElecPara.OutCur < 100)||(0 == gsStateDelay))//�������С��1Aʱ����ػ�
            {
                CLR_KPOUT;
                PFC_OFF;
                gseStateStep = StepThree;
            }
            break;
        case StepThree:
            gsUSBShutDownFlag = 1;
            break;
        case StepFour:
            gsUSBShutDownFlag = 0;
            gseStateStep = StepOne;
            gseSystemState = StandBy_State;
            break;
        default:
            break;
    }
}


/*******************************************************************************
** ��������:    SysTimer
** ��������:    ϵͳ��ʱ������
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
Uint16 g1sCnt = 0;
Uint32 gAllChargeTime = 0;
Uint32 gAllChargeAH = 0;
static void SysTimer(void)
{
    if(gsStateDelay)
    {
        gsStateDelay --;
    }
    if(gsStateOver)
    {
        gsStateOver --;
    }
    g1sCnt ++;
    if(g1sCnt > 1000)
    {
        g1sCnt = 0;
        if(ChargeOn_State == gseSystemState)
        {
            gAllChargeTime ++;
            gAllChargeAH +=  gstElecPara.OutCur;
        }
    }
}

/*******************************************************************************
** ��������:    TestFlashManage
** ��������:    Flash��⴦����,��ʱΪ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void TestFlashManage(void)
{
    ;
}

/*******************************************************************************
** ��������:    TestFailManage
** ��������:    �Լ���״̬[�Լ�ʧ��]������,�����Լ�״̬,�Լ�ʧ����λ
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
 void TestFailManage(void)
{
 ///   CLR_KPIN;/////2020 0822 Ϊ����standby ����״̬λ
    PFC_OFF;
    CLR_KPOUT;
//    SysErr.bit.HardwareErr = 1;
}

/*******************************************************************************
** ��������:    SelectCurve
** ��������:    ���ݶ�ȡ�����߱�Ÿ�������
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void SelectCurve(void)
{
	static int16 gCurveNumtemp = 0;
	if(gCurveNum<23)
	{
		gCurveNumtemp = gCurveNum;
	}
	else//��IIC����
	{
		gCurveNumtemp = 23;		//����23IIC����
	}

    switch(gCurveNumtemp)
    {
        case 1:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_1;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_1;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_1 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_1;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_1;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_1;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_1;
            CONCURCHARGEENDV =CONCURCHARGEENDV_1;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_1;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_1 ;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_1 ;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_1;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_1 ;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_1;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_1;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_1;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_1;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_1 ;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_1;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_1 ;                         //�����������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_1;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_1;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_1 ;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_1;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_1;
            EQCHARGECMV  = EQVoltCMD_1;
            EQCHARGETIME = EQTimeCMD_1;
            break;
        case 2:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_2;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_2;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_2 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_2;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_2;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_2;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_2;   //CYP�ò���
            CONCURCHARGEENDV =CONCURCHARGEENDV_2 ;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_2;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_2 ;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_2 ;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_2;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_2 ;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_2;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_2;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_2;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_2;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_2 ;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_2;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_2 ;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_2;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_2;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_2 ;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_2;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_2;
            EQCHARGECMV  = EQVoltCMD_2;
            EQCHARGETIME = EQTimeCMD_2;
            break;
        case 3:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_3;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_3;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_3;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_3;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_3;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_3;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_3;
            CONCURCHARGEENDV =CONCURCHARGEENDV_3;                         //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_3;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_3;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_3;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_3;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_3 ;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_3;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_3;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_3;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_3;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_3 ;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_3;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_3 ;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_3;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_3;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_3 ;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_3;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_3;
            EQCHARGECMV  = EQVoltCMD_3;
            EQCHARGETIME = EQTimeCMD_3;
            break;
        case 4:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_4;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_4;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_4 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_4;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_4;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_4;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_4;
            CONCURCHARGEENDV =CONCURCHARGEENDV_4 ;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_4;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_4 ;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_4 ;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_4;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_4;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_4;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_4;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_4;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_4;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_4 ;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_4;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_4 ;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_4;                                          //��ѹ����ʱ��2H

            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_4;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_4 ;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_4;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_4;
            EQCHARGECMV  = EQVoltCMD_4;
            EQCHARGETIME = EQTimeCMD_4;
            break;
        case 5:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_5;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_5;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_5 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_5;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_5;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_5;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_5;
            CONCURCHARGEENDV =CONCURCHARGEENDV_5 ;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_5;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_5 ;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_5 ;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_5;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_5 ;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_5;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_5;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_5;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_5;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_5;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_5;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_5;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_5;        //��ѹ����ʱ��2H            //����׶γ����Ϣ��������ѹָ���תʱ��

            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_5;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_5 ;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_5;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_5;
            EQCHARGECMV  = EQVoltCMD_5;
            EQCHARGETIME = EQTimeCMD_5;
            break;
        case 6:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_6;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_6;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_6 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_6;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_6;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_6;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_6;
            CONCURCHARGEENDV =CONCURCHARGEENDV_6;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_6;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_6 ;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_6 ;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_6;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_6 ;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_6;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_6;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_6;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_6;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI = BCONVOLCHARGECMI_6;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_6;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV = BCONVOLCHARGEENDV_6;                 //������������ܵ�
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_6;             //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_6;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_6;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_6;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_6;
            EQCHARGECMV  = EQVoltCMD_6;
            EQCHARGETIME = EQTimeCMD_6;
            break;
        case 7:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_7;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_7;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_7 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_7;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_7;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_7;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_7;
            CONCURCHARGEENDV =CONCURCHARGEENDV_7;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_7;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_7;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_7;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_7;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_7;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_7;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_7;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_7;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_7;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_7;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_7;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_7;          //������������ܵ�ѹ
            BCONVOLCHARGEENDI =BCONVOLCHARGEENDI_7;
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_7;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_7;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_7;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_7;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_7;
            EQCHARGECMV  = EQVoltCMD_7;
            EQCHARGETIME = EQTimeCMD_7;
            break;
        case 8:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_8;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_8;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_8;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_8;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_8;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_8;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_8;
            CONCURCHARGEENDV =CONCURCHARGEENDV_8;                         //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_8;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_8;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_8;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_8;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_8;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_8;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_8;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_8;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_8;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_8;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_8;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_8;          //������������ܵ�ѹ
            BCONVOLCHARGEENDI =BCONVOLCHARGEENDI_8;
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_8;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_8;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_8;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_8;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_8;
            EQCHARGECMV  = EQVoltCMD_8;
            EQCHARGETIME = EQTimeCMD_8;
            break;
        case 9:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_9;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_9;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_9;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_9;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_9;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_9;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_9;
            CONCURCHARGEENDV =CONCURCHARGEENDV_9;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_9;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_9;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_9;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_9;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_9;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_9;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_9;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_9;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_9;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_9;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_9;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_9;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_9;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_9;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_9;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_9;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_9;
            EQCHARGECMV  = EQVoltCMD_9;
            EQCHARGETIME = EQTimeCMD_9;
            break;
        case 10:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_10;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_10;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_10;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_10;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_10;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_10;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_10;
            CONCURCHARGEENDV =CONCURCHARGEENDV_10;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_10;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_10;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_10;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_10;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_10;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_10;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_10;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_10;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_10;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_10;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_10;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_10;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_10;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_10;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_10;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_10;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_10;
            EQCHARGECMV  = EQVoltCMD_10;
            EQCHARGETIME = EQTimeCMD_10;
            break;
        case 11:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_11;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_11;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_11;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_11;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_11;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_11;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_11;
            CONCURCHARGEENDV =CONCURCHARGEENDV_11;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_11;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_11;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_11;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_11;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_11;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_11;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_11;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_11;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_11;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI = BCONVOLCHARGECMI_11;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_11;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV = BCONVOLCHARGEENDV_11;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_11;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_11;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_11;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_11;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_11;
            EQCHARGECMV  = EQVoltCMD_11;
            EQCHARGETIME = EQTimeCMD_11;
            break;
        case 12:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_12;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_12;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_12 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_12;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_12;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_12;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_12;
            CONCURCHARGEENDV =CONCURCHARGEENDV_12 ;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_12;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_12 ;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_12 ;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_12;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_12 ;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_12;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_12;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_12;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_12;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_12;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_12;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_12 ;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_12;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_12;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_12 ;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_12;                                        //Ԥ�����ʱ��4H
            break;
        case 13:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_13;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_13;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_13;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_13;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_13;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_13;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_13;
            CONCURCHARGEENDV =CONCURCHARGEENDV_13;                         //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_13;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_13;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_13;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_13;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_13;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_13;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_13;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_13;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_13;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_13;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_13;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_13;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_13;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_13;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_13;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_13;                                        //Ԥ�����ʱ��4H
            break;
        case 14:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_14;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_14;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_14;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_14;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_14;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_14;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_14;
            CONCURCHARGEENDV =CONCURCHARGEENDV_14;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_14;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_14;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_14;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_14;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_14;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_14;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_14;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_14;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_14;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_14;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_14;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_14;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_14;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_14;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_14;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_14;                                        //Ԥ�����ʱ��4H
            break;
        case 15:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_15;                                   //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_15;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_15;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_15;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_15;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_15;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_15;
            CONCURCHARGEENDV =CONCURCHARGEENDV_15;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_15;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_15;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_15;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_15;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_15;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_15;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_15;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_15;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_15;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_15;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_15;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_15;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_15;       //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_15;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_15;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_15;                                        //Ԥ�����ʱ��4H
            break;
        case 16:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_16;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_16;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_16;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_16;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_16;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_16;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_16;
            CONCURCHARGEENDV =CONCURCHARGEENDV_16;                        //������������ܵ�ѹ

            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_16;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_16;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_16;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_16;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_16;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_16;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_16;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_16;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_16;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_16;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_16;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_16;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_16;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_16;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_16;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_16;
            EQCHARGECMV  = EQVoltCMD_16;
            EQCHARGETIME = EQTimeCMD_16;
            break;
        case 17:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_17;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_17;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_17;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_17;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_17;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_17;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_17;
            CONCURCHARGEENDV =CONCURCHARGEENDV_17;                        //������������ܵ�ѹ

            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_17;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_17;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_17;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_17;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_17;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_17;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_17;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_17;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_17;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_17;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_17;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_17;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_17;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_17;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_17;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_17;
            EQCHARGECMV  = EQVoltCMD_17;
            EQCHARGETIME = EQTimeCMD_17;
            break;
        case 18:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_18;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_18;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_18 ;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_18;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_18;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_18;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_18;
            CONCURCHARGEENDV =CONCURCHARGEENDV_18;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_18;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_18;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_18;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_18;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_18;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_18;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_18;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_18;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_18;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_18;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_18;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_18;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_18;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_18;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_18;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_18;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_18;
            EQCHARGECMV  = EQVoltCMD_18;
            EQCHARGETIME = EQTimeCMD_18;
            break;
        case 19:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_19;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_19;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_19;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_19;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_19;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_19;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_19;
            CONCURCHARGEENDV =CONCURCHARGEENDV_19;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_19;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_19;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_19;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_19;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_19;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_19;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_19;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_19;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_19;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_19;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_19;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_19;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_19;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_19;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_19;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_19;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_19;
            EQCHARGECMV  = EQVoltCMD_19;
            EQCHARGETIME = EQTimeCMD_19;
            break;
        case 20:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_20;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_20;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_20;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_20;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_20;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_20;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_20;
            CONCURCHARGEENDV =CONCURCHARGEENDV_20;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_20;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_20;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_20;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_20;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_20;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_20;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_20;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_20;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_20;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_20;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_20;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_20;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_20;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_20;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_20;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_20;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_20;
            EQCHARGECMV  = EQVoltCMD_20;
            EQCHARGETIME = EQTimeCMD_20;
            break;
        case 21:
            //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_21;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_21;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_21;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_21;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_21;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_21;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_21;
            CONCURCHARGEENDV =CONCURCHARGEENDV_21;                        //������������ܵ�ѹ
            CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_21;
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_21 ;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_21;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_21;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_21 ;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_21;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_21;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_21;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_21;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_21 ;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_21;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_21;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_21;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_21;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_21 ;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_21;                                        //Ԥ�����ʱ��4H
            ///EQ���׶���Ϣ������ʱ���ѹ����
            EQCHARGECMI  = EQCurrCMD_21;
            EQCHARGECMV  = EQVoltCMD_21;
            EQCHARGETIME = EQTimeCMD_21;
            break;
//����һ������22��
        case 22:
		   //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
		   PRECHARGECMI = PRECHARGECMI_22;                                 //ָ�����0.05C
		   PRECHARGECMV = PRECHARGECMV_22;                                   //ָ���ѹ����ܵ�ѹ
		   PRECHARGEENDV =PRECHARGEENDV_22;     //Ԥ���������ܵ�ѹ
		   PRECHARGEENDCNT = PRECHARGEENDCNT_22;                                 //Ԥ�����ʱ��2H
		   //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
		   CONCURCHARGECMI = CONCURCHARGECMI_22;                                //ָ�����0.167C
		   CONCURCHARGECMV = CONCURCHARGECMV_22;                                //ָ���ѹ����ܵ�ѹ
		   CONCURCHARGEENDI = CONCURCHARGEENDI_22;
		   CONCURCHARGEENDV =CONCURCHARGEENDV_22;                        //������������ܵ�ѹ
		   CONCURCHARGEENDCNT = CONCURCHARGEENDCNT_22;
		   //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
		   SCONCURCHARGECMI =SCONCURCHARGECMI_22 ;                                //ָ�����0.1C
		   SCONCURCHARGECMV =SCONCURCHARGECMV_22;                               //ָ���ѹ����ܵ�ѹ
		   SCONCURCHARGEENDV = SCONCURCHARGEENDV_22;                          //������������ܵ�ѹ
		   SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_22 ;                                        //��������ʱ��2H
		   //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
		   CONVOLCHARGECMI = CONVOLCHARGECMI_22;                                 //ָ�����0.06C
		   CONVOLCHARGECMV = CONVOLCHARGECMV_22;                               //ָ���ѹ����ܵ�ѹ
		   CONVOLCHARGEENDI = CONVOLCHARGEENDI_22;                                    //��ѹ��������ܵ���
		   CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_22;                                        //��ѹ����ʱ��2H
		   //������׶γ����Ϣ��������ѹָ���תʱ��
		   BCONVOLCHARGECMI =BCONVOLCHARGECMI_22 ;                                   //ָ�����0.01C
		   BCONVOLCHARGECMV = BCONVOLCHARGECMV_22;                               //ָ���ѹ����ܵ�ѹ
		   BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_22;          //������������ܵ�ѹ
		   BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_22;                                          //��ѹ����ʱ��2H
		   //����׶γ����Ϣ��������ѹָ���תʱ��
		   FLOATCHARGECMI = FLOATCHARGECMI_22;                                   //ָ�����0.01C
		   FLOATCHARGECMV =FLOATCHARGECMV_22 ;                                   //ָ���ѹ����ܵ�ѹ
		   FLOATCHARGEENDCNT = FLOATCHARGEENDCNT_22;                                 //Ԥ�����ʱ��4H
		   ///EQ���׶���Ϣ������ʱ���ѹ����
		   EQCHARGECMI  = EQCurrCMD_22;
		   EQCHARGECMV  = EQVoltCMD_22;
		   EQCHARGETIME = EQTimeCMD_22;
		   break;

        case 23://��ȡIIC��������
		   //Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��

		   PRECHARGECMI    = gPreChargeComi1;       //Ԥ��ָ�����0.05C  ��Ϊ��0.01A
		   PRECHARGECMV    = gPreChargeComv1;       //ָ���ѹ����ܵ�ѹ   ��Ϊ��0.1V
		   PRECHARGEENDV   = gPreChargeEndv1;       //Ԥ���������ܵ�ѹ   ��Ϊ��0.1V
		   PRECHARGEENDCNT = gPreChargeEndcnt1*1800000;      //Ԥ�����  ��Ϊ�ǰ�Сʱ

		   //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
		   CONCURCHARGECMI    = gConstantChargeComi1;    //ָ�����0.167C
		   CONCURCHARGECMV    = gConstantChargeComv1;    //ָ���ѹ����ܵ�ѹ
		   CONCURCHARGEENDV   = gConstantChargeEndv1;    //������������ܵ�ѹ
		   CONCURCHARGEENDCNT = gConstantChargeEndcnt1*1800000;      //Ԥ�����  ��Ϊ�ǰ�Сʱ

		  // CONCURCHARGEENDCNT = ASCCharge16System(gConstantChargeEndcnt1);
		   //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
		   SCONCURCHARGECMI    = gConstant2ChargeComi1;                                  //ָ�����0.1C
		   SCONCURCHARGECMV    = gConstant2ChargeComv1;                                //ָ���ѹ����ܵ�ѹ
		   SCONCURCHARGEENDV   = gConstant2ChargeEndv1;                            //������������ܵ�ѹ
		   SCONCURCHARGEENDCNT = gConstant2ChargeEndcnt1*1800000;      //Ԥ�����  ��Ϊ�ǰ�Сʱ

		 //  SCONCURCHARGEENDCNT = ASCCharge16System(gConstant2ChargeEndcnt1);                                          //��������ʱ��2H
		   //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
		   CONVOLCHARGECMI    = gConstantVoltageChargeComi1;                                   //ָ�����0.06C
		   CONVOLCHARGECMV    = gConstantVoltageChargeComv1;                                 //ָ���ѹ����ܵ�ѹ
		   CONVOLCHARGEENDI   =  gConstantVoltageChargeEndi1;                                     //��ѹ��������ܵ���
		   CONVOLCHARGEENDCNT = gConstantVoltageChargeEndcnt1*1800000;      //Ԥ�����  ��Ϊ�ǰ�Сʱ

		 //  CONVOLCHARGEENDCNT =  ASCCharge16System(gConstantVoltageChargeEndcnt1);                                         //��ѹ����ʱ��2H
		   //������׶γ����Ϣ��������ѹָ���תʱ��
		   BCONVOLCHARGECMI    = gTrickleChargeComi1;                                     //ָ�����0.01C
		   BCONVOLCHARGECMV    = gTrickleChargeComv1;                               //ָ���ѹ����ܵ�ѹ
		   BCONVOLCHARGEENDV   = gTrickleChargeEndv1;               //������������ܵ�ѹ
		   BCONVOLCHARGEENDCNT = gTrickleChargeEndcnt1*1800000;      //Ԥ�����  ��Ϊ�ǰ�Сʱ

		  // BCONVOLCHARGEENDCNT = ASCCharge16System(gTrickleChargeEndcnt1);                                         //��ѹ����ʱ��2H
		   //����׶γ����Ϣ��������ѹָ���תʱ��
		   FLOATCHARGECMI    = gFloatChargeComi1;                                  //ָ�����0.01C
		   FLOATCHARGECMV    = gFloatChargeComv1;                                    //ָ���ѹ����ܵ�ѹ
		   FLOATCHARGEENDCNT  = gFloatChargeEndcnt1*1800000;      //Ԥ�����  ��Ϊ�ǰ�Сʱ

		 //  FLOATCHARGEENDCNT = ASCCharge16System(gFloatChargeEndcnt1);                                 //Ԥ�����ʱ��4H
		   ///EQ���׶���Ϣ������ʱ���ѹ����
		   EQCHARGECMI  = gEQCurrCMD1;
		   EQCHARGECMV  = gEQVoltCMD1;
		   EQCHARGETIME = gEQTimeCMD1*1800000;
		   break;




        default://����22�����йػ�
            /*//Ԥ��׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            PRECHARGECMI = PRECHARGECMI_2;                                 //ָ�����0.05C
            PRECHARGECMV = PRECHARGECMV_2;                                   //ָ���ѹ����ܵ�ѹ
            PRECHARGEENDV =PRECHARGEENDV_2;     //Ԥ���������ܵ�ѹ
            PRECHARGEENDCNT = PRECHARGEENDCNT_2;                                 //Ԥ�����ʱ��2H
            //�������һ�׶γ����Ϣ��������ѹָ���ת��ѹ����תʱ���ڳ��������
            CONCURCHARGECMI = CONCURCHARGECMI_2;                                //ָ�����0.167C
            CONCURCHARGECMV = CONCURCHARGECMV_2;                                //ָ���ѹ����ܵ�ѹ
            CONCURCHARGEENDI = CONCURCHARGEENDI_2;
            CONCURCHARGEENDV =CONCURCHARGEENDV_2;                        //������������ܵ�ѹ
            //���������׶γ����Ϣ��������ѹָ���ת��ѹ��ʱ��
            SCONCURCHARGECMI =SCONCURCHARGECMI_2;                                //ָ�����0.1C
            SCONCURCHARGECMV =SCONCURCHARGECMV_2;                               //ָ���ѹ����ܵ�ѹ
            SCONCURCHARGEENDV = SCONCURCHARGEENDV_2;                          //������������ܵ�ѹ
            SCONCURCHARGEENDCNT =SCONCURCHARGEENDCNT_2;                                        //��������ʱ��2H
            //��ѹ���׶γ����Ϣ��������ѹָ���ת������ʱ��
            CONVOLCHARGECMI = CONVOLCHARGECMI_2;                                 //ָ�����0.06C
            CONVOLCHARGECMV = CONVOLCHARGECMV_2;                               //ָ���ѹ����ܵ�ѹ
            CONVOLCHARGEENDI = CONVOLCHARGEENDI_2;                                    //��ѹ��������ܵ���
            CONVOLCHARGEENDCNT = CONVOLCHARGEENDCNT_2;                                        //��ѹ����ʱ��2H
            //������׶γ����Ϣ��������ѹָ���תʱ��
            BCONVOLCHARGECMI =BCONVOLCHARGECMI_2;                                   //ָ�����0.01C
            BCONVOLCHARGECMV = BCONVOLCHARGECMV_2;                               //ָ���ѹ����ܵ�ѹ
            BCONVOLCHARGEENDV =BCONVOLCHARGEENDV_2;          //������������ܵ�ѹ
            BCONVOLCHARGEENDCNT = BCONVOLCHARGEENDCNT_2;                                          //��ѹ����ʱ��2H
            //����׶γ����Ϣ��������ѹָ���תʱ��
            FLOATCHARGECMI = FLOATCHARGECMI_2;                                   //ָ�����0.01C
            FLOATCHARGECMV =FLOATCHARGECMV_2;                                   //ָ���ѹ����ܵ�ѹ
            FLOATCHARGEENDCNT = FLOATCHARGECMV_2;                                        //Ԥ�����ʱ��4H*/
            PFC_OFF;     //����21�����йػ�
            CLR_KPOUT;
///            SysErr.bit.HardwareErr = 1;
            break;
    }
}



/*******************************************************************************
** ��������:    ChargeStateTest
** ��������:    ��ʼ���״̬��⺯��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void ChargeStartStateTest(void)
{
//			G_CanSend[6] = gstElecPara.OutVol&0X00FF;
//			G_CanSend[7] = (gstElecPara.OutVol>>8)&0X00FF;
    if(gstElecPara.OutVol <= PRECHARGEENDV)  //0 24
    {
    	G_CanSend[0]=0X04;
        ChargeStateParam.ChargeMode = CHGSTAT_PRECHARGE;  // Ԥ��״̬
        ChargeStateParam.ChargeCmdV = PRECHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
        ChargeStateParam.ChargeCmdI = PRECHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
    }
    else if(gstElecPara.OutVol <= CONCURCHARGEENDV) //�������һ�׶γ����Ϣ 1
    {
    	G_CanSend[0]=0X08;
        ChargeStateParam.ChargeMode = CHGSTAT_CONCURCHARGE;
        ChargeStateParam.ChargeCmdV = CONCURCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
        ChargeStateParam.ChargeCmdI = CONCURCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
    }
    else if(gstElecPara.OutVol < SCONCURCHARGEENDV) //���������׶γ����Ϣ2
    {
    	G_CanSend[0]=0X10;
        ChargeStateParam.ChargeMode = CHGSTAT_SCONCURCHARGE;
        ChargeStateParam.ChargeCmdV = SCONCURCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
        ChargeStateParam.ChargeCmdI = SCONCURCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
    }
    else if(gstElecPara.OutVol < BCONVOLCHARGEENDV)//������׶γ�� 4
    {
    	G_CanSend[0]=0X20;
        ChargeStateParam.ChargeMode = CHGSTAT_BCONVOLCHARGE;
        ChargeStateParam.ChargeCmdV = BCONVOLCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
        ChargeStateParam.ChargeCmdI = BCONVOLCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
    }
    else
    {
    	G_CanSend[0]=0X40;
        ChargeStateParam.ChargeMode = CHGSTAT_PRECHARGE;
        ChargeStateParam.ChargeCmdV = PRECHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
        ChargeStateParam.ChargeCmdI = PRECHARGECMI;
    }
}

/*******************************************************************************
** ��������:    PreChargeManage
** ��������:    Ԥ��״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void PreChargeManage(void)
{
	static	Uint16 gCurveNumCUNT = 1;
	gCurveNumCUNT++;

//	  G_CanSend[5] = ChargeStateParam.ChargeStat&0X00FF;
	G_CanSend[4] = ChargeStateParam.ChargeStat&0X00FF;
    G_CanSend[5] = (ChargeStateParam.ChargeStat>>8)&0X00FF;
    switch(ChargeStateParam.ChargeStat)
    {
//             G_CanSend[6] = gCurveNumCUNT&0X00FF;
//            G_CanSend[7] = (gCurveNumCUNT>>8)&0X00FF;
        case StepOne:
        	G_CanSend[6] = gCurveNumCUNT&0X00FF;
            G_CanSend[7] = (gCurveNumCUNT>>8)&0X00FF;
        	G_CanSend[0]=0X00;
            gsVolBatCompVal = gstElecPara.BatVol + BATCELLS;
            ChargeStateParam.ChargeStat = StepTwo;
            break;
        case StepTwo:
        	G_CanSend[0]=0X01;
        	G_CanSend[2] = gCurveNumCUNT&0X00FF;
			G_CanSend[3] = (gCurveNumCUNT>>8)&0X00FF;
            ChargeStateParam.ChargeCnt ++;
            if(ChargeStateParam.ChargeCnt > PRECHARGEENDCNT)
            {

               ChargeStateParam.ChargeMode = CHGSTAT_CONCURCHARGE;
               ChargeStateParam.ChargeCmdV = CONCURCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
               ChargeStateParam.ChargeCmdI = CONCURCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
               gsChargeCnt = 0;
               ChargeStateParam.ChargeStat = StepOne;
               ChargeStateParam.ChargeCnt = 0;
            }
            if((gstElecPara.BatVol) > (PRECHARGEENDV - gstSetLlcPARAM.LlcTempCompenVol))
            {


                gsChargeCnt ++;
                if(gsChargeCnt > 100)
                {
                	G_CanSend[0]=0X02;
                    ChargeStateParam.ChargeMode = CHGSTAT_CONCURCHARGE;
                    ChargeStateParam.ChargeCmdV = CONCURCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;////������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
                    ChargeStateParam.ChargeCmdI = CONCURCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
                    gsChargeCnt = 0;
                    ChargeStateParam.ChargeStat = StepOne;
                    ChargeStateParam.ChargeCnt = 0;
                }
            }
            else
            {
                gsChargeCnt = 0;
            }
            break;
    }
}

/*******************************************************************************
** ��������:    ConCurChargeManage
** ��������:    ����һ״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void ConCurChargeManage(void)
{

    switch(ChargeStateParam.ChargeStat)
    {
        case StepOne:
        	G_CanSend[0]=0X80;
            gsVolBatCompVal = gstElecPara.BatVol + 2 * BATCELLS;
            ChargeStateParam.ChargeStat = StepTwo;
            break;
        case StepTwo:
        //	G_CanSend[1]=0X00;
            ChargeStateParam.ChargeCnt ++;
            if(ChargeStateParam.ChargeCnt > CONCURCHARGEENDCNT)
            {
                /*if(gstElecPara.BatVol < gsVolBatCompVal)
                {
                    SysErr.bit.BatErr = 1;
                }
                else
                {*/
                    ChargeStateParam.ChargeMode = CHGSTAT_SCONCURCHARGE;
                    ChargeStateParam.ChargeCmdV = SCONCURCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;
                    ChargeStateParam.ChargeCmdI = SCONCURCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
                    gsChargeCnt = 0;
                    ChargeStateParam.ChargeStat = StepOne;
                    ChargeStateParam.ChargeCnt = 0;
                //}
            }
            if((gstElecPara.BatVol) > (CONCURCHARGEENDV - gstSetLlcPARAM.LlcTempCompenVol))
            {
                gsChargeCnt ++;
                if(gsChargeCnt > 100)
                {
                    ChargeStateParam.ChargeMode = CHGSTAT_SCONCURCHARGE;
                    ChargeStateParam.ChargeCmdV = SCONCURCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;
                    ChargeStateParam.ChargeCmdI = SCONCURCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
                    gsChargeCnt = 0;
                    ChargeStateParam.ChargeStat = StepOne;
                    ChargeStateParam.ChargeCnt = 0;
                }
            }
            else
            {
                gsChargeCnt = 0;
            }
            break;
    }
}

/*******************************************************************************
** ��������:    SConCurChargeManage
** ��������:    ������״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void SConCurChargeManage(void)
{


    switch(ChargeStateParam.ChargeStat)
    {
        case StepOne:
        //	G_CanSend[1]=0X01;

            gsVolBatCompVal = gstElecPara.BatVol;
            ChargeStateParam.ChargeStat = StepTwo;
            break;
        case StepTwo:
        //	G_CanSend[1]=0X02;
            ChargeStateParam.ChargeCnt ++;
            if(ChargeStateParam.ChargeCnt > SCONCURCHARGEENDCNT)
            {
                    ChargeStateParam.ChargeMode = CHGSTAT_CONVOLCHARGE;
                    ChargeStateParam.ChargeCmdV = CONVOLCHARGECMV  - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
                    ChargeStateParam.ChargeCmdI = CONVOLCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
                    gsChargeCnt = 0;
                    ChargeStateParam.ChargeStat = StepOne;
                    ChargeStateParam.ChargeCnt = 0;
            }
            if((gstElecPara.BatVol) > (SCONCURCHARGEENDV - gstSetLlcPARAM.LlcTempCompenVol))
            {
                gsChargeCnt ++;
                if(gsChargeCnt > 100)
                {
                    ChargeStateParam.ChargeMode = CHGSTAT_CONVOLCHARGE;
                    ChargeStateParam.ChargeCmdV = CONVOLCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
                    ChargeStateParam.ChargeCmdI = CONVOLCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
                    gsChargeCnt = 0;
                    ChargeStateParam.ChargeStat = StepOne;
                    ChargeStateParam.ChargeCnt = 0;
                }
            }
            else
            {
                gsChargeCnt = 0;
            }
            break;
    }
}
/*******************************************************************************
** ��������:    ConVolChargeManage
** ��������:    ��ѹ״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void ConVolChargeManage(void)
{
    ChargeStateParam.ChargeCnt ++;
    if(ChargeStateParam.ChargeCnt > CONVOLCHARGEENDCNT)
    {
        ChargeStateParam.ChargeMode = CHGSTAT_BCONVOLCHARGE;
        ChargeStateParam.ChargeCmdV = BCONVOLCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
        ChargeStateParam.ChargeCmdI = BCONVOLCHARGECMI; //��λ��0.01A ��27A��������Ϊ2700
        gsChargeCnt = 0;
        ChargeStateParam.ChargeCnt = 0;
    }
    if(gstElecPara.OutCur < CONVOLCHARGEENDI)
    {
        gsChargeCnt ++;
        if(gsChargeCnt > 100)
        {
            ChargeStateParam.ChargeMode = CHGSTAT_BCONVOLCHARGE;
            ChargeStateParam.ChargeCmdV = BCONVOLCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;
            ChargeStateParam.ChargeCmdI = BCONVOLCHARGECMI;      //��λ��0.01A ��27A��������Ϊ2700
            gsChargeCnt = 0;
            ChargeStateParam.ChargeCnt = 0;
        }
    }
    else
    {
        gsChargeCnt = 0;
    }
}

/*******************************************************************************
** ��������:    BConVolChargeManage
** ��������:    ���״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void BConVolChargeManage(void)
{

    switch(ChargeStateParam.ChargeStat)
    {
        case StepOne:
        	//G_CanSend[1]=0X04;
            gsVolBatCompVal = gstElecPara.BatVol + BATCELLS;
            ChargeStateParam.ChargeStat = StepTwo;
            break;
        case StepTwo:

            ChargeStateParam.ChargeCnt ++;
            if(ChargeStateParam.ChargeCnt > BCONVOLCHARGEENDCNT)
            {
            //	G_CanSend[1]=0X08;
                    gsChargeCnt = 0;
                    ChargeStateParam.ChargeStat = StepThree;
                    ChargeStateParam.ChargeCnt = 0;

            }
            if((gstElecPara.BatVol > (BCONVOLCHARGEENDV - gstSetLlcPARAM.LlcTempCompenVol))//�����������ܵ�ѹ
            		||(gstElecPara.OutCur < BCONVOLCHARGEENDI ))////�����������
            {
            	G_CanSend[1]=0X10;
                gsChargeCnt ++;
                if(gsChargeCnt > 50)
                {
                    gsChargeCnt = 0;
                    ChargeStateParam.ChargeStat = StepThree;
                    ChargeStateParam.ChargeCnt = 0;
                }
            }
            else
            {
                gsChargeCnt = 0;
            }
            break;
        case StepThree:
        	G_CanSend[1]=0X20;
        	ChargeStateParam.ChargeMode = CHGSTAT_FLOATCHARGE;
//            if(1 == EQ_MFlag)
//            {
//                ChargeStateParam.ChargeMode = CHGSTAT_EQMODECHARGE;
//                ChargeStateParam.ChargeCmdV = EQCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
//                ChargeStateParam.ChargeCmdI = EQCHARGECMI;                                  //��λ��0.01A ��27A��������Ϊ2700
//
//            }
//            else
//            {
//                gsChargeOffFlag = 1;
//                ChargeOffFlag = 1;
//                DC_EN;
//                ChargeStateParam.ChargeStat = StepOne;
//                gseSystemState = ShutDown_State;
//                gseStateStep = StepOne;
//                CLR_KPOUT;
//            }
            break;
        default:
            break;
    }
}
/******************************************************************************
 ��������:EQChargeManage();
 EQģʽ��������ÿ�³���ʼ����
 EQģʽʹ���Ժ��½����ڿ���;
 ���⻹����ͨ���ֶ���ť��EQ��־�����޸�;
 * ****************************************************************************/
static void EQChargeManage(void)
{
    ChargeStateParam.ChargeCnt++;
    if(ChargeStateParam.ChargeCnt > EQCHARGETIME)
    {
        EQ_MFlag = 0;
/*        if(WriteByte(11,EQ_MFlag))
        {*/
            ChargeStateParam.ChargeStat = StepOne;
            gseSystemState = ShutDown_State;
            gseStateStep = StepFour;
            gsChargeOffFlag = 1;
            ChargeOffFlag = 1;
            DC_EN;
 //       }
    }
}
/******************************************************************************
 ��������:PAUSEChargeManage();
 ������Ԥ��׶���Ч
���ڴ��ô���
 * ****************************************************************************/
static void PAUSEChargeManage(void)
{

}
/*******************************************************************************
** ��������:    FloatChargeManage
** ��������:    ����״̬����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void FloatChargeManage(void)
{
    ChargeStateParam.ChargeCnt ++;
    if(ChargeStateParam.ChargeCnt > FLOATCHARGEENDCNT)
    {

    	G_CanSend[1]=0X40;
        ChargeStateParam.ChargeStat = StepOne;
        gseSystemState = ShutDown_State;
        gseStateStep = StepFour;
        gsChargeOffFlag = 1;
        ChargeOffFlag = 1;
        DC_EN;
    }
    else
    {
     	G_CanSend[1]=0X80;
        ChargeStateParam.ChargeMode = CHGSTAT_FLOATCHARGE;
		ChargeStateParam.ChargeCmdV = FLOATCHARGECMV - gstSetLlcPARAM.LlcTempCompenVol;//������ѹ�ŵ�10�� ��27.4V�趨Ϊ274
		ChargeStateParam.ChargeCmdI = FLOATCHARGECMI;
    }
}

/*******************************************************************************
** ��������:    InputPowerLimiting
** ��������:    ������Ӱ��Ĺ�������ֵ����
** ��ʽ����:    ���뷴��,ϵͳ������������
** ���ز���:    �������뷴���õ���ϵͳ��ǰ����������ֵ
*******************************************************************************/
static int32 InputPowerLimiting(int16 InputFeed)
{
    int32 tmpInputPowerLimiting = 0;
    tmpInputPowerLimiting = InputFeed * 7;
    return (tmpInputPowerLimiting);
}

/*******************************************************************************
** ��������:    InputPowerLimiting
** ��������:    ������Ӱ��Ĺ�������ֵ����
** ��ʽ����:    ���뷴��,ϵͳ������������
** ���ز���:    �������뷴���õ���ϵͳ��ǰ����������ֵ
*******************************************************************************/
static int16 TempPowerLimiting(int16 TempFeed)
{
    int16 tmpDeratingRate;

    if(TempFeed >= TEMP_PROTECT)
    {
        tmpDeratingRate = 0;
    }
    else if(TempFeed <= TEMP_DERATING)
    {
        tmpDeratingRate = 128;
    }
    else
    {
        tmpDeratingRate = (int32)(TEMP_PROTECT - TempFeed) * 128 / (TEMP_PROTECT - TEMP_DERATING);
    }
    return (tmpDeratingRate);
}

/*******************************************************************************
** ��������:    PowerLmiting
** ��������:    �������ƺ���,���ݵ�ǰ�������ѹ���¶ȼ��㵱ǰϵͳ���������������
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
int32 PowerLimiting(void)
{
    int32 tmpMaxPwr;
    int16 tmpDeratingRate;
    tmpMaxPwr = POWER_MAX;
    if(gsPFCUploadData.PFCAcInVolt < AC_DERATING_VOL)
    {

		//G_CanSend[4] = 0XFF;
		//G_CanSend[5] = (gsPFCUploadData.PFCAcInVolt>>8)&0X00FF;
        tmpMaxPwr = (int32)gsPFCUploadData.PFCAcInVolt * 800;  //800000
    //	G_CanSend[6] = tmpMaxPwr&0X00FF;
    	//G_CanSend[7] = (tmpMaxPwr>>8)&0X00FF;
    }
    if(TEMP_DATA > TEMP_DERATING)
    {
        tmpDeratingRate = TempPowerLimiting(TEMP_DATA);
        tmpMaxPwr = (tmpMaxPwr * tmpDeratingRate)>>7;
    }
    return (tmpMaxPwr);
}


/*******************************************************************************
** ��������:    RenewCurrentLmiting
** ��������:    ���ݵ�ǰϵͳ���������������뵱ǰ�����ѹ,���㵱ǰ����������
**              ������,����ϵͳ����������Ƚ�,��Сֵ����ϵͳ������������ָ��ֵ
** ��ʽ����:    ��ǰϵͳ���������,��ǰ����ָ��,��ǰ�����ѹ����ֵ
** ���ز���:    ����õ��ĵ�ǰ����������
*******************************************************************************/
static int16 RenewCurrentLmiting()
{
    int16 tmpMaxCur;
    int32 tmpMaxPwr;
    tmpMaxPwr = PowerLimiting();
    if(gstElecPara.BatVol > 35)/*120*/
    {
        tmpMaxCur = tmpMaxPwr / gstElecPara.BatVol;
    }
    else
    {
        tmpMaxCur = IOUT_MAX;
    }
    if(tmpMaxCur > IOUT_MAX)
    {
        tmpMaxCur = IOUT_MAX;
    }
    else if(tmpMaxCur < 0)
    {
        tmpMaxCur = 0;
    }
    return (tmpMaxCur);
}

/*******************************************************************************
** ��������:    GetLedIndex
** ��������:    LED������:��ȡ��ǰLEDָʾ״̬
** ��ʽ����:    ��
** ���ز���:    ��ǰ��LED��ʾ����������define����define�������λ��
*******************************************************************************/
#define LED_NO_ACTION_INDEX   0
#define LED_ONE_FLASH_INDEX   1
#define LED_TWO_FLASH_INDEX   2
#define LED_THREE_FLASH_INDEX 3
#define LED_FOUR_FLASH_INDEX  4
#define LED_ALL_ON_INDEX      5
#define LED_ALL_FLASH_INDEX   6

Uint16 GetLedIndex(void)
{
    Uint16 tmpIndex = LED_NO_ACTION_INDEX;
    switch(gseSystemState)
    {
        case SelfTest_State:
            tmpIndex = LED_ALL_ON_INDEX;
            break;
        case StandBy_State:
            tmpIndex = LED_ALL_ON_INDEX;
            break;
        case PulseOn_State:
            tmpIndex = LED_ONE_FLASH_INDEX;
            break;
        case ChargeOn_State:
            switch(ChargeStateParam.ChargeMode)
            {
                case CHGSTAT_PRECHARGE:
                    tmpIndex = LED_ONE_FLASH_INDEX;
                    break;
                case CHGSTAT_CONCURCHARGE:
                    tmpIndex = LED_TWO_FLASH_INDEX;
                    break;
                case CHGSTAT_SCONCURCHARGE:
                    tmpIndex = LED_THREE_FLASH_INDEX;
                    break;
                case CHGSTAT_CONVOLCHARGE:
                    tmpIndex = LED_FOUR_FLASH_INDEX;
                    break;
                case CHGSTAT_BCONVOLCHARGE:
                    tmpIndex = LED_FOUR_FLASH_INDEX;
                    break;
                case CHGSTAT_FLOATCHARGE:
                    tmpIndex = LED_ALL_ON_INDEX;
                    break;
                default:
                     break;
            }
            break;
        case Protection_State:
            tmpIndex = LED_ALL_FLASH_INDEX;
            break;
        case ShutDown_State:
            tmpIndex = LED_ALL_ON_INDEX;
            break;
        default:
            break;
    }
    return(tmpIndex);
}
