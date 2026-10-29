/*© [2025] Microchip Technology Inc. and its subsidiaries
 
Subject to your compliance with the terms and conditions of the reference design license agreement accompanying this software, 
you may use this Microchip software and any derivatives exclusively with Microchip products. You are responsible for complying with 
third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software. 
SOFTWARE IS ?AS IS.? NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, 
MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE 
DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, 
IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE. */

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include "definitions.h"                // SYS function prototypes
#include <stdio.h>  
#include <math.h>
#include <string.h>
#include "global.h"
#include "app_atm90e26.h"
#include "iso_uart.h"
#include "afe_calib_mem.h"
#include "myutils.h"

#define WR_TX_BUFFER_LENGTH    5
#define WR_RX_BUFFER_LENGTH    1
#define RD_TX_BUFFER_LENGTH    3
#define RD_RX_BUFFER_LENGTH    3




#define READ_90E26             0x80
#define READ_PARAM_NUM 28

afeuart_params afe_uart_data;
AFE_task_sta_ afe_task_status;
afe_params afe_data;
afe_calibration_reg_ afe_cal_reg;
afe_calibration_reg_eeprom afe_cal_reg_eeprom;



static uint8_t read_m90e26_reg[READ_PARAM_NUM] = {
    APenegy,    //22
	ANenergy,   //23
	ATenergy,   //24
	RPenergy,   //25
	RNenergy,   //26
	RTenergy,   //27

    SysStatus,  //0
    //Calibration
    PLconstH,     //1
	PLconstL,   //2
	Lgain,      //3
	Lphi,       //4
	MMode,      //5
	Ugain,      //6
	IgainL,     //7
    PoffsetL,   //8
    QoffsetL,   //9
    //metering
	Enstatus,	//10
    Irms,   //11
	Urms,   //12
	Pmean,  //13
	Qmean,  //14
	Freq,   //15
	PowerF, //16
	Pangle, //17
	Smean,  //18
	Irms2,  //19
	Pmean2, //20
	Qmean2, //21
    
};




volatile uint8_t read_param_counter=0;
uint16_type afe_read_param[READ_PARAM_NUM];
volatile uint16_t afe_wait_for_read=0;
volatile uint8_t afe_read_register_ready=0;
volatile uint8_t afe_time_to_read_energy=0;
volatile uint16_t afe_wait_for_read_energy=0;
volatile uint8_t is_afe_ready;
afe_meas_results meter_data;
float meter_at_energy=0;
float meter_rt_energy=0;
buffer_t calib_buff;
//================ATM90E UART Driver Start====================

void AFE_uart_TX_callback(uintptr_t context )
{
    afe_uart_data.txdone=1;

}

void AFE_uart_RX_callback(uintptr_t context)
{
    switch(afe_uart_data.rxstatus)
    {
        case RX_IDEAL:
        {}break;
        case RX_BYTE1_REC:
        {
 
        
        }break;
        case RX_COMPLETE:
        {}break;
        case RX_WAIT: //when required data received
        {
            afe_uart_data.rx_overflow=0;
            afe_uart_data.rxstatus = RX_COMPLETE;
        
        }break;
        default: break;
    }
   
    
}

void AFE_uart_init(void)
{
    
    SERCOM0_USART_WriteCallbackRegister(AFE_uart_TX_callback,0);
    SERCOM0_USART_ReadCallbackRegister(AFE_uart_RX_callback,0);    
    afe_uart_data.txdone=1;
    afe_task_status =AFE_INIT;
  
    
}

void AFE_uart_send_data(void)
{
    afe_uart_data.txdone=0;
    SERCOM0_USART_Write(afe_uart_data.tx_buff,afe_uart_data.txbufflen);
    while(afe_uart_data.txdone<1);
}


void AFE_uart_send_data_wait(void)
{
    uint8_t i;
    SERCOM0_REGS->USART_INT.SERCOM_INTENCLR = (uint8_t)SERCOM_USART_INT_INTFLAG_DRE_Msk;
            
    for(i=0;i<afe_uart_data.txbufflen;i++)
    {
        while (((SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) != SERCOM_USART_INT_INTFLAG_DRE_Msk));
        SERCOM0_REGS->USART_INT.SERCOM_DATA = afe_uart_data.tx_buff[i];
    }
   
}

void AFE_uart_receive_data_start(void)
{
    SERCOM0_USART_Read(afe_uart_data.rx_buff,1);
}

void AFE_uart_rx_overflow_fun(void)
{
    if(afe_uart_data.rx_overflow>0)
    {
        afe_uart_data.rx_overflow--;
        if((afe_uart_data.rx_overflow==0) &&(afe_uart_data.rxstatus == RX_WAIT) )
        {
            afe_uart_data.rxbufflen = SERCOM0_USART_ReadCountGet();
            afe_uart_data.rxstatus = RX_OVERFLOW;//RX_TIMEOUT;
            SERCOM0_USART_ReadAbort();
            
            
        }
    }
}

void AFE_write_command(uint8_t addr, uint16_t value)
{
                    SERCOM0_USART_ReadAbort();
                    SYSTICK_DelayMs(20);
                    afe_uart_data.tx_buff[0] = 0xFE;
                    afe_uart_data.tx_buff[1] = addr;
                    afe_uart_data.tx_buff[2] = (uint8_t)(value>>8);
                    afe_uart_data.tx_buff[3] = (uint8_t)value;
                    afe_uart_data.tx_buff[4] = (uint8_t) (afe_uart_data.tx_buff[1]+afe_uart_data.tx_buff[2]+afe_uart_data.tx_buff[3]);
                    afe_uart_data.txbufflen = 5;
                    afe_data.last_write_checksum = afe_uart_data.tx_buff[4];
                    afe_uart_data.rx_buff[0] = 0;
                    afe_uart_data.rx_buff[1] = 0;
                    afe_uart_data.rx_buff[2] = 0;
                    afe_uart_data.rxstatus = RX_WAIT;
                    
                     
                    SERCOM0_USART_Read(afe_uart_data.rx_buff,1);
                    AFE_uart_send_data_wait();
                    afe_uart_data.rx_overflow = AFE_MS_OVF_WAIT;
          
}

void AFE_read_command(uint8_t addr)
{
                 SERCOM0_USART_ReadAbort();
                 SYSTICK_DelayMs(20);
                 afe_uart_data.tx_buff[0] = 0xFE;
                 afe_uart_data.tx_buff[1] = 0x80|addr;
                 afe_uart_data.tx_buff[2] = 0x80|addr;
                 afe_uart_data.txbufflen = 3;
                    afe_uart_data.rx_buff[0] = 0;
                    afe_uart_data.rx_buff[1] = 0;
                    afe_uart_data.rx_buff[2] = 0;

                 afe_uart_data.rxstatus = RX_WAIT;
                 afe_uart_data.rx_overflow = AFE_MS_OVF_WAIT;
                  
                 SERCOM0_USART_Read(afe_uart_data.rx_buff,3);
                 AFE_uart_send_data_wait();
                
}

uint8_t afe_uart_rx_error_check(void)
{
    uint8_t  u8dummyData=0;
  
    
  if(SERCOM0_REGS->USART_INT.SERCOM_STATUS & 0x07) 
  {
      SERCOM0_REGS->USART_INT.SERCOM_INTENSET = 0;
         /* Clear error flag */
        SERCOM0_REGS->USART_INT.SERCOM_INTFLAG = (uint8_t)SERCOM_USART_INT_INTFLAG_ERROR_Msk;
        /* Clear all errors */
        SERCOM0_REGS->USART_INT.SERCOM_STATUS = (uint16_t)(SERCOM_USART_INT_STATUS_PERR_Msk | SERCOM_USART_INT_STATUS_FERR_Msk | SERCOM_USART_INT_STATUS_BUFOVF_Msk);

        /* Flush existing error bytes from the RX FIFO */
        while((SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & (uint8_t)SERCOM_USART_INT_INTFLAG_RXC_Msk) == (uint8_t)SERCOM_USART_INT_INTFLAG_RXC_Msk)
        {
            u8dummyData = (uint8_t)SERCOM0_REGS->USART_INT.SERCOM_DATA;
        }
        
        app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"==AFE RX ERROR==\n");
        app_uart_iso_send_data_wait();
        
        return 1;
  }
  else
      return 0;
    (void)u8dummyData;
}

//============================================================
//================ATM90E UART Driver Finish===================
//============================================================
/*
ATM90E TASK API STARTS FROM HERE 
 
*/

void AFE_set_default_calib_values(void)
{
                    afe_cal_reg.IgainL =    IGAINL;
                    afe_cal_reg.Lgain= LGAIN;
                    afe_cal_reg.Lphi= LPHI;
                    afe_cal_reg.PoffsetL= POFFSETL;
                    afe_cal_reg.QoffsetL=   QOFFSETL;
                    afe_cal_reg.Ugain= UGAIN;
}
void AFE_calibration(void)
{
    app_uart_iso_print("$CALIBRATION START@\n");
    app_uart_iso_print("""""""""""""""""""""""\n");
    AFE_write_command(SoftReset,0x789A);
    SYSTICK_DelayMs(500);
    app_uart_iso_print("$AFE RESET DONE@\n");
    app_uart_iso_print("$AFE READING DEAFUALT VALUE@\n");
    afe_print_read_registers();//Verify Default Values
    app_uart_iso_print("$CALIBRATION MODE ENABLE@\n");
    uint8_t i;
    uint32_t active_pwr;
    uint32_t reactive_pwr;
    uint16_t a,r;
 
    do{
        AFE_write_command(CalStart,0x5678);
    }while(!check_write_checksum());
       
    do{
        AFE_write_command(AdjStart,0x5678);
    }while(!check_write_checksum());
        
     do{
        AFE_write_command(MMode,MMODE_VALUE);
    }while(!check_write_checksum());
    
      do{
        AFE_write_command(PLconstH,PLCONSTH);
    }while(!check_write_checksum());
    
      do{
        AFE_write_command(PLconstL,PLCONSTL);
    }while(!check_write_checksum());
    
    app_uart_iso_print("$SETTING OFFSET@\n");
    do{
        AFE_write_command(SmallPMod,0xA987);
    }while(!check_write_checksum());
    
    active_pwr = 0;
    reactive_pwr = 0;
    app_uart_iso_print("$GETTING AVG N P Q@\n");
    for(i=0;i<8;i++)
    {
        while(!read_register(MEAS_Pmean));
        active_pwr += afe_read_param[MEAS_Pmean].int_value;
        while(!read_register(MEAS_Qmean));
        reactive_pwr += afe_read_param[MEAS_Qmean].int_value;
        app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "%d,%04x,%04x\n",i,afe_read_param[MEAS_Pmean].int_value,
          afe_read_param[MEAS_Qmean].int_value);
        app_uart_iso_send_data_wait();
        SYSTICK_DelayMs(500);
    }
    a = (active_pwr/8);
    r = (reactive_pwr/8);
    a = (~a) + 1;
    r = (~r) + 1;
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"CV a=%04x r=%04x\n",a,r);
    app_uart_iso_send_data_wait();
    
    do{
       AFE_write_command(PoffsetL,a);
    }while(!check_write_checksum());

    do{
       AFE_write_command(QoffsetL,r);
    }while(!check_write_checksum());

    do{
       AFE_write_command(SmallPMod,0x0000);
    }while(!check_write_checksum());

    afe_cal_reg.PoffsetL = a;
    afe_cal_reg.QoffsetL = r; 
    app_uart_iso_print("$SETTING OFFSET DONE@\n");
    app_uart_iso_print("$PRINTING AFTER OFFSET CALIB@\n");  
    app_uart_iso_print("""""""""""""""""""""""\n");
    app_uart_iso_print("$CALIBRATION FINISH@\n");
    
}
void AFE_Load_Cal_Data(void)
{
     afe_cal_reg.IgainL =    IGAINL;
                    afe_cal_reg.Lgain= LGAIN;
                    afe_cal_reg.Lphi= LPHI;
                    afe_cal_reg.PoffsetL= POFFSETL;
                    afe_cal_reg.QoffsetL=   QOFFSETL;
                    afe_cal_reg.Ugain= UGAIN;
}

void AFE_init(void)
{
    AFE_uart_init(); //Register RX and TX Callback
    afe_uart_data.tx_buff[0] = 0xFE; //TX BUFF init
    afe_uart_data.tx_buff[1] = 0;
    afe_uart_data.tx_buff[2] = 0;
    afe_uart_data.tx_buff[3] = 0;
    afe_uart_data.tx_buff[4] = 0;
    
    afe_uart_data.txbufflen = 5;
    afe_data.fail_counter=0;
    
    is_afe_ready=0;
    
}


void AFE_Task(void)
{
    switch(afe_task_status)
    {
        case AFE_INIT: //init uart callback and AFE initialization
        {
           
            AFE_init();
              afe_data.afe_calib_start_sta = AFE_CALIB_START; //write to calibration register intitiated
              afe_task_status = AFE_CALIB;
            
            
          
        }break;
        
        case AFE_RESET:
        {}break;
        
        case AFE_CALIB:
        {
            AFE_write_calibration_register();
           
        }break;
        case AFE_IDLE: //wait for metering command here
        {
            if( afe_wait_for_read == 0)//time to read register
            {
                
                afe_task_status = AFE_READ;
                read_param_counter =  MEAS_sysstatus;; //start from sys_status
                if(afe_time_to_read_energy==0)
                {
                    afe_time_to_read_energy = 1;
                    afe_wait_for_read_energy=500;//wait for 5 seconds.
                }
                else 
                {
                    if(afe_wait_for_read_energy==0)
                    {
                        read_param_counter=0; //read everything
                        afe_wait_for_read_energy = 0; //5 seconds

                    }
                }
            }
        }break;
        case AFE_READ:
        {
            if(read_param_counter<READ_PARAM_NUM)  //read all 28 register
            {
                AFE_read_command(read_m90e26_reg[read_param_counter]);
                afe_task_status = AFE_READ_WAIT;
            }
            else //when all readings done wait for 500mSec
            {
                afe_wait_for_read = 50;  
                afe_task_status = AFE_IDLE;
                afe_read_register_ready =1;
                update_metering_data();
                //calculate metering data 
            }
        }break;
        case AFE_READ_WAIT:
        {
            //gets AFE read register
            if(afe_uart_data.rxstatus==RX_COMPLETE)
            {
                uint8_t cks;
                cks  = (uint8_t)(afe_uart_data.rx_buff[0]+afe_uart_data.rx_buff[1]);
                if(cks ==afe_uart_data.rx_buff[2])
                {
                    afe_read_param[read_param_counter].byteH = afe_uart_data.rx_buff[0];
                    afe_read_param[read_param_counter].byteL = afe_uart_data.rx_buff[1];
                    read_param_counter++;
                    afe_task_status = AFE_READ; //read next
                    
                }
                else //checksum not matching
                {
                    SYSTICK_DelayMs(30);
                    afe_task_status = AFE_READ; //read next
                }
   
                afe_uart_data.rx_buff[0] =0;
                afe_uart_data.rx_buff[1] =0;
                afe_uart_data.rx_buff[2] =0;
                
            }
            else if (afe_uart_data.rxstatus==RX_OVERFLOW)
            {
                afe_task_status = AFE_READ; //read again
            
            }
            

        }break;
        case AFE_WACK_WAIT:
        {}break;
        case AFE_WRITE:
        {
        
        
        
        }break;
        default:
            break;
    
    }//end of switch(afe_task_status)    
}


void update_metering_data(void)
{

    float ftemp;
    uint16_t u16temp;
    int iint;
   u16temp = (afe_read_param[MEAS_Pmean].int_value);
   if(u16temp & 0x8000)
   {
       u16temp = 0xffff-u16temp;
   }
   meter_data.pmean.float_var = (float_t)u16temp;
 
   //reactive power kva - Qmean
   iint =  afe_read_param[MEAS_Qmean].signed_value;
   //iint = iint/4; //0.001 KW
   meter_data.qmean.int1 = iint;
      
   //apparent power - Smean
   iint =  afe_read_param[MEAS_Smean].signed_value;
   //iint = iint/4; //0.001 KVA
   meter_data.smean.int1 = iint;
   
   //vrms
   u16temp = afe_read_param[MEAS_Urms].int_value;
   ftemp = (float)u16temp/100;
   meter_data.urms.float_var = ftemp;
   
   //irms
   u16temp = afe_read_param[MEAS_Irms].int_value;
   ftemp = (float)u16temp/(1000);
   meter_data.irms.float_var = ftemp;
   
   //freq
   u16temp = afe_read_param[MEAS_Freq].int_value;
   ftemp = (float)u16temp/(100);
   meter_data.freq.float_var = ftemp;
   
   //pf
   u16temp = (afe_read_param[MEAS_PowerF].int_value);
   if(u16temp & 0x8000)
   {
       u16temp = u16temp & 0x7FF;
   }

   ftemp = (float)u16temp/(1000);
   meter_data.pf.float_var = ftemp;
   
   
   //active energy - AT absolute Active energy - KWh at resolution of 0.1
   u16temp = (afe_read_param[MEAS_ATenergy].int_value);
   ftemp = (float)u16temp;///10; //convert into pulses
   meter_data.at_energy.float_var = ftemp;
   ftemp = (ftemp) *(0.069444444); 
   meter_at_energy = meter_at_energy + ftemp; 
   u16temp = (afe_read_param[MEAS_RTenergy].int_value);
   ftemp = (float)u16temp/10; //convert into pulses
   meter_data.rt_energy.float_var = ftemp;
   ftemp = (ftemp) *(0.069444444); 
   
   meter_rt_energy = meter_rt_energy + ftemp; 
   
   u16temp = (afe_read_param[MEAS_ENstatus].int_value);
   meter_data.enstatus.word1 = u16temp;
   
}


/*
 * 
 BELOW SECTION BELONGS TO AUTO CALIBRATION 
 ONCE AUTO CALIBRATION ENABLED ALL OTHER FUNCTIONALITY OF 
 MCU IS ON HALT MODE AND ONLY ISOLATED UART AND AFE SECTION IS WORKING
 */

extern volatile uint8_t app_mode;

/*
 CALIBRATION COMMANDS
  
 1. START CALIBRATION - $C,C,C@
             RESPONCE - $MCSTART=DONE@\n 
    ACTION   - WRITE TO   adjstart/calstart/mmode/plconst/
 
 2. POWER OFFSET CALIBRATION - $C,OFFSET@
                    RESPONCE - $MCOFFSET=DONE@\n
    ACTION   - SWITCH TO SMALL POWER MODE (04H = A987H)
    READ ACTIVE /REAACTIVE POWER (PMEAN QMEAN)
    WRITE COMPLEMENT VALUE + 1 TO 37H AND 38H
    SWITCH TO NORMAL POWER MODE - (04H = 0000H)
 
 3. CALIBRATION - $C,VIA,XXX.XX,X.XXX,XX.XXCRC@ CRC= SUM ($ TO XX)
     $C, VIA, X X X .  X  X  ,  X  .  X  X  X  ,  X  X  .  X  X  CRC  @
     012 3456 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25   26
 4. STOP CALIBRATION  - $C,BREAK@  - $MCSTOP=DONE@\n
 3.  
 
 */
void process_calib_cmd(void)
{
    uint8_t *ptr;
    
    
    if(app_mode == APP_MODE_APP)
    {
                    ptr = (uint8_t *)strstr((const char *)&calib_buff.buff[0], "$C,C,C");


					if (ptr!=NULL)
					{
                        app_uart_iso_print("$CALIBRATION START@\n");
                        app_mode = APP_MODE_CALIBRATION;
                        afe_data.afe_calib_start_sta = AFE_CALIB_START;
					}       
    
    }
    else //if in calibration mode 
    {
    
                    ptr =  (uint8_t *)strstr((const char *)&calib_buff.buff[0],"$C,BREAK");
					if (ptr!=NULL)
					{
                        app_uart_iso_print("$CALIBRATION STOP\n");
                        
                        write_AFEcalibration_data();
                        app_mode = APP_MODE_APP;
                        afe_task_status = AFE_CALIB;
                        return;
					} 
                    
                    ptr =  (uint8_t *)strstr((const char *)&calib_buff.buff[0],"$C,CANCLE");
					if (ptr!=NULL)
					{
                        app_uart_iso_print("$CALIBRATION CANCLE\n");
                        app_mode = APP_MODE_APP;
                        afe_task_status = AFE_CALIB;
                        return;
					}
                    
                    ptr =  (uint8_t *)strstr((const char *)&calib_buff.buff[0],"$C,OFFSET");
					if (ptr!=NULL)
					{
                        app_uart_iso_print("$CALIBRATION OFFSET@\n");
                        afe_data.afe_calib_start_sta = AFE_CALIB_POFFSET;
                        return;
					}
                    ptr =  (uint8_t *)strstr((const char *)&calib_buff.buff[0],"$C,VIA");
					if (ptr!=NULL)
					{
                        app_uart_iso_print("$CALIBRATION VIA@\n");
                        afe_data.afe_calib_start_sta = AFE_CALIB_ALL;
                        return;
					}
                    ptr =  (uint8_t *)strstr((const char *)&calib_buff.buff[0],"$C,VCAL");
					if (ptr!=NULL)
					{
                        app_uart_iso_print("$CALIBRATION VOLTS@\n");
                        afe_data.afe_calib_start_sta = AFE_CALIB_UGAIN;
                        return;
					}
                    ptr =  (uint8_t *)strstr((const char *)&calib_buff.buff[0],"$C,ICAL");
					if (ptr!=NULL)
					{
                        app_uart_iso_print("$CALIBRATION CURRENT@\n");
                        afe_data.afe_calib_start_sta = AFE_CALIB_IGAINL;
                        return;
					}
                    
    }

    calib_buff.process_buff = 0;
}


void AFE_setcalibration_mode(void)
{
    do{
        AFE_write_command(CalStart,0x5678);
    }while(!check_write_checksum());
       
    do{
        AFE_write_command(AdjStart,0x5678);
    }while(!check_write_checksum());
        
     do{
        AFE_write_command(MMode,MMODE_VALUE);
    }while(!check_write_checksum());
    
      do{
        AFE_write_command(PLconstH,PLCONSTH);
    }while(!check_write_checksum());
    
      do{
        AFE_write_command(PLconstL,PLCONSTL);
    }while(!check_write_checksum());
    
}


void AFE_process_calibration(void)
{
    
    switch(afe_data.afe_calib_start_sta)
    {
        case AFE_CALIB_START:
        {
               //Reset the Metering Device
            AFE_write_command(SoftReset,0x789A);
            SYSTICK_DelayMs(500);
            app_uart_iso_print("$AFE RESET DONE@\n");
            app_uart_iso_print("$AFE READING DEAFUALT VALUE@\n");
            afe_print_read_registers();//Verify Default Values
            app_uart_iso_print("$CALIBRATION MODE ENABLE@\n");
            afe_data.afe_calib_start_sta = AFE_CALIB_WAIT;
            AFE_setcalibration_mode();
            
        }break;
        case AFE_CALIB_POFFSET: //calibrate offset
        {
            offset_calibration();
            afe_data.afe_calib_start_sta = AFE_CALIB_WAIT;
        }break;
        case AFE_CALIB_ALL: //calibrate ugain igain lphi
        {
            AFE_via_autocalibration();
            afe_data.afe_calib_start_sta = AFE_CALIB_WAIT;
        }break;
        case AFE_CALIB_UGAIN: //calibrate ugain igain lphi
        {
            AFE_ugain_calibration();
            afe_data.afe_calib_start_sta = AFE_CALIB_WAIT;
        }break;
        case AFE_CALIB_IGAINL: //calibrate ugain igain lphi
        {
            AFE_igain_calibration();
            afe_data.afe_calib_start_sta = AFE_CALIB_WAIT;
        }break;
        
        
        
        case AFE_CALIB_WAIT:
        {
        
        }
        
        default:break;
    }


}

/*
  $C, VCAL, X X X  .  X  X  CRC  @
  012 34567 8 9 10 11 12 13 14   15
 */
void AFE_ugain_calibration()
{
   
    uint16_t urms;
    float vref,f1,f2;
    
    
    urms = 0;
    //get referance voltage value
    f1 = (calib_buff.buff[8]-0x30);
    f2 = f1*100;
    f1 = (calib_buff.buff[9]-0x30);
    f2 = f2+(f1*10);
    f1 = (calib_buff.buff[10]-0x30);
    f2 = f2+f1;
    f1 = (calib_buff.buff[12]-0x30);
    f2 = f2+(f1*0.1);
    f1 = (calib_buff.buff[13]-0x30);
    vref = f2+(f1*0.01);

     app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "@REF U=%f\n",vref);
        app_uart_iso_send_data_wait();

    if((vref>260)||(vref<200))
    {
        app_uart_iso_print("$WRONG VCAL Value@\n");
        return;
    }
        
        
    
    app_uart_iso_print("$GETTING Urms Register@\n");
    app_uart_iso_print("$===================@\n");
    while(!read_register(MEAS_Urms));
    urms = afe_read_param[MEAS_Urms].int_value;

    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "Urms=%04x\n",afe_read_param[MEAS_Urms].int_value);
        app_uart_iso_send_data_wait();

    //=======================================    
    //calibrate V
    // ugain = (26400 * vref)/f1    
    //=======================================    
    f1 =  (float)urms/100; //meas voltage
    f2 = 26400 * vref;    
    f2 = f2/f1;
    app_uart_iso_print("$Calculated Ugain= ");
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
    "%04x\n",(uint16_t)f2);        
    app_uart_iso_send_data_wait();
    afe_cal_reg.Ugain = (uint16_t)f2;
    do{
        AFE_write_command(Ugain,afe_cal_reg.Ugain);
    }while(!check_write_checksum());    
        
     app_uart_iso_print("$UGAIN CALIBRATION DONE@\n");
     app_uart_iso_print("\n\n$AFE READING VALUE@\n");
     afe_print_read_registers();//Verify Default Values
     app_uart_iso_print("$WAITING CMD!!!@\n");
}

/*
  $C, ICAL, X . X  X  X  CRC @
  012 34567 8 9 10 11 12 13  14
 */
void AFE_igain_calibration()
{
   
    uint16_t irms;
    float iref,f1,f2;
   
    
    irms = 0;
  //get reference current value
    f1 = (calib_buff.buff[8]-0x30);
    f2 = f1;
    f1 = (calib_buff.buff[10]-0x30);
    f2 = f2+(f1*0.1);
    f1 = (calib_buff.buff[11]-0x30);
    f2 = f2+(f1*0.01);
    f1 = (calib_buff.buff[12]-0x30);
    iref = f2+(f1*0.001);

     app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "@REF I=%f\n",iref);
        app_uart_iso_send_data_wait();

    if(iref>8)
    {
        app_uart_iso_print("$WRONG ICAL Value@\n");
        return;
    }
        
        
    
    app_uart_iso_print("$GETTING Irms Register@\n");
    app_uart_iso_print("$===================@\n");
    while(!read_register(MEAS_Irms));
    irms = afe_read_param[MEAS_Irms].int_value;

    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "Irms=%04x\n",afe_read_param[MEAS_Irms].int_value);
        app_uart_iso_send_data_wait();

    //=======================================    
    //calibrate I
    //igainL = (31251*iref)/f1
    //=======================================     
    f1 =  (float)irms/1000; //meas voltage
    f2 = 31251 * iref;    
    f2 = f2/f1;
    app_uart_iso_print("$Calculated igain= ");
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
    "%04x\n",(uint16_t)f2);        
    app_uart_iso_send_data_wait();
    afe_cal_reg.IgainL = (uint16_t)f2;
    do{
        AFE_write_command(IgainL,afe_cal_reg.IgainL);
    }while(!check_write_checksum());
        
     app_uart_iso_print("$IGAIN CALIBRATION DONE@\n");
     app_uart_iso_print("\n\n$AFE READING VALUE@\n");
     afe_print_read_registers();//Verify Default Values
     app_uart_iso_print("$WAITING CMD!!!@\n");
}



void offset_calibration(void)
{
    uint8_t i;
    uint32_t active_pwr;
    uint32_t reactive_pwr;
    uint16_t a,r;
 
    do{
        AFE_write_command(CalStart,0x5678);
    }while(!check_write_checksum());
       
    do{
        AFE_write_command(AdjStart,0x5678);
    }while(!check_write_checksum());
        
     do{
        AFE_write_command(MMode,MMODE_VALUE);
    }while(!check_write_checksum());
    
      do{
        AFE_write_command(PLconstH,PLCONSTH);
    }while(!check_write_checksum());
    
      do{
        AFE_write_command(PLconstL,PLCONSTL);
    }while(!check_write_checksum());
    
    app_uart_iso_print("$SETTING OFFSET@\n");
    do{
        AFE_write_command(SmallPMod,0xA987);
    }while(!check_write_checksum());
    
    active_pwr = 0;
    reactive_pwr = 0;
    app_uart_iso_print("$GETTING AVG N P Q@\n");
    for(i=0;i<8;i++)
    {
        while(!read_register(MEAS_Pmean));
        active_pwr += afe_read_param[MEAS_Pmean].int_value;
        while(!read_register(MEAS_Qmean));
        reactive_pwr += afe_read_param[MEAS_Qmean].int_value;
        app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "%d,%04x,%04x\n",i,afe_read_param[MEAS_Pmean].int_value,
          afe_read_param[MEAS_Qmean].int_value);
        app_uart_iso_send_data_wait();
        SYSTICK_DelayMs(500);
    }
    a = (active_pwr/8);
    r = (reactive_pwr/8);
    a = (~a) + 1;
    r = (~r) + 1;
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"CV a=%04x r=%04x\n",a,r);
    app_uart_iso_send_data_wait();
    
    do{
       AFE_write_command(PoffsetL,a);
    }while(!check_write_checksum());

    do{
       AFE_write_command(QoffsetL,r);
    }while(!check_write_checksum());

    do{
       AFE_write_command(SmallPMod,0x0000);
    }while(!check_write_checksum());

    afe_cal_reg.PoffsetL = a;
    afe_cal_reg.QoffsetL = r; 
    app_uart_iso_print("$SETTING OFFSET DONE@\n");
    app_uart_iso_print("$PRINTING AFTER OFFSET CALIB@\n");
     
    afe_print_read_registers();
}
/*
 $C, VIA, X X X .  X  X  ,  X  .  X  X  X  ,  X  X  .  X  X  CRC  @
 012 3456 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25   26
 */
void AFE_via_autocalibration(void)
{
    uint16_t urms,irms,smean,temp_16;
    int pmean,angle;
    float vref,iref,aref,f1,f2;
    urms = 0;
    irms = 0;
    pmean = 0;
    smean = 0;
    //get referance voltage value
    f1 = (calib_buff.buff[7]-0x30);
    f2 = f1*100;
    f1 = (calib_buff.buff[8]-0x30);
    f2 = f2+(f1*10);
    f1 = (calib_buff.buff[9]-0x30);
    f2 = f2+f1;
    f1 = (calib_buff.buff[11]-0x30);
    f2 = f2+(f1*0.1);
    f1 = (calib_buff.buff[12]-0x30);
    vref = f2+(f1*0.01);
    
    //get reference current value
    f1 = (calib_buff.buff[14]-0x30);
    f2 = f1;
    f1 = (calib_buff.buff[16]-0x30);
    f2 = f2+(f1*0.1);
    f1 = (calib_buff.buff[17]-0x30);
    f2 = f2+(f1*0.01);
    f1 = (calib_buff.buff[18]-0x30);
    iref = f2+(f1*0.001);

    
    //get reference angle value
    f1 = (calib_buff.buff[20]-0x30);
    f2 = f1*10;
    f1 = (calib_buff.buff[21]-0x30);
    f2 = f2+(f1);
    f1 = (calib_buff.buff[23]-0x30);
    f2 = f2+(f1*0.1);
    f1 = (calib_buff.buff[24]-0x30);
    aref = f2+(f1*0.01);
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "@REF U=%f I=%f A=%f\n",vref,iref,aref);
        app_uart_iso_send_data_wait(); 
    app_uart_iso_print("$GETTING VI Register@\n");
    app_uart_iso_print("$===================@\n");
    while(!read_register(MEAS_Urms));
    urms = afe_read_param[MEAS_Urms].int_value;
    while(!read_register(MEAS_Irms));
    irms = afe_read_param[MEAS_Irms].int_value;

    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
          "Urms=%04x,Irms=%04x\n",afe_read_param[MEAS_Urms].int_value,
          afe_read_param[MEAS_Irms].int_value);
        app_uart_iso_send_data_wait();

    //=======================================    
    //calibrate V
    // ugain = (26400 * vref)/f1    
    //=======================================    
    f1 =  (float)urms/100; //meas voltage
    f2 = 26400 * vref;    
    f2 = f2/f1;
    app_uart_iso_print("$Calculated Ugain= ");
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
    "%04x\n",(uint16_t)f2);        
    app_uart_iso_send_data_wait();
    afe_cal_reg.Ugain = (uint16_t)f2;
    do{
        AFE_write_command(Ugain,afe_cal_reg.Ugain);
    }while(!check_write_checksum());
    //=======================================    
    //calibrate I
    //igainL = (31251*iref)/f1
    //=======================================     
    f1 =  (float)irms/1000;
    f2 = 31251 * iref;    
    f2 = f2/f1;
    app_uart_iso_print("$Calculated igain= ");
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
    "%04x\n",(uint16_t)f2);        
    app_uart_iso_send_data_wait();
    afe_cal_reg.IgainL = (uint16_t)f2;
    do{
        AFE_write_command(IgainL,afe_cal_reg.IgainL);
    }while(!check_write_checksum());
    
    app_uart_iso_print("$=====================@\n");
    app_uart_iso_print("$=VI CALIBRATION DONE=@\n");
    app_uart_iso_print("$=====================@\n");
    
    SYSTICK_DelayMs(1500);
    app_uart_iso_print("$=READING REGISTER AFTER VI=@\n");
    app_uart_iso_print("$==================================@\n");
    afe_print_read_registers();//Reading Register after VI Calib
    app_uart_iso_print("$==================================@\n");

     pmean = afe_read_param[MEAS_Pmean].signed_value; //KW

     smean = afe_read_param[MEAS_Smean].int_value;//KVA

     angle = afe_read_param[MEAS_Pangle].signed_value;//KVA
     
     app_uart_iso_print("$PMEAN SMEAN ANGLE@\n");
     app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
     "$%05d,%05d,%05d\n",pmean,smean,angle);        
     app_uart_iso_send_data_wait();
     

     app_uart_iso_print("\r\n$Calculation based on Angle Reg@\n");
     f1 = angle/10;
     f2 = f1 - aref;
 
     
     app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
     "\r\nRoundoff F2 =%f\n",f2);        
     app_uart_iso_send_data_wait();
     
     f2 = f2 *113.778;
     
     temp_16 = f2;
     app_uart_iso_print("\r\n$Calculated Lphi on Angle= ");
     app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
     "\r\n$%f %04x@\n",f2,temp_16);        
     app_uart_iso_send_data_wait();
     
     if(temp_16 & 0x8000)
     {
         temp_16 = 65536-temp_16;
     }
     afe_cal_reg.Lphi = temp_16;
     app_uart_iso_print("\r\n$Lphi Reg= ");
     app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
     "%04x@\n",temp_16);        
     app_uart_iso_send_data_wait();
     
     do{
        AFE_write_command(Lphi,afe_cal_reg.Lphi);
    }while(!check_write_checksum());    
    
     app_uart_iso_print("\r\n$CALIBRATION DONE@\n");
     app_uart_iso_print("\r\n$AFE READING VALUE@\n");
     afe_print_read_registers();//Verify Default Values
     app_uart_iso_print("\r\n$WAITING CMD!!!@\n");
    

}

uint8_t check_write_checksum(void)
{
    
    while(afe_uart_data.rxstatus!=RX_COMPLETE)
    {
            if(afe_uart_data.rxstatus==RX_OVERFLOW)
            {
                return 0;
            }
    }
    
    if(afe_uart_data.rx_buff[0] == afe_data.last_write_checksum)
    {
        return 1;
    }
    else
    {
        SYSTICK_DelayMs(20);
        return 0;
    }
}

uint8_t read_register(uint8_t reg_addr)
{
        AFE_read_command(read_m90e26_reg[reg_addr]);
        while(afe_uart_data.rxstatus!=RX_COMPLETE)
        {
            if(afe_uart_data.rxstatus==RX_OVERFLOW)
            {
                return 0;
            }
        }
            
            uint8_t cks;
            cks  = (uint8_t)(afe_uart_data.rx_buff[0]+afe_uart_data.rx_buff[1]);
            if(cks ==afe_uart_data.rx_buff[2])
            {
                  afe_read_param[reg_addr].byteH = afe_uart_data.rx_buff[0];
                  afe_read_param[reg_addr].byteL = afe_uart_data.rx_buff[1];
                  afe_uart_data.rx_buff[0] =0;
                  afe_uart_data.rx_buff[1] =0;
                  afe_uart_data.rx_buff[2] =0;
                  return 1;
            }
            else //checksum not matching
            {
                SYSTICK_DelayMs(30);
                    afe_uart_data.rx_buff[0] =0;
                    afe_uart_data.rx_buff[1] =0;
                    afe_uart_data.rx_buff[2] =0;
                return 0;
            }
                
}

void afe_print_read_registers(void)
{
    uint8_t i,j;
    j=0;
    for(i=0;i<READ_PARAM_NUM;i++)
    {
        AFE_read_command(read_m90e26_reg[j]);
        while(afe_uart_data.rxstatus!=RX_COMPLETE)
        {
            if(afe_uart_data.rxstatus==RX_OVERFLOW)
            {
                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\n NO RESPONSE \n");
                 app_uart_iso_send_data_wait();
                return;
                break;
            }
        }
        if(afe_uart_data.rxstatus!=RX_OVERFLOW)
        {
            
            uint8_t cks;
            cks  = (uint8_t)(afe_uart_data.rx_buff[0]+afe_uart_data.rx_buff[1]);
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"i=%d,j=%d CKS =%04x ADDR=%04x VALUE=%02x%02x\n",i,j,afe_uart_data.rx_buff[2],read_m90e26_reg[j],afe_uart_data.rx_buff[0],afe_uart_data.rx_buff[1]);
            app_uart_iso_send_data_wait();
            
            if(cks ==afe_uart_data.rx_buff[2])
            {
                  afe_read_param[i].byteH = afe_uart_data.rx_buff[0];
                  afe_read_param[i].byteL = afe_uart_data.rx_buff[1];
                  j++;
            }
            else //checksum not matching
            {
                i--;
                 app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\nCKS ERROR on REG no %d\n",i);
                 app_uart_iso_send_data_wait();
                 SYSTICK_DelayMs(20);
                 
            }
                
        }
        
        afe_uart_data.rx_buff[0] =0;
        afe_uart_data.rx_buff[1] =0;
        afe_uart_data.rx_buff[2] =0;
    
    }
}

void afe_print_read_registers_buff(void)
{
    uint8_t i;
    for(i=0;i<READ_PARAM_NUM;i++)
    {
        
                  app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"%04x\n",afe_read_param[i].int_value);
                  app_uart_iso_send_data_wait();
        
    }

}

void AFE_write_calibration_register(void)
{
    
    switch(afe_data.afe_calib_start_sta)
    {
        case AFE_CALIB_START:
        {
            #ifdef DEBUG_AFE
           app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\n==AFE Soft Reset==\n");
           app_uart_iso_send_data_wait();
           #endif

            AFE_write_command(SoftReset,0x789A);
            SYSTICK_DelayMs(500);//doing soft reset and wait
            afe_data.afe_calib_start_sta = AFE_CALIB_CALSTART;
            afe_data.fail_counter=0;
        
        }break;
        case AFE_CALIB_CALSTART:
        {
            AFE_write_command(CalStart,0x5678);
            SYSTICK_DelayMs(20);
            afe_data.afe_calib_sta_next = AFE_CALIB_ADSTART;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
            #ifdef DEBUG_AFE
           app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\n==AFE Writing 0x5678 in CalStart==\n");
           app_uart_iso_send_data_wait();
           #endif

        }break;
        case AFE_CALIB_ADSTART:
        {
            AFE_write_command(AdjStart,0x5678);
            afe_data.afe_calib_sta_next = AFE_CALIB_MMODE;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
            #ifdef DEBUG_AFE
           app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\n==AFE Writing 0x5678 in AdjStart==\n");
           app_uart_iso_send_data_wait();
            #endif
        }break;
        case AFE_CALIB_MMODE:
        {
            AFE_write_command(MMode,MMODE_VALUE);
            afe_data.afe_calib_sta_next = AFE_CALIB_PLCONSTH;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
            #ifdef DEBUG_AFE
           app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\n==AFE Writing MMODE in mmode==\n");
           app_uart_iso_send_data_wait();
            #endif
        }break;
        case AFE_CALIB_PLCONSTH:
        {
            AFE_write_command(PLconstH,PLCONSTH);
            afe_data.afe_calib_sta_next = AFE_CALIB_PLCONSTL;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
        }break;
        case AFE_CALIB_PLCONSTL:
        {
            AFE_write_command(PLconstL,PLCONSTL);
            afe_data.afe_calib_sta_next = AFE_CALIB_IGAINL;//AFE_CALIB_POFFSET;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
        }break;
        case AFE_CALIB_POFFSET:
        {
            afe_cal_reg.PoffsetL = 0; //VIVI
            AFE_write_command(PoffsetL,afe_cal_reg.PoffsetL);
            afe_data.afe_calib_sta_next = AFE_CALIB_QOFFSET;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
            
            
        }break;
        case AFE_CALIB_QOFFSET:
        {
            afe_cal_reg.QoffsetL = 0;
            AFE_write_command(QoffsetL,afe_cal_reg.QoffsetL);
            afe_data.afe_calib_sta_next = AFE_CALIB_IGAINL;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
        
        }break;
        case AFE_CALIB_LGAIN:
        {}break;
        case AFE_CALIB_IGAINL:
        {
            AFE_write_command(IgainL,afe_cal_reg.IgainL);
            afe_data.afe_calib_sta_next = AFE_CALIB_UGAIN;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
        }break;
        case AFE_CALIB_UGAIN:
        {
            AFE_write_command(Ugain,afe_cal_reg.Ugain);
            afe_data.afe_calib_sta_next = AFE_CALIB_LPHI;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
        
        }break;
        case AFE_CALIB_LPHI:
        {
            afe_cal_reg.Lphi = 0;//VIVI
            AFE_write_command(Lphi,afe_cal_reg.Lphi);
            afe_data.afe_calib_sta_next = AFE_CALIB_STOP;
            afe_data.afe_calib_sta_last = afe_data.afe_calib_start_sta;
            afe_data.afe_calib_start_sta =AFE_CALIB_WAIT;
        
        }break;
        case AFE_CALIB_STOP:
        {
             #ifdef DEBUG_AFE
                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\n==Calibration Write Done ==\n");
                app_uart_iso_send_data_wait();
             #endif
                afe_task_status = AFE_IDLE;
                is_afe_ready=1;  //AFE is ready for measurement
                afe_wait_for_read=0;
                afe_time_to_read_energy=0;

        
        }break;
        case AFE_CALIB_WAIT:
        {
            if(afe_uart_data.rxstatus==RX_COMPLETE) //success
            {
             #ifdef DEBUG_AFE
                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"CKS=%04x RD=%04x\n",afe_data.last_write_checksum,afe_uart_data.rx_buff[0]);
                app_uart_iso_send_data_wait();
             #endif 
                if(afe_uart_data.rx_buff[0] == afe_data.last_write_checksum)
                {
                    //goto next command
                    afe_data.afe_calib_start_sta = afe_data.afe_calib_sta_next;
                    afe_data.fail_counter=0;
                }
                else
                {
                    SYSTICK_DelayMs(30);
                    afe_data.fail_counter++;
                    if(afe_data.fail_counter>10)
                    {
                            afe_data.afe_calib_start_sta = AFE_CALIB_START;//start with start
                    }
                    else
                    {
                        //retry same command
                            
                        afe_data.afe_calib_start_sta = afe_data.afe_calib_sta_last;
                    }
                }
                
            }
            else if(afe_uart_data.rxstatus==RX_OVERFLOW) //no data received
            {
                #ifdef DEBUG_AFE
                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"\r\n==AFE RX OVERFLOW\n");
                app_uart_iso_send_data_wait();
                 #endif 
                SYSTICK_DelayMs(20);
                afe_data.fail_counter++;
                if(afe_data.fail_counter>10)
                {
                        afe_data.afe_calib_start_sta = AFE_CALIB_START;//start with start
                }
                else
                {
                    //retry same command
                    afe_data.afe_calib_start_sta = afe_data.afe_calib_sta_last;
                }
            }
            
        
        }break;
        
        default:
            break;
    
    
    }
}

//================ATM90E EMETER ==============================