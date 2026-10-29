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
#include <string.h>

#include "definitions.h"                // SYS function prototypes
#include "global.h"
//#include "pc_uart.h"
#include "iso_uart.h"
#include "chg_mem_data.h"
#include "app_atm90e26.h"
#include "cp_pp.h"
/*
SERCOM4 MCU DEBUG
 */


#define BTL_TRIGGER_RAM_START   0x20000000
#define BTL_TRIGGER_PATTERN (0x5048434DUL)
static uint32_t *ramStart = (uint32_t *)BTL_TRIGGER_RAM_START;


uart_params app_uart_iso_data;

void usart_isoTX_callback(uintptr_t context )
{
    app_uart_iso_data.txdone=1;
    

}

void usart_isoRX_callback(uintptr_t context)
{
    switch(app_uart_iso_data.rxstatus)
    {
        case RX_IDEAL:
        {}break;
        case RX_BYTE1_REC:
        {
            SERCOM4_USART_Read(&app_uart_iso_data.rx_buff[1],63);
            app_uart_iso_data.rxstatus = RX_WAIT;
            app_uart_iso_data.rx_overflow = MS_500_WAIT;
        
        }break;
        case RX_COMPLETE:
        {}break;
        case RX_WAIT:
        {
            app_uart_iso_data.rx_overflow=0;
            app_uart_iso_data.rxbufflen = 64;
            app_uart_iso_data.rxstatus = RX_COMPLETE;
        
        }break;
        default: break;
    }
}

void app_uart_iso_init(void)
{
    
    SERCOM4_USART_WriteCallbackRegister(usart_isoTX_callback,0);
    SERCOM4_USART_ReadCallbackRegister(usart_isoRX_callback,0);    
    app_uart_iso_data.rxstatus = RX_IDEAL;
    
    app_uart_iso_data.txdone=1;
}

void app_uart_iso_send_data(void)
{
    app_uart_iso_data.txdone=0;
    SERCOM4_USART_Write(app_uart_iso_data.tx_buff,app_uart_iso_data.txbufflen);
    while(app_uart_iso_data.txdone<1);
}

void app_uart_iso_send_data_wait(void)
{
    uint8_t i;
    SERCOM4_REGS->USART_INT.SERCOM_INTENCLR = (uint8_t)SERCOM_USART_INT_INTFLAG_DRE_Msk;
            
    for(i=0;i<app_uart_iso_data.txbufflen;i++)
    {
        while (((SERCOM4_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) != SERCOM_USART_INT_INTFLAG_DRE_Msk));
        SERCOM4_REGS->USART_INT.SERCOM_DATA = app_uart_iso_data.tx_buff[i];
        //SYSTICK_DelayMs(2);
    }
}

//print ascii msg
void app_uart_iso_print(char *msg)
{
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"%s",msg);
    app_uart_iso_send_data_wait();

}

void app_uart_iso_receive_data_start(void)
{
    SERCOM4_USART_Read(app_uart_iso_data.rx_buff,1);
}


void app_uart_iso_rx_task(void)
{
    switch(app_uart_iso_data.rxstatus)
    {
        case RX_IDEAL:
        {
            app_uart_iso_receive_data_start();
            app_uart_iso_data.rxstatus = RX_BYTE1_REC;
        }break;
        case RX_BYTE1_REC:
        {}break;
        case RX_COMPLETE:
        {
            //app_uart_iso_print("PC CMD:");
            app_uart_iso_data.rxstatus = RX_IDEAL;
            check_cmd();
                memcpy(app_uart_iso_data.tx_buff,app_uart_iso_data.rx_buff,app_uart_iso_data.rxbufflen);
                app_uart_iso_data.txbufflen = app_uart_iso_data.rxbufflen;
                app_uart_iso_send_data_wait();
            //Call your parsing Task
        }break;
        case RX_WAIT:
        {}break;
        default: break;
    }

}

void app_uart_iso_rx_overflow_fun(void)
{
    if(app_uart_iso_data.rx_overflow>0)
    {
        app_uart_iso_data.rx_overflow--;
        if((app_uart_iso_data.rx_overflow==0) &&(app_uart_iso_data.rxstatus == RX_WAIT) )
        {
            app_uart_iso_data.rxbufflen = SERCOM4_USART_ReadCountGet()+1;
            app_uart_iso_data.rxstatus = RX_COMPLETE;
            SERCOM4_USART_ReadAbort();
        }
    }
}


void check_cmd(void)
{
     uint8_t *ptr;
     uint16_t temp_t1;
    /*
    if((app_uart_iso_data.rx_buff[0]=='A')&&(app_uart_iso_data.rx_buff[1]=='5'))
    {
     //   check_bootloader_cmd();
        test_chg_memory();
    }
    else if((app_uart_iso_data.rx_buff[0]=='A')&&(app_uart_iso_data.rx_buff[1]=='6'))
    {
        test_op_id();
    }
    */
    if((app_uart_iso_data.rx_buff[0]=='$')&&(app_uart_iso_data.rx_buff[app_uart_iso_data.rxbufflen-1]=='@')
            &&(app_uart_iso_data.rx_buff[app_uart_iso_data.rxbufflen-2]=='#'))
    {
         memcpy(calib_buff.buff,app_uart_iso_data.rx_buff,app_uart_iso_data.rxbufflen);
         calib_buff.buff_len = app_uart_iso_data.rxbufflen;
         calib_buff.process_buff = 1;
    }
    else
    {
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$E,CON");
       if (ptr!=NULL)
       {
           EVSE_Status.chager_on_cmd=1;
                charger_on();
                EVSE_Status.is_charger_on=1;
          //charger_on();
           
           return;
       }
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$RFID,AUTHPASS");
       if (ptr!=NULL)
       {
           if(EVSE_Status.is_charger_on==0)
           {
            EVSE_Status.chager_on_cmd=1;
           }
           else 
           {
                        charger_off();
                        EVSE_Status.is_charger_on = 0;
                        EVSE_Status.chager_on_cmd=0;
                        cp_chg_status = CP_CHG_IDLE;
                        cp_stop_pwm();
           
           }
           return;
       }
        
        
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$E,COFF");
       if (ptr!=NULL)
       {
            charger_off();
                        EVSE_Status.is_charger_on = 0;
                        EVSE_Status.chager_on_cmd=0;
                        cp_chg_status = CP_CHG_IDLE;
                        cp_stop_pwm();
           //EVSE_Status.chager_on_cmd=0;
           //RELAY_MAIN_R1_Clear();
           return;
       }
     
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$E,CPON");
       if (ptr!=NULL)
       {
           cp_start_pwm();
           return;
       }
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$E,CPOFF");
       if (ptr!=NULL)
       {
           cp_stop_pwm();
           return;
       }
        /*
            $E,CPD, 100 0  @
         *  0123456 789 10 11
         */
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$E,CPD");
       if (ptr!=NULL)
       {
           temp_t1 = 1000*(app_uart_iso_data.rx_buff[7]-0x30);
           temp_t1 = temp_t1 + (100*(app_uart_iso_data.rx_buff[8]-0x30));
           temp_t1 = temp_t1 + (10*(app_uart_iso_data.rx_buff[9]-0x30));
           temp_t1 = temp_t1 + ((app_uart_iso_data.rx_buff[10]-0x30));
           
          cp_change_duty_cycle(temp_t1);
           return;
       }
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,CPR");
       if (ptr!=NULL)
       {
           cp_test_cp_signal_value();
           return;
       }
    
        ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$E,CPSTA");
       if (ptr!=NULL)
       {
           check_ev_connection_status();
           return;
       }
        
       /*Get MEter Data*/
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,VRMS");
       if (ptr!=NULL)
       {
           app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$VRMS=%3.1f#\n",meter_data.urms.float_var);
           app_uart_iso_send_data_wait();
           
           return;
       }
       /*Get MEter Data*/
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,IRMS");
       if (ptr!=NULL)
       {
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$IRMS=%2.2f#\n",meter_data.irms.float_var);
            app_uart_iso_send_data_wait();
            return;
       }
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,FREQ");
       if (ptr!=NULL)
       {
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$FREQ=%2.2f#\n",meter_data.freq.float_var);
            app_uart_iso_send_data_wait();
            return;
       }
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,PFACTOR");
       if (ptr!=NULL)
       {
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$PF=%1.3f#\n",meter_data.pf.float_var);
            app_uart_iso_send_data_wait();
            return;
       }
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,ACTPW");
       if (ptr!=NULL)
       {
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$ATCPWT=%5.3f#\n",meter_data.pmean.float_var);
            app_uart_iso_send_data_wait();
            return;
       }
       
       
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,ENERGY");
       if (ptr!=NULL)
       {
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$Wh=%3.2f#\n",meter_at_energy);
            app_uart_iso_send_data_wait();
            return;
       }
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,LOAD");
       if (ptr!=NULL)
       {
           // app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$AP=%5.3f#\n",meter_data.smean.float_var);
          app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$AP=%05dVA#\n",meter_data.smean.word1);
           app_uart_iso_send_data_wait();
           return;
       }
       ptr =  (uint8_t *)strstr((const char *)&app_uart_iso_data.rx_buff[0],"$G,AFER");
       if (ptr!=NULL)
       {
           afe_print_read_registers_buff();
           return;
       }
       
       
    
    }
    
    
    
}



void check_bootloader_cmd(void)
{
        if((app_uart_iso_data.rx_buff[1]=='B')&&
                (app_uart_iso_data.rx_buff[2]=='O')&&
                (app_uart_iso_data.rx_buff[3]=='O')&&
                (app_uart_iso_data.rx_buff[4]=='T')
                )
        {
           app_uart_iso_print("::::BootLoader CMD rec::::\n");
           app_uart_iso_print("::::BootLoader Starting::::\n");
           app_uart_iso_print("::::Please Close Terminal::::\n");
           
           //LED_D7_Clear();
            ramStart[0] = BTL_TRIGGER_PATTERN;
            ramStart[1] = BTL_TRIGGER_PATTERN;
            ramStart[2] = BTL_TRIGGER_PATTERN;
            ramStart[3] = BTL_TRIGGER_PATTERN;
            NVIC_SystemReset();

            
        }
    
}

/* *****************************************************************************
 End of File
 */
