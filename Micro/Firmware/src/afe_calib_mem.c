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
#include "afe_calib_mem.h"
#include "iso_uart.h"
#include "i2c_eeprom.h"
#include "myutils.h"

#define CALIB_MEM_LEN 14


uint8_t read_AFEcalibration_data(void)
{
    uint8_t *pstruct;
    pstruct = (uint8_t *) &afe_cal_reg;
    if(read_eeprom(METERING_CONFIG_ZONE_START_ADDRESS,CALIB_MEM_LEN))
    {
        memcpy(pstruct,I2CRxData,CALIB_MEM_LEN);
        app_uart_iso_print("\nREAD CALIBRATION DATA:\n");
        
        pstruct =  (uint8_t *)&afe_cal_reg;
        uint8_t i;
        for(i=0;i<CALIB_MEM_LEN;i++)
        {
           const char* str[] = {"Lgain(LSB)","Lgain(MSB)", "Lphi(LSB)","Lphi(MSB)", "Ugain(LSB)","Ugain(MSB)",
           "IgainL(LSB)","IgainL(MSB)", "PoffsetL(LSB)","PoffsetL(MSB)", "QoffsetL(LSB)","QoffsetL(MSB)", "cks(LSB)","cks(MSB)"};

           
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
         "%s= 0x%02x\n",str[i], *pstruct++);        
         app_uart_iso_send_data_wait();

        }
        app_uart_iso_print("====================\n");
      
      

        if(afe_cal_reg.cks == calculate_cks(CALIB_MEM_LEN-2,(uint8_t *)&afe_cal_reg))
        {
            app_uart_iso_print("\nCALIBRATION READ DONE\n");
            return 1;
        }
        else
        {
            app_uart_iso_print("CALIBRATION DATA CURRUPTED\n");
            afe_cal_reg.IgainL =    IGAINL;
                    afe_cal_reg.Lgain= LGAIN;
                    afe_cal_reg.Lphi= LPHI;
                    afe_cal_reg.PoffsetL= POFFSETL;
                    afe_cal_reg.QoffsetL=   QOFFSETL;
                    afe_cal_reg.Ugain= UGAIN;
            write_AFEcalibration_data();
            app_uart_iso_print("CALIBRATION SET TO DEFAULT VALUE\n");
            return 0;
        }
    }
    return 0;
}

uint8_t write_AFEcalibration_data(void)
{
    uint8_t *pstruct;
    pstruct = (uint8_t *) &afe_cal_reg;
    afe_cal_reg.cks = calculate_cks(CALIB_MEM_LEN-2,(uint8_t *)&afe_cal_reg);
    
    uint8_t i;
     app_uart_iso_print("\nWRITE CALIBRATION DATA: \n");
    for(i=0;i<CALIB_MEM_LEN;i++)
    {
        const char* str[] = {"Lgain(LSB)","Lgain(MSB)", "Lphi(LSB)","Lphi(MSB)", "Ugain(LSB)","Ugain(MSB)",
           "IgainL(LSB)","IgainL(MSB)", "PoffsetL(LSB)","PoffsetL(MSB)", "QoffsetL(LSB)","QoffsetL(MSB)", "cks(LSB)","cks(MSB)"};

        app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,
     "%s= 0x%02x\n",str[i],*pstruct++);        
     app_uart_iso_send_data_wait();
        
    }
    pstruct = (uint8_t *) &afe_cal_reg;
    write_eeprom(METERING_CONFIG_ZONE_START_ADDRESS,pstruct,CALIB_MEM_LEN);
    SYSTICK_DelayMs(5);
    return 0;
}


