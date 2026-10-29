/*******************************************************************************
  Timer/Counter(TCC1) PLIB

  Company
    Microchip Technology Inc.

  File Name
    plib_TCC1.c

  Summary
    TCC1 PLIB Implementation File.

  Description
    This file defines the interface to the TCC peripheral library. This
    library provides access to and control of the associated peripheral
    instance.

  Remarks:
    None.

*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
/* This section lists the other files that are included in this file.
*/
#include "interrupts.h"
#include "plib_tcc1.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data
// *****************************************************************************
// *****************************************************************************

volatile static TCC_CALLBACK_OBJECT TCC1_CallbackObject;
// *****************************************************************************
// *****************************************************************************
// Section: TCC1 Implementation
// *****************************************************************************
// *****************************************************************************

void TCC1_CaptureInitialize( void )
{
    /* Reset TCC */
    TCC1_REGS->TCC_CTRLA = TCC_CTRLA_SWRST_Msk;

    while((TCC1_REGS->TCC_SYNCBUSY & TCC_SYNCBUSY_SWRST_Msk) == TCC_SYNCBUSY_SWRST_Msk)
    {
        /* Wait for Write Synchronization */
    }

    /* Configure prescaler, standby & capture mode */
    TCC1_REGS->TCC_CTRLA = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_PRESCSYNC_PRESC
                                  | TCC_CTRLA_CPTEN0_Msk | TCC_CTRLA_CPTEN1_Msk ;


    TCC1_REGS->TCC_EVCTRL = TCC_EVCTRL_TCEI1_Msk | TCC_EVCTRL_EVACT1_PWP | TCC_EVCTRL_MCEI0_Msk | TCC_EVCTRL_MCEI1_Msk;

    /* Clear all interrupt flags */
    TCC1_REGS->TCC_INTFLAG = TCC_INTFLAG_Msk;

    TCC1_CallbackObject.callback_fn = NULL;
    TCC1_REGS->TCC_INTENSET = TCC_INTENSET_MC0_Msk | TCC_INTENSET_MC1_Msk;
    while((TCC1_REGS->TCC_SYNCBUSY) != 0U)
    {
        /* Wait for Write Synchronization */
    }
}


void TCC1_CaptureStart( void )
{
    /* Enable TCC */
    TCC1_REGS->TCC_CTRLA |= TCC_CTRLA_ENABLE_Msk;

    while((TCC1_REGS->TCC_SYNCBUSY & TCC_SYNCBUSY_ENABLE_Msk) == TCC_SYNCBUSY_ENABLE_Msk)
    {
        /* Wait for Write Synchronization */
    }
}

void TCC1_CaptureStop( void )
{
    /* Disable TCC */
    TCC1_REGS->TCC_CTRLA &= ~TCC_CTRLA_ENABLE_Msk;

    while((TCC1_REGS->TCC_SYNCBUSY & TCC_SYNCBUSY_ENABLE_Msk) == TCC_SYNCBUSY_ENABLE_Msk)
    {
        /* Wait for Write Synchronization */
    }
}


void TCC1_CaptureCommandSet(TCC_COMMAND command)
{
    TCC1_REGS->TCC_CTRLBSET = (uint8_t)((uint32_t)command << TCC_CTRLBSET_CMD_Pos);
    while((TCC1_REGS->TCC_SYNCBUSY) != 0U)
    {
        /* Wait for Write Synchronization */
    }
}


uint32_t TCC1_Capture24bitValueGet( TCC1_CHANNEL_NUM channel )
{
    while(((TCC1_REGS->TCC_SYNCBUSY) & (1UL << (TCC_SYNCBUSY_CC0_Pos + (uint32_t)channel))) != 0U)
    {
        /* Wait for Write Synchronization */
    }

    return (TCC1_REGS->TCC_CC[channel] & 0xFFFFFFU);
}


uint32_t TCC1_CaptureFrequencyGet( void )
{
    return (uint32_t)(8000000U);
}

/* Get the current counter value */
uint32_t TCC1_Capture24bitCounterGet( void )
{
    /* Write command to force COUNT register read synchronization */
    TCC1_REGS->TCC_CTRLBSET |= (uint8_t)TCC_CTRLBSET_CMD_READSYNC;

    while((TCC1_REGS->TCC_SYNCBUSY & TCC_SYNCBUSY_CTRLB_Msk) == TCC_SYNCBUSY_CTRLB_Msk)
    {
        /* Wait for Write Synchronization */
    }

    while((TCC1_REGS->TCC_CTRLBSET & TCC_CTRLBSET_CMD_Msk) != 0U)
    {
        /* Wait for CMD to become zero */
    }

    /* Read current count value */
    return TCC1_REGS->TCC_COUNT;
}


/* Register callback function */
void TCC1_CaptureCallbackRegister( TCC_CALLBACK callback, uintptr_t context )
{
    TCC1_CallbackObject.callback_fn = callback;

    TCC1_CallbackObject.context = context;
}

/* Capture interrupt handler */
void __attribute__((used)) TCC1_InterruptHandler( void )
{
    uint32_t status;
    /* Additional local variable to prevent MISRA C violations (Rule 13.x) */
    uintptr_t context;
    context = TCC1_CallbackObject.context;    
    status = TCC1_REGS->TCC_INTFLAG;
    /* clear period interrupt */
    TCC1_REGS->TCC_INTFLAG = TCC_INTFLAG_Msk;
    (void)TCC1_REGS->TCC_INTFLAG;
    if(TCC1_CallbackObject.callback_fn != NULL)
    {
        TCC1_CallbackObject.callback_fn(status, context);
    }
}

  

