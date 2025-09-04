#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include "main.h"
#include "sys.h"


#define	MAX_SOCK_NUM		8	 	/**< Maxmium number of socket  */

typedef unsigned long uint32;

#define USE_UDP_DATA_BUF_SIZE   512   //定义UDP缓冲区大小

//区分缓冲区和结构体
//这个结构体包含UDP接收数据长度、UDP数据帧
typedef struct _UDP_BUF_
{
	unsigned char  udpdata[USE_UDP_DATA_BUF_SIZE];	//udp数据帧
  u16 length;		//接收数据长度  不包括UDP帧头
}stcUDPBuf;

#define USE_UDP_cycRCV_BUF_SIZE 10 //定义UDP接收缓冲区大小

//定义UDP接收环形缓冲区结构体，全局保存（重要）
typedef	struct	_RcvUDPDataCycleBuf_
{
	u8		WritePoint	;			//写指针
	u8		ReadPoint	;				//读指针
	u8		FullFlag ;				//缓冲区满标志，注意操作
  stcUDPBuf   RcvBuf[USE_UDP_cycRCV_BUF_SIZE];//结构体里面包含stcUDPBuf结构体
}stcRcvUDPCyBuf,*P_stcRcvUDPCyBuf;

//配置文件
#define UDP_DATA_HEAD    0xDF  //数据发送头字

extern u8 dayin_flag ;

#define blink_or_not 0 //1表示闪烁，0表示不闪烁

typedef struct _TIMER_
{
	unsigned short int count;
	unsigned char arrived;
}stcTIMER;

extern stcTIMER  Temp100ms,Temp500ms,D1s,Temp5ms,D10ms,D20ms,D35ms,D5s,D50ms,D100ms,D500ms,D300ms;


#endif


