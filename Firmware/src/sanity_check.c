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
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */
#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include "definitions.h"                // SYS function prototypes
#include "time.h"
#include "global.h"
#include "i2c_eeprom.h"
#include "iso_uart.h"
#include "app_atm90e26.h"
#include "myutils.h"
#include "ble_app.h"
#include "ble_uart.h"
#include "afe_calib_mem.h"
#include "cp_pp.h"
#include "sanity_check.h"

#define DEBUG_EARTH
#define DEBUG_EARTH1

#define RCD_FAULT_STATE 0
#define RCD_NORMAL_STATE 1
#define EMG_SWITCH_PRESS_STATE 0
#define EMG_SWITCH_NOT_PRESS_STATE 1
#define PEN_NORMAL_STATE 0
#define PEN_FAULT_STATE 1

fault_params fault= {.PEN_fault_status = false,
    .check_status = false,
    .check_status_1 = false,
    .WELD_fault_status=false,
    .EMG_SWITCH_fault_status = false,
    .EARTH_presense_status= false,
    .RCD_fault_status = false};



void RCD_Fault_Handler(uintptr_t context)
{
    if(RCD_INT_Get()== RCD_FAULT_STATE)
    {
        fault.RCD_fault_status = true;
    }

}
void EMG_SWITCH_Fault_Handler(uintptr_t context)
{
    
    if(EMG_OFF_INT_Get()== EMG_SWITCH_PRESS_STATE)
    {
        fault.EMG_SWITCH_fault_status = true;
    }

}

void PEN_Fault_Handler(uintptr_t context)
{
    if(PEN_DET_INT_Get()== PEN_FAULT_STATE)
    {
        fault.PEN_fault_status = true;
    }

}

//TCC1
void WELD_capture_handler(uint32_t status, uintptr_t context)
{
   if((status & TCC_INTFLAG_MC1_Msk)== TCC_INTFLAG_MC1_Msk) 
   {
       fault.tcc_buffer_ready = true;
   }
    
}

//TC5
void EARTH_capture_handler(unsigned char status, uintptr_t context)
{
   if((status & TC_CAPTURE_STATUS_CAPTURE1_READY)== TC_CAPTURE_STATUS_CAPTURE1_READY) 
   {
       fault.tc_buffer_ready = true;
   }
}

void init_fault_det(void)
{
    
    EIC_CallbackRegister(EIC_PIN_1,RCD_Fault_Handler,0);
    EIC_CallbackRegister(EIC_PIN_2,EMG_SWITCH_Fault_Handler,0);
    EIC_CallbackRegister(EIC_PIN_6,PEN_Fault_Handler,0);
    
    TCC1_CaptureCallbackRegister(WELD_capture_handler,0);
    TC5_CaptureCallbackRegister(EARTH_capture_handler,0);
    
    TCC1_CaptureStart();
    TC5_CaptureStart();
}

void check_weld_detect(void)
{
    if(fault.tcc_buffer_ready)
    {
        fault.weld_detect_period = TCC1_Capture24bitValueGet(TCC1_CHANNEL1);
        fault.weld_detect_freq = (TCC1_CaptureFrequencyGet()/fault.weld_detect_period);
        
        if(((30<fault.weld_detect_freq)&&(fault.weld_detect_freq<70))&&(EVSE_Status.is_charger_on==0))
        {
            fault.WELD_fault_status = true;
            
        }
        else
        {
            fault.WELD_fault_status = false;
        }
        fault.tcc_buffer_ready = false;
    }

}

void check_earth_detect(void)
{
    if(fault.tc_buffer_ready)
    {
        fault.earth_detect_period = TC5_Capture16bitChannel0Get();
        fault.earth_detect_freq = (TC5_CaptureFrequencyGet()/fault.earth_detect_period);
    
        if(fault.earth_detect_freq>0)
        {
            fault.EARTH_presense_status = true;    
        
        }
        else
        {
            fault.EARTH_presense_status = false;    
        
        }
            
        fault.tc_buffer_ready = false;
    }

}

void fault_check_task(void)
{

    if(fault.EMG_SWITCH_fault_status || fault.RCD_fault_status||fault.PEN_fault_status || fault.WELD_fault_status)
    {
        if(fault.EMG_SWITCH_fault_status)
        {
            app_uart_iso_print("Emergency Button Press\n");
            LED_TRIP_Set();
        }
        if(fault.PEN_fault_status)
        {
            app_uart_iso_print("\n PEN Fault\n"); // PEN FAULT THRESHOLD IS 30mA (HARDWARE CONFIGURE)
        }
        if(fault.RCD_fault_status) //RCD THRESHOLD IS 30mA (HARDWARE CONFIGURE)
        {
            app_uart_iso_print("\n RCD Fault\n");
        }
        if( fault.WELD_fault_status) 
        {
             app_uart_iso_print("\n Weld detected\n");
        }
        while(1)                  // IF ANY FAULT OCCURED, CODE HALTS HERE
        {
        charger_off();            // MAIN RELAY OFF
        earth_off();              //  EARTH RELAY OFF
        EVSE_Status.is_charger_on =0;
        buzzer_modeE();          //BUZZER ON
        LED_FLT_Toggle();        //FAULT LED TURN ON

        }
    }
}

uint8_t check_PE_presense(void)
{
    uint8_t temp_t;
    for(temp_t=0;temp_t<100;temp_t++)
    {
        check_earth_detect();
        if(fault.EARTH_presense_status)
        {
            return 1;
        }
        SYSTICK_DelayMs(1);
        
    }
    
    return 0;
}


/* *****************************************************************************
 End of File
 */
