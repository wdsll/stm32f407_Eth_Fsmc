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

#ifndef _GLOBAL_H    /* Guard against multiple inclusion */
#define _GLOBAL_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files            
 * SERCOM1 - ISOLATED UART
 * SERCOM2 - BLE UART
 * SERCOM0 - METERING UART
 * SERCOM3 - I2C
 * 
 *                                         */
/* ************************************************************************** */
/* ************************************************************************** */



/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif


#define MS_100_WAIT 10    
#define MS_200_WAIT 20    
#define MS_500_WAIT 50    
#define MS_1000_WAIT 100        
#define MS_50_WAIT  5
#define SEC_60_WAIT 6000

#define  CR_CHAR 13

#define  LF_CHAR 10

#define YEAR_OFFSET 128
    
#define APP_MODE_CALIBRATION    1
#define APP_MODE_APP            0
    
//Common    
typedef union 
{
	unsigned int int_value;
    int signed_value;
	struct{
		uint8_t byteL;//lsb
		uint8_t byteH;//msb
    };
}uint16_type;

typedef union long_float_ref
{
	float float_var;
	unsigned long long_var;
	struct{
        uint16_t word1; //lsb
        uint16_t word2; //msb
    };
    
    struct{
        int int1;
        int int2;
    };
    
    struct{
		uint8_t byte1;//LSB
		uint8_t byte2;
		uint8_t byte3;
		uint8_t byte4;//MSB
	};
    
	
}type_float_n_long;

//UART Related Parameters
 typedef enum{
    RX_IDEAL=0,
    RX_BYTE1_REC,
    RX_WAIT,
    RX_COMPLETE,
    RX_OVERFLOW,
    RX_TIMEOUT
}rxbuff_sta;    

typedef struct
{
    uint8_t buff[32];
    uint8_t buff_len;
    uint8_t process_buff;
}buffer_t;

typedef struct
{
    uint8_t tx_buff[100];
    uint8_t rx_buff[64];
    volatile uint8_t txbufflen;
    volatile uint8_t txdone;
    volatile uint8_t rxbufflen;
    volatile rxbuff_sta rxstatus;
    volatile uint16_t rx_overflow;
    volatile uint8_t rx_overflow_flag;
}uart_params;


//END of uart Related parameters    

typedef struct
{
    uint8_t chager_on_cmd;
    uint8_t is_charger_on;
   // uint8_t button_charger_on=0;
    
}charger_status;

extern charger_status EVSE_Status;
extern  uint8_t button_charger_on;
extern uint8_t button_charger_off;

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
