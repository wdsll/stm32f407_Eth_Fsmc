/*
 * w5500_conf.c
 *
 *  Created on: Aug 4, 2025
 *      Author: huyan
 */

/*
**************************************************************************************************
* @file    		w5500_conf.c
* @author  		WIZnet Software Team
* @version 		V1.0
* @date    		2015-02-14
* @brief  		配置MCU，移植W5500程序需要修改的文件，配置W5500的MAC和IP地址
**************************************************************************************************
*/
#include <stdio.h>
#include <string.h>

#include "w5500_conf.h"
//#include "bsp/i2c/bsp_i2c_eeprom.h"
#include "utility.h"
#include "w5500.h"
#include "dhcp.h"
#include "spi.h"
//#include "SysTick.h"
//#include "timer.h"
//#include "SEGGER_RTT.h"
//#include "tim.h"

//#include "bsp/TimBase/bsp_TiMbase.h" //TIM2

CONFIG_MSG  ConfigMsg;																	/*配置结构体*/


u16 SSIZE[MAX_SOCK_NUM]={0,0,0,0,0,0,0,0}; // Max Tx buffer
u16 RSIZE[MAX_SOCK_NUM]={0,0,0,0,0,0,0,0}; // Max Rx buffer
u8 txsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};//tx buffer set	K bits
u8 rxsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};//rx buffet set  K bits

//EEPROM_MSG_STR EEPROM_MSG;															/*EEPROM存储信息结构体*/

extern SPI_HandleTypeDef hspi1;

/*网络配置*/
u8 local_ip[4]  ={192,168,0,2};
u8 subnet[4]    ={255,255,255,0};
u8 gateway[4]   ={192,168,0,3};
u8 dns_server[4]={192,168,0,3};
u8 IO_number = 0x00;    //IO控制器编号
u8 mac[6]={0x00,0x08,0xdc,0x01,0x02,0x03};

u16 local_port=8080;                                                            /*定义本地端口 8080*/
u16 local_port_setIP=9999;              //设置IP使用

/*定义远端IP信息*/
u8  remote_ip[4]={192,168,0,3};
u16 remote_port=5000;                                                                                                          /*远端端口号*/



/*IP配置方法选择，请自行选择*/
u8	ip_from=IP_FROM_DEFINE;

u8   dhcp_ok   = 0;													   			/*dhcp成功获取IP*/
uint32	ms        = 0;															  	/*毫秒计数*/
uint32	dhcp_time = 0;															  	/*DHCP运行计数*/
vu8	    ntptimer  = 0;															  	/*NPT秒计数*/

/**
*@brief		硬件复位W5500
*@param		无
*@return	无
*/
void reset_w5500(void)
{
//  GPIO_ResetBits(WIZ_RESET_PORT, WIZ_RESET);
//  delay_ms(10);
//  GPIO_SetBits(WIZ_RESET_PORT, WIZ_RESET);
//  delay_ms(1600);


	HAL_GPIO_WritePin(WIZ_RESET_PORT,WIZ_RESET,GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(WIZ_RESET_PORT,WIZ_RESET,GPIO_PIN_SET);
	HAL_Delay(1600);
}

/**
*@brief		配置W5500的IP地址
*@param		无
*@return	无
*/
void set_w5500_ip(void)
{
	/*复制定义的配置信息到配置结构体*/
	memcpy(ConfigMsg.mac, mac, 6);
	memcpy(ConfigMsg.lip,local_ip,4);
	memcpy(ConfigMsg.sub,subnet,4);
	memcpy(ConfigMsg.gw,gateway,4);
	memcpy(ConfigMsg.dns,dns_server,4);
	if(ip_from==IP_FROM_DEFINE)
		SEGGER_RTT_printf(0,"Use the defined IP information configuration W5500\r\n"); //使用定义的IP信息配置W5500


	/*使用DHCP获取IP参数，需调用DHCP子函数*/
	if(ip_from==IP_FROM_DHCP)
	{
		/*复制DHCP获取的配置信息到配置结构体*/
		if(dhcp_ok==1)
		{
			SEGGER_RTT_printf(0," IP from DHCP\r\n");
			memcpy(ConfigMsg.lip,DHCP_GET.lip, 4);
			memcpy(ConfigMsg.sub,DHCP_GET.sub, 4);
			memcpy(ConfigMsg.gw,DHCP_GET.gw, 4);
			memcpy(ConfigMsg.dns,DHCP_GET.dns,4);
		}
		else
		{
			SEGGER_RTT_printf(0," The DHCP subroutine did not run, or was unsuccessful\r\n");	//DHCP子程序未运行,或者不成功
			SEGGER_RTT_printf(0," Configure W5500 using the defined IP information\r\n");	//使用定义的IP信息配置W5500
		}
	}

	/*以下配置信息，根据需要选用*/
	ConfigMsg.sw_ver[0]=FW_VER_HIGH;
	ConfigMsg.sw_ver[1]=FW_VER_LOW;

	/*将IP配置信息写入W5500相应寄存器*/
	setSUBR(ConfigMsg.sub);
	setGAR(ConfigMsg.gw);
	setSIPR(ConfigMsg.lip);

	getSIPR (local_ip);
	SEGGER_RTT_printf(0," W5500 IP   : %d.%d.%d.%d\r\n", local_ip[0],local_ip[1],local_ip[2],local_ip[3]);		//IP地址
	getSUBR(subnet);
	SEGGER_RTT_printf(0," W5500 subnet : %d.%d.%d.%d\r\n", subnet[0],subnet[1],subnet[2],subnet[3]);					//子网掩码
	getGAR(gateway);
	SEGGER_RTT_printf(0," W5500 gateway     : %d.%d.%d.%d\r\n", gateway[0],gateway[1],gateway[2],gateway[3]);	//网关
}

void W5500_Init(void)
{
    reset_w5500();
    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Deselect);
    reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
    set_w5500_ip();
}

/**
*@brief		配置W5500的MAC地址
*@param		无
*@return	无
*/
void set_w5500_mac(void)
{
	memcpy(ConfigMsg.mac, mac, 6);
	setSHAR(ConfigMsg.mac);	/**/
	memcpy(DHCP_GET.mac, mac, 6);
}

/**
*@brief		配置W5500的GPIO接口
*@param		无
*@return	无
*/
//void gpio_for_w5500_config(void)
//{
//	SPI_InitTypeDef  SPI_InitStructure;
//  GPIO_InitTypeDef GPIO_InitStructure;
//
//	RCC_APB2PeriphClockCmd(WIZ_SPIx_INT_CLK|WIZ_RESET_CLK|RCC_APB2Periph_AFIO, ENABLE);
//
//	RCC_APB2PeriphClockCmd(WIZ_SPIx_GPIO_CLK|WIZ_SPIx_SCS_CLK, ENABLE);
//
//	WIZ_SPIx_CLK_CMD(WIZ_SPIx_CLK, ENABLE);
//
//	/*!< Configure SPI_FLASH_SPI pins: SCK */
//  GPIO_InitStructure.GPIO_Pin = WIZ_SPIx_SCLK;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//  GPIO_Init(WIZ_SPIx_GPIO_PORT, &GPIO_InitStructure);

//  /*!< Configure SPI_FLASH_SPI pins: MISO */
//  GPIO_InitStructure.GPIO_Pin = WIZ_SPIx_MISO;
//  GPIO_Init(WIZ_SPIx_GPIO_PORT, &GPIO_InitStructure);

//  /*!< Configure SPI_FLASH_SPI pins: MOSI */
//  GPIO_InitStructure.GPIO_Pin = WIZ_SPIx_MOSI;
//  GPIO_Init(WIZ_SPIx_GPIO_PORT, &GPIO_InitStructure);

//  /* Disable the Serial Wire Jtag Debug Port SWJ-DP */
////  GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);//是否需要？？？
//
//  /*!< Configure SPI_FLASH_SPI_CS_PIN pin: SPI_FLASH Card CS pin */
//  GPIO_InitStructure.GPIO_Pin = WIZ_SPIx_SCS;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//  GPIO_Init(WIZ_SPIx_SCS_PORT, &GPIO_InitStructure);

//  /* SPI1 configuration */
//  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
//  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
//  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
//  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
//  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
//  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
//  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
//  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
//  SPI_InitStructure.SPI_CRCPolynomial = 7;
//  SPI_Init(WIZ_SPIx, &SPI_InitStructure);
//  SPI_Cmd(WIZ_SPIx, ENABLE);
//
//  /*定义INT引脚*/
//  GPIO_InitStructure.GPIO_Pin = WIZ_INT;						       /*选择要控制的GPIO引脚*/
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		     /*设置引脚速率为50MHz*/
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;				     /*设置引脚模式为通用推挽模拟上拉输入*/
//  GPIO_Init(WIZ_SPIx_INT_PORT, &GPIO_InitStructure);			 /*调用库函数，初始化GPIO*/

//  /*定义RESET引脚*/
//  GPIO_InitStructure.GPIO_Pin = WIZ_RESET;					 /*选择要控制的GPIO引脚*/
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 /*设置引脚速率为50MHz */
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		 /*设置引脚模式为通用推挽输出*/
//  GPIO_Init(WIZ_RESET_PORT, &GPIO_InitStructure);							 /*调用库函数，初始化GPIO*/
//  GPIO_SetBits(WIZ_RESET_PORT, WIZ_RESET);
//
//}




/**
*@brief		配置W5500的GPIO接口
*@param		无
*@return	无
*/
void gpio_for_w5500_config(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_SPI1_CLK_ENABLE() ;
  WIZ_SPI_GPIO_ClK_ENABLE();
  WIZ_SPIx_SCS_CLK_ENABLE();
	WIZ_INT_CLK_ENABLE();
  WIZ_RESET_CLK_ENABLE();

  GPIO_InitStruct.Pin = WIZ_SPIx_SCLK_PIN|WIZ_SPIx_MOSI_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(WIZ_SPIx_GPIO_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = WIZ_SPIx_MISO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(WIZ_SPIx_GPIO_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(WIZ_SPIx_SCS_PORT, WIZ_SPIx_SCS_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = WIZ_SPIx_SCS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(WIZ_SPIx_SCS_PORT, &GPIO_InitStruct);

  /* Disable the Serial Wire Jtag Debug Port SWJ-DP */
//  __HAL_AFIO_REMAP_SWJ_DISABLE();

  hspi1.Instance = WIZ_SPIx;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  HAL_SPI_Init(&hspi1);

  /*定义INT引脚*/
  GPIO_InitStruct.Pin = WIZ_INT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(WIZ_SPIx_SCS_PORT, &GPIO_InitStruct);

  /*定义RESET引脚*/
  HAL_GPIO_WritePin(WIZ_RESET_PORT, WIZ_RESET_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = WIZ_RESET_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(WIZ_RESET_PORT, &GPIO_InitStruct);
}

/**
*@brief		W5500片选信号设置函数
*@param		val: 为“0”表示片选端口为低，为“1”表示片选端口为高
*@return	无
*/
void wiz_cs(u8 val)
{
	if (val == LOW)
	{
//	  GPIO_ResetBits(WIZ_SPIx_SCS_PORT, WIZ_SPIx_SCS);
		HAL_GPIO_WritePin(WIZ_SPIx_SCS_PORT,WIZ_SPIx_SCS,GPIO_PIN_RESET);
	}
	else if (val == HIGH)
	{
//	  GPIO_SetBits(WIZ_SPIx_SCS_PORT, WIZ_SPIx_SCS);
		HAL_GPIO_WritePin(WIZ_SPIx_SCS_PORT,WIZ_SPIx_SCS,GPIO_PIN_SET);
	}
}

/**
*@brief		设置W5500的片选端口SCSn为低
*@param		无
*@return	无
*/
void iinchip_csoff(void)
{
	wiz_cs(LOW);
}

/**
*@brief		设置W5500的片选端口SCSn为高
*@param		无
*@return	无
*/
void iinchip_cson(void)
{
   wiz_cs(HIGH);
}

//u8 SPI_SendByte(u8 byte)
//{
//  /* Loop while DR register in not emplty */
//  while (SPI_I2S_GetFlagStatus(WIZ_SPIx, SPI_I2S_FLAG_TXE) == RESET);

//  /* Send byte through the SPI1 peripheral */
//  SPI_I2S_SendData(WIZ_SPIx, byte);

//  /* Wait to receive a byte */
//  while (SPI_I2S_GetFlagStatus(WIZ_SPIx, SPI_I2S_FLAG_RXNE) == RESET);

//  /* Return the byte read from the SPI bus */
//  return SPI_I2S_ReceiveData(WIZ_SPIx);
//}

u8 SPI_SendByte(u8 byte)
{
  uint8_t d_read,d_send=byte;
  if(HAL_SPI_TransmitReceive(&hspi1,&d_send,&d_read,1,0xFFFFFF)!=HAL_OK)
    d_read=0XFF;

  return d_read;
}

/**
*@brief		STM32 SPI1读写8位数据
*@param		dat：写入的8位数据
*@return	无
*/
u8  IINCHIP_SpiSendData(u8 dat)
{
   return(SPI_SendByte(dat));
}

/**
*@brief		写入一个8位数据到W5500
*@param		addrbsb: 写入数据的地址
*@param   data：写入的8位数据
*@return	无
*/
//void IINCHIP_WRITE( uint32 addrbsb,  u8 data)
//{
//   iinchip_csoff();
//   IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);
//   IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);
//   IINCHIP_SpiSendData( (addrbsb & 0x000000F8) + 4);
//   IINCHIP_SpiSendData(data);
//   iinchip_cson();
//}

/**
*@brief		从W5500读出一个8位数据
*@param		addrbsb: 写入数据的地址
*@param   data：从写入的地址处读取到的8位数据
*@return	无
*/
//u8 IINCHIP_READ(uint32 addrbsb)
//{
//   u8 data = 0;
//   iinchip_csoff();
//   IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);
//   IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);
//   IINCHIP_SpiSendData( (addrbsb & 0x000000F8))    ;
//   data = IINCHIP_SpiSendData(0x00);
//   iinchip_cson();
//   return data;
//}

/**
*@brief		向W5500写入len字节数据
*@param		addrbsb: 写入数据的地址
*@param   buf：写入字符串
*@param   len：字符串长度
*@return	len：返回字符串长度
*/
u16 wiz_write_buf(uint32 addrbsb,u8* buf,u16 len)
{
   u16 idx = 0;
   if(len == 0) SEGGER_RTT_printf(0,"Unexpected2 length 0\r\n");
   iinchip_csoff();
   IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);
   IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);
   IINCHIP_SpiSendData( (addrbsb & 0x000000F8) + 4);
   for(idx = 0; idx < len; idx++)
   {
     IINCHIP_SpiSendData(buf[idx]);
   }
   iinchip_cson();
   return len;
}

/**
*@brief		从W5500读出len字节数据
*@param		addrbsb: 读取数据的地址
*@param 	buf：存放读取数据
*@param		len：字符串长度
*@return	len：返回字符串长度
*/
u16 wiz_read_buf(uint32 addrbsb, u8* buf,u16 len)
{
  u16 idx = 0;
  if(len == 0)
  {
    SEGGER_RTT_printf(0,"Unexpected2 length 0\r\n");
  }
  iinchip_csoff();
  IINCHIP_SpiSendData( (addrbsb & 0x00FF0000)>>16);
  IINCHIP_SpiSendData( (addrbsb & 0x0000FF00)>> 8);
  IINCHIP_SpiSendData( (addrbsb & 0x000000F8));
  for(idx = 0; idx < len; idx++)
  {
    buf[idx] = IINCHIP_SpiSendData(0x00);
  }
  iinchip_cson();
  return len;
}

/**
*@brief		写配置信息到EEPROM
*@param		无
*@return	无
*/
//void write_config_to_eeprom(void)
//{
//	u16 dAddr=0;
//	eeprom_WriteBytes(ConfigMsg.mac,dAddr,(u8)EEPROM_MSG_LEN);
//	delay_ms(10);
//}

/**
*@brief		从EEPROM读配置信息
*@param		无
*@return	无
*/
//void read_config_from_eeprom(void)
//{
//	eeprom_ReadBytes(EEPROM_MSG.mac,0,EEPROM_MSG_LEN);
//	delay_us(10);
//}

/**
*@brief		STM32定时器2初始化
*@param		无
*@return	无
*/
void timer2_init(void)
{
//	TIM2_Configuration();																		/* TIM2 定时配置 */
//	TIM2_NVIC_Configuration();															/* 定时器的中断优先级 */
//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);		/* TIM2 重新开时钟，开始计时 */
}

/**
*@brief		dhcp用到的定时器初始化
*@param		无
*@return	无
*/
void dhcp_timer_init(void)
{
//  timer2_init();
//	TIM4_Configuration();
//	TIM4_NVIC_Configuration();

//	MX_TIM4_Init();

}

/**
*@brief		ntp用到的定时器初始化
*@param		无
*@return	无
*/
void ntp_timer_init(void)
{
//  timer2_init();
//	TIM4_Configuration();
//	TIM4_NVIC_Configuration();

//	MX_TIM4_Init();
}

/**
*@brief		定时器2中断函数
*@param		无
*@return	无
*/
//void timer2_isr(void)
//{
//  ms++;
//  if(ms>=1000)
//  {
//    ms=0;
//    dhcp_time++;																					/*DHCP定时加1S*/
//	  #ifndef	__NTP_H__
//	  ntptimer++;																						/*NTP重试时间加1S*/
//	  #endif
//  }

//}

void timer4_isr(void)
{
  ms++;
  if(ms>=1000)
  {
    ms=0;
    dhcp_time++;																					/*DHCP定时加1S*/
	  #ifndef	__NTP_H__
	  ntptimer++;																						/*NTP重试时间加1S*/
	  #endif
  }

}

/**
*@brief		STM32系统软复位函数
*@param		无
*@return	无
*/
//void reboot(void)
//{
//  pFunction Jump_To_Application;
//  uint32 JumpAddress;
//  printf(" 系统重启中……\r\n");
//  JumpAddress = *(vuint32*) (0x00000004);
//  Jump_To_Application = (pFunction) JumpAddress;
//  Jump_To_Application();
//}
