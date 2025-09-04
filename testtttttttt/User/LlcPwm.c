/*******************************************************************************
 * LlcPwm.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcPwm.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define PERIODINIT (30000/250)  //250K

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
void PwmInit(void);

/*******************************************************************************
** ��������:    PwmInit
** ��������:    PWM��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void PwmInit(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;          // ֹͣ����TBʱ��
    EPwm1Regs.TZFRC.bit.OST = 1;                    // ��ʼʱ,�ֶ�����OST�¼�,��ֹPWM���
    // ���� TBCLK
    EPwm1Regs.TBPRD = PERIODINIT;                   // ���ü�������ֵ
    EPwm1Regs.TBPHS.half.TBPHS = 0x0000;            // 0��λ
    EPwm1Regs.TBCTR = 0x0000;                       // �������ֵ

    // ���ñȽ�ֵ
    EPwm1Regs.CMPA.half.CMPA = PERIODINIT + 1;      // ����CMPA
    // ���ü���������ģʽ
    EPwm1Regs.TBCTL.bit.CTRMODE   = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.PHSEN     = TB_ENABLE;     //
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        // PWMʱ����SYSCLKOUT��ͬ
    EPwm1Regs.TBCTL.bit.CLKDIV    = TB_DIV1;
    EPwm1Regs.TBCTL.bit.PRDLD     = TB_SHADOW;      // ʹ��Ӱ�ӼĴ���
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 2;              // PWM��������
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;     // Sync down-stream module
    // �Ƚ�ֵģ��Ӱ�ӼĴ�������
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;   // �ڼ���ֵΪ0ʱ����
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    // ����PWM2Ain����PWM2Bin�Ķ���
    EPwm1Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;
    EPwm1Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm1Regs.AQCTLA.bit.CBD = AQ_NO_ACTION;
    EPwm1Regs.AQCTLA.bit.CBD = AQ_NO_ACTION;
    EPwm1Regs.AQCTLB.bit.PRD = AQ_NO_ACTION;
    EPwm1Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
    EPwm1Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
    EPwm1Regs.AQCTLB.bit.CAU = AQ_NO_ACTION;
    EPwm1Regs.AQCTLB.bit.CBD = AQ_NO_ACTION;
    EPwm1Regs.AQCTLB.bit.CBD = AQ_NO_ACTION;

    EPwm1Regs.DBCTL.bit.IN_MODE  = DBA_ALL;         // ����ģ��ȡPWM2A��Ϊ����
    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;  // �����غ��½��ض���ʱ
    EPwm1Regs.DBCTL.bit.POLSEL   = DB_ACTV_HIC;     // �������,A���ͬ��,B�������
    EPwm1Regs.DBRED = LLC_DB;                       // �����������ӳ�����,RED = DBRED * Ttbclk
    EPwm1Regs.DBFED = LLC_DB;                       // �����½����ӳ�����,FED = DBFED * Ttbclk

    EPwm1Regs.TZCTL.bit.TZA = TZ_FORCE_LO;
    EPwm1Regs.TZCTL.bit.TZB = TZ_FORCE_LO;
    EPwm1Regs.TZFRC.bit.OST = 1;




    EPwm2Regs.TZCLR.bit.OST = 1;                    // ��ʼʱ,�ֶ�����OST�¼�,��ֹPWM���

    // ���� TBCLK
    EPwm2Regs.TBPRD = PERIODINIT;                   // ���ü�������ֵ
    EPwm2Regs.TBCTR = 0x0000;                       // �������ֵ
    // ���ñȽ�ֵ
    EPwm2Regs.CMPA.half.CMPA = PERIODINIT + 1;      // ����CMPA
    //EPwm2Regs.CMPB = SR_START;
    // ���ü���������ģʽ
    EPwm2Regs.TBCTL.bit.CTRMODE   = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.PHSEN     = TB_ENABLE;      // Enable phase loading
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        // PWMʱ����SYSCLKOUT��ͬ
    EPwm2Regs.TBCTL.bit.CLKDIV    = TB_DIV1;
    EPwm2Regs.TBCTL.bit.PRDLD     = TB_SHADOW;      // ʹ��Ӱ�ӼĴ���
    EPwm2Regs.TBCTL.bit.FREE_SOFT = 2;              // PWM��������

    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;      // sync flow-through
    EPwm2Regs.TBCTL.bit.PHSEN = TB_ENABLE;          // Slave module
    /*start��ʼ��λSR_PHASEƫ��ȡ������ͬ��������������*/
    EPwm2Regs.TBPHS.half.TBPHS = 0;////SR_PHASE;          // ��λ
    // �Ƚ�ֵģ��Ӱ�ӼĴ�������
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;   // �ڼ���ֵΪ0ʱ����
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    // ����PWM2Ain����PWM2Bin�Ķ���
    EPwm2Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;
    EPwm2Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
    EPwm2Regs.AQCTLA.bit.CAU = AQ_NO_ACTION;
    EPwm2Regs.AQCTLA.bit.CBD = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CBU = AQ_NO_ACTION;
    EPwm2Regs.AQCTLB.bit.PRD = AQ_NO_ACTION;
    EPwm2Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
    EPwm2Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
    EPwm2Regs.AQCTLB.bit.CAU = AQ_NO_ACTION;
    EPwm2Regs.AQCTLB.bit.CBD = AQ_NO_ACTION;
    EPwm2Regs.AQCTLB.bit.CBU = AQ_NO_ACTION;

    EPwm2Regs.TZCTL.bit.TZA = TZ_FORCE_LO;
    EPwm2Regs.TZFRC.bit.OST = 1;




    EPwm3Regs.TZCLR.bit.OST = 1;                    // ��ʼʱ,�ֶ�����OST�¼�,��ֹPWM���
    EPwm3Regs.TBPRD = PERIODINIT;                   // ���ü�������ֵ
    EPwm3Regs.TBCTR = 0x0000;                       // �������ֵ
    // ���ñȽ�ֵ
    EPwm3Regs.CMPA.half.CMPA = PERIODINIT + 1;      // ����CMPA
    //EPwm3Regs.CMPB = SR_START;
    // ���ü���������ģʽ
    EPwm3Regs.TBCTL.bit.CTRMODE   = TB_COUNT_UPDOWN;
    EPwm3Regs.TBCTL.bit.PHSEN     = TB_ENABLE;      // Enable phase loading
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        // PWMʱ����SYSCLKOUT��ͬ
    EPwm3Regs.TBCTL.bit.CLKDIV    = TB_DIV1;
    EPwm3Regs.TBCTL.bit.PRDLD     = TB_SHADOW;      // ʹ��Ӱ�ӼĴ���
    EPwm3Regs.TBCTL.bit.FREE_SOFT = 2;              // PWM��������

    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;      // sync flow-through
    EPwm3Regs.TBCTL.bit.PHSEN = TB_ENABLE;          // Slave module
    /*start��ʼ��λSR_PHASEƫ��ȡ������ͬ��������������*/
    EPwm3Regs.TBPHS.half.TBPHS = 0;/////SR_PHASE;          // ��λ
    // �Ƚ�ֵģ��Ӱ�ӼĴ�������
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;   // �ڼ���ֵΪ0ʱ����
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    // ����PWM2Ain����PWM2Bin�Ķ���
    EPwm3Regs.AQCTLA.bit.PRD = AQ_NO_ACTION;
    EPwm3Regs.AQCTLA.bit.ZRO = AQ_NO_ACTION;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CBD = AQ_NO_ACTION;
    EPwm3Regs.AQCTLA.bit.CBU = AQ_NO_ACTION;
    EPwm3Regs.AQCTLB.bit.PRD = AQ_NO_ACTION;
    EPwm3Regs.AQCTLB.bit.ZRO = AQ_NO_ACTION;
    EPwm3Regs.AQCTLB.bit.CAD = AQ_NO_ACTION;
    EPwm3Regs.AQCTLB.bit.CAU = AQ_NO_ACTION;
    EPwm3Regs.AQCTLB.bit.CBD = AQ_NO_ACTION;
    EPwm3Regs.AQCTLB.bit.CBU = AQ_NO_ACTION;


    EPwm3Regs.TZCTL.bit.TZA = TZ_FORCE_LO;
    EPwm3Regs.TZFRC.bit.OST = 1;

    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm3Regs.AQCTLA.bit.CBD = AQ_SET;

    EPwm1Regs.DBRED = LLC_DB;
    EPwm1Regs.DBFED = LLC_DB;

    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC   = 1;        // ����TBʱ��
    EDIS;
}
