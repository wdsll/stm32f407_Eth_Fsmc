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
#include "i2c_eeprom.h"

#define APP_AT24_DEVICE_ADDR             0x50
#define APP_AT24_MEMORY_ADDR             0x00
#define APP_AT24_MEMORY_ADDR1            0x00
#define APP_TRANSMIT_DATA_LENGTH            6
#define APP_ACK_DATA_LENGTH                 1
#define APP_RECEIVE_DUMMY_WRITE_LENGTH      2
#define APP_RECEIVE_DATA_LENGTH             4    




typedef enum
{
    APP_STATE_EEPROM_STATUS_VERIFY,
    APP_STATE_EEPROM_WRITE,
    APP_STATE_EEPROM_WAIT_WRITE_COMPLETE,
    APP_STATE_EEPROM_CHECK_INTERNAL_WRITE_STATUS,
    APP_STATE_EEPROM_READ,
    APP_STATE_EEPROM_WAIT_READ_COMPLETE,
    APP_STATE_VERIFY,
    APP_STATE_IDLE,
    APP_STATE_XFER_SUCCESSFUL,
    APP_STATE_XFER_ERROR

} APP_STATES;

typedef enum
{
    APP_TRANSFER_STATUS_IN_PROGRESS,
    APP_TRANSFER_STATUS_SUCCESS,
    APP_TRANSFER_STATUS_ERROR,
    APP_TRANSFER_STATUS_IDLE,

} APP_TRANSFER_STATUS;

uint8_t I2CTxData[66];
uint8_t I2CRxData[66];

static uint8_t I2Cdatalen;
APP_STATES state = APP_STATE_EEPROM_STATUS_VERIFY;
volatile APP_TRANSFER_STATUS transferStatus = APP_TRANSFER_STATUS_ERROR;
uint8_t ackData = 0;
volatile uint8_t i2ctimeout;



void APP_I2CCallback(uintptr_t context )
{
    APP_TRANSFER_STATUS* transferStatus = (APP_TRANSFER_STATUS*)context;

    if(SERCOM2_I2C_ErrorGet() == SERCOM_I2C_ERROR_NONE)
    {
        if (transferStatus)
        {
            *transferStatus = APP_TRANSFER_STATUS_SUCCESS;
        }
    }
    else
    {
        if (transferStatus)
        {
            *transferStatus = APP_TRANSFER_STATUS_ERROR;
        }
    }
}


void eeprom_init(void)
{
/* Register the TWIHS Callback with transfer status as context */
SERCOM2_I2C_CallbackRegister( APP_I2CCallback, (uintptr_t)&transferStatus );

}


uint8_t write_eeprom(uint16_t addr,uint8_t *dataW,uint8_t datalen)
{
    uint8_t i;
    if(datalen>0)
    {
        I2CTxData[0] = addr>>8;
        I2CTxData[1] = (uint8_t)addr;
        for(i=0;i<datalen;i++)
        {
            I2CTxData[2+i] = *dataW++;
        }
        I2Cdatalen = 2+datalen;
        state = APP_STATE_EEPROM_STATUS_VERIFY;
        transferStatus = APP_TRANSFER_STATUS_ERROR;
        i2ctimeout=10; //100msec
        while(1)
        {
         switch (state)
        {
            case APP_STATE_EEPROM_STATUS_VERIFY:
               /* Verify if EEPROM is ready to accept new requests */
                transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                SERCOM2_I2C_Write(APP_AT24_DEVICE_ADDR, &ackData, APP_ACK_DATA_LENGTH);

                state = APP_STATE_EEPROM_WRITE;
                break;

            case APP_STATE_EEPROM_WRITE:

                if (transferStatus == APP_TRANSFER_STATUS_SUCCESS)
                {
                    /* Write data to EEPROM */
                    transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                    SERCOM2_I2C_Write(APP_AT24_DEVICE_ADDR, &I2CTxData[0], I2Cdatalen);
                    state = APP_STATE_EEPROM_WAIT_WRITE_COMPLETE;
                }
                else if (transferStatus == APP_TRANSFER_STATUS_ERROR)
                {
                    /* EEPROM is not ready to accept new requests. 
                     * Keep checking until the EEPROM becomes ready. */
                    state = APP_STATE_EEPROM_STATUS_VERIFY;
                }
                break;

            case APP_STATE_EEPROM_WAIT_WRITE_COMPLETE:

                if (transferStatus == APP_TRANSFER_STATUS_SUCCESS)
                {
                    /* Read the status of internal write cycle */
                    transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                    SERCOM2_I2C_Write(APP_AT24_DEVICE_ADDR, &ackData, APP_ACK_DATA_LENGTH);
                    state = APP_STATE_EEPROM_CHECK_INTERNAL_WRITE_STATUS;
                }
                else if (transferStatus == APP_TRANSFER_STATUS_ERROR)
                {
                    state = APP_STATE_XFER_ERROR;
                }
                break;

             case APP_STATE_EEPROM_CHECK_INTERNAL_WRITE_STATUS:

                if (transferStatus == APP_TRANSFER_STATUS_SUCCESS)
                {
                    state = APP_STATE_XFER_SUCCESSFUL;
                    return 1;
                }
                else if (transferStatus == APP_TRANSFER_STATUS_ERROR)
                {
                    /* EEPROM's internal write cycle is not complete. Keep checking. */
                    transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                    SERCOM2_I2C_Write(APP_AT24_DEVICE_ADDR, &ackData, APP_ACK_DATA_LENGTH);
                }
                break;

            case APP_STATE_XFER_ERROR:
            {
                //LED2_OFF();
                break;
            }
            
             case APP_STATE_XFER_SUCCESSFUL:
             {
             
             }break;
            default:
                break;
        }            
            
         if(state==APP_STATE_XFER_SUCCESSFUL)
             break;
         
         if(i2ctimeout==0)
         {
             return 0;
         }
        }//end of while(1)
        
        
    }
    return 0;
}


uint8_t read_eeprom(uint16_t addr, uint8_t datalen)
{
    if(datalen>0)
    {
     I2CTxData[0] = addr>>8;
     I2CTxData[1] = (uint8_t)addr;
     
    transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
    i2ctimeout=10;
    SERCOM2_I2C_WriteRead(APP_AT24_DEVICE_ADDR, &I2CTxData[0], APP_RECEIVE_DUMMY_WRITE_LENGTH,  &I2CRxData[0],datalen);
    
    while(1)
    {
         if (transferStatus == APP_TRANSFER_STATUS_ERROR)
         {
             //error
             return 0;
         }
         else if(transferStatus == APP_TRANSFER_STATUS_SUCCESS)
         {
             return 1;
         }
              
         if(i2ctimeout==0)
         {
             //error
             return 0;
         }
    }
    
  
    }
    return 0;
}


void i2c_timeout(void)
{
    if(i2ctimeout>0)
    {
        i2ctimeout--;
    }

}

