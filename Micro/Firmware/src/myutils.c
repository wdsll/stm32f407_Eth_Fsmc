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
#include "myutils.h"
buzzer_t buzzer;

uint16_t calculate_cks(uint8_t len,uint8_t *buff)
{
    uint8_t i;
    uint16_t ckss;
    ckss=0;
    for(i=0;i<len;i++)
    {
        ckss = ckss + *buff++;
    }
    return ckss;
}

void charger_on(void)
{
    //RELAY_MAIN_R1_Set();
    TCC0_PWM24bitDutySet (1,3599U);
    LED_CHG_Set();
}

void charger_off(void)
{
    //RELAY_MAIN_R1_Clear();
    TCC0_PWM24bitDutySet (1,0);
    LED_CHG_Clear();
}
void earth_on(void)
{
    RELAY_EARTH_R2_Set();
   // LED_CHG_Set();
}

void earth_off(void)
{
    RELAY_EARTH_R2_Clear();
    //LED_CHG_Clear();
}

void buzzer_int(void)
{
    buzzer.buzzer_flag=0;
    buzzer.buzzer_time =0;
    buzzer.buzzer_timeset=0;
    buzzer.buzzer_tick=0;
}
void buzzer_modeE(void)
{
    buzzer.buzzer_flag=1;
    buzzer.buzzer_timeset=2000;
    buzzer.buzzer_time =buzzer.buzzer_timeset;
    buzzer.buzzer_tick=10;
                TCC2_TimerStart();

    
}

void buzzer_modeN(void)
{
     buzzer.buzzer_flag=1;
    buzzer.buzzer_timeset=500;
    buzzer.buzzer_time =buzzer.buzzer_timeset;
    buzzer.buzzer_tick=5;
                TCC2_TimerStart();
    
}

/* *****************************************************************************
 End of File
 */
