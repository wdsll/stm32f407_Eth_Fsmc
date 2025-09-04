/*
 * w5500_conf.h
 *
 *  Created on: Aug 4, 2025
 *      Author: huyan
 */

#ifndef W5500_CONF_H_
#define W5500_CONF_H_

#include "board_config.h"
#include "stdio.h"
#include <string.h>

#include "w5500_conf.h"
//#include "bsp_i2c_eeprom.h"
#include "utility.h"
#include "w5500.h"
#include "dhcp.h"
//#include "bsp/TimBase/bsp_TiMbase.h"
//#include "types.h"
#define __GNUC__
extern u8    remote_ip[4];                            	  /* 远端IP地址                   */
extern u16   remote_port;                            	  /* 远端端口号                   */
extern u16   local_port;                             	  /* 定义本地端口                 */
extern u8    use_dhcp;                              	    /* 是否使用DHCP获取IP           */
extern u8    use_eeprom;                             	  /* 是否使用EEPROM中的IP配置信息 */

extern u16   local_port_setIP;

typedef  void (*pFunction)(void);

/*定义默认IP信息*/
extern u8 local_ip[4]  ;										/*定义W5500默认IP地址*/
extern u8 subnet[4]   ;										/*定义W5500默认子网掩码*/
extern u8 gateway[4];											/*定义W5500默认网关*/
extern u8 dns_server[4];									/*定义W5500默认DNS*/
extern u8 IO_number ;	//IO控制器编号
extern u8  guangbo_ip[4];		//广播用
extern u16 guangbo_port;																/*广播端口号*/
extern u8  ZH1_IP[4];
extern u8  CJ2_IP[4];
extern u16 ZH1_port;
//本来是5000 待测试
extern u16 local_port;	                       					/*定义本地端口*/

extern u8 mac[6];

extern u16 SSIZE[MAX_SOCK_NUM]; // Max Tx buffer
extern u16 RSIZE[MAX_SOCK_NUM]; // Max Rx buffer
extern u8 txsize[];
extern u8 rxsize[];

//注意按照原理图修改一下引脚
/*定义SPI作为W5500的硬件接口*/
//#define WIZ_SPIx                         SPI3                   	     /* 定义W5500所用的SPI接口       */
#define WIZ_SPIx_RCC_CLK_ENABLE()        __HAL_RCC_SPI1_CLK_ENABLE()   /* 定义W5500所用的SPI接口时钟   */

#define WIZ_SPI_GPIO_ClK_ENABLE()        __HAL_RCC_GPIOB_CLK_ENABLE() /* GPIO端口时钟                 */
//#define WIZ_SPIx_GPIO_PORT               GPIOB									  	  /* GPIO端口                     */
#define WIZ_SPIx_SCLK_PIN                GPIO_PIN_3						   	    /* 定义W5500的时钟管脚          */
#define WIZ_SPIx_MISO_PIN                GPIO_PIN_4						   	    /* 定义W5500的MISO管脚          */
#define WIZ_SPIx_MOSI_PIN                GPIO_PIN_5						   	    /* 定义W5500的MOSI管脚          */

#define WIZ_SPIx_SCS_PIN                 GPIO_PIN_5						   	  /* 定义W5500的片选管脚          */
//#define WIZ_SPIx_SCS_PORT                GPIOF									   	  /* GPIO端口                     */
#define WIZ_SPIx_SCS_CLK_ENABLE()        __HAL_RCC_GPIOC_CLK_ENABLE()	/* GPIO端口时钟                 */

#define WIZ_INT_PIN                      GPIO_PIN_4									  /* 定义W5500的INT管脚           */
#define WIZ_INT_PORT                     GPIOC									  	  /* GPIO端口                     */
#define WIZ_INT_CLK_ENABLE()             __HAL_RCC_GPIOC_CLK_ENABLE() /* GPIO端口时钟                 */

#define WIZ_RESET_PIN                   GPIO_PIN_0									  /* 定义W5500的RESET管脚           */
//#define WIZ_RESET_PORT                  GPIOF									  	    /* GPIO端口                     */
#define WIZ_RESET_CLK_ENABLE()           __HAL_RCC_GPIOB_CLK_ENABLE() /* GPIO端口时钟                 */



/*定义SPI1作为W5500的硬件接口*/
#define WIZ_SPIx_GPIO_PORT      GPIOB									  	  /* GPIO端口                     */
#define WIZ_SPIx_GPIO_CLK       RCC_APB2Periph_GPIOB	  	  /* GPIO端口时钟                 */
#define WIZ_SPIx                SPI1                   	    /* 定义W5500所用的SPI接口       */
#define WIZ_SPIx_CLK_CMD        RCC_APB2PeriphClockCmd
#define WIZ_SPIx_CLK            RCC_APB2Periph_SPI1    	    /* 定义W5500所用的SPI接口时钟   */
#define WIZ_SPIx_SCLK           GPIO_PIN_3						   	  /* 定义W5500的时钟管脚          */
#define WIZ_SPIx_MISO           GPIO_PIN_4						   	  /* 定义W5500的MISO管脚          */
#define WIZ_SPIx_MOSI           GPIO_PIN_5						   	  /* 定义W5500的MOSI管脚          */

#define WIZ_SPIx_SCS            GPIO_PIN_5						   	  /* 定义W5500的片选管脚          */
#define WIZ_SPIx_SCS_PORT       GPIOC									   	  /* GPIO端口                     */
#define WIZ_SPIx_SCS_CLK        RCC_APB2Periph_GPIOC	   	  /* GPIO端口时钟                 */

#define WIZ_INT                 GPIO_PIN_4									/* 定义W5500的INT管脚           */
#define WIZ_SPIx_INT_PORT       GPIOC									  	  /* GPIO端口                     */
#define WIZ_SPIx_INT_CLK        RCC_APB2Periph_GPIOC		    /* GPIO端口时钟                 */

#define WIZ_RESET               GPIO_PIN_0									/* 定义W5500的RESET管脚         */
#define WIZ_RESET_PORT          GPIOB									  	  /* GPIO端口                     */
#define WIZ_RESET_CLK           RCC_APB2Periph_GPIOB		    /* GPIO端口时钟                 */

#undef WIZ_SPIx_SCS_PORT
#define WIZ_SPIx_SCS_PORT SPI1_CS_GPIO_Port
#undef WIZ_SPIx_SCS
#define WIZ_SPIx_SCS SPI1_CS_Pin
#undef WIZ_RESET_PORT
#define WIZ_RESET_PORT ETH_RESET_GPIO_Port
#undef WIZ_RESET
#define WIZ_RESET ETH_RESET_Pin
#define FW_VER_HIGH  						1
#define FW_VER_LOW    					0
#define ON	                 		1
#define OFF	                 		0
#define HIGH	           	 			1
#define LOW		             			0

#define MAX_BUF_SIZE		 				1460       			            /*定义每个数据包的大小*/
#define KEEP_ALIVE_TIME	     		30	// 30sec
#define TX_RX_MAX_BUF_SIZE      2048
//#define EEPROM_MSG_LEN        	sizeof(EEPROM_MSG)

#define IP_FROM_DEFINE	        0       			              /*使用初始定义的IP信息*/
#define IP_FROM_DHCP	          1       			              /*使用DHCP获取IP信息*/
#define IP_FROM_EEPROM	        2       			              /*使用EEPROM定义的IP信息*/
extern u8	ip_from;
#pragma pack(1)
/*此结构体定义了W5500可供配置的主要参数*/
typedef struct _CONFIG_MSG
{
  u8 mac[6];																							/*MAC地址*/
  u8 lip[4];																							/*local IP本地IP地址*/
  u8 sub[4];																							/*子网掩码*/
  u8 gw[4];																							/*网关*/
  u8 dns[4];																							/*DNS服务器地址*/
  u8 rip[4];																							/*remote IP远程IP地址*/
  u8 sw_ver[2];																					/*软件版本号*/

}CONFIG_MSG;
#pragma pack()

//#pragma pack(1)
///*此结构体定义了eeprom写入的几个变量，可按需修改*/
//typedef struct _EEPROM_MSG
//{
//	u8 mac[6];																							/*MAC地址*/
//  u8 lip[4];																							/*local IP本地IP地址*/
//  u8 sub[4];																							/*子网掩码*/
//  u8 gw[4];																							/*网关*/
//}EEPROM_MSG_STR;
//#pragma pack()

//extern EEPROM_MSG_STR EEPROM_MSG;
extern CONFIG_MSG  	ConfigMsg;
extern u8 dhcp_ok;																				/*DHCP获取成功*/
extern uint32	dhcp_time;																		/*DHCP运行计数*/
extern vu8	ntptimer;																				/*NPT秒计数*/


/*MCU配置相关函数*/
//void gpio_for_w5500_config(void);														/*SPI接口reset 及中断引脚*/
void timer2_init(void);																			/*STM32定时器2初始化*/
//void timer2_isr(void);																			/*定时器中断执行的函数*/
void timer4_isr(void);
void reboot(void);																					/*STM32软复位*/
void write_config_to_eeprom(void);													/*写配置信息到EEPROM中*/
void read_config_from_eeprom(void);													/*从EEPROM中读出信息*/

/*W5500SPI相关函数*/
//void IINCHIP_WRITE( uint32 addrbsb,  u8 data);						/*写入一个8位数据到W5500*/
//u8 IINCHIP_READ(uint32 addrbsb);													/*从W5500读出一个8位数据*/
u16 wiz_write_buf(uint32 addrbsb,u8* buf,u16 len);	/*向W5500写入len字节数据*/
u16 wiz_read_buf(uint32 addrbsb, u8* buf,u16 len);	/*从W5500读出len字节数据*/

/*W5500基本配置相关函数*/
void reset_w5500(void);																			/*硬复位W5500*/
void set_w5500_mac(void);																		/*配置W5500的MAC地址*/
void set_w5500_ip(void);																		/*配置W5500的IP地址*/
void W5500_Init(void);

/*需要用定时的的应用函数*/
void dhcp_timer_init(void);																	/*dhcp用到的定时器初始化*/
void ntp_timer_init(void);																	/*npt用到的定时器初始化*/

/**brief Subnet mask Register address*/
#define SUBR0                       (0x000500)
#define SUBR1                       (0x000600)
#define SUBR2                       (0x000700)
#define SUBR3                       (0x000800)

/**@brief Source IP Register address*/
#define SIPR0                       (0x000F00)
#define SIPR1                       (0x001000)
#define SIPR2                       (0x001100)
#define SIPR3                       (0x001200)

void gpio_for_w5500_config(void);

#endif /* W5500_CONF_H_ */
