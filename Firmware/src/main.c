/*© [2025] Microchip Technology Inc. and its subsidiaries
 
Subject to your compliance with the terms and conditions of the reference design license agreement accompanying this software, 
you may use this Microchip software and any derivatives exclusively with Microchip products. You are responsible for complying with 
third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software. 
SOFTWARE IS ?AS IS.? NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, 
MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE 
DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, 
IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE. */

  /*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

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
#include "host_uart.h"
// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
uint8_t time_100ms=0;
volatile uint16_t timercnt1sec;
volatile uint8_t timer_flag;
volatile uint8_t frame_counter;
volatile uint8_t app_mode;
charger_status EVSE_Status;
uint8_t button_charger_on=0;
uint8_t button_charger_off=0;
uint8_t button_on_flag=0;
uint8_t button_off_flag=0;
uint8_t button_on_flag_loop=0;
uint8_t button_off_flag_loop=0;
uint8_t button_not_on_flag=0;
void print_afe_data(void);

void user_timer10ms(uintptr_t context)
{
    if(time_100ms>0)
    {
        time_100ms--;
        if(time_100ms==0)
        {
            time_100ms = 10;
        }
    
    }
    if(timercnt1sec>0)
    {
        timercnt1sec--;
        if(timercnt1sec==0)
        {
            timer_flag = 1;
            timercnt1sec=MS_1000_WAIT;
        
        }
    
    }
     if (afe_wait_for_read>0)
    {
        afe_wait_for_read--;
    }
    if(afe_wait_for_read_energy>0)
    {
        afe_wait_for_read_energy--;
    }
    
   
    if(cp_adc.cp_timekeeper>0)
    {
        cp_adc.cp_timekeeper--;
        if(cp_adc.cp_timekeeper==0)
        {
            cp_adc.cp_ready = 1;
            cp_adc.cp_timekeeper = 250;
        }
    }
    i2c_timeout();
    AFE_uart_rx_overflow_fun();
    app_uart_iso_rx_overflow_fun();
    app_uart_ble_rx_overflow_fun();
    app_uart_host_rx_overflow_fun();
}

void buzzer_tomer(uint32_t status,uintptr_t context)
{

    if(buzzer.buzzer_flag==1)
    {
        BUZZER_Toggle();
        
        
        if(buzzer.buzzer_tick>0)
        {
            if(buzzer.buzzer_time>0)
            {
                buzzer.buzzer_time--;
                
           
            }
            else
            {
                buzzer.buzzer_time = buzzer.buzzer_timeset;
                buzzer.buzzer_tick--;
                
            }
        
        }
        else
        {
            buzzer.buzzer_flag=0;
            BUZZER_Clear();
            TCC2_TimerStop();
        }
    
    }
}

void user_timer_1ms(TC_CAPTURE_STATUS status, uintptr_t context)
{
       
}

void buttons_task()
{
    if ( CHARGER_ON_Get()== 0)    
    {
        
    button_charger_on =1;
	TCC0_PWM24bitDutySet (1,3599U); // 1 is channel no since PA09 is configured with channel 1 of TCC0, 
    LED_CHG_Set();
    }
    if ( CHARGER_OFF_Get()== 0)    
    {
    button_charger_off=1;
    TCC0_PWM24bitDutySet (1,0); 
    LED_CHG_Clear();
    }

}

void init_user_timer(void)
{
    SYSTICK_TimerCallbackSet(user_timer10ms,0);
    SYSTICK_TimerStart();
    time_100ms = 10;
    
    TC3_TimerCallbackRegister(user_timer_1ms,0);
    TC3_TimerStart();
    
    TCC2_TimerCallbackRegister(buzzer_tomer,0);
    
    
}

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    init_user_timer();
    init_fault_det();
    timer_flag=0;
    timercnt1sec=MS_1000_WAIT;
    frame_counter=0;
    TCC0_PWM24bitDutySet (1,0); //RELAY_MAIN_R1_Clear(); // MAKE SURE THAT RELAY IS TURN OFF AFTER POWER UP
    app_uart_iso_init();
    app_uart_host_init();
    {
    app_uart_iso_print("\r\n==EV Charger Reference DESIGN==\n");
    }
   
   EMG_SWITCH_Fault_Handler(0);  // CHECKING WHETHER EMERGENGY SWITCH IS PRESSED AT POR. IF PRESSED THE CODE HALTS HERE.
   if(fault.EMG_SWITCH_fault_status==true)   
   {
       app_uart_iso_print("\r\n==EMERGENCY SWITCH PRESSED AT POR==\n");
       while(1)
       {
       LED_TRIP_Set();
       buzzer_modeE();
       }
   }
   
   if(check_PE_presense())         //EARTH DETECTION CHECK AND TURN ON EARTH RELAY 
    {
        app_uart_iso_print("\r\n==EARTH_DETECTED==\n");
        earth_on();
        
    }
    else
    {
        app_uart_iso_print("\r\n==EARTH NOT PRESENSE==\n");
        app_uart_iso_print("\r\n==PLEASE CONNECT EARTH==\n");
        buzzer_modeE();
        earth_off();
        LED_FLT_Set();
        while(1);           // IF EARTH NOT DETECTED CODE HALTS HERE
    }
     
    AFE_Task();

    ble_init();
    app_uart_ble_init();
    eeprom_init();
   
    app_uart_iso_print("\r\n==LET START==\n");

    app_mode = APP_MODE_APP;
    
    AFE_Load_Cal_Data();          // LOADING CALCULATED CALIBRATION VALUES TO THE MACROS DEFINED. CALIBRATION METHOD CAN BE FOUND IN THE AN46102 APP NOTE

    write_AFEcalibration_data(); // WRITING CALIBRATION VALUES TO EEPROM
    SYSTICK_DelayMs(10);
    read_AFEcalibration_data();  // READING CALIBRATION VALUES FROM THE EEPROM FOR VERIFICATION
    cp_chg_init();
  
    while ( true )
    {            
        app_uart_iso_rx_task();  // // DEBUG UART TASK
      if(calib_buff.process_buff==1) //CALIBRATION OF ENERGY METER THROUGH DOCKLIGHT COMMAND
        {
            process_calib_cmd();
            calib_buff.process_buff=0;
        }
        if(app_mode==APP_MODE_CALIBRATION)
        {
            AFE_process_calibration();
        }
        else if(app_mode == APP_MODE_APP)
        { 
           
            app_uart_ble_rx_task();   // BLE UART TASK
           
            cp_task();                // CP-PP TASK
            
            AFE_Task();               // ENERGY METER TASK
            
            fault_check_task();       // FAULT DETECTION TASK: RCD, PEN, EMERGENCY PRESS, WELD
            
/**********************************************************
DISCLAIMER: IF BUTTON OR BLUETOOTH APP IS CHOSEN FOR RELAY CONTROL, 
PLEASE BE ADVISED THAT HIGH VOLTAGE WILL BE PRESENT AT THE OUTPUT CONNECTOR.
**********PLEASE ENSURE SAFETY*******************/
            
/*UNCOMMENT THE BELOW LINE FOR CONTROLLLING THE RELAY THROUGH BLUETOOTH APP */   
            
            //ble_task();             //  BLUETOOTH APPLICATION TASK TO TURN ON/OFF THE MAIN RELAY THROUGH APP. 
           
/*UNCOMMENT THE BELOW PART OF THE CODE TO TURN ON/OFF THE RELAY USING CHARGER_ON/CHARGER_OFF BUTTONS IN THE BOARD */
            
      /*  if (CHARGER_ON_Get() == 0)  // CHARGER ON BUTTON PRESS CHECK TO TURN ON MAIN RELAY. 
        {
        while(CHARGER_ON_Get() == 0)
        {
            
        }
        
        if (!button_charger_on)
        {
            button_charger_on = 1;
            button_charger_off = 0;

            RELAY_MAIN_R1_Set();  // Enable relay
            LED_CHG_Set();        // Turn on charger LED
           
        }
    }


    if (CHARGER_OFF_Get() == 0)  // CHARGER OFF BUTTON PRESS CHECK TO TURN OFF MAIN RELAY
    {
         while(CHARGER_OFF_Get() == 0)
        {
            
        }
        
        if (!button_charger_off)
        {
            button_charger_off = 1;
            button_charger_on = 0;
            RELAY_MAIN_R1_Clear();  // Disable relay
            LED_CHG_Clear();        // Turn off charger LED
            
        }
    } */
          
        }
           
        // print_afe_data();  //PRINTING ENERGY METER REGISTER VALUES IN DOCKLIGHT
            
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
    }
    /* Execution should not come here during normal operation */
//  }
    
    return ( EXIT_FAILURE );
}

void print_afe_data(void)
{
      if(afe_read_register_ready==1)
        {
          afe_read_register_ready =0;
            if(timer_flag==1)
            {
                timer_flag = 0;
                //cp_test_cp_signal_value();
                /*Checking Calender Functionality*/
                /*
                RTC_RTCCTimeGet(&charger.app_time);
                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$===========\n");
                app_uart_iso_send_data_wait();

                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$DATE = %d-%d-%04d\n",
                        charger.app_time.tm_mday,charger.app_time.tm_mon,charger.app_time.tm_year-YEAR_OFFSET);
                app_uart_iso_send_data_wait();

                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$TIME = %d-%d-%d\n",
                        charger.app_time.tm_hour,charger.app_time.tm_min,charger.app_time.tm_sec);
                app_uart_iso_send_data_wait();

                app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$===========\n");
                app_uart_iso_send_data_wait();
                  */ 
       
                
                    
                    /*
                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$===========\n");
                    app_uart_iso_send_data_wait();
                    
                    afe_print_read_registers_buff();
                    
                    
                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$VRMS=%3.1f#\n",meter_data.urms.float_var);
                    app_uart_iso_send_data_wait();
                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$IRMS=%2.2f#\n",meter_data.irms.float_var);
                    app_uart_iso_send_data_wait();
                   
                    uint16_t te;
                    te = meter_data.pmean.word1;
                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$ACTIVE PWR=%05dW#\n",te);
                    app_uart_iso_send_data_wait();
                    
                    
                    te = meter_data.smean.word1;
                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$APPARENT PWR=%05dVA#\n",te);
                    app_uart_iso_send_data_wait();

                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$FREQ=%2.2f#\n",meter_data.freq.float_var);
                    app_uart_iso_send_data_wait();
                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$PF=%1.3f#\n",meter_data.pf.float_var);
                    app_uart_iso_send_data_wait();

                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$AT ENERGY=%f#\n",meter_at_energy);
                    app_uart_iso_send_data_wait();
                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$RT ENERGY=%f#\n",meter_rt_energy);
                    app_uart_iso_send_data_wait();

                    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$===========\n");
                    app_uart_iso_send_data_wait();
                    */
                    
            
            }//end of (timer_flag)
        }

}

/*******************************************************************************
 End of File
*/

