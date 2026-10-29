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
#include "iso_uart.h"
#include "app_atm90e26.h"
#include "cp_pp.h"
#include "host_uart.h"
/*
SERCOM3 
 */



uart_params app_uart_host_data;

uint8_t rfid_timeout_in_sec=10;
uint8_t rfid_is_detected=0;


void usart_hostTX_callback(uintptr_t context )
{
    app_uart_host_data.txdone=1;
    

}

void usart_hostRX_callback(uintptr_t context)
{
    switch(app_uart_host_data.rxstatus)
    {
        case RX_IDEAL:
        {}break;
        case RX_BYTE1_REC:
        {
            SERCOM3_USART_Read(&app_uart_host_data.rx_buff[1],63);
            app_uart_host_data.rxstatus = RX_WAIT;
            app_uart_host_data.rx_overflow = MS_500_WAIT;
        
        }break;
        case RX_COMPLETE:
        {}break;
        case RX_WAIT:
        {
            app_uart_host_data.rx_overflow=0;
            app_uart_host_data.rxbufflen = 64;
            app_uart_host_data.rxstatus = RX_COMPLETE;
        
        }break;
        default: break;
    }
}

void app_uart_host_init(void)
{
    
    SERCOM3_USART_WriteCallbackRegister(usart_hostTX_callback,0);
    SERCOM3_USART_ReadCallbackRegister(usart_hostRX_callback,0);    
    app_uart_host_data.rxstatus = RX_IDEAL;
    
    app_uart_host_data.txdone=1;
}

void app_uart_host_send_data(void)
{
    app_uart_host_data.txdone=0;
    SERCOM3_USART_Write(app_uart_host_data.tx_buff,app_uart_host_data.txbufflen);
    while(app_uart_host_data.txdone<1);
}

void app_uart_host_send_data_wait(void)
{
    uint8_t i;
    SERCOM3_REGS->USART_INT.SERCOM_INTENCLR = (uint8_t)SERCOM_USART_INT_INTFLAG_DRE_Msk;
            
    for(i=0;i<app_uart_host_data.txbufflen;i++)
    {
        while (((SERCOM3_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) != SERCOM_USART_INT_INTFLAG_DRE_Msk));
        SERCOM3_REGS->USART_INT.SERCOM_DATA = app_uart_host_data.tx_buff[i];
        //SYSTICK_DelayMs(2);
    }
}

//print ascii msg
void app_uart_host_print(char *msg)
{
    app_uart_host_data.txbufflen = sprintf((char *)app_uart_host_data.tx_buff,"%s",msg);
    app_uart_host_send_data_wait();

}

void app_uart_host_receive_data_start(void)
{
    SERCOM3_USART_Read(app_uart_host_data.rx_buff,1);
}


void app_uart_host_rx_task(void)
{
    switch(app_uart_host_data.rxstatus)
    {
        case RX_IDEAL:
        {
            app_uart_host_receive_data_start();
            app_uart_host_data.rxstatus = RX_BYTE1_REC;
        }break;
        case RX_BYTE1_REC:
        {}break;
        case RX_COMPLETE:
        {
            app_uart_host_data.rxstatus = RX_IDEAL;
            app_uart_iso_print("Host Rec: ");
            memcpy(app_uart_iso_data.tx_buff,app_uart_host_data.rx_buff,app_uart_host_data.rxbufflen);
            app_uart_iso_data.txbufflen = app_uart_host_data.rxbufflen;
            app_uart_iso_send_data_wait();
            check_host_cmd();
        }break;
        case RX_WAIT:
        {}break;
        default: break;
    }

}

void app_uart_host_rx_overflow_fun(void)
{
    if(app_uart_host_data.rx_overflow>0)
    {
        app_uart_host_data.rx_overflow--;
        if((app_uart_host_data.rx_overflow==0) &&(app_uart_host_data.rxstatus == RX_WAIT) )
        {
            app_uart_host_data.rxbufflen = SERCOM3_USART_ReadCountGet()+1;
            app_uart_host_data.rxstatus = RX_COMPLETE;
            SERCOM3_USART_ReadAbort();
        }
    }
}

//RFID ISO14443 type A: [2DA595EFF2,40]

void check_host_cmd(void)
{
     uint8_t *ptr,i;
     uint8_t arr_rfid[11];
    // uint16_t temp_t1;


        ptr =  (uint8_t *)strstr((const char *)&app_uart_host_data.rx_buff[0],"ISO14443 type A:");
       if (ptr!=NULL)
       {
           ptr = (uint8_t *)strchr((const char *)&app_uart_host_data.rx_buff[0], '[' );
           ptr++;
           if(ptr!=NULL)
           {
               for(i=0;i<11;i++)
               {
                   arr_rfid[i] = *ptr++;
               }
               rfid_process();
               app_uart_iso_print("\nRec RFID: ");
                memcpy(app_uart_iso_data.tx_buff,&arr_rfid[0],10);
                app_uart_iso_data.txbufflen = 10;
                app_uart_iso_send_data_wait();
                return;
           
           }
       }

        ptr =  (uint8_t *)strstr((const char *)&app_uart_host_data.rx_buff[0],"$E,CON");
       if (ptr!=NULL)
       {
           EVSE_Status.chager_on_cmd=1;
           //VIVI 
           charger_on();
                EVSE_Status.is_charger_on=1;

           app_uart_host_print("$OK@");
           app_uart_iso_print("Host: rec CON\n");
           
           //RELAY_MAIN_R1_Set();
           
           return;
       }
        ptr =  (uint8_t *)strstr((const char *)&app_uart_host_data.rx_buff[0],"$E,COFF");
       if (ptr!=NULL)
       {
           EVSE_Status.chager_on_cmd=0;
           app_uart_host_print("$OK@");
           app_uart_iso_print("Host: rec COFF\n");
            charger_off();
                        EVSE_Status.is_charger_on = 0;
                        EVSE_Status.chager_on_cmd=0;
                        cp_chg_status = CP_CHG_IDLE;
                        cp_stop_pwm();
           return;
       }
     
        
    
    
    
}


void rfid_process(void)
{
    if(rfid_is_detected==0)
    {
        rfid_is_detected=1;
        
        if(EVSE_Status.is_charger_on==0)
        {
            EVSE_Status.chager_on_cmd=1;
               //VIVI 
            charger_on();
            EVSE_Status.is_charger_on=1;
        
        }
        else
        {
                        charger_off();
                        EVSE_Status.is_charger_on = 0;
                        EVSE_Status.chager_on_cmd=0;
                        cp_chg_status = CP_CHG_IDLE;
                        cp_stop_pwm();
        }
            rfid_timeout_in_sec=10;
        
        
    }


}

void rfid_check_timeout_10sec(void)
{
    if(rfid_is_detected==1)
    {
        if(rfid_timeout_in_sec>0)
        {
        rfid_timeout_in_sec--;
        
        }
        else
        {
        rfid_is_detected=0;
    
        }
    }
}