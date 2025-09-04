/*******************************************************************************
 * LlcTimer.c
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

/*******************************************************************************
**                               ��������
*******************************************************************************/
static Uint16 gs1msFlagCnt = 0;
Uint16 gs1msFlag = 0;
Uint16 gsIShortCounter = 0;
Uint16 PaPaCharge_Cn = 0;
extern Uint16 USBWriteEnd;
TIME100uS  g_time100uS;
TIME1mS    g_time1mS;
TIME100mS  g_time100mS;
/*******************************************************************************
**                               ��������
*******************************************************************************/
#pragma CODE_SECTION(cpu_timer0_isr, "rammanual");
#pragma CODE_SECTION(cpu_timer1_isr, "rammanual");

interrupt void cpu_timer0_isr(void);
interrupt void cpu_timer1_isr(void);
void TimerInit(void);
void Timer(void);
void TimerWindow(Uint16 CMD_ON);
extern void  Display(Uint16, Uint16, Uint16, Uint16);
extern void sridsplaydata(void);
extern Uint16  ShutDownDsplayFlag;
#define Pause_Priod  500///500/50000 = 10ms
/*******************************************************************************
** ��������:    cpu_timer0_isr
** ��������:    ��ʱ��0�жϴ�����:����PID
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
interrupt void cpu_timer0_isr(void)
{
	static Uint16	s_time100uS = 1;
	static Uint16	s_time1mS   = 1;
	static Uint16   s_time100mS = 1;
	//static Uint16	gCurveNum = 1;
//********************************LLC control********************************
    gs1msFlagCnt ++;
    if(gs1msFlagCnt > 50)//50
    {
        gs1msFlag = 1;
        gs1msFlagCnt = 0;
    }
///////////////////////////////////////��ʱ������///////////////////////////////////////////
    s_time100uS ++ ;
    switch(s_time100uS)
               {
          //������100US�ı���
               case 1:
            	   if(g_time100uS.Default1)  g_time100uS.Default1--;
            	   if(g_time100uS.Default2)  g_time100uS.Default2--;
            	   if(g_time100uS.Default3)  g_time100uS.Default3--;
              	   if(g_time100uS.Default4)  g_time100uS.Default4--;
                   if(g_time100uS.Default5)  g_time100uS.Default5--;
            	   if(g_time100uS.Default6)  g_time100uS.Default6--;
                   break;
               case 2:
            	   if(g_time100uS.Default7)  g_time100uS.Default7--;
				   if(g_time100uS.Default8)  g_time100uS.Default8--;
				   if(g_time100uS.Default9)  g_time100uS.Default9--;
            	   if(g_time100uS.Default10)  g_time100uS.Default10--;
				   if(g_time100uS.Default11)  g_time100uS.Default11--;
				   if(g_time100uS.Default12)  g_time100uS.Default12--;

                   break;
               case 3:
            	   if(g_time100uS.Default13)  g_time100uS.Default13--;
				   if(g_time100uS.Default14)  g_time100uS.Default14--;
				   if(g_time100uS.Default15)  g_time100uS.Default15--;
            	   if(g_time100uS.Default16)  g_time100uS.Default16--;
				   if(g_time100uS.Default17)  g_time100uS.Default17--;
            	   if(g_time100uS.Default18)  g_time100uS.Default18--;

                   break;
               case 4:
				   if(g_time100uS.Default19)  g_time100uS.Default19--;
            	   if(g_time100uS.Default20)  g_time100uS.Default20--;
				   if(g_time100uS.Default21)  g_time100uS.Default21--;
            	   if(g_time100uS.Default22)  g_time100uS.Default22--;
				   if(g_time100uS.Default23)  g_time100uS.Default23--;
				   if(g_time100uS.Default24)  g_time100uS.Default24--;
				   //////////////////������100US�ı���
                   break;
               case 5:
            	   s_time1mS++;
            	   s_time100uS = 0;
            	   switch(s_time1mS)
            	   //������1MS�ı���
					  {
					  case 1:
					   if(g_time1mS.Default1)  g_time1mS.Default1--;
					   if(g_time1mS.Default2)  g_time1mS.Default2--;
					   if(g_time1mS.Default3)  g_time1mS.Default3--;
					   if(g_time1mS.Default4)  g_time1mS.Default4--;


						  break;
					  case 2:
					   if(g_time1mS.Default5)  g_time1mS.Default5--;
					   if(g_time1mS.Default6)  g_time1mS.Default6--;
					   if(g_time1mS.Default7)  g_time1mS.Default7--;
					   if(g_time1mS.Default8)  g_time1mS.Default8--;

						  break;
					  case 3:
						   if(g_time1mS.Default9)  g_time1mS.Default9--;
						   if(g_time1mS.Default10)  g_time1mS.Default10--;
						   if(g_time1mS.Default11)  g_time1mS.Default11--;
						   if(g_time1mS.Default12)  g_time1mS.Default12--;

						  break;
					  case 4:
						   if(g_time1mS.Default13)  g_time1mS.Default13--;
						   if(g_time1mS.Default14)  g_time1mS.Default14--;
						   if(g_time1mS.Default15)  g_time1mS.Default15--;
						   if(g_time1mS.Default16)  g_time1mS.Default16--;

						  break;
					  case 5:
						   if(g_time1mS.Default17)  g_time1mS.Default17--;
						   if(g_time1mS.Default18)  g_time1mS.Default18--;
						   if(g_time1mS.Default19)  g_time1mS.Default19--;
						   if(g_time1mS.Default20)  g_time1mS.Default20--;
						  break;

					  case 6:
						   if(g_time1mS.Default21)  g_time1mS.Default21--;
						   if(g_time1mS.Default22)  g_time1mS.Default22--;
						   if(g_time1mS.Default23)  g_time1mS.Default23--;
						   if(g_time1mS.Default24)  g_time1mS.Default24--;
						  break;
					  case 7:
						   if(g_time1mS.Default25)  g_time1mS.Default25--;
						   if(g_time1mS.Default26)  g_time1mS.Default26--;
						   if(g_time1mS.Default27)  g_time1mS.Default27--;
						   if(g_time1mS.Default28)  g_time1mS.Default28--;
						  break;
					  case 8:
						   if(g_time1mS.Default29)  g_time1mS.Default29--;
						   if(g_time1mS.Default30)  g_time1mS.Default30--;
						   if(g_time1mS.Default31)  g_time1mS.Default31--;
						   if(g_time1mS.Default32)  g_time1mS.Default32--;
						  break;
					  case 9:
						   if(g_time1mS.Default33)  g_time1mS.Default33--;
						   if(g_time1mS.Default34)  g_time1mS.Default34--;
						   if(g_time1mS.Default35)  g_time1mS.Default35--;
						   if(g_time1mS.Default36)  g_time1mS.Default36--;
						  break;
						  //������1MS�ı���
					  case 10:
						 ////////////////////////////////////////////����100mS/////////////////////////////////////////////////////////
					   s_time100mS++;
					   s_time1mS=0;
						 switch(s_time100mS)
						   {
						   case 1:
							   if(g_time100mS.Default1)  g_time100mS.Default1--;
							   if(g_time100mS.Default2)  g_time100mS.Default2--;
							   break;
						   case 2:
							   if(g_time100mS.Default3)  g_time100mS.Default3--;
							   if(g_time100mS.Default4)  g_time100mS.Default4--;
							   break;
						   case 3:
							  if(g_time100mS.Default5)  g_time100mS.Default5--;
						      if(g_time100mS.Default6)  g_time100mS.Default6--;
							   break;
						   case 4:
							   if(g_time100mS.Default7)  g_time100mS.Default7--;
							   if(g_time100mS.Default8)  g_time100mS.Default8--;
							   break;
						   case 5:
							   if(g_time100mS.Default9)  g_time100mS.Default9--;
							   if(g_time100mS.Default10)  g_time100mS.Default10--;
							   break;

						   case 6:
							if(g_time100mS.Default11)  g_time100mS.Default11--;
							if(g_time100mS.Default12)  g_time100mS.Default12--;
							   break;
						   case 7:
							   if(g_time100mS.Default13)  g_time100mS.Default13--;
							   if(g_time100mS.Default14)  g_time100mS.Default14--;

							   break;
						   case 8:
							   if(g_time100mS.Default15)  g_time100mS.Default15--;
							   if(g_time100mS.Default16)  g_time100mS.Default16--;

							   break;
						   case 9:
							if(g_time100mS.Default17)  g_time100mS.Default17--;
							if(g_time100mS.Default18)  g_time100mS.Default18--;

							   break;
						   case 10:
							 if(g_time100mS.Default19)  g_time100mS.Default19--;

							 s_time100mS=0;
							   break;
						  }
					  ////////////////////////////////////////����100mS/////////////////////////////////
						  break;
					  }
                   break;
               }
///////////////////////////////////////��ʱ������///////////////////////////////////////////

    if(gsIShortCounter < 3)
    {
        if((((AdcResult.ADCRESULT8 + AdcResult.ADCRESULT9) >> 1) > 3750)
                &&(AdcResult.ADCRESULT4 < 400))
        {
            SysErr.bit.IShort = 1;
            gstSetLlcPARAM.llcOnCommand = eLlcSysCmdShutDown;
            SysSetLoop(gstSetLlcPARAM);   // ��ϵͳ������ֵ����·
            gsIShortCounter ++;
        }
        else
        {
            SysErr.bit.IShort = 0;
        }
    }
    else
    {
        SysErr.bit.IShort = 1;
        gstSetLlcPARAM.llcOnCommand = eLlcSysCmdShutDown;
        SysSetLoop(gstSetLlcPARAM);   // ��ϵͳ������ֵ����·
    }
// =============================================================================
// PWM����,�ضϼ����Լ�,��0ʱ��ʼPWM���ƹ���
// =============================================================================
    LlcLoop();
    if((USBON == 1)&&(g_time100uS.Default24==0)&&(USBWriteEnd == 0))//��U��ʱ����ˢ����ʾ
    {
    	g_time100uS.Default24=50; //5000US
        Display(26, 24, 5, 8);   //���ж�����ʾ    �� ��U,S,B(U58)
    }

// =============================================================================
// �˳��ж�
// =============================================================================
    PieCtrlRegs.PIEACK.bit.ACK1 = 1;
 	CpuTimer0Regs.TCR.bit.TIF=1;  //�嶨ʱ��0�жϱ�־λ
    EINT;                         //�����ж�
    ERTM;
}

/*******************************************************************************
** ��������:    cpu_timer1_isr
** ��������:    ��ʱ��1�жϴ�����:����USB��ʾ ��USB����׶�20msˢ��һ����ʾ
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
extern Uint16 gsDisplay[4];

interrupt void cpu_timer1_isr(void)
{

  //  Display(26, 24, 5, 8);
    // =============================================================================
    // �˳��ж�
    // =============================================================================
        PieCtrlRegs.PIEACK.bit.ACK1 = 1;
        CpuTimer1Regs.TCR.bit.TIF=1;  //�嶨ʱ��1�жϱ�־λ
        EINT;                         //�����ж�
        ERTM;

}
/*******************************************************************************
** ��������:    TimerInit
** ��������:    ��ʱ����ʼ��:��ʱ��0 39us��ʱ
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void TimerInit(void)
{
    EALLOW;
    PieVectTable.TINT0 = cpu_timer0_isr;
    PieVectTable.TINT1 = cpu_timer1_isr;
    EDIS;
    // Enable CPU INT1 which is connected to CPU-Timer 0:
    IER |= M_INT1;
//    IER |= M_INT13;

    // Enable TINT0 in the PIE: Group 1 interrupt 7
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
    InitCpuTimers();                        // ��ʼ�����ж�ʱ��
    ConfigCpuTimer(&CpuTimer0, 60, 20);     // ��ʱ��0,60MHZ,20us
    ConfigCpuTimer(&CpuTimer1, 60, 5000);    // ��ʱ��0,60MHZ,390us
    ConfigCpuTimer(&CpuTimer2, 60, 40);     // ��ʱ��0,60MHZ,40us
// To ensure precise timing, use write-only instructions to write to the entire register. Therefore, if any
// of the configuration bits are changed in ConfigCpuTimer and InitCpuTimers (in DSP2803x_CpuTimers.h), the
// below settings must also be updated.
    CpuTimer0Regs.TCR.all = 0x4001; // Use write-only instruction to set TSS bit = 0
    CpuTimer1Regs.TCR.all = 0x4001; // Use write-only instruction to set TSS bit = 0
    CpuTimer2Regs.TCR.all = 0x4001; // Use write-only instruction to set TSS bit = 0
}

/*******************************************************************************
** ��������:    Timer
** ��������:    ʱ�Ӷ�ʱ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void Timer(void)
{
    if(gWriteEEPDelay)
    {
        gWriteEEPDelay--;
    }


    if(PaPaCharge_Cn ++ > Pause_Priod)
    {
        PaPaCharge_Cn = 0;
    }
}
/*******************************************************************************
** ��������:    TimerWindow
** ��������:    ʱ�Ӵ���
** ��ʽ����:    ����ʱ��
** ���ز���:    ��
*******************************************************************************/
void TimerWindow(Uint16 CMD_ON)
{

}



