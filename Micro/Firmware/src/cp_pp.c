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
#include "cp_pp.h"
#include "myutils.h"
#define DUTY_CYCLE_1PER     40

#define CHARGER_CAPACITY_6A     (10*DUTY_CYCLE_1PER)//25  //10% Duty
#define CHARGER_CAPACITY_12A    (20*DUTY_CYCLE_1PER)//50  //20% Duty
#define CHARGER_CAPACITY_18A    (30*DUTY_CYCLE_1PER)//75  //30% Duty
#define CHARGER_CAPACITY_24A    (40*DUTY_CYCLE_1PER)//150//100 //40% Duty
#define CHARGER_CAPACITY_30A    (50*DUTY_CYCLE_1PER) //50% Duty
#define CHARGER_CAPACITY_40A    (66*DUTY_CYCLE_1PER)//66%
#define CHARGER_CAPACITY_48A    (80*DUTY_CYCLE_1PER)//80%
#define CHARGER_CAPACITY_65A    (90*DUTY_CYCLE_1PER)//90%
#define CHARGER_CAPACITY_75A    (94*DUTY_CYCLE_1PER)//94%
#define CHARGER_CAPACITY_80A    (96*DUTY_CYCLE_1PER)//96%


#define STATE_A     3300  //3363  //EV Not Connected 
#define STATE_B     2850  //2950 EV Connected 9v
#define STATE_C     2500  //2576 EV Charging  6v
#define STATE_D     1800  //88a Ventilation Required 3v
#define STATE_FAULT 1200  //800 


volatile uint8_t pp_status;
volatile cp_adc_t cp_adc;
volatile cp_chg_sta cp_chg_status;
volatile cp_chg_sta ev_connection_status;
volatile uint8_t pwm_status=0;
uint8_t evse_capacity;

void adc_res_callback(unsigned char channel, uintptr_t context)
{
    cp_adc.cp_result =  ADC_ConversionResultGet();
   cp_adc.cp_result_at_pwm[cp_adc.cp_polarity] = cp_adc.cp_result; //0 at low side 1 at high side
   cp_adc.cp_result_ready =1;
}

void tcc0_callback(uint32_t status, uintptr_t context)
{
    if(status & TCC_INTFLAG_OVF_Msk)
    {
        cp_adc.cp_polarity=0;
    }
    else if(status & TCC_INTFLAG_MC0_Msk)
    {
        cp_adc.cp_polarity=1;
    
    }
        TC4_TimerStart();

}

void tc4_callback(unsigned char channel,uintptr_t context)
{
    ADC_Enable();
    ADC_ConversionStart();
}

void cp_chg_init(void)
{
     cp_chg_status = CP_CHG_IDLE;
     ADC_CallbackRegister(adc_res_callback,0);
     TCC0_PWMCallbackRegister(tcc0_callback,0);
     TC4_TimerCallbackRegister(tc4_callback,0);
     
     cp_adc.cp_polarity=0;
     
     
}

void cp_start_pwm(void)
{
    cp_change_duty_cycle(CHARGER_CAPACITY_30A);
    TCC0_PWMStart();
    pwm_status = 1;

}

void cp_stop_pwm(void)
{
   TCC0_PWMStop();
   pwm_status = 0;
   cp_adc.cp_polarity=0;
   cp_adc.cp_result_at_pwm[0] = 0; //0 at low side 1 at high side
   cp_adc.cp_result_at_pwm[1] = 0; //0 at low side 1 at high side

}

void cp_change_duty_cycle(uint16_t duty_t)
{
    TCC0_PWM24bitDutySet(TCC0_CHANNEL0,duty_t);
     TCC0_PWMForceUpdate();
}

void cp_test_cp_signal_value(void)
{
    app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$CP H= 0x%x  CP L=0x%x\n",
    cp_adc.cp_result_at_pwm[0],cp_adc.cp_result_at_pwm[1]);
    app_uart_iso_send_data_wait();
}

void cp_task(void)
{
     cp_chg_sta w_status;
    
 switch(cp_chg_status)
    {
        case CP_CHG_IDLE: //no pwm output  
        {
            ADC_Enable();
            ADC_ConversionStart();
            cp_chg_status = CP_CHG_WAIT_FOR_EVCONN;
            cp_adc.cp_result_ready =0;
            cp_adc.cp_timekeeper=100;
            ev_connection_status = CP_CHG_WAIT_FOR_EVCONN;
            app_uart_iso_data.txbufflen = sprintf((char *)app_uart_iso_data.tx_buff,"$CP TASK STARTED\n");
             app_uart_iso_send_data_wait();  
        }break;
        
        case CP_CHG_WAIT_FOR_EVCONN://read adc here 9V
        {
            if(cp_adc.cp_result_ready==1)
            {
               w_status = adc_read_cp();
               cp_adc.cp_result_ready = 0;
                ev_connection_status = w_status;
            }
            else
            {
            
                w_status = ev_connection_status;
                switch(w_status)
                {
                    case CP_CHG_WAIT_FOR_EVCONN:
                    {
                        ADC_ConversionStart();
                       
                    }
                        break;
                    case CP_CHG_EV_CONNECTED:
                    {
                        cp_start_pwm();//pwm_status=1;
                        cp_chg_status = CP_CHG_EV_MONITOR;   
                        #ifdef DEBUG_MSG_CP
                            app_uart_iso_print("$CP- EV Connected\n");
                            app_uart_iso_print("$CP- EV Monitoring\n");
                        #endif                        

                    }
                    break;
                    default:
                    {
                        ADC_ConversionStart();
                    
                    }
                    break;
                
                }
        }
            
            
        }break;
  
        case CP_CHG_EV_MONITOR:
        {
            if(cp_adc.cp_ready==1)
            {
                cp_adc.cp_ready=0;
                w_status = adc_read_cp();
                ev_connection_status = w_status;
                switch(w_status)
                {
                    
                    case  CP_CHG_IDLE:
                    {

                            charger_off();
                            EVSE_Status.is_charger_on = 0;
                            EVSE_Status.chager_on_cmd = 0;
                            cp_chg_status = CP_CHG_IDLE;
//#ifdef DEBUG_MSG_CP
                        app_uart_iso_print("$CP- EV Idle\n");
//#endif
                    }break;
                    
                    case  CP_CHG_EV_CONNECTED:
                    {
//                        
//#ifdef DEBUG_MSG_CP
                           EVSE_Status.chager_on_cmd=0;
                            charger_off();
                            EVSE_Status.is_charger_on = 0;
                        
                             app_uart_iso_print("$CP- EVM Connected\n");
//#endif
                        
                    }break;
                    case  CP_CHG_EV_CHARGING:
                    {
                        
//#ifdef DEBUG_MSG_CP   
                        charger_on();
                        app_uart_iso_print("$CP- EV Charging\n");                    
//#endif
                        
                    }break;
                    case  CP_CHG_EV_VENTILATION_REQ:
                    {
                  
//#ifdef DEBUG_MSG_CP

                        charger_on();
                        app_uart_iso_print("$CP- EV Ventilation Req\n");
//#endif
                    
                    }break;
                    case  CP_CHG_ERROR:
                    {
                    
                        charger_off();
                        EVSE_Status.is_charger_on = 0;
                        EVSE_Status.chager_on_cmd=0;
                        cp_chg_status = CP_CHG_IDLE;
//#ifdef DEBUG_MSG_CP
                        app_uart_iso_print("$CP- EV Diode Error\n");
//#endif
                    }break;

                    default:
                        break;
                    
                }    
            }
        }break;
        case CP_CHG_EV_CHARGING: //read 6v
        {}break;
        case CP_CHG_EV_VENTILATION_REQ: //3v
        {}break;
        case CP_CHG_ERROR:
        {
                  charger_off();
                        EVSE_Status.is_charger_on = 0;
                        EVSE_Status.chager_on_cmd=0;
                        cp_chg_status = CP_CHG_IDLE;        
        }break;
        default:
            break;
    
    }  
}

    
    
 cp_chg_sta adc_read_cp(void)
{
    uint16_t adcres;
    uint16_t adcres1;        
    adcres1 =cp_adc.cp_result_at_pwm[1];
    adcres =cp_adc.cp_result_at_pwm[0];
    if(pwm_status==0)//pwm is off then check adc value only not need polarity status
    {
                adcres = cp_adc.cp_result;;
                if((adcres>STATE_B)&&(adcres<STATE_A))
                {
                    cp_start_pwm();
                    return(CP_CHG_EV_CONNECTED);
                }
                else{
                    return(CP_CHG_WAIT_FOR_EVCONN);
                }
            }
             
            else //pwm is on
            {
                
                if(adcres1>STATE_FAULT) //if negative side is above threshold
                {
                    cp_stop_pwm();
                    return(CP_CHG_ERROR);
                }
                
                if(adcres>STATE_A)
                {
                     cp_stop_pwm();
                     return(CP_CHG_IDLE);
                     
                }
                else if((adcres>STATE_B)&&(adcres<STATE_A)) //for state B
                {
                    return(CP_CHG_EV_CONNECTED);
                }
                else if((adcres>STATE_C)&&(adcres<STATE_B))// for state C
                {
                   
                     return(CP_CHG_EV_CHARGING);
                }
                else if((adcres<STATE_C)) //State D
                {
                   
                    return(CP_CHG_EV_VENTILATION_REQ);
                }                
            }           
        return(CP_CHG_ERROR);     
        
}   
    
    
void check_ev_connection_status(void)
{
    switch(ev_connection_status)
    {
        case CP_CHG_WAIT_FOR_EVCONN:
        {
        
            app_uart_iso_print("==EVSE WAITING for CONNECTION==\n");
        }break;
        case CP_CHG_EV_CONNECTED:
        {
            app_uart_iso_print("==EVSE CONNECTED==\n");
        }break;
        case CP_CHG_EV_CHARGING:
        {
            app_uart_iso_print("==EVSE CHARGING==\n");

        }break;
        case CP_CHG_EV_VENTILATION_REQ:
        {
            app_uart_iso_print("==EVSE VENTILATION REQUIRED==\n");
        
        }break;
        case CP_CHG_ERROR:
        {
            app_uart_iso_print("==EVSE DIODE FAULT==\n");
        
        }break;
        default:
            break;
    
    
    }


}
    
    
    
    
    
    
    


/* *****************************************************************************
 End of File
 */
