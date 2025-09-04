
/*
	Ӧ��˵����
	�ڷ���I2C�豸ǰ�����ȵ��� i2c_CheckDevice() ���I2C�豸�Ƿ��������ú���������GPIO
*/

#include "i2c_gpio.h"

/* �����дSCL��SDA�ĺ� */
#define I2C_WP_1()   myGPIO_WritePin(PIN_I2C_WP,0)
#define I2C_WP_0()   myGPIO_WritePin(PIN_I2C_WP,0)

#define I2C_SCL_1()  myGPIO_WritePin(PIN_I2C_SCL,1)
#define I2C_SCL_0()  myGPIO_WritePin(PIN_I2C_SCL,0)

#define I2C_SDA_1()  myGPIO_WritePin(PIN_I2C_SDA,1)
#define I2C_SDA_0()  myGPIO_WritePin(PIN_I2C_SDA,0)

#define I2C_SDA_READ()  myGPIO_ReadPin(PIN_I2C_SDA)	/* ��SDA����״̬ */
#define I2C_SCL_READ()  myGPIO_ReadPin(PIN_I2C_SCL)   /* ��SDA����״̬ */


#define I2C_SDA_IN()    myGPIO_SetupPinOptions(PIN_I2C_SDA,myGPIO_INPUT,myGPIO_PULLUP)
#define I2C_SCL_IN()    myGPIO_SetupPinOptions(PIN_I2C_SCL,myGPIO_INPUT,myGPIO_PULLUP)

#define I2C_SDA_OUT()   myGPIO_SetupPinOptions(PIN_I2C_SDA,myGPIO_OUTPUT,myGPIO_PULLUP)
#define I2C_SCL_OUT()   myGPIO_SetupPinOptions(PIN_I2C_SCL,myGPIO_OUTPUT,myGPIO_PULLUP)
#define I2C_WP_OUT()    myGPIO_SetupPinOptions(PIN_I2C_WP,myGPIO_OUTPUT,myGPIO_PULLUP)

/*
*********************************************************************************************************
*   �� �� ��: myGPIO_SetupPinOptions
*   ����˵��: gpio�����Ͽⲻ֧�֣���Ҫ��д
*   ��    ��:  ��
*   �� �� ֵ: ��
*********************************************************************************************************
*/
void myGPIO_SetupPinOptions(Uint16 gpioNumber,Uint16 output ,Uint16 flags)
{
    volatile Uint32 *gpioBaseAddr;
    volatile Uint32 *dir,*pud,*mux;
    Uint32 pin32,pinMask;

    pin32 = gpioNumber % 32;
    pinMask = 1UL << pin32;
    gpioBaseAddr = (Uint32 *)&GpioCtrlRegs + (gpioNumber / 32)*MYGPY_CTRL_OFFSET;
    dir = gpioBaseAddr + MYGPYDIR;
    pud = gpioBaseAddr + MYGPYPUD;
    mux = gpioBaseAddr + MYGPYMUX;
    EALLOW;
    *dir &= ~pinMask;
    if(output  == 1)
    {
        *dir |= pinMask;
        if(flags & myGPIO_PULLUP)
        {
            *pud &=  ~pinMask;
        }
        else
        {
            *pud |=  pinMask;
        }
    }
    else
    {
        *dir &= ~pinMask;
        if(flags & myGPIO_PULLUP)
        {
            *pud &=  ~pinMask;
        }
        else
        {
            *pud |=  pinMask;
        }
    }
    if(gpioNumber<16)
    {
        *mux &= ((~3UL) << (gpioNumber * 2));
    }
    else if(gpioNumber<32)
    {
        mux[1] &= ((~3UL) << ((gpioNumber-16) * 2));
    }
    else if(gpioNumber<44)
    {
        *mux &= ((~3UL) << ((gpioNumber-32) * 2));
    }
    EDIS;
}

/*
*********************************************************************************************************
*   �� �� ��: myGPIO_ReadPin
*   ����˵��: gpio�����Ͽⲻ֧�֣���Ҫ��д
*   ��    ��:  ��
*   �� �� ֵ: ��
*********************************************************************************************************
*/
Uint16 myGPIO_ReadPin(Uint16 gpioNumber)
{
    volatile Uint32 *gpioDataReg;
    Uint32 pinVal;
    gpioDataReg = (volatile Uint32 *)&GpioDataRegs + (gpioNumber / 32)*MYGPY_DATA_OFFSET;
    pinVal = (gpioDataReg[MYGPYDAT] >> (gpioNumber % 32)) & 0x1;
    return pinVal;
}
/*
*********************************************************************************************************
*   �� �� ��: myGPIO_WritePin
*   ����˵��: gpioд�������Ͽⲻ֧�֣���Ҫ��д
*   ��    ��:  ��
*   �� �� ֵ: ��
*********************************************************************************************************
*/
void myGPIO_WritePin(Uint16 gpioNumber,Uint16 outVal)
{
    volatile Uint32 *gpioDataReg;
    Uint32 pinMask;
    gpioDataReg = (volatile Uint32 *)&GpioDataRegs + (gpioNumber / 32)*MYGPY_DATA_OFFSET;
    pinMask = 1UL<<(gpioNumber % 32);
    if(outVal)
    {
        gpioDataReg[MYGPYSET] = pinMask;
    }
    else
    {
        gpioDataReg[MYGPYCLEAR] = pinMask;
    }
}
/*
*********************************************************************************************************
*   �� �� ��: GPIO_WritePin
*   ����˵��: gpioд�������Ͽⲻ֧�֣���Ҫ��д
*   ��    ��:  ��
*   �� �� ֵ: ��
*********************************************************************************************************
*/
void GPIO_WritePin(Uint16 gpioNumber,Uint16 outVal)
{
    volatile Uint32 *gpioDataReg;
    Uint32 pinMask;
    gpioDataReg = (volatile Uint32 *)&GpioDataRegs + (gpioNumber / 32)*MYGPY_DATA_OFFSET;
    pinMask = 1UL<<(gpioNumber % 32);
    if(outVal)
    {
        gpioDataReg[MYGPYCLEAR] = pinMask;
    }
    else
    {
        gpioDataReg[MYGPYSET] = pinMask;
    }
}
/*
*********************************************************************************************************
*	�� �� ��: bsp_InitI2C
*	����˵��: ����I2C���ߵ�GPIO������ģ��IO�ķ�ʽʵ��
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void InitI2C(void)
{
	/* ��һ��ֹͣ�ź�, ��λI2C�����ϵ������豸������ģʽ */
    I2C_SDA_OUT();
    I2C_SCL_OUT();
    I2C_WP_OUT();
    I2C_WP_1();
    i2c_Stop();
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_Delay
*	����˵��: I2C����λ�ӳ٣����400KHz
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void i2c_Delay(void)
{
	Uint16 i;

	/*��
		CPU��Ƶ168MHzʱ�����ڲ�Flash����, MDK���̲��Ż�����̨ʽʾ�����۲Ⲩ�Ρ�
		ѭ������Ϊ5ʱ��SCLƵ�� = 1.78MHz (����ʱ: 92ms, ��д������������ʾ����̽ͷ���ϾͶ�дʧ�ܡ�ʱ��ӽ��ٽ�)
		ѭ������Ϊ10ʱ��SCLƵ�� = 1.1MHz (����ʱ: 138ms, ���ٶ�: 118724B/s)
		ѭ������Ϊ30ʱ��SCLƵ�� = 440KHz�� SCL�ߵ�ƽʱ��1.0us��SCL�͵�ƽʱ��1.2us

		��������ѡ��2.2Kŷʱ��SCL������ʱ��Լ0.5us�����ѡ4.7Kŷ����������Լ1us

		ʵ��Ӧ��ѡ��400KHz���ҵ����ʼ���
	*/
	for (i = 0; i < 300; i++);
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_Start
*	����˵��: CPU����I2C���������ź�
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void i2c_Start(void)
{
	/* ��SCL�ߵ�ƽʱ��SDA����һ�������ر�ʾI2C���������ź� */
	I2C_SDA_1();
	I2C_SCL_1();
	i2c_Delay();
	I2C_SDA_0();
	i2c_Delay();
	
	I2C_SCL_0();
	i2c_Delay();
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_Start
*	����˵��: CPU����I2C����ֹͣ�ź�
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void i2c_Stop(void)
{
	/* ��SCL�ߵ�ƽʱ��SDA����һ�������ر�ʾI2C����ֹͣ�ź� */
	I2C_SDA_0();
	I2C_SCL_1();
	i2c_Delay();
	I2C_SDA_1();
	i2c_Delay();
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_SendByte
*	����˵��: CPU��I2C�����豸����8bit����
*	��    ��:  _ucByte �� �ȴ����͵��ֽ�
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void i2c_SendByte(Uint16 _ucByte)
{
	Uint16 i;

	/* �ȷ����ֽڵĸ�λbit7 */
	for (i = 0; i < 8; i++)
	{
		if (_ucByte & 0x80)
		{
			I2C_SDA_1();
		}
		else
		{
			I2C_SDA_0();
		}
		i2c_Delay();
		I2C_SCL_1();
		i2c_Delay();
		I2C_SCL_0();
		if (i == 7)
		{
			 I2C_SDA_IN(); // �ͷ�����
		}
		_ucByte <<= 1;	/* ����һ��bit */
		i2c_Delay();
	}
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_ReadByte
*	����˵��: CPU��I2C�����豸��ȡ8bit����
*	��    ��:  ��
*	�� �� ֵ: ����������
*********************************************************************************************************
*/
Uint16 i2c_ReadByte(void)
{
	Uint16 i;
	Uint16 value;
	I2C_SDA_IN(); // �ͷ�����
	i2c_Delay();
	/* ������1��bitΪ���ݵ�bit7 */
	value = 0;
	for (i = 0; i < 8; i++)
	{
		value <<= 1;
		I2C_SCL_1();
		i2c_Delay();
		if (I2C_SDA_READ())
		{
			value++;
		}
		I2C_SCL_0();
		i2c_Delay();
	}
	I2C_SDA_OUT(); // �ͷ�����
	i2c_Delay();
	return value;
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_WaitAck
*	����˵��: CPU����һ��ʱ�ӣ�����ȡ������ACKӦ���ź�
*	��    ��:  ��
*	�� �� ֵ: ����0��ʾ��ȷӦ��1��ʾ��������Ӧ
*********************************************************************************************************
*/
Uint16 i2c_WaitAck(void)
{
	Uint16 re;

	I2C_SDA_1();	/* CPU�ͷ�SDA���� */
	i2c_Delay();
	I2C_SCL_1();	/* CPU����SCL = 1, ��ʱ�����᷵��ACKӦ�� */
	i2c_Delay();
	if (I2C_SDA_READ())	/* CPU��ȡSDA����״̬ */
	{
		re = 1;
	}
	else
	{
		re = 0;
	}
	I2C_SCL_0();
	i2c_Delay();
	I2C_SDA_OUT(); // �ͷ�����
	i2c_Delay();
	return re;
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_Ack
*	����˵��: CPU����һ��ACK�ź�
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void i2c_Ack(void)
{
	I2C_SDA_0();	/* CPU����SDA = 0 */
	i2c_Delay();
	I2C_SCL_1();	/* CPU����1��ʱ�� */
	i2c_Delay();
	I2C_SCL_0();
	i2c_Delay();
	I2C_SDA_1();	/* CPU�ͷ�SDA���� */
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_NAck
*	����˵��: CPU����1��NACK�ź�
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void i2c_NAck(void)
{
	I2C_SDA_1();	/* CPU����SDA = 1 */
	i2c_Delay();
	I2C_SCL_1();	/* CPU����1��ʱ�� */
	i2c_Delay();
	I2C_SCL_0();
	i2c_Delay();
}

/*
*********************************************************************************************************
*	�� �� ��: i2c_CheckDevice
*	����˵��: ���I2C�����豸��CPU�����豸��ַ��Ȼ���ȡ�豸Ӧ�����жϸ��豸�Ƿ����
*	��    ��:  _Address���豸��I2C���ߵ�ַ
*	�� �� ֵ: ����ֵ 0 ��ʾ��ȷ�� ����1��ʾδ̽�⵽
*********************************************************************************************************
*/
Uint16 i2c_CheckDevice(Uint16 _Address)
{
	Uint16 ucAck;

	if (I2C_SDA_READ() && I2C_SCL_READ())
	{
		i2c_Start();		/* ���������ź� */

		/* �����豸��ַ+��д����bit��0 = w�� 1 = r) bit7 �ȴ� */
		i2c_SendByte(_Address | I2C_WR);
		ucAck = i2c_WaitAck();	/* ����豸��ACKӦ�� */

		i2c_Stop();			/* ����ֹͣ�ź� */

		return ucAck;
	}
	return 1;	/* I2C�����쳣 */
}


void WriteEnable(void)
{
    I2C_WP_0();i2c_Delay();i2c_Delay();
}
void WriteDisable(void)
{
    I2C_WP_1();i2c_Delay();i2c_Delay();
}
