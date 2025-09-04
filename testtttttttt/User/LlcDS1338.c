/*******************************************************************************
 * charger_sci.c
 *
 *  Created on: 2013-5-16
 *      Author: Administrator
*******************************************************************************/
#include "LlcDS1338.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define DS1338_SDA_R   GpioDataRegs.GPADAT.bit.GPIO8     //SDA ��״̬
#define DS1338_SDA_W0  GpioDataRegs.GPACLEAR.bit.GPIO8=1 //SDA ���0 д״̬
#define DS1338_SDA_W1  GpioDataRegs.GPASET.bit.GPIO8=1   //SDA ���1 д״̬
#define DS1338_SCL_0   GpioDataRegs.GPACLEAR.bit.GPIO16=1 //SCL ���0
#define DS1338_SCL_1   GpioDataRegs.GPASET.bit.GPIO16=1   //SCL ���1
#define DELAY_UNIT  5                              //�궨����ʱʱ�䳣��
/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
static Uint16 DS1338_Err;                                  //DS1338��д����ָʾ
extern Uint16 Charge_state;
Uint16 Second  = 0;
Uint16 Minute  = 0;
Uint16 Hour    = 0;
//Uint16 Hour_start = 0;
///Uint16 Hour_stop = 0;
Uint16 Date    = 0;
///Uint16 Date_start = 0;
///Uint16 Date_stop = 0;
Uint16 Month   = 0;
///Uint16 Month_start = 0;
///Uint16 Month_stop = 0;
Uint16 Year    = 0;
///Uint16 Year_start = 0;
///Uint16 Year_stop = 0;
/*******************************************************************************
**                               ��������
*******************************************************************************/
static __inline void DS1338_SDA_READ(void);
static __inline void DS1338_SDA_WRITE(void);
static void DS1338_Delay(Uint16 time);
static void DS1338_BeginTrans();
static void DS1338_StopTrans();
static void DS1338_Ack();
static void DS1338_ByteIn(Uint16 ch);
static Uint16 DS1338_ByteOut(void);
void DS1338_WriteByte(Uint16 addr,Uint16 data);
Uint16 DS1338_ReadByte(Uint16 addr);
Uint16 DS1338_ReadTime(void);

/******************************IO������****************************/
static __inline void DS1338_SDA_READ(void)
{
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 0;       //Input, SDA
    EDIS;
}

static __inline void DS1338_SDA_WRITE(void)
{
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;       //Output. SDA
    EDIS;
}

/************************GPIOģ��IICͨ��****************************/
static void DS1338_Delay(Uint16 time)                  //��ʱ����
{
    for(; time>0 ; time--)
    {
        asm(" nop");
        asm(" nop");
        asm(" nop");
        asm(" nop");
        asm(" nop");
        asm(" nop");
        asm(" nop");
        asm(" nop");
    }
}

static void DS1338_BeginTrans(void)       //����START �ź�
{
    DS1338_SDA_W1;                        //SDA=1
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SDA_WRITE();                   //SDA ����Ϊ�����DS1338
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SCL_1;                         //SCL=1
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SDA_W0;                        //SDA=0
    DS1338_Delay(DELAY_UNIT);             //��ʱ
}

static void DS1338_StopTrans(void)        //����STOP �ź�
{
    DS1338_SDA_WRITE();                   //SDA����Ϊ�����DS1338
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SDA_W0;                        //SDA=0
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SCL_1;                         //SCL=1
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SDA_W1;                        //SDA=1
    DS1338_Delay(DELAY_UNIT);
}

static void DS1338_Ack(void)              //�ȴ�DS1338 ��ACK �ź�
{
    Uint16 d;
    Uint16  i;
    DS1338_SDA_READ();                    //SDA����Ϊ��DS1338 ����
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SCL_1;                         //SCL=1
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    i = 0;
    do
    {
        d = DS1338_SDA_R;
        i++;
        DS1338_Delay(DELAY_UNIT);
    }
    while((d == 1) && (i <= 500));        //�ȴ�DS1338 ����͵�ƽ,4ms���˳�ѭ��

    if (i >= 499)
    {
        DS1338_Err = 0xff;
    }

    i = 0;
    DS1338_SCL_0;                         //SCL=0
    DS1338_Delay(DELAY_UNIT);             //��ʱ
}

static void DS1338_ByteIn(Uint16 ch)      //��DS1338 д��һ���ֽ�
{
    Uint16 i;
    DS1338_SCL_0;                         //SCL=0
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    DS1338_SDA_WRITE();                   //SDA����Ϊ�����DS1338
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    for(i=8;i>0;i--)
    {
        if ((ch & 0x80)== 0)
        {
            DS1338_SDA_W0;                //����ͨ��SDA ��������DS1338
            DS1338_Delay(DELAY_UNIT);     //��ʱ
        }
        else
        {
            DS1338_SDA_W1;
            DS1338_Delay(DELAY_UNIT);     //��ʱ
        }
        DS1338_SCL_1;                     //SCL=1
        DS1338_Delay(DELAY_UNIT);         //��ʱ
        ch <<= 1;
        DS1338_SCL_0;                     //SCL=0
        DS1338_Delay(DELAY_UNIT);         //��ʱ
    }
    DS1338_Ack();
}

static Uint16 DS1338_ByteOut(void)        //��DS1338 ���һ���ֽ�
{
    unsigned char i;
    Uint16 ch;
    ch = 0;
    DS1338_SDA_READ();                    //SDA �ķ���Ϊ��DS1338 ���
    DS1338_Delay(DELAY_UNIT);             //��ʱ
    for(i=8;i>0;i--)
    {
        ch <<= 1;
        DS1338_SCL_1;                     //SCL=1
        DS1338_Delay(DELAY_UNIT);         //��ʱ
        ch |= DS1338_SDA_R;               //����ͨ��SDA �����Ƴ�DS1338
        DS1338_Delay(DELAY_UNIT);         //��ʱ
        DS1338_SCL_0;                     //SCL=0
        DS1338_Delay(DELAY_UNIT);         //��ʱ
    }
    return(ch);
}

void DS1338_WriteByte(Uint16 addr,Uint16 data)  //��DS1338 ָ����ַд��һ���ֽڵ�����
{
    DS1338_BeginTrans();                  //��ʼ
    DS1338_ByteIn(0xD0);                  //д��д������0xD0
    DS1338_ByteIn(addr);                  //д��ָ����ַ
    DS1338_ByteIn(data);                  //д���д��DS1338 ������
    DS1338_StopTrans();                   //ֹͣ
    DS1338_Delay(4000);
}

Uint16 DS1338_ReadByte(Uint16 addr)    //��DS1338 ָ����ַ��ȡһ���ֽڵ�����
{
    Uint16 c;
    DS1338_BeginTrans();                  //��ʼ
    DS1338_ByteIn(0xD0);                  //д��д������0xD0
    DS1338_ByteIn(addr);                  //д��ָ����ַ
    DS1338_BeginTrans();                  //��ʼ
    DS1338_ByteIn(0xD1);                  //д���������0xA1
    c = DS1338_ByteOut();                 //����DS1338 ���������
    DS1338_StopTrans();                   //ֹͣ
    //DS1338_Delay(200);                  //��ʱ
    c = (c >> 4)* 10 + c % 16;
    return(c);
}

Uint16 gDS1338ReadStep = 0;
Uint16 DS1338_ReadTime(void)         //��DS1338 ָ����ַ��ȡһ���ֽڵ�����
{
    switch(gDS1338ReadStep)
    {
        case 0:
            Second = DS1338_ReadByte(0);  //352us����
            break;
        case 1:
            Minute = DS1338_ReadByte(1);
            break;
        case 2:
            Hour   = DS1338_ReadByte(2);
            break;
        case 3:
            Date   = DS1338_ReadByte(4);
            break;
        case 4:
            Month  = DS1338_ReadByte(5);
            break;
        case 5:
            Year   = DS1338_ReadByte(6);
            break;
        default:
            break;
    }
    gDS1338ReadStep ++;
    if(gDS1338ReadStep > 5)
    {
        gDS1338ReadStep = 0;
        return(1);
    }
    else
    {
        return(0);
    }
}

