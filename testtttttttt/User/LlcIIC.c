/*******************************************************************************
 * LlcIIC.c
 *
 *  Created on: 2020-1-10
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcIIC.h"

/*******************************************************************************
**                               �궨��
*******************************************************************************/
#define SDA_R   GpioDataRegs.GPADAT.bit.GPIO28     //SDA ��״̬
#define SDA_W0  GpioDataRegs.GPACLEAR.bit.GPIO28=1 //SDA ���0 д״̬
#define SDA_W1  GpioDataRegs.GPASET.bit.GPIO28=1   //SDA ���1 д״̬
#define SCL_0   GpioDataRegs.GPACLEAR.bit.GPIO29=1 //SCL ���0
#define SCL_1   GpioDataRegs.GPASET.bit.GPIO29=1   //SCL ���1

#define Delay_UNIT  20                              //�궨����ʱʱ�䳣��
//static Uint16 EEPRomErr = 0;                              //Eeprom��д����ָʾ

/*******************************************************************************
**                               ��ؽṹ��
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/

/*******************************************************************************
**                               ��������
*******************************************************************************/
static __inline void SDA_READ(void);
static __inline void SDA_WRITE(void);
static void Delay(Uint16 time);
static void BeginTrans();
static void StopTrans();
static void Ack();
static void ByteIn(Uint16 ch);
static Uint16 ByteOut(void);
Uint16 WriteByte(Uint16 addr,Uint16 data);
Uint16 ReadByte(Uint16 addr);
Uint16 USBReadByte(Uint16 addr);

/******************************IO�ڳ�ʼ��****************************/
__inline void SDA_READ(void)
{
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO28 = 0;       //Input, SDA
    EDIS;
}

__inline void SDA_WRITE(void)
{
    EALLOW;
    GpioCtrlRegs.GPADIR.bit.GPIO28 = 1;       //Output. SDA
    EDIS;
}

/************************GPIOģ��IICͨ��****************************/
void Delay(Uint16 time)                     //��ʱ����
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
        asm(" nop");
        asm(" nop");
    }
}

void BeginTrans(void)                       //����START �ź�
{
    SDA_W1;                                 //SDA=1
    Delay(Delay_UNIT * 8);                  //��ʱ
    SDA_WRITE();                            //SDA ����Ϊ�����EEPROM
    Delay(Delay_UNIT);                      //��ʱ
    SCL_1;                                  //SCL=1
    Delay(Delay_UNIT * 8);                  //��ʱ
    SDA_W0;                                 //SDA=0
    Delay(Delay_UNIT * 8);                  //��ʱ
}

void StopTrans(void)                        //����STOP �ź�
{
    SDA_WRITE();                            //SDA����Ϊ�����EEPROM
    Delay(Delay_UNIT);                      //��ʱ
    SDA_W0;                                 //SDA=0
    Delay(Delay_UNIT * 5);                  //��ʱ
    SCL_1;                                  //SCL=1
    Delay(Delay_UNIT * 5);                  //��ʱ
    SDA_W1;                                 //SDA=1
    Delay(Delay_UNIT * 5);
}

void IIC_Ack(void)                 // ����ACK�źŵ�EPPROM
{

//        IIC_SCL_0;                      // SCL = 0
//        IIC_SDA_WRITE();                // SDA����Ϊ�����EEPROM
//        IIC_SDA_W0;                     // ����ͨ��SDA ��������EEPROM
//        IIC_Delay(DELAY_UNIT2 * 2);      // ��ʱ
//        IIC_SCL_1;                      // SCL = 1
//        IIC_Delay(DELAY_UNIT2 * 2);      // ��ʱ
//        IIC_SCL_0;                      // SCL = 0

        SCL_0;                      // SCL = 0
        SDA_WRITE();                // SDA����Ϊ�����EEPROM
        SDA_W0;                     // ����ͨ��SDA ��������EEPROM
	    Delay(Delay_UNIT * 2);      // ��ʱ
	    SCL_1;                      // SCL = 1
	    Delay(Delay_UNIT * 2);      // ��ʱ
	    SCL_0;                      // SCL = 0

}



void Ack(void)                              //�ȴ�EEPROM ��Ack �ź�
{
    Uint16 i;
    SDA_READ();                             //SDA����Ϊ��EEPROM ����
    Delay(Delay_UNIT);                      //��ʱ
    SCL_1;                                  //SCL=1
    Delay(Delay_UNIT * 2);                  //��ʱ
    i = 0;
    while((SDA_R == 1) && (i <= 500))       //�ȴ�EEPROM ����͵�ƽ,5ms���˳�ѭ��
    {
        i++ ;
        Delay(Delay_UNIT);
    }

    if (i >= 499)
    {
        //EEPRomErr = 0xff;
    }

    i = 0;
    SCL_0;                                  //SCL=0
    Delay(Delay_UNIT);                      //��ʱ
}

void ByteIn(Uint16 DATA)                    //��EEPROM д��һ���ֽ�
{
    Uint16 tmpCpunter;
    SCL_0;                                  //SCL=0
    Delay(Delay_UNIT * 2);                  //��ʱ
    SDA_WRITE();                            //SDA����Ϊ�����EEPROM
    for(tmpCpunter = 8; tmpCpunter > 0; tmpCpunter --)
    {
        if ((DATA & 0x80)== 0)
        {
            SDA_W0;                         //����ͨ��SDA ��������EEPROM
            Delay(Delay_UNIT);              //��ʱ
        }
        else
        {
            SDA_W1;
            Delay(Delay_UNIT);              //��ʱ
        }
        SCL_1;                              //SCL=1
        Delay(Delay_UNIT);                  //��ʱ
        DATA <<= 1;
        SCL_0;                              //SCL=0
        Delay(Delay_UNIT);                  //��ʱ
    }
    Ack();
}

Uint16 ByteOut(void)                        //��EEPROM ���һ���ֽ�
{
    unsigned char i;
    Uint16 tmpData = 0;
    SDA_READ();                             //SDA �ķ���Ϊ��EEPROM ���
    for(i=8;i>0;i--)
    {
        tmpData <<= 1;
        SCL_1;                              //SCL=1
        Delay(Delay_UNIT);                  //��ʱ
        tmpData |= SDA_R;                   //����ͨ��SDA �����Ƴ�EEPROM
        Delay(Delay_UNIT);                  //��ʱ
        SCL_0;                              //SCL=0
        Delay(Delay_UNIT);                  //��ʱ
    }
    return(tmpData);
}

Uint16 gWriteEEPDelay = 0;
Uint16 WriteByte(Uint16 addr,Uint16 data)     //��EEPROM ָ����ַд��һ���ֽڵ����ݣ�дһ��ʱ����1.6ms����������д�룬��Ҫ��ʱ5ms
{
    if(!gWriteEEPDelay)
    {
        GpioDataRegs.GPACLEAR.bit.GPIO9 = 1;
        Uint16 Haddr;
        Uint16 Laddr;
        Haddr = addr >> 8;
        Laddr = addr&0x00ff;
        BeginTrans();                           //��ʼ
        ByteIn(0xA0);                           //д��д������0xA0
        ByteIn(Haddr);                          //д��ָ����ַ
        ByteIn(Laddr);
        ByteIn(data);                           //д���д��EEPROM ������
        StopTrans();                            //ֹͣ
        GpioDataRegs.GPASET.bit.GPIO9 = 1;
        Delay(20000);
        gWriteEEPDelay = 5;
        return(1);
    }
    else
    {
        return(0);
    }
}

Uint16 gReadEEPROMStep = 0;
Uint16 ReadByte(Uint16 addr)                //��EEPROM ָ����ַ��ȡһ���ֽڵ����ݣ�ʱ��2ms������������ȡ
{
    Uint16 Haddr;
    Uint16 Laddr;
    Uint16 tmpReadData=256;

    BeginTrans();                           //��ʼ //227us
    ByteIn(0xA0);                           //д��д������0xA0 //288us

    Haddr = addr>>8;
    Laddr = addr&0x00ff;
    ByteIn(Haddr);                          //д��ָ����ַ��λ//288us
    ByteIn(Laddr);                          //д��ָ����ַ��λ//288us

    BeginTrans();                           //��ʼ//227us
    ByteIn(0xA1);                           //д���������0xA1//288us

    tmpReadData = ByteOut();                //����EEPROM ��������� //231us
    StopTrans();                            //ֹͣ//147ns

    return(tmpReadData);
}


Uint16 USBReadByte(Uint16 addr)                //��EEPROM ָ����ַ��ȡһ���ֽڵ����ݣ�ʱ��2ms������������ȡ
{
    Uint16 Haddr;
    Uint16 Laddr;
    Uint16 tmpReadData=256;
    BeginTrans();                           //��ʼ //227us
    ByteIn(0xA0);                           //д��д������0xA0 //288us

    tmpReadData = 256;
    Haddr = addr>>8;
    Laddr = addr&0x00ff;
    ByteIn(Haddr);                          //д��ָ����ַ��λ//288us
    ByteIn(Laddr);                          //д��ָ����ַ��λ//288us

    tmpReadData = 256;
    BeginTrans();                           //��ʼ//227us
    ByteIn(0xA1);                           //д���������0xA1//288us

    tmpReadData = 256;
    tmpReadData = ByteOut();                //����EEPROM ��������� //231us
    StopTrans();                            //ֹͣ//147ns

    return(tmpReadData);
}

  Uint16 g_16data[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//�������ֽڲ�����
Uint16 USBReadByteS(Uint16 addr, Uint16 length) //��EEPROM ָ����ַ��ȡ16���ֽڵ����ݣ�ʱ��2ms������������ȡ
{
    Uint16 Haddr;
    Uint16 Laddr;
  //  Uint16 tmpReadData=256;
   // Uint16 s_succeed_flag;
    static  Uint16 temp = 0;
    BeginTrans();                           //��ʼ //227us
    ByteIn(0xA0);                           //д��д������0xA0 //288us
   // gReadEEPROMStep ++;
   // g_16data[0] = 256;
    Haddr = addr>>8;
    Laddr = addr&0x00ff;
    ByteIn(Haddr);                          //д��ָ����ַ��λ//288us
    ByteIn(Laddr);                          //д��ָ����ַ��λ//288us
   // gReadEEPROMStep ++;
  //  g_16data[0] = 256;
    BeginTrans();                           //��ʼ//227us
    ByteIn(0xA1);                           //д���������0xA1//288us
  //  gReadEEPROMStep ++;
 //   g_16data[0] = 256;

    for(temp = 0; temp < length;temp ++) //
    {
    g_16data[temp] = ByteOut();                //����EEPROM ��������� //231us
    IIC_Ack();                               //Ӧ��EEPROM������һ���ֽ�
    Delay(Delay_UNIT);
    Delay(Delay_UNIT);

    }

    StopTrans();                            //ֹͣ//147ns

    return(1);
}

//Uint16 g_16data[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//�������ֽڲ�����
Uint16 USBReadByteD(Uint16 addr, Uint16 *g_16data, Uint16 length) //��EEPROM ָ����ַ��ȡ16���ֽڵ����ݣ�ʱ��2ms������������ȡ
{

   Uint16 Haddr;
   Uint16 Laddr;
 //  Uint16 tmpReadData=256;
  // Uint16 s_succeed_flag;
   static  Uint16 temp = 0;
   BeginTrans();                           //��ʼ //227us
   ByteIn(0xA0);                           //д��д������0xA0 //288us
  // gReadEEPROMStep ++;
  // g_16data[0] = 256;
   Haddr = addr>>8;
   Laddr = addr&0x00ff;
   ByteIn(Haddr);                          //д��ָ����ַ��λ//288us
   ByteIn(Laddr);                          //д��ָ����ַ��λ//288us
  // gReadEEPROMStep ++;
 //  g_16data[0] = 256;
   BeginTrans();                           //��ʼ//227us
   ByteIn(0xA1);                           //д���������0xA1//288us
 //  gReadEEPROMStep ++;
//   g_16data[0] = 256;

   for(temp = 0; temp < length;temp ++) //
   {
   g_16data[temp] = ByteOut();                //����EEPROM ��������� //231us
   IIC_Ack();                               //Ӧ��EEPROM������һ���ֽ�
   Delay(Delay_UNIT);
   Delay(Delay_UNIT);

   }

   StopTrans();                            //ֹͣ//147ns

   return(1);
}

