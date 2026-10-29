/*© [2025] Microchip Technology Inc. and its subsidiaries
 
Subject to your compliance with the terms and conditions of the reference design license agreement accompanying this software, 
you may use this Microchip software and any derivatives exclusively with Microchip products. You are responsible for complying with 
third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software. 
SOFTWARE IS ?AS IS.? NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, 
MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE 
DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, 
IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE. */

/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 * 
 * BLE UART is Using SERCOM5 P2 & P3
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>
#include "time.h"
#include "definitions.h"                // SYS function prototypes
#include "global.h"
#include "iso_uart.h"
#include "chg_mem_data.h"
#include "app_atm90e26.h"
#include "cp_pp.h"
#include "ble_uart.h"
#include "ble_app.h"


uart_params app_uart_ble_data;

void usart_bleTX_callback(uintptr_t context) {
    app_uart_ble_data.txdone = 1;


}

void usart_bleRX_callback(uintptr_t context) {
    switch (app_uart_ble_data.rxstatus) {
        case RX_IDEAL:
        {
        }
        break;
        case RX_BYTE1_REC:
        {
            SERCOM5_USART_Read(&app_uart_ble_data.rx_buff[1], 63);
            app_uart_ble_data.rxstatus = RX_WAIT;
            app_uart_ble_data.rx_overflow = MS_100_WAIT;

        }
        break;
        case RX_COMPLETE:
        {
        }
        break;
        case RX_WAIT:
        {
            app_uart_ble_data.rx_overflow = 0;
            app_uart_ble_data.rxbufflen = 64;
            app_uart_ble_data.rxstatus = RX_COMPLETE;

        }
        break;
        default: break;
    }
}

void app_uart_ble_init(void) {

    SERCOM5_USART_WriteCallbackRegister(usart_bleTX_callback, 0);
    SERCOM5_USART_ReadCallbackRegister(usart_bleRX_callback, 0);
    app_uart_ble_data.rxstatus = RX_IDEAL;

    app_uart_ble_data.txdone = 1;
}

void app_uart_ble_send_data(void) {
    app_uart_ble_data.txdone = 0;
    SERCOM5_USART_Write(app_uart_ble_data.tx_buff, app_uart_ble_data.txbufflen);
    while (app_uart_ble_data.txdone < 1);
}

void app_uart_ble_send_data_wait(void) {
    uint8_t i;
#ifdef DEBUG_BLE

    switch(charger.arai_status){
        case 0:
            app_uart_iso_print("         IDLE\n");
            break;
        case 1:
            app_uart_iso_print("         INTIALIZATION\n");
            break;
        case 2:
            app_uart_iso_print("         AUTHORIZATION\n");
            break;
        case 3:
                app_uart_iso_print("     CHARGING...  \n");
            break;
        case 4:
            app_uart_iso_print("         CHARGING END \n");
            break;
        case 5:
            app_uart_iso_print("         ERROR        \n");            
            break;            
    }
#endif    
    SERCOM5_REGS->USART_INT.SERCOM_INTENCLR = (uint8_t) SERCOM_USART_INT_INTFLAG_DRE_Msk;

    for (i = 0; i < app_uart_ble_data.txbufflen; i++) {
        while (((SERCOM5_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) != SERCOM_USART_INT_INTFLAG_DRE_Msk));
        SERCOM5_REGS->USART_INT.SERCOM_DATA = app_uart_ble_data.tx_buff[i];
    }
}


void app_uart_ble_print(char *msg) {
    app_uart_ble_data.txbufflen = sprintf((char *) app_uart_ble_data.tx_buff, "%s", msg);
    app_uart_ble_send_data_wait();

}

void app_uart_ble_receive_data_start(void) {
    SERCOM5_USART_Read(app_uart_ble_data.rx_buff, 1);
}

void app_uart_ble_rx_task(void) {
    switch (app_uart_ble_data.rxstatus) {
        case RX_IDEAL:
        {
            app_uart_ble_receive_data_start();
            app_uart_ble_data.rxstatus = RX_BYTE1_REC;
        }
        break;
        case RX_BYTE1_REC:
        {
        }
        break;
        case RX_COMPLETE:
        {
            if (ble_data.process_ble_data == 0) {
                memcpy(ble_data.rx_buff, app_uart_ble_data.rx_buff, app_uart_ble_data.rxbufflen);
                ble_data.rxbufflen = app_uart_ble_data.rxbufflen;
                ble_data.process_ble_data = 1;

                app_uart_ble_receive_data_start();
                app_uart_ble_data.rxstatus = RX_BYTE1_REC;
            }

            app_uart_iso_print("BLE:Rec Data:\n");
            memcpy(app_uart_iso_data.tx_buff,app_uart_ble_data.rx_buff,app_uart_ble_data.rxbufflen);
            app_uart_iso_data.txbufflen = app_uart_ble_data.rxbufflen;
            app_uart_iso_send_data_wait();
        }
        break;
        case RX_WAIT:
        {
        }
        break;
        default: break;
    }

}

void app_uart_ble_rx_overflow_fun(void) {
    if (app_uart_ble_data.rx_overflow > 0) {
        app_uart_ble_data.rx_overflow--;
        if ((app_uart_ble_data.rx_overflow == 0) &&(app_uart_ble_data.rxstatus == RX_WAIT)) {
            app_uart_ble_data.rxbufflen = SERCOM5_USART_ReadCountGet() + 1;
            app_uart_ble_data.rxstatus = RX_COMPLETE;
            SERCOM5_USART_ReadAbort();
        }
    }
}





/* *****************************************************************************
 End of File
 */
