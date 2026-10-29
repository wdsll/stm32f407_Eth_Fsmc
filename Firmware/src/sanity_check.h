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

#ifndef _SANITY_CHECK_H    /* Guard against multiple inclusion */
#define _SANITY_CHECK_H


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
    
  

typedef struct
{
    volatile bool check_status;
    volatile bool check_status_1;
    volatile bool PEN_fault_status;
    volatile bool RCD_fault_status;
    volatile bool WELD_fault_status;
    volatile bool EMG_SWITCH_fault_status;
    
    volatile uint32_t weld_detect_period;
    volatile uint32_t weld_detect_freq;
    volatile bool tcc_buffer_ready;
    
    volatile bool EARTH_presense_status;
    volatile uint16_t earth_detect_period;
    volatile uint16_t earth_detect_freq;
    volatile bool tc_buffer_ready;
    
            
}fault_params;    
    
extern fault_params fault; 
uint8_t check_PE_presense(void);
void init_fault_det(void);
void check_weld_detect(void);
void check_earth_detect(void);
void fault_check_task(void);
void EMG_SWITCH_Fault_Handler(uintptr_t context);
    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
