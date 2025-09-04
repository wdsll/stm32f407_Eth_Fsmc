/*
*********************************************************************************************************
*

*
*********************************************************************************************************
*/

#ifndef _EEPROM_24XX_H
#define _EEPROM_24XX_H
#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include "i2c_gpio.h"
//#define AT24C02
//#define AT24C128
#define AT24C256

#ifdef AT24C02
	#define EE_MODEL_NAME		"AT24C02"
	#define EE_DEV_ADDR			0xA0		/* �豸��ַ */
	#define EE_PAGE_SIZE		8			/* ҳ���С(�ֽ�) */
	#define EE_SIZE				256			/* ������(�ֽ�) */
	#define EE_ADDR_BYTES		1			/* ��ַ�ֽڸ��� */
#endif

#ifdef AT24C128
	#define EE_MODEL_NAME		"AT24C128"
	#define EE_DEV_ADDR			0xA0		/* �豸��ַ */
	#define EE_PAGE_SIZE		64			/* ҳ���С(�ֽ�) */
	#define EE_SIZE				(16*1024)	/* ������(�ֽ�) */
	#define EE_ADDR_BYTES		2			/* ��ַ�ֽڸ��� */
#endif

#ifdef AT24C256
    #define EE_MODEL_NAME       "AT24C256"
    #define EE_DEV_ADDR         0xA0        /* �豸��ַ */
    #define EE_PAGE_SIZE        64          /* ҳ���С(�ֽ�) */
    #define EE_SIZE             (32*1024)   /* ������(�ֽ�) */
    #define EE_ADDR_BYTES       2           /* ��ַ�ֽڸ��� */
#endif

Uint16 ee_ReadWord(Uint16 *_pReadBuf, Uint16 _usAddress, Uint16 _usSize);
Uint16 ee_WriteWord(Uint16 *_pWriteBuf, Uint16 _usAddress, Uint16 _usSize);
Uint16 ee_ReadByte(Uint8 *_pReadBuf,  Uint16 _usAddress, Uint16 _usSize);
Uint16 ee_WriteByte(Uint8 *_pWriteBuf, Uint16 _usAddress, Uint16 _usSize);
#endif

