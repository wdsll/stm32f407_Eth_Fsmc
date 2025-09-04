/*******************************************************************************
 * LlcLin.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcMain.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define DEFAULT_K  1000    // Ĭ��Kֵ
#define LINErrTIME 1000    // SCI��ʱ����ʱ��
typedef enum{
    Start=0,            //��ʼ��
    Cmd,                //������
    Addre,              //��ַ
    DataL,              //����L
    DataH,              //����H
    Chm,                //У���
    End,                //������
}COM_type;

typedef enum{
    NO_Pro = 0 ,
    ReadP = 0xa0,       //����������
    WritP = 0x0a,       //����д����
    AnsrP = 0xaa,       //����Ӧ��
}CMD_type;

typedef enum{
    VinSamp_ADJ,        // �����ѹ AD����У׼
    IinSamp_ADJ,        // ������� AD����У׼
    FreqSamp_ADJ,       // ����Ƶ�� AD����У׼
    PFCSamp_ADJ,        // PFC��ѹ AD����У׼
    TempSamp_ADJ,       // �¶� AD����У׼
    PFCVolt_ADJ,        // PFC��ѹ����У׼
    PFCVolt_Set,        // PFC��ѹ����ֵ
    Ctrl_CMD,           // ��������
    Vin_VAL,            // �����ѹ
    Iin_VAL,            // �������
    Freq_VAL,           // ����Ƶ��
    PFC_VAL,            // PFC��ѹ
    Temp_VAL,           // �¶�
    Work_State,         // ����״̬
    ProCfg_State,       // ����״̬
    AdjCfg_State,       // ����״̬
    Cmd_State,          // ����״̬
    Sub_State,          // �ӹ���״̬
    Err_State,          // ����״̬
}CMDadds_type;
/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/
sPFCUPLOADDATA gsPFCUploadData;
ePfcCmd_t gPfcCmd = PfcOff;
/*******************************************************************************
**                               ��������
*******************************************************************************/
static Uint16 gLinRDataA[8] = {0,0,0,0,0,0,0,0};  // Received data for SCI-A
static Uint16 RxABuffer[7]  = {0,0,0,0,0,0,0};
static Uint16 TxABuffer[7]  = {0,0,0,0,0,0,0};
static Uint32 LinL0IntVect  = 0;
static int16 gPfcVoutSet   = 0;
static Uint16 gLinRecOkFlag = 0;
static Uint16 PfcCfgCounter = 0;
static Uint16 sLINErrConter = 0;

extern Uint16 ChargeOffFlag;
/*******************************************************************************
**                               ��������
*******************************************************************************/
void LinInit(void);
void LinManage(void);
void PfcConfi(void);
static void LINErrCheck(void);
__interrupt void Lina_Level0_ISR(void);

/*******************************************************************************
** ��������:    LinInit
** ��������:    ��ʼ��LIN
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void LinInit(void)
{
    //Allow write to protected registers
    EALLOW;

    LinaRegs.SCIGCR0.bit.RESET = 0; //Into reset
    LinaRegs.SCIGCR0.bit.RESET = 1; //Out of reset

    LinaRegs.SCIGCR1.bit.SWnRST = 0; //Into software reset

    //SCI Configurations
    LinaRegs.SCIGCR1.bit.COMMMODE = 0;   //Idle-Line Mode
    LinaRegs.SCIGCR1.bit.TIMINGMODE = 1; //Asynchronous Timing
    LinaRegs.SCIGCR1.bit.PARITYENA = 0;  //No Parity Check
    LinaRegs.SCIGCR1.bit.PARITY = 0;     //Odd Parity
    LinaRegs.SCIGCR1.bit.STOP = 0;       //One Stop Bit
    LinaRegs.SCIGCR1.bit.CLK_MASTER = 1; //Enable SCI Clock
    LinaRegs.SCIGCR1.bit.LINMODE = 0;    //SCI Mode ����SCI����ģʽ
    LinaRegs.SCIGCR1.bit.SLEEP = 0;      //Ensure Out of Sleep
    LinaRegs.SCIGCR1.bit.MBUFMODE = 1;   //Buffered Mode
    LinaRegs.SCIGCR1.bit.LOOPBACK = 0;   //External Loopback ����ģʽ�����Բ�ģʽ
    LinaRegs.SCIGCR1.bit.CONT = 1;       //Continue on Suspend
    LinaRegs.SCIGCR1.bit.RXENA = 1;      //Enable RX
    LinaRegs.SCIGCR1.bit.TXENA = 1;      //Enable TX

    //Ensure IODFT is disabled
    LinaRegs.IODFTCTRL.bit.IODFTENA = 0x0;

    //Set transmission length
    LinaRegs.SCIFORMAT.bit.CHAR = 7;        //8������λ
    LinaRegs.SCIFORMAT.bit.LENGTH = 7;   // ��LENGTH+1�����ֽ���ȵ�BUFFER�� ��ζ��28035�� LIN-SCI���յ���LENGTH+1�� ���ֽ�֮�󣬽����жϷ�����

   // ��LinaRegs.BRSR.bit.M = 0 ʱ     LinaRegs.BRSR.bit.SCI_LIN_PSLȡֵ  194-->9600bps ;  97--> 19200bps
   // ��LinaRegs.BRSR.bit.M = 4 ʱ     LinaRegs.BRSR.bit.SCI_LIN_PSLȡֵ  15-->115200bps ;
    LinaRegs.BRSR.bit.SCI_LIN_PSL = 194;
    LinaRegs.BRSR.bit.M = 0;

    //Set interrupt priority
    LinaRegs.SCICLEARINTLVL.all = 0xFFFFFFFF; //Set Int level of all interrupts to LVL 0
    LinaRegs.SCIGCR1.bit.SWnRST = 1;  //bring out of software reset
    //�ж����ò���-----1,����ģ���ж�ʹ�ܣ�
    LinaRegs.SCISETINT.bit.SETRXINT = 1; //Enable RX interrupt
    //�ж����ò���-----2����ӳ���жϷ�����
    PieVectTable.LIN0INTA  = &Lina_Level0_ISR;
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;   // Enable the PIE block
    //�ж����ò���-----3������CPU�жϾ���Y
    IER |= M_INT9;// Enable CPU INT
    //�ж����ò���-----4������CPU�жϾ���X
    PieCtrlRegs.PIEIER9.bit.INTx3=1;     // PIE Group 9, INT3
    EDIS;   // This is needed to disable write to EALLOW protected registers
    while(LinaRegs.SCIFLR.bit.IDLE == 1);   //Wait for SCI to be idle and ready for transmission
}

/*******************************************************************************
** ��������:    Lina_Level0_ISR
** ��������:    LIN�жϺ���
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
__interrupt void Lina_Level0_ISR(void)
{
    //Read-clear the interrupt vector
    LinL0IntVect = LinaRegs.SCIINTVECT0.all;

    //RXINT����ֵΪ11,�����ж�����,P26,����� Table 3,SCI/BLIN Interrupts
    if((LinL0IntVect == 11)&&(0 == gLinRecOkFlag))
    {
        sLINErrConter = 0;
        //��byte��ʽ���գ����ν���8��
        gLinRDataA[0] = LinaRegs.LINRD0.bit.RD0;
        gLinRDataA[1] = LinaRegs.LINRD0.bit.RD1;
        gLinRDataA[2] = LinaRegs.LINRD0.bit.RD2;
        gLinRDataA[3] = LinaRegs.LINRD0.bit.RD3;
        gLinRDataA[4] = LinaRegs.LINRD1.bit.RD4;
        gLinRDataA[5] = LinaRegs.LINRD1.bit.RD5;
        gLinRDataA[6] = LinaRegs.LINRD1.bit.RD6;
        gLinRDataA[7] = LinaRegs.LINRD1.bit.RD7;
        gLinRecOkFlag = 1;
    }
    //Acknowledge the PIE to be able to receive more interrupts
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

Uint16 acrmsfile(Uint16 data,Uint16 num)
{

	static Uint16 temp[64]={0,0,0,0,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,0,0,
							0,0,0,0,0,0,0,0,0,0,
							0,0,0,0,};
	static int32 sumcunt=0;
	static Uint16 sumcunt1=0;
	static Uint16 sumcunt2=0;

	static Uint16 i=0;
	sumcunt1 = 1<<num;
			if( i < sumcunt1 )  //6-8
			{
				temp[i] = data;
				sumcunt +=temp[i];  //�ӵ�ǰ
				if(i== sumcunt1-1)       //������������
				{
				sumcunt -=temp[0];//����������е���
				}
				else
				{
				sumcunt -=temp[i+1];//����������е���
				}
				sumcunt2 = sumcunt>>num;
				 i++;
			}
			else
			{
			i=0;
			}

	   return   (Uint16)sumcunt2;
}



/*******************************************************************************
** ��������:    LinManage
** ��������:    LIN������Ϣ����
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void LinManage(void)
{
    int Counter = 0;
    Uint8 SCIARx_CHM_reg;
    LINErrCheck();
//    if((gsPFCUploadData.PFCAcInVolt > 6000)&&(gsPFCUploadData.PFCAcInVolt > 750))
//                                       {
//                                           gsPFCUploadData.PFCAcInVolt = 0;
//                                       }
    if(1 == gLinRecOkFlag)
    {
        for(Counter = 0; Counter < 8; Counter++)
        {
            RxABuffer[Start] = RxABuffer[Cmd];
            RxABuffer[Cmd]   = RxABuffer[Addre];
            RxABuffer[Addre] = RxABuffer[DataL];
            RxABuffer[DataL] = RxABuffer[DataH];
            RxABuffer[DataH] = RxABuffer[Chm];
            RxABuffer[Chm]   = RxABuffer[End];
            RxABuffer[End]   &= 0xFF;
            RxABuffer[End]   = gLinRDataA[Counter];   //Rx_BUFΪUART���ռĴ���

            if((RxABuffer[Start] == 0xA5) && (RxABuffer[End] == 0x5A))
            {
                SysErr.bit.LINErr = 0;
                SCIARx_CHM_reg = RxABuffer[Cmd];
                SCIARx_CHM_reg += RxABuffer[Addre];
                SCIARx_CHM_reg += RxABuffer[DataL];
                SCIARx_CHM_reg += RxABuffer[DataH];  //���㵱ǰ֡��CHM

                if(RxABuffer[Chm] == (SCIARx_CHM_reg & 0xFF)) //�ӻ���������CHM�뵱ǰ�����CHMһ�£����ݽ�����ȷ
                {
                    //SCIA_Msg.ScitimerOut = 0;
                    //PfcCfgParam.CfgState.Bits.bOverTime = 0;
                    switch(RxABuffer[Cmd])
                    {
                        case WritP:
                            switch(RxABuffer[Addre])
                            {
                                case Vin_VAL:      //AC��ѹ
                                    gsPFCUploadData.PFCAcInVolt = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    if(gsPFCUploadData.PFCAcInVolt > 6000)
                                    {
                                        gsPFCUploadData.PFCAcInVolt = 0;
                                    }

                                    break;
                                case Iin_VAL:       // �������
                                    gsPFCUploadData.PFCAcInCrt = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case Freq_VAL:      // ����Ƶ��
                                    gsPFCUploadData.PFCAcInFrq = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case PFC_VAL:      // PFC��ѹ
                                    gsPFCUploadData.PFCBusVolt = (int16)(RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case Temp_VAL:      // PFC�¶�
                                    gsPFCUploadData.PFCTemp = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case Work_State:     // PFC����״̬
                                    gsPFCUploadData.PFCWorkSts = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case ProCfg_State:     // ����״̬
                                    //gStateParam.PFCProCfg.Word = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case AdjCfg_State:      // ����״̬
                                    //gStateParam.PFCAdjCfg.Word = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case Cmd_State:      // ����״̬
                                    gsPFCUploadData.PFCCmdSts = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case Sub_State:      // �ӹ���״̬
                                    gsPFCUploadData.PFCSubSts = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                case Err_State:      // ����״̬
                                    gsPFCUploadData.PFCErr = (RxABuffer[DataH] * 256 + RxABuffer[DataL]);
                                    break;
                                default :
                                    break;
                            }
                            break;
                        default :
                            break;
                    }
                }
            }
        }
        gLinRecOkFlag = 0;
    }
}

/*******************************************************************************
** ��������:    PfcConfi
** ��������:    �������PICоƬ������PFC
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
Uint16 gPfcSetBackup = 3850;
void PfcConfi(void)
{
    static Uint16  VsetPfc_reg = 0 ;
    Uint32 LinaTxData = 0;
    Uint16 Vtemp = 0;
    Uint16 tmpVAcPeak;
    Uint32 Vout=0;

    switch(PfcCfgCounter)
    {
        case VinSamp_ADJ:   // �����ѹ AD����У׼
            LinaTxData = DEFAULT_K;
            break;

        case IinSamp_ADJ:   // ������� AD����У׼
            LinaTxData = DEFAULT_K;
            break;

        case FreqSamp_ADJ:  // ����Ƶ�� AD����У׼
            LinaTxData = DEFAULT_K;
            break;

        case PFCSamp_ADJ:   // PFC��ѹ AD����У׼
            LinaTxData = DEFAULT_K;
            break;

        case TempSamp_ADJ:  // �¶� AD����У׼
            LinaTxData = DEFAULT_K;
            break;

        case PFCVolt_ADJ:   // PFC��ѹ����У׼
            LinaTxData = DEFAULT_K;
            break;

        case PFCVolt_Set:                                   // PFC��ѹ����ֵ
            if(gPfcCmd == PfcOn)
            {
                if(1 == LlcNormalRunningFlag)
                {
                    if(EPwm1Regs.TBPRD > 300) //С��100KƵ��,PFC��ѹ+5V
                    {
                        gPfcSetBackup += 50;
                    }
                    else if(EPwm1Regs.TBPRD < 250 )//����120K,PFC��ѹ-5V//&& (((int32)VOUT_DATA * K_VOUT_A + K_VOUT_B) >> K_EXP) < CONCURCHARGEENDV
                    {
                        gPfcSetBackup -= 50;
                    }
                    if(gPfcSetBackup < 3850)
                    {
                        gPfcSetBackup = 3850;
                    }
                    else if(gPfcSetBackup > 4350)///4200
                    {
                        gPfcSetBackup = 4350;
                    }
                }
            }

            VsetPfc_reg = gPfcSetBackup;

            if(VsetPfc_reg > 4400)   //����PFC�Ʋ�BUS��ѹ���� 420V   20190618   430V 20200629
            {
                gPfcVoutSet = 4400;
            }
            else if(VsetPfc_reg < 3850)
            {
                gPfcVoutSet = 3850;
            }
            else
            {
                gPfcVoutSet = VsetPfc_reg;
            }
            gstElecPara.PfcVoltSet = gPfcVoutSet;
            Vtemp = 4409 - gPfcVoutSet;
            LinaTxData = Vtemp;
            LinaTxData *= 251;
            LinaTxData = LinaTxData >> 8;

            if(LinaTxData > 800)
            {
                LinaTxData = 800;
            }
            if(LinaTxData < 140)
            {
                LinaTxData = 140;
            }
            if(gPfcCmd == PfcOff)
            {
                LinaTxData = 801;
            }
            break;
        case Ctrl_CMD:      // ��������
            break;
        default :
            break;
    }

    TxABuffer[Start] =  0xA5;             //֡��ʼ
    TxABuffer[Cmd]   =  WritP;            //����
    TxABuffer[Addre] =  PfcCfgCounter;    //��ַ  PFCVolt_Set
    TxABuffer[DataL] =  LinaTxData % 256; //����L
    TxABuffer[DataH] =  LinaTxData / 256; //����H
    TxABuffer[Chm]   =  TxABuffer[DataL];
    TxABuffer[Chm]   += TxABuffer[DataH];
    TxABuffer[Chm]   += TxABuffer[Cmd];
    TxABuffer[Chm]   += TxABuffer[Addre];  //����У����
    TxABuffer[End]   =  0x5A;              //֡����

    PfcCfgCounter++;
    if(PfcCfgCounter > PFCVolt_Set)
    {
        PfcCfgCounter = 0;                 //��������
    }

    LinaRegs.LINTD0.bit.TD0 = TxABuffer[Start];
    LinaRegs.LINTD0.bit.TD1 = TxABuffer[Cmd];
    LinaRegs.LINTD0.bit.TD2 = TxABuffer[Addre];
    LinaRegs.LINTD0.bit.TD3 = TxABuffer[DataL];
    LinaRegs.LINTD1.bit.TD4 = TxABuffer[DataH];
    LinaRegs.LINTD1.bit.TD5 = TxABuffer[Chm];
    LinaRegs.LINTD1.bit.TD6 = TxABuffer[End];
}

/*******************************************************************************
** ��������:    LINErrCheck
** ��������:    LINErr���
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void LINErrCheck(void)
{
    if(!SysErr.bit.LINErr)
    {
        sLINErrConter ++;
        if(sLINErrConter > LINErrTIME)
        {
            sLINErrConter = 0;
          //  SysErr.bit.LINErr = 1;
            LinInit();
        }
    }
}

