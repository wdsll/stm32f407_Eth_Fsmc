/**
  ******************************************************************************
  * �ļ�����: tcp_echoserver.h
  * ��    ��: ӲʯǶ��ʽ�����Ŷ�
  * ��    ��: V1.0
  * ��д����: 2022-11-20
  * ��    ��: tcp_echoserverͷ�ļ�
  ******************************************************************************
  * ˵����
  * ����������Ӳʯstm32������YS-F4STDʹ�á�
  * 
  * �Ա���
  * ��̳��http://www.ing10bbs.com
  * ��Ȩ��ӲʯǶ��ʽ�����Ŷ����У��������á�
  ******************************************************************************
  */
#ifndef __TCP_ECHOSERVER_H__
#define __TCP_ECHOSERVER_H__

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "main.h"
#include "stdio.h"
#include "string.h"
//#include "key/bsp_key.h"
#include "app_ethernet.h"
#include "ethernetif.h"
#include "lwip/timeouts.h"
//#include "key/bsp_key.h"

/* ���Ͷ��� ------------------------------------------------------------------*/
/* TCP����������״̬ */
enum tcp_echoserver_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

/* LwIP�ص�����ʹ�ýṹ�� */
struct tcp_echoserver_struct
{
  u8_t state;             /* ��ǰ����״̬ */
  u8_t retries;
  struct tcp_pcb *pcb;    /* ָ��ǰ��pcb */
  struct pbuf *p;         /* ָ��ǰ���ջ����pbuf */
};

/* �궨�� --------------------------------------------------------------------*/
#define TCP_SERVER_PORT			  1234	//����tcp server�Ķ˿�
#define TCP_SERVER_RX_BUFSIZE	100	  //����tcp server���������ݳ���

/* ��չ���� ------------------------------------------------------------------*/
/* �������� ------------------------------------------------------------------*/
void tcp_echoserver_close(void);
void YSF4_TCP_SENDData(void);


#endif /* __TCP_ECHOSERVER */

/******************* (C) COPYRIGHT 2020-2030 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/

