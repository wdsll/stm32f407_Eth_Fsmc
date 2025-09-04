/*******************************************************************************
 * LlcGpio.c
 *
 *  Created on: 2019-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcGpio.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
static void Init_PwmGpio(void);
static void Init_CanGpio(void);
static void Init_SciGpio(void);
static void Init_IICGpio(void);
static void Init_LinGpio(void);
static void Init_DS1338Gpio(void);

/*******************************************************************************
** ��������:    InitGpio
** ��������:    GPIO��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void InitGpio(void)
{
    Init_PwmGpio();
    Init_CanGpio();
    Init_SciGpio();
    Init_IICGpio();
    Init_LinGpio();
    Init_DS1338Gpio();
    EALLOW;
// -----------------------------------------------------------------------------
    /*GpioCtrlRegs.GPAMUX1.bit.GPIO4  = 0x00;    // GPIO4,����LED��ɫ
    GpioCtrlRegs.GPADIR.bit.GPIO4   = 1;       // GPIO4��Ϊ���
    GpioCtrlRegs.GPAQSEL1.bit.GPIO4 = 0x00;    // GPIO4ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO4   = 0;       // GPIO4������
    LED_GRNOFF;*/
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX1.bit.GPIO5  = 0x00;    // GPIO5,����LED��ɫ
    GpioCtrlRegs.GPADIR.bit.GPIO5   = 1;       // GPIO5��Ϊ���
    GpioCtrlRegs.GPAQSEL1.bit.GPIO5 = 0x00;    // GPIO5ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO5   = 0;       // GPIO5������
    LED_REDON;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX1.bit.GPIO6  = 0x00;    // GPIO6,���USB��INT
    GpioCtrlRegs.GPADIR.bit.GPIO6   = 0;       // GPIO6��Ϊ����
    GpioCtrlRegs.GPAQSEL1.bit.GPIO6 = 0x00;    // GPIO6ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO6   = 0;       // GPIO6������
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX1.bit.GPIO9  = 0x00;    // GPIO9,����IICоƬ��д����
    GpioCtrlRegs.GPADIR.bit.GPIO9   = 1;       // GPIO9��Ϊ���
    GpioCtrlRegs.GPAQSEL1.bit.GPIO9 = 0x00;    // GPIO9ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO9   = 0;       // GPIO9������
    GpioDataRegs.GPASET.bit.GPIO9   = 1;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX1.bit.GPIO10  = 0x00;    // GPIO10,���Ʒ��ȿ���
    GpioCtrlRegs.GPADIR.bit.GPIO10   = 1;       // GPIO10��Ϊ���
    GpioCtrlRegs.GPAQSEL1.bit.GPIO10 = 0x00;    // GPIO10ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO10   = 0;       // GPIO10������
    FAN_OFF;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX1.bit.GPIO11  = 0x00;   // GPIO11,����DC_EN
    GpioCtrlRegs.GPADIR.bit.GPIO11   = 1;      // GPIO11��Ϊ���
    GpioCtrlRegs.GPAQSEL1.bit.GPIO11 = 0x00;   // GPIO11ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO11   = 0;      // GPIO11������
    //DC_DIS;
    DC_EN;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX2.bit.GPIO17  = 0x00;   // GPIO17,���Ʒ����߼̵���
    GpioCtrlRegs.GPADIR.bit.GPIO17   = 1;      // GPIO17��Ϊ���
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 0x00;   // GPIO17ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO17   = 0;      // GPIO17������
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX2.bit.GPIO18  = 0x00;   // GPIO18,��������̵���
    GpioCtrlRegs.GPADIR.bit.GPIO18   = 1;      // GPIO18��Ϊ���
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 0x00;   // GPIO18ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO18   = 0;      // GPIO18������
    CLR_KPOUT;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX2.bit.GPIO19  = 0x00;   // GPIO19,����Ƿ��������ѹ
    GpioCtrlRegs.GPADIR.bit.GPIO19   = 0;      // GPIO19��Ϊ����
    GpioCtrlRegs.GPAQSEL2.bit.GPIO19 = 0x00;   // GPIO19ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO19   = 0;      // GPIO19������
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX2.bit.GPIO20  = 0x00;   // GPIO20,����ST-RCK
    GpioCtrlRegs.GPADIR.bit.GPIO20   = 1;      // GPIO20��Ϊ���
    GpioCtrlRegs.GPAQSEL2.bit.GPIO20 = 0x00;   // GPIO20ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO20   = 0;      // GPIO20������
    RST_RCK;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX2.bit.GPIO21  = 0x00;   // GPIO21,����CLK-SRCK
    GpioCtrlRegs.GPADIR.bit.GPIO21   = 1;      // GPIO21Ϊ���
    GpioCtrlRegs.GPAQSEL2.bit.GPIO21 = 0x00;   // GPIO21ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO21   = 0;      // GPIO21������
    RST_SRCK;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPAMUX2.bit.GPIO24  = 0x00;   // GPIO24,����SER
    GpioCtrlRegs.GPADIR.bit.GPIO24   = 1;      // GPIO24��Ϊ���
    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 0x00;   // GPIO24ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPAPUD.bit.GPIO24   = 0;      // GPIO24������
    RST_SER;
// -----------------------------------------------------------------------------
    GpioCtrlRegs.GPBMUX1.bit.GPIO32  = 0x00;   // GPIO32,����Ƿ��а�������
    GpioCtrlRegs.GPBDIR.bit.GPIO32   = 0;      // GPIO32��Ϊ����
    GpioCtrlRegs.GPBQSEL1.bit.GPIO32 = 0x00;   // GPIO32ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPBPUD.bit.GPIO32   = 0;      // GPIO32������

    /////standby--------------------
    // -----------------------------------------------------------------------------
    GpioCtrlRegs.GPBMUX1.bit.GPIO33  = 0x00;    // GPIO33,����standby����
    GpioCtrlRegs.GPBDIR.bit.GPIO33   = 1;       // GPIO33��Ϊ���
    GpioCtrlRegs.GPBQSEL1.bit.GPIO33 = 0x00;    // GPIO33ʱ����sysclkoutһ��,������
    GpioCtrlRegs.GPBPUD.bit.GPIO33   = 0;       // GPIO33������
    EDIS;
}

/*******************************************************************************
** ��������:    InitPwmGpio
** ��������:    PWM��GPIO��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void Init_PwmGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;    // Disable pull-up on GPIO0 (EPWM1A)
    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 1;    // Disable pull-up on GPIO1 (EPWM1B)
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // Configure GPIO0 as EPWM1A
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;   // Configure GPIO1 as EPWM1B

    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 1;    // Disable pull-up on GPIO2 (EPWM2A)
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;   // Configure GPIO2 as EPWM2A

    GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1;    // Disable pull-up on GPIO2 (EPWM3A)
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;   // Configure GPIO2 as EPWM3A
    EDIS;
}

/*******************************************************************************
** ��������:    InitCanGpio
** ��������:    Can��GPIO��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void Init_CanGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO30 = 0;     // Enable pull-up for GPIO30 (CANRXA)
    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 0;     // Enable pull-up for GPIO31 (CANTXA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO30 = 3;   // Asynch qual for GPIO30 (CANRXA)
    GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 1;    // Configure GPIO30 for CANRXA operation
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 1;    // Configure GPIO31 for CANTXA operation
    EDIS;
}

/*******************************************************************************
** ��������:    InitSciGpio
** ��������:    SCI��GPIO��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void Init_SciGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO7 = 0;     // Enable pull-up for GPIO7  (SCIRXDA)
    GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0;    // Enable pull-up for GPIO12 (SCITXDA)
    GpioCtrlRegs.GPAQSEL1.bit.GPIO7 = 3;   // Asynch input GPIO7 (SCIRXDA)
    GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 2;    // Configure GPIO7  for SCIRXDA operation
    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 2;   // Configure GPIO12 for SCITXDA operation
    EDIS;
}

/*******************************************************************************
** ��������:    InitIICGpio
** ��������:    IIC��GPIO��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void Init_IICGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO28= 0;      //����
    GpioCtrlRegs.GPADIR.bit.GPIO28 = 1;     // ����˿�
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 0;    // IO��
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;   // ��ͬ��

    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;     //����
    GpioCtrlRegs.GPADIR.bit.GPIO29 = 1;     // ����˿�
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 0;    // IO��
    GpioCtrlRegs.GPAQSEL2.bit.GPIO29 = 3;   // ��ͬ��
    EDIS;
}

/*******************************************************************************
** ��������:    InitLinGpio
** ��������:    Lin��SCIģʽGPIO��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
static void Init_LinGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO22 = 0;     // Enable pull-up for GPIO22 (LIN TX)
    GpioCtrlRegs.GPAPUD.bit.GPIO23 = 0;     // Enable pull-up for GPIO23 (LIN RX)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO23 = 3;  // Asynch input GPIO23 (LINRXA)
    GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 3;   // Configure GPIO19 for LIN TX operation  (3-Enable,0-Disable)
    GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 3;   // Configure GPIO23 for LIN RX operation (3-Enable,0-Disable)
    EDIS;
}

/*******************************************************************************
** ��������:    Init_DS1338Gpio
** ��������:    DS1338��IICͨ�ŵ�GPIO��ʼ��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
void Init_DS1338Gpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO8= 0;       //����
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;      // ����˿�
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;     // IO��
    GpioCtrlRegs.GPAQSEL1.bit.GPIO8 = 3;    // ��ͬ��

    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 0;     //����
    GpioCtrlRegs.GPADIR.bit.GPIO16 = 1;     // ����˿�
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 0;    // IO��
    GpioCtrlRegs.GPAQSEL2.bit.GPIO16 = 3;   // ��ͬ��
    EDIS;
}
