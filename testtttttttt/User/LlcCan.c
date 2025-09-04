/*******************************************************************************
 * LlcCan.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcCan.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
interrupt void Ecan_Isr(void);// Can�����ж�

Uint16 G_CanSend[16];
Uint16 G_CanReceive[16];
Uint32 G_ReceiveID = 0;
Uint16 G_CanReceiveFlag = 0;// Can���ձ�־
extern Uint16 USBReadByte(Uint16 addr);

extern void WriteCorrectionDate(viod);
extern  Uint16 u16Vout_K;     //��ѹϵ��K981;
extern  Uint16 s16Vout_B;     //��ѹϵ��B

extern  Uint16 u16BatVol_K;   //���ϵ��K981;
extern  Uint16 s16BatVol_B;   //���ϵ��B
// �ṹ�嶨��--------------------------------------------------------------
struct SYSTEM_INFO_ECAN_OUT SystemInfoECANOut;   // ϵͳCAN�������
volatile union ECAN_MESSAGE_STRU CanData;
CAN_GUI_CMD CanGuiCmd = INIT_CAN_GUI_CMD;
// ========================================================================
// �������ƣ�CAN_Init
// �������ܣ�CAN��ʼ��
// ========================================================================
void CAN_Init(void)//OK
{
    struct ECAN_REGS ECanaShadow;

    EALLOW;

    ECanaShadow.CANMIM.all  = 0x20000000;   // Enable interrupts for 29 mailboxes
    ECanaRegs.CANMIM.all = ECanaShadow.CANMIM.all;

    ECanaShadow.CANMIL.all  = 0x00000000 ; // Interrupts asserted on eCAN0INT
    ECanaRegs.CANMIL.all = ECanaShadow.CANMIL.all;

    ECanaShadow.CANGIM.all = 0;
    ECanaShadow.CANGIM.bit.I0EN = 1;   // Enable eCAN1INT or eCAN0INT
    ECanaRegs.CANGIM.all = ECanaShadow.CANGIM.all;

    // ����TX��RX����
    ECanaShadow.CANTIOC.all = ECanaRegs.CANTIOC.all;
    ECanaShadow.CANTIOC.bit.TXFUNC = 1;
    ECanaRegs.CANTIOC.all = ECanaShadow.CANTIOC.all;

    ECanaShadow.CANRIOC.all = ECanaRegs.CANRIOC.all;
    ECanaShadow.CANRIOC.bit.RXFUNC = 1;
    ECanaRegs.CANRIOC.all = ECanaShadow.CANRIOC.all;

    // ѡ��eCANģʽ
    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.SCB = 1;   //Ecanģʽ
    ECanaShadow.CANMC.bit.ABO = 1;   //�����Զ��ָ�����
    ECanaShadow.CANMC.bit.DBO = 1;   //��λ���ȷ���
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

    // ��ֹ��������
    ECanaRegs.CANME.all = 0;

    //ECanaRegs.CANGAM.bit.AMI = 1;
    //����֡���Ͷ���-------------------------------------------------------
    #if Can_Message_Type  // ��չ֡
        ECanaMboxes.MBOX29.MSGID.all = 0xCBC80000;
    #else                 //��׼֡
        ECanaMboxes.MBOX29.MSGID.all = 0x4BC80000;
    #endif

    // �������俪�������˲�
    ECanaLAMRegs.LAM29.all = 0xFFFFFFFF;

    // ������������ƼĴ�����0
    ECanaMboxes.MBOX0.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX1.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX2.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX3.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX4.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX5.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX6.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX7.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX8.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX9.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX10.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX11.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX12.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX13.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX14.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX15.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX16.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX17.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX18.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX19.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX20.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX21.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX22.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX23.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX24.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX25.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX26.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX27.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX28.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX29.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX30.MSGCTRL.all = 0x00000000;
    ECanaMboxes.MBOX31.MSGCTRL.all = 0x00000000;

    // ������TAn��RMPn���жϱ�־λ��0
    ECanaRegs.CANTA.all = 0xFFFFFFFF;       // �������TAnλ
    ECanaRegs.CANRMP.all = 0xFFFFFFFF;      // �������RMPnλ
    ECanaRegs.CANGIF0.all = 0xFFFFFFFF;     // ��������жϱ�־λ
    ECanaRegs.CANGIF1.all = 0xFFFFFFFF;     // ��������жϱ�־

    // ���ý������䳤��----------------------------------------------------
    ECanaMboxes.MBOX16.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX17.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX18.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX19.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX20.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX21.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX22.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX23.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX24.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX25.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX26.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX27.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX28.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX29.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX30.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;
    ECanaMboxes.MBOX31.MSGCTRL.bit.DLC = CAN_DATA_LENGTH;

    // ����0~15Ϊ�������䣬16~31Ϊ��������
    ECanaRegs.CANMD.all = 0xFFFF0000;

    // ����29�������0��15������
    ECanaRegs.CANME.all = 0x2000FFFF;

    // ���������޸����üĴ���

    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.CCR = 1 ;
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

    // �ȴ������޸����üĴ���
    do
    {
        ECanaShadow.CANES.all = ECanaRegs.CANES.all;
    }
    while (1 != ECanaShadow.CANES.bit.CCE);

    ECanaShadow.CANBTC.all = 0;

    // ���ò�����
    ECanaShadow.CANBTC.bit.BRPREG = BRP_REG;
    ECanaShadow.CANBTC.bit.TSEG1REG = TSEG1_REG;
    ECanaShadow.CANBTC.bit.TSEG2REG = TSEG2_REG;

    ECanaShadow.CANBTC.bit.SAM = 1;
    ECanaRegs.CANBTC.all = ECanaShadow.CANBTC.all;

    // �����ֹ�޸����üĴ���
    ECanaShadow.CANMC.all = ECanaRegs.CANMC.all;
    ECanaShadow.CANMC.bit.CCR = 0 ;
    ECanaRegs.CANMC.all = ECanaShadow.CANMC.all;

    // �ȴ���ֹ�޸����üĴ���
    do
    {
        ECanaShadow.CANES.all = ECanaRegs.CANES.all;
    }
    while (0 != ECanaShadow.CANES.bit.CCE);


    PieVectTable.ECAN0INTA = &Ecan_Isr; // CAN�����жϷ�����
    PieCtrlRegs.PIEIER9.bit.INTx5 = 1;  // Can�����ж�
    IER |= M_INT9;                      // ʹ��CPU�ж�9��INT9��

    EDIS;
}

// ========================================================================
// �������ƣ�EcanaMailBoxSend
// �������ܣ�CAN���ͺ���
//========================================================================
void EcanaMailBoxSend(Uint16 Num,Uint32 ID,Uint32 Low,Uint32 High)
{
    Uint32 IDTemp = 0;
    Uint32 Temp = 0;
    Uint32 CANMERegs = 0;
    volatile struct MBOX *pECanaMboxs;

    #if Can_Message_Type  // ��չ֡
         IDTemp = ID | 0x80000000;
    #else                 //��׼֡
         IDTemp = ID << 18;
    #endif

    Temp = 1UL << Num;

    CANMERegs = ECanaRegs.CANME.all;
    CANMERegs &=~Temp;
    ECanaRegs.CANME.all = CANMERegs;

    pECanaMboxs = &ECanaMboxes.MBOX0 + Num;
    pECanaMboxs ->MSGID.all = IDTemp;
    pECanaMboxs ->MSGCTRL.all = 8;
    pECanaMboxs ->MDL.all = Low;
    pECanaMboxs ->MDH.all = High;

    CANMERegs = ECanaRegs.CANME.all;
    CANMERegs |= Temp;
    ECanaRegs.CANME.all = CANMERegs;

    ECanaRegs.CANTRS.all = Temp;
    ECanaRegs.CANTA.all = Temp;
}

// ========================================================================
// �������ƣ�EcanaMailBoxSendword
// ���������Uint16 Num,����� Uint32 ID,CANͨѶID Uint16 LowLW,��32λ����
// Uint16 LowHW,��32λ���� Uint16 HighLW,��32λ���� Uint16 HighHW,��32λ����
// �������ܣ�CAN���ͺ���
//========================================================================
void EcanaMailBoxSendword(Uint16 Num,Uint32 ID,Uint16 LowLW,Uint16 LowHW,Uint16 HighLW,Uint16 HighHW)
{
    Uint32 IDTemp = 0;
    Uint32 Temp = 0;
    Uint32 CANMERegs = 0;
    volatile struct MBOX *pECanaMboxs;

    #if Can_Message_Type  // ��չ֡
         IDTemp = ID | 0x80000000;
    #else                 //��׼֡
         IDTemp = ID << 18;
    #endif

    Temp = 1UL << Num;

    CANMERegs = ECanaRegs.CANME.all;
    CANMERegs &=~Temp;
    ECanaRegs.CANME.all = CANMERegs;

    pECanaMboxs = &ECanaMboxes.MBOX0 + Num;
    pECanaMboxs ->MSGID.all = IDTemp;
    pECanaMboxs ->MSGCTRL.all = 8;
    //pECanaMboxs ->MDL.all = Low;
    //pECanaMboxs ->MDH.all = High;
    pECanaMboxs ->MDL.word.LOW_WORD = LowLW;
    pECanaMboxs ->MDL.word.HI_WORD = LowHW;
    pECanaMboxs ->MDH.word.LOW_WORD = HighLW;
    pECanaMboxs ->MDH.word.HI_WORD = HighHW;

    CANMERegs = ECanaRegs.CANME.all;
    CANMERegs |= Temp;
    ECanaRegs.CANME.all = CANMERegs;

    ECanaRegs.CANTRS.all = Temp;
    ECanaRegs.CANTA.all = Temp;
}

// ========================================================================
// �������ƣ�GetCanReceiveData
// �������ܣ���ȡCAN��������
//========================================================================
void GetCanReceiveData(void)
{
    // ���ޱ���------------------------------------------------------------
    if (ECanaRegs.CANRMP.bit.RMP29 == 1)
    {
        ECanaRegs.CANME.bit.ME29 = 0;

        #if Can_Message_Type  // ��չ֡
         SystemInfoECANOut.ECanRxID = ECanaMboxes.MBOX29.MSGID.all & 0x1FFFFFFF;
        #else                 //��׼֡
         SystemInfoECANOut.ECanRxID = (ECanaMboxes.MBOX29.MSGID.all & 0x1FFC0000)>>18;
        #endif

        SystemInfoECANOut.ECanRxDataLow  = ECanaMboxes.MBOX29.MDL.all;  //����CAN��4λ����
        SystemInfoECANOut.ECanRxDataHigh = ECanaMboxes.MBOX29.MDH.all;  //����CAN��4λ����

        G_CanReceiveFlag = 1;

        ECanaRegs.CANME.bit.ME29 = 1;

        ECanaRegs.CANRMP.bit.RMP29 = 1;
    }
    else
    {
        ;
    }//if (ECanaRegs.CANRMP.bit.RMP29 == 1)����

}

// ========================================================================
// �������ƣ�Ecan_Isr
// �������ܣ�Can�����ж�
//========================================================================
interrupt void Ecan_Isr(void)
{
    EINT;

    GetCanReceiveData();// CAN�����ж�������

    DINT;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9; // �ж�ȷ��
}

// ========================================================================
// �������ƣ�CanCommunication
// �������ܣ�Canͨ��
// ========================================================================
void CanCommunication(void)
{
	//�ſ��Ϳ�������
        CanSendDebug();//����λ������Ϣ��
        CanReceDebug();//����λ����Ϣ��
}

// ========================================================================
// �������ƣ�InitCanVariable
// �������ܣ���ʼ��CAN����
// ========================================================================
void InitCanVariable(void)
{
    Uint16 i = 0;

    for(i = 0; i < 50; i++)
    {
        G_CanSend[i] = 0;
        G_CanReceive[i] = 0;
    }

    // �ֶ�����
     G_CanReceive[1] = 3100;   // �����ѹָ��ֵ��31V;
     G_CanReceive[2] = 2700;   // �������ָ��ֵ��27A
}


// ========================================================================
// �������ƣ�CanSendDebug
// �������ܣ�CAN���͵���
// ========================================================================
void CanSendDebug(void)
{
    static Uint32 S_SendCount = 0;             // ���ķ������ڼ�����
    // ��ʼ���ݷ���--------------------------------------------------------
    CanData.all.Low  = 0x0FFFFFFFF;             // ������ݵ�λ
    CanData.all.High = 0x0FFFFFFFF;             // ������ݸ�λ

    // ���͵��Ա���----------------------------------------------------------
    S_SendCount++;

    if (S_SendCount == Time_500ms)
    {


        CanData.CanSend.CanSend1 = (Uint16)((G_CanSend[0]<<8)|G_CanSend[1]) ;
        CanData.CanSend.CanSend2 = (Uint16)((G_CanSend[2]<<8)|G_CanSend[3]) ;
        CanData.CanSend.CanSend3 = (Uint16)((G_CanSend[4]<<8)|G_CanSend[5]) ;
        CanData.CanSend.CanSend4 = (Uint16)((G_CanSend[6]<<8)|G_CanSend[7]) ;
        EcanaMailBoxSend(Email_num1, ID_0x18FFABC1, CanData.all.Low, CanData.all.High);
        S_SendCount = 0;

    }
//    else if (S_SendCount == Time_1000ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)((G_CanSend[9]<<8)|G_CanSend[8]) ;
//        CanData.CanSend.CanSend2 = (Uint16)((G_CanSend[11]<<8)|G_CanSend[10]) ;
//        CanData.CanSend.CanSend3 = (Uint16)((G_CanSend[13]<<8)|G_CanSend[12]) ;
//        CanData.CanSend.CanSend4 = (Uint16)((G_CanSend[15]<<8)|G_CanSend[14]) ;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC2, CanData.all.Low, CanData.all.High);
//        S_SendCount = 0;
//    }
//    else if (S_SendCount == Time_30ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[9] ;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[10] ;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[11];
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[12] ;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC3, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_40ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[13] ;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[14] ;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[15] ;
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[16] ;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC4, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_50ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[17] ;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[18] ;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[19] ;
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[20] ;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC5, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_60ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[21] ;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[22] ;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[23] ;
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[24] ;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC6, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_70ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[25] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[26] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[27] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[28] + CAN_DATA_OFFSET;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC7, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_80ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[29] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[30] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[31] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[32] + CAN_DATA_OFFSET;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC8, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_90ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[33] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[34] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[35] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[36] + CAN_DATA_OFFSET;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABC9, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_100ms)
//    {
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[37] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[38] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[39] + CAN_DATA_OFFSET;
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[40] + CAN_DATA_OFFSET;
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABCA, CanData.all.Low, CanData.all.High);
//    }
//    else if (S_SendCount == Time_110ms)
//    {
//
//        CanData.CanSend.CanSend1 = (Uint16)G_CanSend[41];
//        CanData.CanSend.CanSend2 = (Uint16)G_CanSend[42];
//        CanData.CanSend.CanSend3 = (Uint16)G_CanSend[43];
//        CanData.CanSend.CanSend4 = (Uint16)G_CanSend[44];
//        EcanaMailBoxSend(Email_num1, ID_0x18FFABCD, CanData.all.Low, CanData.all.High);
//        S_SendCount = 0;
//    }

    else
    {
        ;
    }
}

// ========================================================================
// �������ƣ�CanReceDebug
// �������ܣ�Can���յ���
//========================================================================
void CanReceDebug()
{
    // ��������--------------------------------------------------------
    if (G_CanReceiveFlag == 1)
    {
        // ��֤��ȡ��ID�������Ƕ�Ӧ��--------------------------------------
        G_ReceiveID = SystemInfoECANOut.ECanRxID;
        CanData.all.Low  = SystemInfoECANOut.ECanRxDataLow;
        CanData.all.High = SystemInfoECANOut.ECanRxDataHigh;

        // ������λ�����ͱ���----------------------------------------------
        if (G_ReceiveID == ID_0x18FFABD1)
        {
            G_CanReceive[0] = CanData.CanSend.CanSend1&0x00FF;
            G_CanReceive[1] = CanData.CanSend.CanSend1>>8;
            G_CanReceive[2] = CanData.CanSend.CanSend2&0x00FF;
            G_CanReceive[3] = CanData.CanSend.CanSend2>>8;
            G_CanReceive[4] = CanData.CanSend.CanSend3&0x00FF;
            G_CanReceive[5] = CanData.CanSend.CanSend3>>8;
            G_CanReceive[6] = CanData.CanSend.CanSend4&0x00FF;
            G_CanReceive[7] = CanData.CanSend.CanSend4>>8;

            WriteCorrectionDate();//У��

        }
        else if (G_ReceiveID == ID_0x18FFABD2)
        {
            G_CanReceive[8] = CanData.CanSend.CanSend1&0x00FF;
            G_CanReceive[9] = CanData.CanSend.CanSend1>>8;
            G_CanReceive[10] = CanData.CanSend.CanSend2&0x00FF;
            G_CanReceive[11] = CanData.CanSend.CanSend2>>8;
            G_CanReceive[12] = CanData.CanSend.CanSend3&0x00FF;
            G_CanReceive[13] = CanData.CanSend.CanSend3>>8;
            G_CanReceive[14] = CanData.CanSend.CanSend4&0x00FF;
            G_CanReceive[15] = CanData.CanSend.CanSend4>>8;
        }
//        else if (G_ReceiveID == CANRECE_ID3)
//        {
//            G_CanReceive[9] = CanData.CanSend.CanSend1;
//            G_CanReceive[10] = CanData.CanSend.CanSend2;
//            G_CanReceive[11] = CanData.CanSend.CanSend3;
//            G_CanReceive[12] = CanData.CanSend.CanSend4;
//        }
//        else if (G_ReceiveID == CANRECE_ID4)
//        {
//            G_CanReceive[13] = CanData.CanSend.CanSend1;
//            G_CanReceive[14] = CanData.CanSend.CanSend2;
//            G_CanReceive[15] = CanData.CanSend.CanSend3;
//            G_CanReceive[16] = CanData.CanSend.CanSend4;
//        }
//        else if (G_ReceiveID == CANRECE_ID5)
//        {
//            G_CanReceive[17] = CanData.CanSend.CanSend1;
//            G_CanReceive[18] = CanData.CanSend.CanSend2;
//            G_CanReceive[19] = CanData.CanSend.CanSend3;
//            G_CanReceive[20] = CanData.CanSend.CanSend4;
//        }
        else
        {
            ;
        }

        G_CanReceiveFlag = 0;// ����
    }
    else
    {
        ;
    }

}
//===========================================================================
// End of file.
//===========================================================================

