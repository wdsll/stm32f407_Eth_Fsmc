#ifndef __UDP_DEMO_H
#define __UDP_DEMO_H

//#include "stm32f10x.h"
#include "main.h"

//#include "bsp/Ethernet/W5500/Types.h"
extern u16 udp_port;/*����UDP��һ���˿ڲ���ʼ��*/

extern u8 gUDPRevBuf[512];       // ������ʱ�Ĵ�����ֻ����ʱ��

extern u16 UDP_count;
extern u8 UDP_count_En;

void UDP_DATA_CHECK(void);
void UDP_RcvDeal(void);
u8 CRC_Check(u8 *Buf,u16 Length);

void UDP_SEND_TASK(u8 LX,u8 LEN,u8 *DATA,u8* IP,u16 PORT);
#endif 


