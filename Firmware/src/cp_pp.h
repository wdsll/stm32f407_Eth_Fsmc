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
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _CP_PP_H    /* Guard against multiple inclusion */
#define _CP_PP_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    CP_CHG_IDLE=0,
    CP_CHG_WAIT_PP,
    CP_CHG_WAIT_FOR_EVCONN,
    CP_CHG_EV_CONNECTED,
    CP_CHG_EV_MONITOR,
    CP_CHG_EV_CHARGING,
    CP_CHG_EV_VENTILATION_REQ,
    CP_CHG_ERROR
}cp_chg_sta;   

typedef struct
{
    uint16_t cp_result;
    uint16_t cp_result_at_pwm[2];
    uint8_t cp_polarity;
    uint8_t cp_ready;
    uint8_t cp_result_ready;
    uint8_t cp_timekeeper;

}cp_adc_t;

extern volatile cp_adc_t cp_adc;    
extern volatile cp_chg_sta cp_chg_status;    
void cp_start_pwm(void);
void cp_chg_init(void);
void cp_test_cp_signal_value(void);
void cp_stop_pwm(void);
void cp_change_duty_cycle(uint16_t duty_t);
cp_chg_sta adc_read_cp(void);
void cp_task(void);
void check_ev_connection_status(void);


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
