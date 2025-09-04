/**
******************************************************************************
* @file   		udp_demo.c
* @author  		WIZnet Software Team 
* @version 		V1.0
* @date    		2015-02-14
* @brief   		UDP��ʾ����
******************************************************************************
**/
#include <stdio.h>
#include <string.h>
#include "w5500_conf.h"
#include "w5500.h"
#include "socket.h"
#include "utility.h"
#include "udp_demo.h"
//#include "SysTick.h"
//#include "SEGGER_RTT.h"
#include "board_config.h"
//#include "MFRC522.h"
//#include "out_driver.h"
u8 gUDPRevBuf[512];       // ������ʱ�Ĵ�����ֻ����ʱ��

stcUDPBuf stcUDP_RcvTemp;

stcRcvUDPCyBuf UDPRcvCyBufAPP; //Ӧ��UDP���ջ��λ���������Ҫ��

#define G_STATUS_IDLE  0x0000	//����̬
#define G_UDP_DEAL     0xF001 //UDP���ݴ����� 
#define G_UDP_WAIT     0xF002 //UDP���ݵȴ�
#define G_UDP_RtnCMD   0xF003 //UDP���ݴ�����ϣ�����������
#define G_GUI_DEAL     0xF004 //GUI���ݴ�����
#define G_GUI_WAIT     0xF005 //GUI���ݵȴ�
#define G_TIMING_CHECK 0xF006

u16 G_NOW_STATUS = G_STATUS_IDLE;//���嵱ǰ״̬
u16 G_OLD_STATUS = G_STATUS_IDLE;//����ɵ�״̬
u16 G_NEW_STATUS_0 = G_STATUS_IDLE;//�����µ�״̬  ֻ�е�ǰ̬Ϊ����ʱ��ִ��
u16 G_NEW_STATUS_1 = G_STATUS_IDLE;//�����µ�״̬  ֻ�е�ǰ̬Ϊ����ʱ��ִ��

u8 RcvUDPIndex;//�����������

//�汾��
u8 version[20] = {2,3,2,3,20,7,20,25};

//Ҫ���͵�Ŀ���ַ�Ͷ˿�
u8 DST_macaddr[6];
u8 DST_ipaddr[4];
u16 DST_port;

u16 UDP_count = 0;
u8 UDP_count_En = 0;

/******************************************
* ���ƣ�CRC_Check()
* ���ܣ����У�� ��BCCУ��
*******************************************/
u8 CRC_Check(u8 *Buf,u16 Length)
{
   u16 temp1=0;
   u8 temp2 = 0;
   for(temp1=0;temp1<Length;temp1++)
   {
      temp2 ^= Buf[temp1];
   }
   return temp2;
}

/*
***********************************************************************************************************
**����ԭ��		:  	void	ReadUDPRxBuf(stcRcvUDPCyBuf *RcvCyBuf)
**����˵��		:
**					RcvCyBuf-->>	Ŀ�껷�λ�����ָ��
**����ֵ		:		�ɹ�����0��ʧ�ܷ���1
**˵	��		:	RcvCyBufָ��Buf������ָ�룬�ı䴫��
**ʾ����ReadUDPRxBuf(&UDPRcvCyBufAPP,&stcUDP_RcvTemp);
**���RcvCyBuf�����stcUDPBuf�ṹ������ĳ��Ⱥ�����
************************************************************************************************************/
u32	ReadUDPRxBuf(stcRcvUDPCyBuf *RcvCyBuf,stcUDPBuf *Buf)//unsigned short LengthTemp)
{
	u32	PointTemp = 0;
	u32	Statue = 0;		//����һ������ֵ
	u16 temp1;				
	u16 LengthTemp;		//����
	PointTemp = RcvCyBuf->WritePoint;
	if((0 == RcvCyBuf->FullFlag))//�����������������
	{
	    //�˴�д��UDP�������� 
	    LengthTemp = Buf->length;  
	    RcvCyBuf->RcvBuf[PointTemp].length = LengthTemp;//д�����ݳ���
	    for(temp1=0;temp1<LengthTemp;temp1++)
	    {
	     RcvCyBuf->RcvBuf[PointTemp].udpdata[temp1] = Buf->udpdata[temp1];//д������
	    }
		 
	    //RcvCyBuf->RcvBuf[][USE_UDP_DATA_BUF_SIZE];
		if((++RcvCyBuf->WritePoint) >= USE_UDP_cycRCV_BUF_SIZE)
		{
			RcvCyBuf->WritePoint=0;
		}
		if(RcvCyBuf->WritePoint == RcvCyBuf->ReadPoint)
		{
			RcvCyBuf->FullFlag =1;	 //��ʾ����������
		}
//		SEGGER_RTT_printf(0,"ReadUDPRxBuf_RcvCyBuf->FullFlag = %d\n",RcvCyBuf->FullFlag);//�ı伴�ı�UDPRcvCyBufAPP.FullFlag
	}
	else
	{
		Statue = 1;	 
	}
	//RelCanRecBuf(CanNum);//�ͷŻ�����
	return Statue;
}


/******************************************************************************************* 
**����ԭ��  :  uint32  ReadUDPRcvCyBuf(stcUDPBuf *Buf,uint8 Mode)    
**����˵��  : 
    Buf   �����λ������н��յ�1֡���ݷŵ�Buf��
    Mode  ��ModeΪFALSEʱΪԤȡģʽ  ��ʹ�õ�������ɾ������ 
**����ֵ   :  ����0 Buf������Ч
**     �� 0   Buf������Ч
**˵ ��   :  �ú�������ʹ�û��λ�������1֡���õ����� 
**ʾ����i = ReadUDPRcvCyBuf(&TempUDP,DISABLE);//Ԥȡ���λ�����������
**����UDPRcvCyBufAPP
**���θı����
******************************************************************************************/ 
u32  ReadUDPRcvCyBuf(stcUDPBuf *Buf,u8 Mode) 
{
 u32 status=1;
 u16 i; 
   
 if((0 !=  UDPRcvCyBufAPP.FullFlag) || ( UDPRcvCyBufAPP.ReadPoint !=  UDPRcvCyBufAPP.WritePoint))  //��������������д��ָ�벻���ڶ�ָ��  
   {
    *Buf = UDPRcvCyBufAPP.RcvBuf[UDPRcvCyBufAPP.ReadPoint];  //Bufָ���ָ���ʱָ�������
     if(Mode == ENABLE)//��ModeΪ DISABLE ʱΪԤȡģʽ  ��ʹ�õ�������ɾ������
     {
         //ʹ����Ϻ�ִ����յ�ǰ��¼
         for(i=0;i<USE_UDP_DATA_BUF_SIZE;i++)//��ʾȫ����գ�����������ָ�ĳ��ȶ����
           UDPRcvCyBufAPP.RcvBuf[UDPRcvCyBufAPP.ReadPoint].udpdata[i]=0;         
         if((++ UDPRcvCyBufAPP.ReadPoint) >= USE_UDP_cycRCV_BUF_SIZE)   
         {
           UDPRcvCyBufAPP.ReadPoint =0; 
         } 
           UDPRcvCyBufAPP.FullFlag=0; //����־λ��0
     }
   }
   else
   {
    status=0;       //��Ч
   }
   return status; 
}


//u8 buff_udp[2048];  
void UDP_DATA_CHECK(void)
{
	u8 i;
	u16 len=0;
//	u8 buff_udp[2048];                                                          /*����һ��2KB�Ļ���*/	
	u8 buff_udp[512];
	u8 DataCRC = 0;
	
	switch(getSn_SR(SOCK_UDPS))                                                /*��ȡsocket��״̬*/
	{
		case SOCK_CLOSED:                                                        /*socket���ڹر�״̬*/
		{
			SEGGER_RTT_printf(0,"init socket %d\n",local_port);//����ͨ��		 ��ʼ��socket ��������һֱ����
			socket(SOCK_UDPS,Sn_MR_UDP,local_port,0);                              /*��ʼ��socket*/
		}break;
		
		case SOCK_UDP:																				/*socket��ʼ�����*/
		{
//			SEGGER_RTT_printf(0,"do_udp 1\n");		
//			delay_ms(10);
			if(getSn_IR(SOCK_UDPS) & Sn_IR_RECV)
			{
				setSn_IR(SOCK_UDPS, Sn_IR_RECV);                                     /*������ж�*/
//				SEGGER_RTT_printf(0,"getSn_IR\n");//����ͨ��
			}
			if((len = getSn_RX_RSR(SOCK_UDPS))>0)                                    /*���յ�����*/
			{
//				SEGGER_RTT_printf(0,"do_udp 2\n");		
//				SEGGER_RTT_printf(0,"len-8 = %d\n",len-8);
				
				recvfrom(SOCK_UDPS,buff_udp, len, remote_ip,&remote_port);               /*W5500���ռ����������������*/
				
				//��ӡԶ�̵�IP�Ͷ˿�
				#if 0
				for(i = 0;i < 4;i++)
				{
					SEGGER_RTT_printf(0,"remote_ip[%d] = %d\n",i,remote_ip[i]);
				}
				SEGGER_RTT_printf(0,"remote_port = %d\n",remote_port);
				#endif
				
//				buff_udp[len-8]=0x00;                                                    /*����ַ���������*/
//				SEGGER_RTT_printf(0,"%s\r\n",buff_udp);                                               /*��ӡ���ջ���*/ 
				
				#if 0
				for(i = 0;i < len-8;i++)	//ʵ�ʳ���Ϊlen-8
				{
					SEGGER_RTT_printf(0,"%x",buff_udp[i]);                                               /*��ӡ���ջ���*/ 
				}
				SEGGER_RTT_printf(0,"\n");                                               /*��ӡ���ջ���*/ 
				#endif
				
//				sendto(SOCK_UDPS,buff_udp,len-8, remote_ip, remote_port);                /*W5500�ѽ��յ������ݷ��͸�Remote*/

				if(len-8 > 512)	//������
				{
					len = 512+8;
					SEGGER_RTT_printf(0,"len = 512+8\n");   
				}
				
				for(i=0;i<len-8;i++)
				{
					gUDPRevBuf[i] = buff_udp[i];
					buff_udp[i] = 0;
				}
				if(gUDPRevBuf[0] == 0xDF)//������ɣ�0xDF��ʾUDP����֡֡ͷ
				{
//					SEGGER_RTT_printf(0,"����ȷ��UDP����֡\n");
					stcUDP_RcvTemp.length = gUDPRevBuf[3]*256+gUDPRevBuf[4]+6;//UDP����֡����������
					for(i=0;i<stcUDP_RcvTemp.length;i++)
					{
						stcUDP_RcvTemp.udpdata[i] = gUDPRevBuf[i];//�ѻ�������䵽�ṹ���UDP����֡
						gUDPRevBuf[i] = 0;//�ǵ������ʱ������
						if(i==0)
						 {
							DataCRC = 0;//��λCRC
						 }else
						 {
							DataCRC ^= stcUDP_RcvTemp.udpdata[i]; //CRCУ��
						 }
					}
					if(0 == DataCRC)//CRCУ����ȷ
					{
//						SEGGER_RTT_printf(0,"CRCУ����ȷ\n");
						
						UDPRcvCyBufAPP.FullFlag = 0;
						ReadUDPRxBuf(&UDPRcvCyBufAPP,&stcUDP_RcvTemp);//ȫ�ֱ��浽�ṹ��UDPRcvCyBufAPP
					}
				}
			}
		}break;
	}
	
	 /*��SOCKET��ӦIP�������ߣ���λ����Ӧ�˿�Ϊ9988�������˿�Ϊ9999*/		/*���ɸ��Ķ˿�*/
	switch(getSn_SR(SOCK_UDPS_setIP))                                                /*��ȡsocket��״̬*/
	{
		case SOCK_CLOSED:                                                        /*socket���ڹر�״̬*/
		{
			SEGGER_RTT_printf(0,"init socket %d\n",local_port_setIP);//����ͨ��		 ��ʼ��socket
			socket(SOCK_UDPS_setIP,Sn_MR_UDP,local_port_setIP,0);                              /*��ʼ��socket*/
		}break;
		
		case SOCK_UDP:																				/*socket��ʼ�����*/
		{
			if(getSn_IR(SOCK_UDPS_setIP) & Sn_IR_RECV)
			{
				setSn_IR(SOCK_UDPS_setIP, Sn_IR_RECV);                                     /*������ж�*/
			}
			if((len = getSn_RX_RSR(SOCK_UDPS_setIP))>0)                                    /*���յ�����*/
			{
				recvfrom(SOCK_UDPS_setIP,buff_udp, len, remote_ip,&remote_port);               /*W5500���ռ����������������*/
				
				//��ӡԶ�̵�IP�Ͷ˿�
				#if 0
				for(i = 0;i < 4;i++)
				{
					SEGGER_RTT_printf(0,"remote_ip_set[%d] = %d\n",i,remote_ip[i]);
				}
				SEGGER_RTT_printf(0,"remote_port_set = %d\n",remote_port);
				#endif
				
//				buff_udp[len-8]=0x00;                                                    /*����ַ���������*/
//				SEGGER_RTT_printf(0,"%s\r\n",buff_udp);                                               /*��ӡ���ջ���*/ 
				
				#if 0
				for(i = 0;i < len-8;i++)	//ʵ�ʳ���Ϊlen-8
				{
					SEGGER_RTT_printf(0,"%x",buff_udp[i]);                                               /*��ӡ���ջ���*/ 
				}
				SEGGER_RTT_printf(0,"\n");                                               /*��ӡ���ջ���*/ 
				#endif
				
//				sendto(SOCK_UDPS,buff_udp,len-8, remote_ip, remote_port);                /*W5500�ѽ��յ������ݷ��͸�Remote*/

				if(len-8 > 512)	//������
				{
					len = 512+8;
					SEGGER_RTT_printf(0,"len = 512+8\n");   
				}
				
				for(i=0;i<len-8;i++)
				{
					gUDPRevBuf[i] = buff_udp[i];
					buff_udp[i] = 0;
				}
				if(gUDPRevBuf[0] == 0xDF)//������ɣ�0xDF��ʾUDP����֡֡ͷ
				{
//					SEGGER_RTT_printf(0,"����ȷ��UDP����֡\n");
					stcUDP_RcvTemp.length = gUDPRevBuf[3]*256+gUDPRevBuf[4]+6;//UDP����֡����������
					for(i=0;i<stcUDP_RcvTemp.length;i++)
					{
						stcUDP_RcvTemp.udpdata[i] = gUDPRevBuf[i];//�ѻ�������䵽�ṹ���UDP����֡
						gUDPRevBuf[i] = 0;//�ǵ������ʱ������
						if(i==0)
						 {
							DataCRC = 0;//��λCRC
						 }else
						 {
							DataCRC ^= stcUDP_RcvTemp.udpdata[i]; //CRCУ��
						 }
					}
					if(0 == DataCRC)//CRCУ����ȷ
					{
//						SEGGER_RTT_printf(0,"CRCУ����ȷ\n");
						
						UDPRcvCyBufAPP.FullFlag = 0;
						ReadUDPRxBuf(&UDPRcvCyBufAPP,&stcUDP_RcvTemp);//ȫ�ֱ��浽�ṹ��UDPRcvCyBufAPP
					}
				}
				
			}
			
		}break;
	}
}

//������λ��������������
void UDP_RcvDeal(void)
{
	u16 i;
	stcUDPBuf TempUDP;//�¶���ֲ�������
	u8  UDP_Head,UDP_CMDType;
	static u32 num_udp = 0;
	u8 UDP_CRC_send = 0;
	i = ReadUDPRcvCyBuf(&TempUDP,DISABLE);//Ԥȡ���λ�����������
	
	
	if((i == 0)&&(G_NOW_STATUS == G_UDP_DEAL))//���UDP_BUF�������ݣ�����UDP����������
	 {
	    G_NOW_STATUS = G_STATUS_IDLE;//��ǰ̬Ϊ����
	 }
	 else if((i != 0)&&G_NOW_STATUS == G_STATUS_IDLE )//UDP�������ݣ����ߵ�ǰ̬Ϊ����
	 {
	    G_NOW_STATUS = G_UDP_DEAL;  //UDP���ݴ�����
//		 SEGGER_RTT_printf(0,"G_NOW_STATUS = G_UDP_DEAL\n");//F001
	 }
	 if(G_NOW_STATUS == G_UDP_DEAL)//������������
	 {
		 if(0 != ReadUDPRcvCyBuf(&TempUDP,ENABLE))//����ʹ�û��λ�������ȡ�����
		 {
				UDP_Head = TempUDP.udpdata[0];		//֡ͷ
				UDP_CMDType = TempUDP.udpdata[1];	//֡����
			 	RcvUDPIndex = TempUDP.udpdata[2]; //����
			 
			 if(UDP_Head == UDP_DATA_HEAD)//Check Head ������0xDF
			 {
				 //����
				 UDP_count_En = 1;
				 UDP_count = 30;	//����3���ʱ
				 
				 switch(UDP_CMDType)
				 {
					 //ָ���ʽ��df 00 00 00 01 00 01
					 case 0x00: //���յ�������
					 {
						 SEGGER_RTT_printf(0,"-recieve heard pack %d-\n",num_udp);
						 num_udp++;
						 if(num_udp >= 65000)
						 {
							 num_udp = 0;
						 }
//						 
						 TempUDP.udpdata[0] = 0xdf;
						 TempUDP.udpdata[1] = 0x01;
						 TempUDP.udpdata[2] = 0x00;
						 TempUDP.udpdata[3] = 0x00;
						 TempUDP.udpdata[4] = 250;	//���ȵ�λ ѹ�������޸� 8
						 TempUDP.length = (TempUDP.udpdata[3]*256 + TempUDP.udpdata[4]);//���ͻ��������ݳ���
						 for(i = 0;i < TempUDP.length;i++)
						 {
							 TempUDP.udpdata[5+i] = version[i];
						 }
//						 TempUDP.udpdata[5] = UDP_disconnect_conut;	//������

						 
						 UDP_CRC_send = CRC_Check(&TempUDP.udpdata[1],TempUDP.length+4);//����CRC
						 TempUDP.udpdata[TempUDP.length+5] = UDP_CRC_send;	//���CRC 
						 sendto(SOCK_UDPS,TempUDP.udpdata,6+TempUDP.length, remote_ip, remote_port);                /*W5500�ѽ��յ������ݷ��͸�Remote*/
						 
					 }break;
					 
					 
					 default: break;
				 }
			 }
		 } 
	 }
}

//������С��255�ֽ�ʹ��
void UDP_SEND_TASK(u8 LX,u8 LEN,u8 *DATA,u8* IP,u16 PORT)
{
	stcUDPBuf TempUDP;//�¶���ֲ�������
	u8 UDP_CRC_send = 0;
	u8 i;
			
	TempUDP.udpdata[0] = 0xdf;
	TempUDP.udpdata[1] = LX;
	TempUDP.udpdata[2] = 0x00;
	TempUDP.udpdata[3] = 0x00;
	TempUDP.udpdata[4] = LEN;
	
	for(i = 0; i < LEN;i++)
	{
		TempUDP.udpdata[5+i]= *(DATA+i);
	}

	UDP_CRC_send = CRC_Check(&TempUDP.udpdata[1],4+LEN);//����CRC
	TempUDP.udpdata[5+LEN] = UDP_CRC_send;	//���CRC
	sendto(SOCK_UDPS,TempUDP.udpdata,6+LEN, IP, PORT);                /*W5500�ѽ��յ������ݷ��͸�Remote*/
}
