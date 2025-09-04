/*******************************************************************************
 * LlcUSB.c
 *
 *  Created on: 2022-11-15
 *      Author: Gavin.D
*******************************************************************************/
#include "LlcMain.h"
#include "CH376INC.H"
#include "FILE_SYS.H"
#include <stdio.h>
#include <string.h>

UINT8   ExchgBuf[4];
#define CH376_INT_WIRE  GpioDataRegs.GPADAT.bit.GPIO6
#define Eeprom_size     32768
#define UART_INIT_BAUDRATE  115200
#define UART_WORK_BAUDRATE  115200
#define SBUF  SciaRegs.SCITXBUF
#define TI    SciaRegs.SCICTL2.bit.TXRDY
#define RI    SciaRegs.SCIRXST.bit.RXRDY
Uint16 USBERR = 0;
Uint16 USBON  = 0;
Uint16 HighAdd=0;
Uint16 LowAdd=0;
Uint32 CountStoreAdd = 0;/*0*/
Uint16 EndFlag=0;
Uint16 ErrReFlag=0;
//Uint16 USBCureOver = 0;
Uint16 gDownLoadFlag = 1;//�����Ƿ��л����ߵ�Ҫ�����û��Ϊ0
Uint16 gDownLoadFlag1 = 1;//����USB�������ߵ�Ҫ�����û��Ϊ0
Uint16 gDownLoadFlag2 = 1;//�������EEPROM��Ҫ�����û��Ϊ0
Uint16 gDownLoadFlag3 = 1;///����USB�������ߵ�Ҫ�����û��Ϊ0
Uint16 gDownLoadFlag4 = 1;//�������EEPROM��Ҫ�����û��Ϊ0
Uint16 gDownLoadFlag5 = 1;///����USB�������ߵ�Ҫ�����û��Ϊ0
extern Uint16 ee_ReadByte(Uint8 *_pReadBuf,  Uint16 _usAddress, Uint16 _usSize);
extern Uint16 ee_WriteByte(Uint8 *_pWriteBuf, Uint16 _usAddress, Uint16 _usSize);
extern Uint16 gCurve[21];
extern Uint16 gSelfState;
extern Uint16 Curve_Max;
extern Uint16 Read_Dataiic;
extern Uint16 g_16data[16];
PUINT8 buf[64] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
Uint8  s_bufread1[29]  ={0,0,0,0,0,  0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,   0,0,0,0};
Uint16 s_bufread2[29]  ={0,0,0,0,0,  0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,   0,0,0,0};
PUINT8 s_bufread3[29]  ={0,0,0,0,0,  0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,   0,0,0,0,0,   0,0,0,0};

//struct  CURVE_DATE  Curve_Date;
extern Uint16 USBReadByteS(Uint16 addr, Uint16 length);
static Uint16 USBONCONTER = 0;
static Uint16 USBOFFCONTER = 0;
Uint32 x = 0;
UINT8  s = 0;
Uint16 CounterAddr =0;    //����16λ���ε����ֵ65536��������
Uint16 USBWriteEnd=0;
Uint16 Clear_Eerom = 0;
Uint16 USB_Curve_Flag = 0;
Uint32 j =0;
Uint16 storeagain = 0;//��д��һ��
//UINT32  CH376ReadVar32S( UINT8 var );

void    mDelayuS( UINT8 us )
{
    while ( us -- );
}

void    mDelaymS( UINT8 ms )
{
    while ( ms -- ) {
        mDelayuS( 250 );
        mDelayuS( 250 );
        mDelayuS( 250 );
        mDelayuS( 250 );
    }
}


void    xWriteCH376Cmd( UINT8 mCmd )
{
    TI = 0;
    SBUF = SER_SYNC_CODE1; //���������ĵ�1������ͬ����  0x57 ��ʲô�����57Ҫ����
    while ( TI == 0 );//��SCI�������
    TI = 0;
    SBUF = SER_SYNC_CODE2;  //0xAB���������ĵ�2������ͬ����  �ڶ���Ҫ AB
    while ( TI == 0 );//��SCI�������
    TI = 0;
    SBUF = mCmd;      //���������USB������ŵ�SCI���ͻ�����
    while ( TI == 0 );//��SCI�������
}

void    xWriteCH376Data( UINT8 mData )//��U��д������
{
    TI = 0;
    SBUF = mData;  /* ������� */
    while ( TI == 0 ); //��SCI�������
}

UINT16  xReadCH376Data( void )  /* ��CH376������,��U���ڵ����ݣ�������� */
{
    UINT32  i;
    for ( i = 0; i < 15000; i ++ )
    {
        if ( RI ) {
            RI = 0;
            return( SciaRegs.SCIRXBUF.bit.RXDT );
        }
    }
    return( 0 );  /* ��Ӧ�÷�������� */
}

UINT8   Query376Interrupt( void )
{
    return( CH376_INT_WIRE ? FALSE : TRUE );  /* ���������CH376���ж�������ֱ�Ӳ�ѯ�ж����� */
}
//UINT8   res = 0;

UINT8   mInitCH376Host( void )
{
    UINT8   res;
    xWriteCH376Cmd( CMD11_CHECK_EXIST );  //����ͨѶ�ӿں͹���״̬ 0X 06
    xWriteCH376Data( 0x65 );
    res = xReadCH376Data( );     //��������
    if ( res != 0x9A ) return( ERR_USB_UNKNOWN );  /* ͨѶ�ӿڲ�����,����ԭ����:�ӿ������쳣,�����豸Ӱ��(Ƭѡ��Ψһ),���ڲ�����,һֱ�ڸ�λ,���񲻹��� */
    xWriteCH376Cmd( CMD11_SET_USB_MODE );   //0x15/* ����USB����ģʽ */
    xWriteCH376Data( 0x05 );
    res = xReadCH376Data( );
    if ( res == CMD_RET_SUCCESS ) return( USB_INT_SUCCESS );
    else return( ERR_USB_UNKNOWN );  /* ����ģʽ���� */

}

UINT8 CH376GetIntStatus( void )  /* ��ȡ�ж�״̬��ȡ���ж����� */
{
    UINT8   s;
    xWriteCH376Cmd( CMD01_GET_STATUS );
    s = xReadCH376Data( );
    return( s );
}

UINT8 Wait376Interrupt( void )  /* �ȴ�CH376�ж�(INT#�͵�ƽ)�������ж�״̬��, ��ʱ�򷵻�ERR_USB_UNKNOWN */
{
    UINT32  i;
    for ( i = 0; i < 50000; i ++ ) {  /* ������ֹ��ʱ,Ĭ�ϵĳ�ʱʱ��,�뵥Ƭ����Ƶ�й� */
        if (Query376Interrupt()) return( CH376GetIntStatus());  /* ��⵽�ж� */
/* �ڵȴ�CH376�жϵĹ�����,������Щ��Ҫ��ʱ������������� */
    }
    return( ERR_USB_UNKNOWN );  /* ��Ӧ�÷�������� */
}

UINT8 WriteWait376Interrupt( void )  /* �ȴ�CH376�ж�(INT#�͵�ƽ)�������ж�״̬��, ��ʱ�򷵻�ERR_USB_UNKNOWN */
{
    UINT32  i;
    for ( i = 0; i < 500000; i ++ ) {  /* ������ֹ��ʱ,Ĭ�ϵĳ�ʱʱ��,�뵥Ƭ����Ƶ�й� */
        if ( Query376Interrupt( ) ) return( CH376GetIntStatus( ) );  /* ��⵽�ж� */
/* �ڵȴ�CH376�жϵĹ�����,������Щ��Ҫ��ʱ������������� */
    }
    return( ERR_USB_UNKNOWN );  /* ��Ӧ�÷�������� */
}

void CH376SetFileName( PUINT8 name )  /* ���ý�Ҫ�������ļ����ļ��� */
{
    UINT8   c;
    xWriteCH376Cmd( CMD10_SET_FILE_NAME );
    c = *name;
    xWriteCH376Data( c );
    while ( c ) {
        name ++;
        c = *name;
        if ( c == DEF_SEPAR_CHAR1 || c == DEF_SEPAR_CHAR2 ) c = 0;  /* ǿ�н��ļ�����ֹ */
        xWriteCH376Data( c );
    }
}

void CH376WriteVar32( UINT8 var, UINT32 dat )  /* дCH376оƬ�ڲ���32λ���� */
{
    *(PUINT32)ExchgBuf = dat;
    //ExchgBuf  ;
    xWriteCH376Cmd( CMD50_WRITE_VAR32 );   //д�����ݵ�USB  0X0D
    xWriteCH376Data( var );
    xWriteCH376Data( ExchgBuf[3] );
    xWriteCH376Data( ExchgBuf[2] );
    xWriteCH376Data( ExchgBuf[1] );
    xWriteCH376Data( ExchgBuf[0] );
}

UINT8 CH376SendCmdWaitInt( UINT8 mCmd )  /* �����������,�ȴ��ж� */
{
    xWriteCH376Cmd( mCmd );
    return( Wait376Interrupt( ) );
}

UINT8 CH376FileOpen( PUINT8 name )  /* �ڸ�Ŀ¼���ߵ�ǰĿ¼�´��ļ�����Ŀ¼(�ļ���) */
{
    CH376SetFileName( name );  /* ���ý�Ҫ�������ļ����ļ��� */
    if ( name[0] == DEF_SEPAR_CHAR1 || name[0] == DEF_SEPAR_CHAR2 ) //·�����ķָ��� '\',·�����ķָ��� '/'
    	CH376WriteVar32( VAR_CURRENT_CLUST, 0 );   //дCH376оƬ�ڲ���32λ����
    return( CH376SendCmdWaitInt( CMD0H_FILE_OPEN ) );
}

UINT8 CH376FileCreate( PUINT8 name )  /* �ڸ�Ŀ¼���ߵ�ǰĿ¼���½��ļ�,����ļ��Ѿ�������ô��ɾ�� */
{
    if ( name ) CH376SetFileName( name );  /* ���ý�Ҫ�������ļ����ļ��� */
    return( CH376SendCmdWaitInt( CMD0H_FILE_CREATE ) );
}

UINT8 Wait376InterruptConnect( void )  /* �ȴ�CH376�ж�(INT#�͵�ƽ)�������ж�״̬��, ��ʱ�򷵻�ERR_USB_UNKNOWN */
{
    UINT32  i;
    for ( i = 0; i < 5000; i ++ ) {  /* ������ֹ��ʱ,Ĭ�ϵĳ�ʱʱ��,�뵥Ƭ����Ƶ�й� */
        if ( Query376Interrupt( ) ) return( CH376GetIntStatus( ) );  /* ��⵽�ж� */
/* �ڵȴ�CH376�жϵĹ�����,������Щ��Ҫ��ʱ������������� */
    }
    return( ERR_USB_UNKNOWN );  /* ��Ӧ�÷�������� */
}

UINT8 CH376SendCmdWaitIntConnect( UINT8 mCmd )  /* �����������,�ȴ��ж� */
{
    xWriteCH376Cmd( mCmd );
    return( Wait376InterruptConnect( ) );
}

Uint16 CH376DiskConnect( void )  /* ���U���Ƿ�����,��֧��SD�� */
{
    if ( Query376Interrupt( ) ) CH376GetIntStatus( );  /* ��⵽�ж� */
    return( CH376SendCmdWaitIntConnect( CMD0H_DISK_CONNECT ) );
}

UINT8 CH376DiskMount( void )  /* ��ʼ�����̲����Դ����Ƿ���� */
{
    return( CH376SendCmdWaitInt( CMD0H_DISK_MOUNT ) );
}

UINT8 CH376ByteLocate( UINT32 offset )  /* ���ֽ�Ϊ��λ�ƶ���ǰ�ļ�ָ�� */
{
    xWriteCH376Cmd( CMD4H_BYTE_LOCATE );
    xWriteCH376Data( (UINT8)offset );
    xWriteCH376Data( (UINT8)((UINT16)offset>>8) );
    xWriteCH376Data( (UINT8)(offset>>16) );
    xWriteCH376Data( (UINT8)(offset>>24) );
    return( Wait376Interrupt( ) );
}

UINT8 CH376WriteReqBlock( PUINT8 buf )  /* ���ڲ�ָ��������д����������ݿ�,���س��� */
{
    UINT8 s, l;
    xWriteCH376Cmd( CMD01_WR_REQ_DATA );
    s = l = xReadCH376Data( );  /* ���� */
    if ( l ) {
        do {
            xWriteCH376Data( *buf );
            buf ++;
        } while ( -- l );
    }
    return( s );
}

/*******************************************************************************
* ��  ��  ��      : CH376ReadBlock
* ��      ��      : �ӵ�ǰ�����˵�Ľ��ջ�������ȡ���ݿ�,.
* ��      ��      : PUINT8 buf:
*                   ָ���ⲿ���ջ�����.
* ��      ��      : ���س���.
*******************************************************************************/
UINT8	CH376ReadBlock( PUINT8 buf )
{
	UINT8	s, l;

	xWriteCH376Cmd( CMD01_RD_USB_DATA0 );
	s = l = xReadCH376Data( );  														/* �������ݳ��� */
	if ( l )
	{
		do
		{
			*buf = xReadCH376Data( );
			buf ++;
		} while ( -- l );
	}
//	xEndCH376Cmd( );
	return( s );
}
/*******************************************************************************
* ��  ��  ��      : CH376ByteRead
* ��      ��      : ���ֽ�Ϊ��λ�ӵ�ǰλ�ö�ȡ���ݿ�
* ��      ��      : PUINT8 buf:
*					ָ�����ݻ�����.
*                   UINT16 ReqCount��
*                   �����ȡ���ֽ���.
*                   PUINT16 RealCount:
*                   ʵ�ʶ�ȡ���ֽ���.
* ��      ��      : �ж�״̬.
*******************************************************************************/
UINT8	CH376ByteRead( PUINT8 buf, UINT16 ReqCount, PUINT16 RealCount )
{
	UINT8	s;

	xWriteCH376Cmd( CMD2H_BYTE_READ );        //���ֽ�Ϊ��λ�ӵ�ǰλ�ö�ȡ���ݿ�
	xWriteCH376Data( (UINT8)ReqCount );       //�����ȡ���ֽ���
	xWriteCH376Data( (UINT8)(ReqCount>>8));   //�����ȡ���ֽ���
	//xEndCH376Cmd( );
	if ( RealCount )
	{
	    *RealCount = 0;
	}

	while ( 1 )
	{
		s = Wait376Interrupt( );//12FF56FF9AFFDEFFFFF56789ABCDEF123456789ABCDEF123456789ABCDEF
		if ( s == USB_INT_DISK_READ )              /* �������ݶ��� */
	//  if ( s ==USB_INT_SUCCESS)
		{
			s = CH376ReadBlock( buf );             /* �ӵ�ǰ�����˵�Ľ��ջ�������ȡ���ݿ�,���س��� */
			xWriteCH376Cmd( CMD0H_BYTE_RD_GO );    /* ������ */
//			xEndCH376Cmd( );
			buf += s;                                //����һ��
			if ( RealCount )
			{
			    *RealCount += s;
			}
		}
		else
		{
		    return( s );                                                                /* ���� */
		}
	}
}

UINT8 CH376ByteWrite( PUINT8 buf, UINT16 ReqCount, PUINT16 RealCount )  /* ���ֽ�Ϊ��λ��ǰλ��д�����ݿ� */
{
    UINT8 s;
    xWriteCH376Cmd( CMD2H_BYTE_WRITE );
    xWriteCH376Data( (UINT8)ReqCount );
    xWriteCH376Data( (UINT8)(ReqCount>>8) );
    if (RealCount ) *RealCount = 0;
    while ( 1 )
    {
        s = WriteWait376Interrupt( );
        if ( s == USB_INT_DISK_WRITE )
        {
            s = CH376WriteReqBlock( buf );  /* ���ڲ�ָ��������д����������ݿ�,���س��� */
            xWriteCH376Cmd( CMD0H_BYTE_WR_GO );
            buf += s;
            if ( RealCount ) *RealCount += s;
        }
        else return( s );  /* ���� */
    }
}

UINT32  CH376GetFileSize( void )  /* ��ȡ��ǰ�ļ����� */
{
    return( CH376ReadVar32( VAR_FILE_SIZE ) );
}

UINT8   CH376SendCmdDatWaitInt( UINT8 mCmd, UINT8 mDat )  /* �����������һ�ֽ����ݺ�,�ȴ��ж� */
{
    xWriteCH376Cmd( mCmd );
    xWriteCH376Data( mDat );
    return( Wait376Interrupt( ) );
}

UINT8   CH376FileClose( UINT8 UpdateSz )  /* �رյ�ǰ�Ѿ��򿪵��ļ�����Ŀ¼(�ļ���) */
{
    return( CH376SendCmdDatWaitInt( CMD1H_FILE_CLOSE, UpdateSz ) );
}

UINT32  CH376Read32bitDat( void )  /* ��CH376оƬ��ȡ32λ�����ݲ��������� */
{
    UINT8   c0, c1, c2, c3;
    c0 = xReadCH376Data( );
    c1 = xReadCH376Data( );
    c2 = xReadCH376Data( );
    c3 = xReadCH376Data( );
    return( c0 | (UINT16)c1 << 8 | (UINT32)c2 << 16 | (UINT32)c3 << 24 );
}


UINT32  CH376ReadVar32( UINT8 var )  /* ��CH376оƬ�ڲ���32λ���� */
{
    xWriteCH376Cmd( CMD14_READ_VAR32 );
    xWriteCH376Data( var );
    return( CH376Read32bitDat( ) );  /* ��CH376оƬ��ȡ32λ�����ݲ��������� */
}


void checkaddress(void)
{
    //ServiceDog();
    char str[10]="     ";
    CounterAddr = CH376GetFileSize();
    while((CounterAddr>62000))
    {
        s = sprintf( buf, "%s",str);
        s = CH376ByteWrite( buf, s, NULL );//д���ݵ�U���ļ���
        CounterAddr = CH376GetFileSize();
    }
}



void checkUSB(void)
{
    if(ADRESULT_USBON < 1000)/// && 1 == Read_Dataiic)�Ѳ�USB
    {
        USBONCONTER++;
        if(USBONCONTER >= 500)
        {
            if(USBONCONTER > 600)
            {
                gSelfState = 8;
                USBON = 1;
            }
        }
        USBOFFCONTER = 0;
    }
    else
    {
        USBOFFCONTER ++;
        if(USBOFFCONTER >= 1000)
        {
            USBON = 0;
            USB_Curve_Flag = 0;
            gDownLoadFlag = 1;  //����ز�USB���´ο����ٴλ�����
            gDownLoadFlag1 = 1; //����USB�������ߵ�Ҫ�����û��Ϊ0
            gDownLoadFlag2 = 1; //�������EEPROM��Ҫ�����û��Ϊ0
            gDownLoadFlag3 = 1;///����USB�������ߵ�Ҫ�����û��Ϊ0
            gDownLoadFlag4 = 1;//�������EEPROM��Ҫ�����û��Ϊ0
            gDownLoadFlag5 = 1;///����USB�������ߵ�Ҫ�����û��Ϊ0

            ServiceDog();
            USBWriteEnd = 0;
            if(8 == gSelfState)
                gSelfState = 0;
        }
        USBONCONTER = 0;
    }
    return;
}

Uint16 AS = 0;
Uint16 Timers = 0;

void InitStore(void)
{
    StoreData.Year = 0;
    StoreData.Month = 0;
    StoreData.Day = 0;
    StoreData.Hour = 0;
    StoreData.Minute = 0;
    StoreData.CurveNo = 0;
    StoreData.Event = 0;       //1��������
    StoreData.DurationHour = 0;
    StoreData.DurationMinute =0;
    StoreData.StartVH = 0;
    StoreData.StartVL = 0;
    StoreData.EndVH = 0;
    StoreData.EndVL = 0;
    StoreData.ChargeAHH = 0;
    StoreData.ChargeAHL = 0;
    StoreData.ACVoltage = 0;
}


//USB������
//void ClearEepromData(void)
void USBChangeCurve(void)
   {


	if( gDownLoadFlag == 1)	//���û��USB�����ٴ�ȥ��USB
	{
	  //��ʱ����
		for ( s = 0; s <20; s ++ ) // ��ȴ�ʱ��,10*50mS
		{
		mDelaymS( 50 );
		if ( CH376DiskMount( ) == USB_INT_SUCCESS ) break;  /* ��ʼ�����̲����Դ����Ƿ���� *///����
		}

		for(j = 0; j<30; j++)//�¶�30��
		{
		ServiceDog();
		s = CH376FileOpen( "/B121.TXT" );  // ���ļ�,���ļ��ڸ�Ŀ¼��
		if ( s == USB_INT_SUCCESS )
		{
			 gDownLoadFlag = 1;
			AS =1;
			if(AS !=gCurveNum)
			{
				ServiceDog();
				if(WriteByte(0,AS))//�����ߺ�д��0��ַ��ȥ��
					gCurveNum = AS;
				USB_Curve_Flag = 1;
			}
			break;
		}
	   // else if (USB_INT_SUCCESS == CH376FileOpen( "/2.TXT" ))
		else if (USB_INT_SUCCESS == CH376FileOpen( "/B71.TXT" ))
			{
			 gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =2;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS)) //�����ߺ�д��0��ַ��ȥ��
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/3.TXT" ))
		else if (USB_INT_SUCCESS == CH376FileOpen( "/B156.TXT" ))
		{
			 gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =3;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))//�����ߺ�д��0��ַ��ȥ��
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/4.TXT" ))
		else if (USB_INT_SUCCESS == CH376FileOpen( "/B199.TXT" ))
			{
			 gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =4;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/5.TXT" ))
		else if (USB_INT_SUCCESS == CH376FileOpen( "/B100.TXT" ))
		{
			 gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =5;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS)) //�����ߺ�д��0��ַ��ȥ��
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/6.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B101.TXT" ))
			{
			  gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =6;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
			//else if (USB_INT_SUCCESS == CH376FileOpen( "/7.TXT" ))
			else if (USB_INT_SUCCESS == CH376FileOpen( "/B102.TXT" ))
			{
				 gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =7;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/8.TXT" ))
			else if (USB_INT_SUCCESS == CH376FileOpen( "/B103.TXT" ))
			{
				 gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =8;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/9.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B104.TXT" ))
		  {
			  gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =9;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/10.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B94.TXT" ))
		  {
			  gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =10;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//  else if (USB_INT_SUCCESS == CH376FileOpen( "/11.TXT" ))
			else if (USB_INT_SUCCESS == CH376FileOpen( "/B56.TXT" ))
			{
				 gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =11;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS)) //�����ߺ�д��0��ַ��ȥ��
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
	   // else if (USB_INT_SUCCESS == CH376FileOpen( "/12.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B132.TXT" ))
			{
			  gDownLoadFlag = 1;
				ServiceDog();
				{
					AS =12;
					if(AS != gCurveNum)
					{
						ServiceDog();
						if(WriteByte(0,AS))
							gCurveNum = AS;
						USB_Curve_Flag = 1;
					}
					break;
				}
			}
		//    else if (USB_INT_SUCCESS == CH376FileOpen( "/13.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B129.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =13;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
		//else if (USB_INT_SUCCESS == CH376FileOpen( "/14.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B214.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =14;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
	 //   else if (USB_INT_SUCCESS == CH376FileOpen( "/15.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B11.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =15;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
	   // else if (USB_INT_SUCCESS == CH376FileOpen( "/16.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B202.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =16;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
	   // else if (USB_INT_SUCCESS == CH376FileOpen( "/17.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B105.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =17;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
		//    else if (USB_INT_SUCCESS == CH376FileOpen( "/18.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B106.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =18;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
		   // else if (USB_INT_SUCCESS == CH376FileOpen( "/19.TXT" ))
			else if (USB_INT_SUCCESS == CH376FileOpen( "/B201.TXT" ))
		  {
				 gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =19;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
			 }
	//    else if (USB_INT_SUCCESS == CH376FileOpen( "/20.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B108.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =20;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS))
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
		 }
//    else if (USB_INT_SUCCESS == CH376FileOpen( "/21.TXT" ))
		  else if (USB_INT_SUCCESS == CH376FileOpen( "/B109.TXT" ))
		  {
			  gDownLoadFlag = 1;
				 ServiceDog();
				 {
					 AS =21;
					 if(AS != gCurveNum)
					 {
						 ServiceDog();
						 if(WriteByte(0,AS)) //�����ߺ�д��0��ַ��ȥ��
							 gCurveNum = AS;
						 USB_Curve_Flag = 1;
					 }
					 break;
				 }
		  }
	  else if (USB_INT_SUCCESS == CH376FileOpen( "/B110.TXT" ))
	   {
		  gDownLoadFlag = 1;

			  ServiceDog();
			  {
				  AS =22;
				  if(AS != gCurveNum)
				  {
					  ServiceDog();
					  if(WriteByte(0,AS)) //�����ߺ�д��0��ַ��ȥ��
						  gCurveNum = AS;
					  USB_Curve_Flag = 1;
				  }
				  break;
			  }
	   }
	  else
	  {
		  gDownLoadFlag = 0;
	  }
	   /////////////////////////////
	}//FOR
  }
}



//EEPROM �������
void ClearEepromData(void)
{
	Uint8 s_bufreadClear[3]={0,0,0};
	  if( gDownLoadFlag2 == 1)
	  {

           /////////////////start//EEPROM �������///////////////////
			   for(j = 0; j<5; j++)//5��  Clear
				{
							s = CH376FileOpen( "/B00.TXT" );  // ���ļ�,���ļ��ڸ�Ŀ¼��
							if( s == USB_INT_SUCCESS )
							{
								ee_WriteByte(s_bufreadClear, 1, 3);
								gDownLoadFlag2 = 1;
								Clear_Eerom = 1;
							}
							else
							{
								 gDownLoadFlag2 = 0;
							}
							 ServiceDog();
				 }

	  }

}

/*******************************************************************************
** ��������:    Read_Usb_Data
** ��������:    ��ȡU���У�U109��P109��D109��A109�������ݣ����浽EEPROM��
** ��ʽ����:    ��
** ���ز���:    ��
*******************************************************************************/
//Uint16  s_bufreadTemp[30]  ={0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//Uint16  s_bufreadTemp1[30]  ={0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
Uint16 g_epromAddr = 0;
void Read_Usb_Data(void)
{

	Uint8  s_bufreadTemp[IIC_DATE_SIZE-1]  ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//PUINT8  s_bufreadTemp[30]  ={0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	PUINT8  s_bufread[USBFD_SIZE]  ={0,0,0,0,0, 0,0,0,0,0,0,0,0,0,};
	UINT32 Temp_Data,ddt;
	static Uint16 tempout,temp_data,Addr;
	static Uint16 s_temp,s_svaeNum;
	static Uint16 s_tempflag1 = 0,s_tempflag2 = 0,s_tempflag3 = 0,s_tempflag4 = 0;
	static Uint16 s_tempflago1,s_tempflago2,s_tempflago3,s_tempflago4;
	if((gDownLoadFlag1 == 1)||(gDownLoadFlag3 == 1))
	{

		for(j = 0; j<3; j++)//�¶�30��
		{
		ServiceDog();

		if (USB_INT_SUCCESS == CH376FileOpen( "/U109.TXT" ))      //�ļ����ȶ�
		{
			 gDownLoadFlag1 = 1;

			 Temp_Data = CH376ByteRead(&s_bufread,0x0040, NULL);  //�����ļ��ڵ�ACS��
			 ////�����Ƕ�������д���
			tempout=0;                     //
			for(s_temp = 0; s_temp < USBFD_SIZE; s_temp ++)       //0
			{
				if(s_temp==0)
				{
					ddt= s_bufread[s_temp];
					s_bufreadTemp[tempout]= ddt>>16;
					tempout++;
				}
				else
				{
					s_bufreadTemp[tempout]= (PUINT8)s_bufread[s_temp]; //ACS������ת��
					tempout++;                                //�����ֽ��Լ�
					ddt=s_bufread[s_temp];
					s_bufreadTemp[tempout]= ddt>>16;
					tempout++;
				}

				g_epromAddr = (s_bufreadTemp[2]-48)*78 + s_bufreadTemp[3]-48+32000; //32000�ǵ�ַ��ƫ����
			}
			if(ee_WriteByte(s_bufreadTemp, g_epromAddr, (IIC_DATE_SIZE-2)))//����0����Ϊ�յ����Դ�0��ʼ��
			{
				tempout=0;
			}

		}
		else
		{
			gDownLoadFlag1 = 0;

		}





		if (USB_INT_SUCCESS == CH376FileOpen( "/P109.TXT" ))
		{
			gDownLoadFlag3 = 1;

			 Temp_Data = CH376ByteRead(&s_bufread,0x0040, NULL);
			tempout=0;                     //
			for(s_temp = 0; s_temp < USBFD_SIZE; s_temp ++)       //0
			{
				if(s_temp==0)
				{
					ddt= s_bufread[s_temp];
					s_bufreadTemp[tempout]= ddt>>16;
					tempout++;
				}
				else
				{
					s_bufreadTemp[tempout]= (PUINT8)s_bufread[s_temp]; //ACS������ת��
					tempout++;                                //�����ֽ��Լ�
					ddt=s_bufread[s_temp];
					s_bufreadTemp[tempout]= ddt>>16;
					tempout++;
				}



			}
			if(ee_WriteByte((s_bufreadTemp), (g_epromAddr+IIC_DATE_SIZE-2), (IIC_DATE_SIZE-2)))
			{
				tempout=0;
			}

		}
		else
		{
			gDownLoadFlag3 = 0;

		}

//
//		if (USB_INT_SUCCESS == CH376FileOpen( "/D109.TXT" ))
//		{
//			gDownLoadFlag4 = 1;
//			 Temp_Data = CH376ByteRead(&s_bufread3,0x0040, NULL);
//			 tempout=0;                     //
//			for(s_temp = 0; s_temp < USBFD_SIZE; s_temp ++)       //0
//			{
//
//				s_bufreadTemp[tempout]= (PUINT8)s_bufread[s_temp]; //ACS������ת��
//				temp_data=s_bufreadTemp[tempout];
//				s_svaeNum++;                             //IIC��ַ�Լ�
//				WriteByte((2000+s_svaeNum),temp_data);   //����ż���Ŵ���
//				tempout++;                                //�����ֽ��Լ�
//
//				ddt=s_bufread[s_temp];
//				s_bufreadTemp[tempout]= ddt>>16;
//				ddt = s_bufreadTemp[tempout];
//				s_svaeNum++;                              //IIC��ַ�Լ�
//				WriteByte((2000+s_svaeNum),ddt);         //��������Ŵ���
//			}
//		}
//		else
//		{
//			gDownLoadFlag4 = 0;
//
//		}
//
//		if (USB_INT_SUCCESS == CH376FileOpen( "/A109.TXT" ))
//		{
//			gDownLoadFlag5 = 1;
//
//			 Temp_Data = CH376ByteRead(&s_bufread,0x003B, NULL);
//
//			 tempout=0;                     //
//			for(s_temp = 0; s_temp < USBFD_SIZE; s_temp ++)       //0
//			{
//
//				s_bufreadTemp[tempout]= (PUINT8)s_bufread[s_temp]; //ACS������ת��
//				temp_data=s_bufreadTemp[tempout];
//				s_svaeNum++;                             //IIC��ַ�Լ�
//				WriteByte((2000+s_svaeNum),temp_data);   //����ż���Ŵ���
//				tempout++;                                //�����ֽ��Լ�
//
//				ddt=s_bufread[s_temp];
//				s_bufreadTemp[tempout]= ddt>>16;
//				ddt = s_bufreadTemp[tempout];
//				s_svaeNum++;                              //IIC��ַ�Լ�
//				WriteByte((2000+s_svaeNum),ddt);         //��������Ŵ���
//			}
//		}
//		else
//		{
//				gDownLoadFlag5 = 0;
//
//		}
    }



   }

}
//USB���ݱ�����غ���
void  USBDownLoadData(void)
  {
            DisableDog();
            char strout1[2]="-";
            char stroutL[3]="b";
            char strout2[2]=":";
            char strout3[2]=".";
            char stroutevnts[15]="Start Charging";
            char stroutevntf[15]="Fully Charged";
            char stroutevnte[15];
            char str0[15]=" ";
            char str1[10]="Date";
            char str2[10]="Curve No.";
            char str3[10]="Event";
            char str4[15]="Start Time";
            char str5[15]="Start V";
            char str6[10]="Duration";
            char str7[10]="End V";
            char str8[10]="Charge AH";   //���������ͷXLS��ʾ
            char str9[15]="AC Voltage";  //AC�����ѹ��ͷXLS��ʾ
            Uint16 i =0;
            Uint16 j =0;

            if(ADRESULT_USBON > 1000) return;  //USB�Ѱγ�
            mDelaymS( 100 );
            for ( s = 0; s <20; s ++ ) // ��ȴ�ʱ��,10*50mS
            {
                mDelaymS( 50 );
                if ( CH376DiskMount( ) == USB_INT_SUCCESS ) break;  /* ��ʼ�����̲����Դ����Ƿ���� *///����
            }

            ////////////stop////////////////////
            ServiceDog();
           s = CH376FileCreate( "/MY_DATA.XLS" );//XLS�ļ���
            mDelaymS( 50 );
            s = sprintf( buf, "%s",str0);
            s = CH376ByteWrite( buf, s, NULL );//д���ݵ�U���ļ���
            s = sprintf( buf, "%s\t%s\t%s\t",str1,str2,str3);
            s = CH376ByteWrite( buf, s, NULL );//д���ݵ�U���ļ���
            s = sprintf( buf, "%s\t%s\t%s\t",str4,str5,str6);
            ServiceDog();
            s = CH376ByteWrite( buf, s, NULL );//д���ݵ�U���ļ���
            s = sprintf( buf, "%s\t%s\t%s",str7,str8,str9);
            s = CH376ByteWrite( buf, s, NULL );//д���ݵ�U���ļ���
            ////////////////////////////////////////////

            ServiceDog();
            HighAdd = USBReadByte(1);   //��������EEPROM��һ�����ݵĵ�ַ��
            LowAdd = USBReadByte(2);    //��������EEPROM��һ�����ݵĵ�ַ��
            CountStoreAdd = HighAdd*256 + LowAdd;  //������浽��������ݵĵ�ַ

            storeagain = USBReadByte(3);//��һ�����Ǹ���ַ��д������ȥ


			if(storeagain == 1)                //�Ѿ������PEROMд��1���ˣ�������ӡ��ʽ
			{
				for(i = CountStoreAdd; i <2020; i++) //��EEPREM ��1�����ݵ�ַ��ʼд��USB��
				{
					ServiceDog();
					j = i*16+16;
					StoreData.Event = USBReadByte(j+6);
					switch(StoreData.Event)//���ݲ�ͬ���¼��������п�ʼ���رգ��쳣�رգ���д��USB��
					{
					case 1:
						 ServiceDog();
						StoreData.Year = USBReadByte(j++);
						StoreData.Month = USBReadByte(j++);
						StoreData.Day = USBReadByte(j++);
						StoreData.Hour = USBReadByte(j++);
						StoreData.Minute = USBReadByte(j++);
						//TempData = USBReadByte(j++);
						StoreData.CurveNo = USBReadByte(j++);///gCurve[TempData-1];
						StoreData.Event = USBReadByte(j++);    //1��������
						StoreData.DurationHour = 0;
						j++;
						StoreData.DurationMinute =0;
						j++;
						StoreData.StartVH = USBReadByte(j++);
						StoreData.StartVL = USBReadByte(j++);
						StoreData.EndVH = 0;
						j++;
						StoreData.EndVL = 0;
						j++;
						StoreData.ChargeAHH = 0;
						j++;
						StoreData.ChargeAHL = 0;
						j++;
						StoreData.ACVoltage = USBReadByte(j++);

						s = sprintf(buf,"\n%02d%s%02d%s%02d",StoreData.Year,strout1,StoreData.Month,strout1,StoreData.Day);
						s = CH376ByteWrite( buf, s, NULL );//������
						checkaddress();
						s = sprintf(buf,"\t%s%01d\t%s",stroutL,StoreData.CurveNo,stroutevnts);
						s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
						checkaddress();
						s = sprintf(buf,"\t%02d%s%02d",StoreData.Hour,strout2,StoreData.Minute);
						s = CH376ByteWrite( buf, s, NULL );//��ʼʱ��
						checkaddress();
						s = sprintf(buf,"\t%02d%s%02d",StoreData.StartVH,strout3,StoreData.StartVL);
						s = CH376ByteWrite( buf, s, NULL );//��ʼ��ѹ
						checkaddress();
						s = sprintf(buf,"\t%s\t%s\t%s",strout1,strout1,strout1);
						s = CH376ByteWrite( buf, s, NULL );//����ʱ�������ѹ���AH
						checkaddress();
						s = sprintf(buf,"\t%02d",StoreData.ACVoltage);
						s = CH376ByteWrite( buf, s, NULL );//AC��ѹ
						checkaddress();
						break;
					case 2:
						 ServiceDog();
						StoreData.Year = USBReadByte(j++);
						StoreData.Month = USBReadByte(j++);
						StoreData.Day = USBReadByte(j++);
						StoreData.Hour = USBReadByte(j++);
						StoreData.Minute = USBReadByte(j++);
						/*TempData = USBReadByte(j++);*/
						StoreData.CurveNo = USBReadByte(j++);///gCurve[TempData-1];
						StoreData.Event = USBReadByte(j++);    //1��������
						StoreData.DurationHour = USBReadByte(j++);
						StoreData.DurationMinute =USBReadByte(j++);
						j++;
						StoreData.StartVH = 0;
						j++;
						StoreData.StartVL = 0;
						StoreData.EndVH = USBReadByte(j++);
						StoreData.EndVL = USBReadByte(j++);
						StoreData.ChargeAHH = USBReadByte(j++);
						StoreData.ChargeAHL = USBReadByte(j++);
						StoreData.ACVoltage = USBReadByte(j++);

						s = sprintf(buf,"\n%02d%s%02d%s%02d",StoreData.Year,strout1,StoreData.Month,strout1,StoreData.Day);
						s = CH376ByteWrite( buf, s, NULL );//������
						checkaddress();
						s = sprintf(buf,"\t%s%01d",stroutL,StoreData.CurveNo);
						s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
						checkaddress();
						s = sprintf(buf,"\t%s",stroutevntf);
						s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
						checkaddress();
						s = sprintf(buf,"\t%s\t%s",strout1,strout1);
						s = CH376ByteWrite( buf, s, NULL );//��ʼ��ѹ��ʼʱ��
						checkaddress();
						s = sprintf(buf,"\t%02d%s%02d",StoreData.DurationHour,strout2,StoreData.DurationMinute);
						s = CH376ByteWrite( buf, s, NULL );//����ʱ��
						checkaddress();
						s = sprintf(buf,"\t%02d%s%02d",StoreData.EndVH,strout3,StoreData.EndVL);
						s = CH376ByteWrite( buf, s, NULL );//������ѹ
						checkaddress();
						s = sprintf(buf,"\t%02d%s%02d",StoreData.ChargeAHH,strout3,StoreData.ChargeAHL);
						s = CH376ByteWrite( buf, s, NULL );//���AH
						checkaddress();
						s = sprintf(buf,"\t%02d",StoreData.ACVoltage);
						s = CH376ByteWrite( buf, s, NULL );//AC��ѹ
						checkaddress();
						break;
					case 3:
						 ServiceDog();
						StoreData.Year = USBReadByte(j++);
						StoreData.Month = USBReadByte(j++);
						StoreData.Day = USBReadByte(j++);
						StoreData.Hour = USBReadByte(j++);
						StoreData.Minute = USBReadByte(j++);
						/*TempData = USBReadByte(j++);*/
						StoreData.CurveNo = USBReadByte(j++);///gCurve[TempData-1];
						StoreData.Event = USBReadByte(j++);    //1��������
						StoreData.DurationHour = USBReadByte(j++);
						StoreData.DurationMinute =USBReadByte(j++);
						j++;
						StoreData.StartVH = 0;
						StoreData.StartVL = USBReadByte(j++);
						StoreData.EndVH = USBReadByte(j++);
						StoreData.EndVL = USBReadByte(j++);
						StoreData.ChargeAHH = USBReadByte(j++);
						StoreData.ChargeAHL = USBReadByte(j++);
						StoreData.ACVoltage = USBReadByte(j++);

						s = sprintf(buf,"\n%02d%s%02d%s%02d",StoreData.Year,strout1,StoreData.Month,strout1,StoreData.Day);
						s = CH376ByteWrite( buf, s, NULL );//������
						checkaddress();
						 ServiceDog();
						switch(StoreData.StartVL)
						{
						case 0:
							strcpy(stroutevnte,"ERR0");
							break;
						case 1:
							strcpy(stroutevnte,"ERR1");
							break;
						case 2:
							strcpy(stroutevnte,"ERR2");
							break;
						case 3:
							strcpy(stroutevnte,"ERR3");
							break;
						case 4:
							strcpy(stroutevnte,"ERR4");
							break;
						case 5:
							strcpy(stroutevnte,"ERR5");
							break;
						default:
							strcpy(stroutevnte,"ERR6");
							break;
						}
						s = sprintf(buf,"\t%s%01d\t%s",stroutL,StoreData.CurveNo,stroutevnte);
						s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
						checkaddress();
						s = sprintf(buf,"\t%s\t%s",strout1,strout1);
						s = CH376ByteWrite( buf, s, NULL );//��ʼ��ѹ��ʼʱ��
						checkaddress();
						s = sprintf(buf,"\t%02d%s%02d",StoreData.DurationHour,strout2,StoreData.DurationMinute);
						s = CH376ByteWrite( buf, s, NULL );//����ʱ��
						checkaddress();
						s = sprintf(buf,"\t%02d%s%02d",StoreData.EndVH,strout3,StoreData.EndVL);
						s = CH376ByteWrite( buf, s, NULL );//������ѹ
						checkaddress();
							s = sprintf(buf,"\t%02d%s%02d",StoreData.ChargeAHH,strout3,StoreData.ChargeAHL);
							s = CH376ByteWrite( buf, s, NULL );//���AH
							checkaddress();
							s = sprintf(buf,"\t%02d",StoreData.ACVoltage);
							s = CH376ByteWrite( buf, s, NULL );//AC��ѹ
							checkaddress();
							break;
						default:
						break;
					}
				}
			}

			for(i=0; i<CountStoreAdd; i++)/*i = 0 should is i = 16*/  //û�д�������һ�����ݵĵ�0����ʼ���浽U��
			{
				ServiceDog();
				j = i*16+16;
				USBReadByteS(j,16);
				StoreData.Event =  g_16data[6];
			switch(StoreData.Event)
			{
			case 1:
				 ServiceDog();
				StoreData.Year =    g_16data[0];
				StoreData.Month =   g_16data[1];
				StoreData.Day =     g_16data[2];
				StoreData.Hour =    g_16data[3];
				StoreData.Minute =  g_16data[4];
				StoreData.CurveNo = g_16data[5];
				StoreData.Event =   g_16data[6];    //1��������
				StoreData.DurationHour = 0;

				StoreData.DurationMinute =0;

				StoreData.StartVH = g_16data[9];
				StoreData.StartVL = g_16data[10];
				StoreData.EndVH = 0;

				StoreData.EndVL = 0;

				StoreData.ChargeAHH = 0;

				StoreData.ChargeAHL = 0;

				StoreData.ACVoltage = g_16data[15];

				s = sprintf(buf,"\n%02d%s%02d%s%02d",StoreData.Year,strout1,StoreData.Month,strout1,StoreData.Day);
				s = CH376ByteWrite( buf, s, NULL );//������
				checkaddress();
				s = sprintf(buf,"\t%s%01d\t%s",stroutL,StoreData.CurveNo,stroutevnts);
				s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.Hour,strout2,StoreData.Minute);
				s = CH376ByteWrite( buf, s, NULL );//��ʼʱ��
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.StartVH,strout3,StoreData.StartVL);
				s = CH376ByteWrite( buf, s, NULL );//��ʼ��ѹ
				checkaddress();
				s = sprintf(buf,"\t%s\t%s\t%s",strout1,strout1,strout1);
				s = CH376ByteWrite( buf, s, NULL );//����ʱ�������ѹ���AH
				checkaddress();
				s = sprintf(buf,"\t%02d",StoreData.ACVoltage);
				s = CH376ByteWrite( buf, s, NULL );//AC��ѹ
				checkaddress();
				break;
			case 2:
				 ServiceDog();
				//USBReadByteS(j,16);
				StoreData.Year = g_16data[0];
				StoreData.Month =g_16data[1];
				StoreData.Day = g_16data[2];
				StoreData.Hour = g_16data[3];
				StoreData.Minute = g_16data[4];
				StoreData.CurveNo = g_16data[5];
				StoreData.Event = g_16data[6];    //1��������
				StoreData.DurationHour = g_16data[7];
				StoreData.DurationMinute =g_16data[8];

				StoreData.StartVH = 0;

				StoreData.StartVL = 0;
				StoreData.EndVH = g_16data[11];
				StoreData.EndVL = g_16data[12];
				StoreData.ChargeAHH = g_16data[13];
				StoreData.ChargeAHL = g_16data[14];
				StoreData.ACVoltage = g_16data[15];

				s = sprintf(buf,"\n%02d%s%02d%s%02d",StoreData.Year,strout1,StoreData.Month,strout1,StoreData.Day);
				s = CH376ByteWrite( buf, s, NULL );//������
				checkaddress();
				s = sprintf(buf,"\t%s%01d",stroutL,StoreData.CurveNo);
				s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
				checkaddress();
				s = sprintf(buf,"\t%s",stroutevntf);
				s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
				checkaddress();
				s = sprintf(buf,"\t%s\t%s",strout1,strout1);
				s = CH376ByteWrite( buf, s, NULL );//��ʼ��ѹ��ʼʱ��
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.DurationHour,strout2,StoreData.DurationMinute);
				s = CH376ByteWrite( buf, s, NULL );//����ʱ��
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.EndVH,strout3,StoreData.EndVL);
				s = CH376ByteWrite( buf, s, NULL );//������ѹ
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.ChargeAHH,strout3,StoreData.ChargeAHL);
				s = CH376ByteWrite( buf, s, NULL );//���AH
				checkaddress();
				s = sprintf(buf,"\t%02d",StoreData.ACVoltage);
				s = CH376ByteWrite( buf, s, NULL );//AC��ѹ
				checkaddress();
				break;
			case 3:
				 ServiceDog();
			//	USBReadByteS(j,16);
				StoreData.Year = g_16data[0];
				StoreData.Month = g_16data[1];
				StoreData.Day = g_16data[2];
				StoreData.Hour = g_16data[3];
				StoreData.Minute = g_16data[4];
				StoreData.CurveNo = g_16data[5];
				StoreData.Event = g_16data[6];    //1��������
				StoreData.DurationHour = g_16data[7];
				StoreData.DurationMinute =g_16data[8];

				StoreData.StartVH = 0;
				StoreData.StartVL = g_16data[10];
				StoreData.EndVH = g_16data[11];
				StoreData.EndVL = g_16data[12];
				StoreData.ChargeAHH = g_16data[13];
				StoreData.ChargeAHL = g_16data[14];
				StoreData.ACVoltage = g_16data[15];

				s = sprintf(buf,"\n%02d%s%02d%s%02d",StoreData.Year,strout1,StoreData.Month,strout1,StoreData.Day);
				s = CH376ByteWrite( buf, s, NULL );//������
				checkaddress();
				switch(StoreData.StartVL)
				{
				case 0:
					strcpy(stroutevnte,"ERR0");
					break;
				case 1:
					strcpy(stroutevnte,"ERR1");
					break;
				case 2:
					strcpy(stroutevnte,"ERR2");
					break;
				case 3:
					strcpy(stroutevnte,"ERR3");
					break;
				case 4:
					strcpy(stroutevnte,"ERR4");
					break;
				case 5:
					strcpy(stroutevnte,"ERR5");
					break;
				case 6:
					strcpy(stroutevnte,"ERR6");
					break;
				case 7:
					strcpy(stroutevnte,"ERR7");
					break;
				case 8:
					strcpy(stroutevnte,"ERR8");
					break;
				case 9:
					strcpy(stroutevnte,"ERR9");
					break;
				case 10:
					strcpy(stroutevnte,"ERR10");
					break;
				case 11:
					strcpy(stroutevnte,"ERR11");
					break;
				case 12:
					strcpy(stroutevnte,"ERR12");
					break;
				case 13:
					strcpy(stroutevnte,"ERR13");
					break;
				case 14:
					strcpy(stroutevnte,"ERR14");
					break;
				default:
					strcpy(stroutevnte,"ERR");
					break;
				}
				s = sprintf(buf,"\t%s%01d\t%s",stroutL,StoreData.CurveNo,stroutevnte);
				s = CH376ByteWrite( buf, s, NULL );//���߱�ż��¼�
				checkaddress();
				s = sprintf(buf,"\t%s\t%s",strout1,strout1);
				s = CH376ByteWrite( buf, s, NULL );//��ʼ��ѹ��ʼʱ��
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.DurationHour,strout2,StoreData.DurationMinute);
				s = CH376ByteWrite( buf, s, NULL );//����ʱ��
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.EndVH,strout3,StoreData.EndVL);
				s = CH376ByteWrite( buf, s, NULL );//������ѹ
				checkaddress();
				s = sprintf(buf,"\t%02d%s%02d",StoreData.ChargeAHH,strout3,StoreData.ChargeAHL);
				s = CH376ByteWrite( buf, s, NULL );//���AH
				checkaddress();
					s = sprintf(buf,"\t%02d",StoreData.ACVoltage);
					s = CH376ByteWrite( buf, s, NULL );//AC��ѹ
					checkaddress();
					break;
				default:
					break;
				}
			}

}
//USB�������
void USBFinish(void)
		{
			ServiceDog();
			s = CH376FileClose( TRUE );
			USBWriteEnd = 1;
			USB_Curve_Flag =1;
			ServiceDog();     // Reset the watchdog counter
			EALLOW;
			SysCtrlRegs.WDCR = 0x002F;
			EDIS;
			GoToUSBChargeOn();
//			USBCureOver = 1;
			if(1 == USB_Curve_Flag)
			{
				//StopCpuTimer1();/*������ʱ��1 �ж�*/
				gSelfState = 8;
				USB_Curve_Flag = 0;
			   // IER &= 0xEFFF;
				///IER |= M_INT13;
			}
		}



void USBManage(void)
{
  //  Uint16 TempData = 0;
    /*if((USBON)&&(eLlcSysCmdOff == gstSetLlcPARAM.llcOnCommand||eLlcSysCmdShutDown==gstSetLlcPARAM.llcOnCommand))*/
    if((USBON)&&(USBWriteEnd==0))//�Ѳ�U��
    {
    	 ServiceDog();
        if( GoToUSBShutDown())//��LLC�ص�
        {
        	 ClearEepromData(); //EEPROM �������
        	 USBChangeCurve();  //USB������
        	 Read_Usb_Data();   //��ȡU����������

        	if((gDownLoadFlag2 == 0)&&(gDownLoadFlag1 == 0)&&(gDownLoadFlag == 0)&&
        	   (gDownLoadFlag3 == 0))//USB��û���ļ��Ž�������
			{
        	 USBDownLoadData();//EEPROM���������ص�USB��
        	 USBFinish();//usb�������
			}
        	else
        	{
        	USBFinish();//usb�������
		    }
        }
    }
}

Uint16 USBInit(void)
{
    Uint16 USBInitSUCC = 0;
    Uint16 USBInitCount = 0;
    while(USBInitCount <= 10)
    {
        USBInitSUCC = mInitCH376Host();
        if(USBInitSUCC != 20)
        {
            SciInit();
        }
        else
        {
            USBInitCount = 10;
        }
        USBInitCount++;
    }
    return(USBInitSUCC);
}
