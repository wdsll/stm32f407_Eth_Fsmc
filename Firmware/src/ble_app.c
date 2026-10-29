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

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>
#include "time.h"

#include "definitions.h"                // SYS function prototypes
#include "global.h"
#include "iso_uart.h"
#include "chg_mem_data.h"
#include "app_atm90e26.h"
#include "cp_pp.h"
#include "ble_uart.h"
#include "ble_app.h"
#include "myutils.h"
/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#define NO_EEPROM

ble_params_t ble_data;
kirana_params_t charger;
charger_th_vaule charger_th;
schedule_charging_param charger_sch_time;
extern float meter_at_energy;

void ble_init(void) {
        BLE_RESET_Clear();
        SYSTICK_DelayMs(2);
        BLE_RESET_Set();
}



void ble_task(void) {
    if (ble_data.process_ble_data) {
#ifdef DEBUG_BLE
       app_uart_iso_print("\n\n\r APP DATA RX =>\n");
       app_uart_iso_data.txbufflen = to_ascii(ble_data.rx_buff, app_uart_iso_data.tx_buff, ble_data.rxbufflen);
       app_uart_iso_send_data_wait();
#endif       
        k_buff_play();
        ble_data.process_ble_data = 0;
    }

}


void load_on(void) {
    
    EVSE_Status.chager_on_cmd=1;
    charger_on();
}

void load_off(void) {
    
    charger_off();
    EVSE_Status.is_charger_on = 0;
    EVSE_Status.chager_on_cmd=0;
    cp_chg_status = CP_CHG_IDLE;
    cp_stop_pwm();

}

uint16_t cksR, cksC;
void k_buff_play(void) {
    uint16_t i;//cksR, cksC, i;
    uint8_t pkt_type;

    if ((ble_data.rx_buff[K_FRAME_START_BYTE_LOC] == K_FRAME_START_BYTE)&&
            (ble_data.rx_buff[ble_data.rxbufflen - 1] == K_FRAME_END_BYTE)) {

        ble_data.payload_len = ble_data.rx_buff[K_FRAME_DATA_LEN_LOC];
        cksC = 0;
        for (i = 0; i < ble_data.payload_len; i++) {
            ble_data.payload[i] = ble_data.rx_buff[i + K_FRAME_DATA_LOC];
            cksC = cksC + ble_data.payload[i];
        }
        cksR = ble_data.rx_buff[K_FRAME_DATA_LOC + ble_data.payload_len];
        cksR = cksR * 100;
        cksR = cksR + (0xff & ble_data.rx_buff[K_FRAME_DATA_LOC + ble_data.payload_len + 1]);
    
        if (cksC == cksR) {
            pkt_type = ble_data.rx_buff[K_FRAME_PACKETTYPE_LOC];
            
            responce_to_packet((PKT_TYPE) pkt_type);
            
        } else {

        }

    }
}

void responce_to_packet(PKT_TYPE packet_type) {

    switch (packet_type) {
        case PKT_INIT://INITIALIZATION
        {
#ifdef DEBUG_BLE
            
            app_uart_iso_print("\nINITIALIZATION CMD\n");
#endif
            kirana_initialization();
        }
            break;

        case PKT_AUTH://AUTHENTICATION
        {
#ifdef DEBUG_BLE
            app_uart_iso_print("\nAUTHENTICATION CMD\n");
#endif
          
            kirana_authorization();
        }
            break;

        case PKT_CHG://CHARGING ON
        {
#ifdef DEBUG_BLE
            app_uart_iso_print("\nCHARGING ON CMD\n");
#endif
            
            kirana_charging();
        }
            break;

        case PKT_EOCHG://END OF CHARGING
        {
#ifdef DEBUG_BLE
            app_uart_iso_print("\nEND OF CHARGING CMD\n");
#endif
            
            kirana_endofcharging();
        }
            break;

        case PKT_DIAG://DIAGNOSIS
        {
#ifdef DEBUG_BLE
            app_uart_iso_print("\nDIAGNOSTIC CMD\n");
#endif
            
            kirana_diagnosis();
        }
            break;

        case PKT_SLOG://SESSION LOG
        {
            app_uart_iso_print("SESSION LOG CMD received");
            kirana_sessionlog();
        }
            break;

        case PKT_TIME://TIME
        {
#ifdef DEBUG_BLE
            app_uart_iso_print("\nTIME CMD received\n");
#endif
            kirana_timesync();
        }
            break;

        case PKT_OPR://OPRATOR REGISTRATION
        {
            app_uart_iso_print("----------------------------> OPREG CMD");
            kirana_operator_registration();
        }
            break;
        case PKT_THC://THRESHOLD CONFIGURATION
        {
            app_uart_iso_print("----------------------------> THCONFIG CMD");
            kirana_threshold_config();
        }
            break;


        case PKT_OWR://OWNER REGISTRATION
        {
            app_uart_iso_print("----------------------------> OWNREG CMD");
            kirana_owner_registration();
        }
            break;

        case PKT_SCHCHG://SCHEDULE CHARGING
        {
            app_uart_iso_print("----------------------------> SCHCHG CMD");
#ifdef DEBUG_BLE
            app_uart_iso_print("\nSCHEDULE CHARGING CMD\n");
#endif
            kirana_schdule_charge_request();
        }
            break;

        case PKT_OPIDREQ://OPERATOR ID REQUEST
        {

            app_uart_iso_print("----------------------------> OPID CMD");
            kirana_op_id_request();
        }
            break;

        default:
            break;


    }

}

/*
 24   1A0416112600 08   05          45303033 01  CKS          23
 SB   TIME         PKT  DATALEN     OPERATOR ID        CHECKSUM     EB
  
 */
void kirana_operator_registration(void) {
    uint16_t cks;
    uint8_t i;


    reg_operator_id(&ble_data.payload[0]);
    init_charger();
    fill_header_with_time();


    app_uart_ble_data.tx_buff[1] = PKT_OPR; //SS
    app_uart_ble_data.tx_buff[2] = 1; // BYTES OF DATA 
    app_uart_ble_data.tx_buff[3] = 1; //rESPONCE OK 

    cks = 0;

    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 1); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }

    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 1;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);

    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;

    app_uart_ble_send_data_wait();

#ifdef DEBUG_L1
    //app_uart_iso_print("\n\r LEV RESPONCE OP REG => ");
    //app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    //app_uart_iso_send_data_wait();
#endif   
}

void kirana_owner_registration(void) {
    uint16_t cks;
    uint8_t i;

    reg_owner_id(&ble_data.payload[0]);

    fill_header_with_time();

    app_uart_ble_data.tx_buff[1] = PKT_OWR; //SS
    app_uart_ble_data.tx_buff[2] = 1; //5 BYTES OF DATA 

    cks = 0;
    app_uart_ble_data.tx_buff[3] = 1; //1 operator registered.

    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 1); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }
    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 1;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;

    app_uart_ble_send_data_wait();

#ifdef DEBUG_L1
    //app_uart_iso_print("\n\r LEV RESPONCE EV REG => ");
    //app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    //app_uart_iso_send_data_wait();
#endif   
}

/*
 
 */
void kirana_initialization(void) {
    
    uint16_t cks;
    uint8_t i;
   // uint16_type jj;
    uint8_t match_success;

#ifdef NO_EEPROM
    match_success = 1;
    if (match_success) {
        // Perform some action
    }
#else

#endif   

    charger.operator_id[0] = ble_data.payload[0];
    charger.operator_id[1] = ble_data.payload[1];

    //fill_header_with_time(); //only adding start of header
    
    app_uart_ble_data.tx_buff[0] = K_FRAME_START_BYTE;
    //set packet type
   
    app_uart_ble_data.tx_buff[1] = PKT_INIT; //PKT TYPE
    //size of payload
    
    app_uart_ble_data.tx_buff[2] = 5; //BYTES OF DATA

    if (charger.arai_status == ARAI_IDLE) {
        charger.arai_status = ARAI_INTIALIZATION;
       
        app_uart_ble_data.tx_buff[3] = charger.arai_status;
    } else {
        charger.arai_status = ARAI_AUTHORIZATION;
       
        app_uart_ble_data.tx_buff[3] = charger.arai_status;
    }
    
    app_uart_ble_data.tx_buff[4] = (ACLEV_ID_NUMBER >> 24)&0xFF;
    app_uart_ble_data.tx_buff[5] = (ACLEV_ID_NUMBER >> 16)&0xFF;
    app_uart_ble_data.tx_buff[6] = (ACLEV_ID_NUMBER >> 8)&0xFF;
    app_uart_ble_data.tx_buff[7] = (ACLEV_ID_NUMBER)&0xFF;
    
    //calculate checksum
    cks = 0;
    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 5); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }
    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 5;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (cks/100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (cks%100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;
     
    app_uart_ble_send_data_wait();    
    
    
    
    
#ifdef DEBUG_L1
    app_uart_iso_print("\n\r LEV RESPONCE INIT => ");
    app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    app_uart_iso_send_data_wait();
#endif    
}

/*
 Authorization Command
 SeesionID - 0 1 2 3 4 5 6 7 8 9
 Vehicle Type - 10
 Payment Type - 11
 Unit Required - 12 13
 EV Owner ID - 14 15 16 17
 
 */
void kirana_authorization(void) {
    uint16_t cks;
    uint8_t i;
    uint16_type unit;
    uint8_t match_success;
    //adding new session id
    for (i = 0; i < 2; i++) {
        charger.sessionID[i] = ble_data.payload[i];
    }
    charger.session_active = 1;
    charger.ev_type = ble_data.payload[2];
    charger.pay_type = ble_data.payload[3];

    unit.byteH = ble_data.payload[4];
    unit.byteL = ble_data.payload[5];
    charger.total_unit_req = unit.int_value;

#ifdef NO_EEPROM
    match_success = 1;
    if (match_success) {
        // Perform some action
    }
#else
    match_success = match_owner_id(&ble_data.payload[14]);
    if (match_success == 0) {
        app_uart_iso_print("\n Owner ID not Matching\n");
        app_uart_iso_print("\n Registering again\n");
        reg_owner_id(&ble_data.payload[14]);
    }
#endif
    charger.owner_id[0] = ble_data.payload[9];
    charger.owner_id[1] = ble_data.payload[10];


    //fill_header_with_time();
    app_uart_ble_data.tx_buff[0] = K_FRAME_START_BYTE;

    //packet Type
    app_uart_ble_data.tx_buff[1] = PKT_AUTH; //PKT TYPE
    app_uart_ble_data.tx_buff[2] = 6; //DATA SIZE

    //session id
    for (i = 0; i < 2; i++) {
        app_uart_ble_data.tx_buff[i + 3] = charger.sessionID[i];
    }

    app_uart_ble_data.tx_buff[6] = charger.pay_type; //


    if (is_charging_point_ready()) {
        app_uart_ble_data.tx_buff[5] = 1; //session start if everything ok
        charger.arai_status = ARAI_CHARGING;
        app_uart_ble_data.tx_buff[7] = 1; //charge point ready
        app_uart_ble_data.tx_buff[8] = charger.arai_status; //charging

    } else {
        app_uart_ble_data.tx_buff[5] = 0; //session not accepted
        charger.arai_status = ARAI_AUTHORIZATION;
        app_uart_ble_data.tx_buff[7] = 2; //not ready
        app_uart_ble_data.tx_buff[8] = charger.arai_status; //charging
    }

    cks = 0;
    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 6); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }

    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 6;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;
    
    
    load_on();
    
#ifdef DEBUG_BLE
    
    switch(ble_data.rx_buff[6]){
        case 1:
            app_uart_iso_print("Pay Type = Amex\n");
            break;
        case 2:
            app_uart_iso_print("Pay Type = Visa\n");
            break;
        case 3:
            app_uart_iso_print("Pay Type = Master Card\n");
            break;
        case 4:
            app_uart_iso_print("Pay Type = Paypal\n");
            break;
        default:
            app_uart_iso_print("Pay Type = Cash\n");            
    }
#endif
    app_uart_ble_send_data_wait();
    

#ifdef DEBUG_L1
    //app_uart_iso_print("\n\r LEV RESPONCE AUTH => ");
    //app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    //app_uart_iso_send_data_wait();
#endif    
}

/*
 SESSION ID 0 1 2 3 4 5 6 7 8 9  
 START STOP CHARGING REQUEST 10
 OPERATOR ID 11 12 13 14
 EV ID  15 16 17 18 
 
 */
void kirana_charging(void) {
    uint16_t cks;
    uint8_t i;
    uint8_t error;
   // uint8_t match_success;
    error = 0;

#ifdef NO_EEPROM

    error = 0;
    charger.error2.invalid_session_id = 0;
    charger.error2.invalid_op_id = 0;
#else
    match_success = match_session_id(&ble_data.payload[0]);
    if (match_success == 0) {
        app_uart_iso_print("\n Session ID mismatch\n");
        charger.error2.invalid_session_id = 1;
        error = 1;
    } else {
        charger.error2.invalid_session_id = 0;
    }
    match_success = match_operator_id(&ble_data.payload[11]);
    if (match_success == 0) {
        charger.error2.invalid_op_id = 1;

        app_uart_iso_print("\n Operator ID mismatch\n");
        error = 2;
    } else {
        charger.error2.invalid_op_id = 0;
    }
    match_success = match_owner_id(&ble_data.payload[15]);
    if (match_success == 0) {
        app_uart_iso_print("\n Owner ID mismatch\n");
        error = 3;
    }
#endif


    charger.sessionID[0] = ble_data.payload[0];
    charger.sessionID[1] = ble_data.payload[1];

    charger.operator_id[0] = ble_data.payload[3];
    charger.operator_id[1] = ble_data.payload[4];

    charger.owner_id[0] = ble_data.payload[5];
    charger.owner_id[1] = ble_data.payload[6];


    app_uart_ble_data.tx_buff[0] = K_FRAME_START_BYTE; //Start Header

    app_uart_ble_data.tx_buff[1] = PKT_CHG; //PKT TYPE
    app_uart_ble_data.tx_buff[2] = 8; //BYTES OF DATA

    app_uart_ble_data.tx_buff[3] = charger.sessionID[0];
    app_uart_ble_data.tx_buff[4] = charger.sessionID[1];
    //NOTEME - Add ERROR code
    if (error == 0) {
        if (ble_data.payload[2] == 1) {
            if (charger.charging_on == 0)//CHARGING COMMAND KEEP RUNNING SO AVOID ANY FURHTER ACTION
            {
                charger.charging_on = 1;
                charger.charging_event = 1;
                charger.arai_status = ARAI_CHARGING;
                load_on();
                charger.metervalue.int_value = 0;
                meter_at_energy = 0;
            }
            app_uart_ble_data.tx_buff[5] = 1; //start of charging
        } else if (ble_data.payload[10] == 2) {
            charger.charging_on = 0;
            charger.charging_event = 1;
            charger.timeofstop[0] = charger.app_time.tm_mday;
            charger.timeofstop[1] = charger.app_time.tm_mon;
            charger.timeofstop[2] = charger.app_time.tm_yday;
            charger.timeofstop[3] = charger.app_time.tm_hour;
            charger.timeofstop[4] = charger.app_time.tm_min;
            charger.timeofstop[5] = charger.app_time.tm_sec;
            charger.reason_for_stp = RFS_Normal;
            charger.arai_status = ARAI_EOCHARGING;
            charger.session_active = 1;
            charger.metervalue.int_value = meter_at_energy;
            app_uart_ble_data.tx_buff[5] = 0; //start of charging
        }

    } else {
        app_uart_ble_data.tx_buff[5] = 0;
    }
    app_uart_ble_data.tx_buff[8] = charger.error2.bytes; //error code 2
    app_uart_ble_data.tx_buff[9] = charger.error1.bytes; //error code 1

    app_uart_ble_data.tx_buff[6] = charger.metervalue.byteH;
    app_uart_ble_data.tx_buff[7] = charger.metervalue.byteL;

    app_uart_ble_data.tx_buff[10] = charger.arai_status; //state of charging

    cks = 0;
    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 8); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }
    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 8;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;
    app_uart_ble_send_data_wait();
    
   

#ifdef DEBUG_L1
    //app_uart_iso_print("\n\r LEV RESPONCE CHG => ");
    //app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    //app_uart_iso_send_data_wait();
#endif    
}

uint8_t match_operator_id(uint8_t *buff) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (charger.operator_id[i] != *buff++) {
            return 0;
        }

    }
    return 1;
}

uint8_t match_owner_id(uint8_t *buff) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (charger.owner_id[i] != *buff++) {
            return 0;
        }

    }
    return 1;

}

uint8_t match_session_id(uint8_t *buff) {
    uint8_t i;
    uint16_t j;
    j = 0;
    for (i = 0; i < 10; i++) {
        j += *buff;
        if (charger.sessionID[i] != *buff++) {
            return 0;
        }

    }
    if (j == 0)
        return 0;

    return 1;

}

/*
 SESSION ID 0 1 2 3 4 5 6 7 8 9
 START STOP CHARGING REQUEST 10
 */
void kirana_endofcharging(void) {
    uint16_t cks;
    uint8_t i;
    uint8_t match_success;

#ifdef NO_EEPROM
    match_success = 1;
    if (match_success) {
        // Perform some action
    }
#else
    match_success = match_session_id(&ble_data.payload[0]);
    if (match_success == 0) {
        app_uart_iso_print("\n Session ID mismatch\n");
        return;
    }
#endif

    charger.sessionID[0] = ble_data.payload[0];
    charger.sessionID[1] = ble_data.payload[1];

    if (ble_data.payload[10] == 1) {


    } else if ((ble_data.payload[10] == 2) && (charger.charging_on == 1)) {
        if (charger.charging_on == 1) {
            charger.charging_on = 0;
            charger.charging_event = 1;
            charger.arai_status = ARAI_EOCHARGING;
           
            charger.timeofstop[0] = charger.app_time.tm_mday;
            charger.timeofstop[1] = charger.app_time.tm_mon;
            charger.timeofstop[2] = charger.app_time.tm_yday;
            charger.timeofstop[3] = charger.app_time.tm_hour;
            charger.timeofstop[4] = charger.app_time.tm_min;
            charger.timeofstop[5] = charger.app_time.tm_sec;
            
            load_off();
            charger.reason_for_stp = RFS_Normal;
            charger.session_active = 0;
            charger.metervalue.int_value = meter_at_energy;
            charger.time_out_1min = SEC_60_WAIT; //Status change to IDLE after 60 seconds
        } else {
            

        }
    }
    
    load_off();
    meter_at_energy = 0;

    app_uart_ble_data.tx_buff[0] = K_FRAME_START_BYTE;

    app_uart_ble_data.tx_buff[1] = PKT_EOCHG; //PKT TYPE
    app_uart_ble_data.tx_buff[2] = 6; //LEN OF DATA
    //FILL UP SESSION ID
    app_uart_ble_data.tx_buff[3] = charger.sessionID[0];
    app_uart_ble_data.tx_buff[4] = charger.sessionID[1];

    //FILL STATE

    app_uart_ble_data.tx_buff[5] = charger.arai_status; //State
    app_uart_ble_data.tx_buff[6] = charger.reason_for_stp; //reason for STOP

    app_uart_ble_data.tx_buff[7] = charger.metervalue.byteH;
    app_uart_ble_data.tx_buff[8] = charger.metervalue.byteL;


    cks = 0;
    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 6); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }
    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 6;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;
    
    charger.arai_status = ARAI_EOCHARGING;
    app_uart_ble_send_data_wait();
    


#ifdef DEBUG_L1
    //app_uart_iso_print("\n\r LEV RESPONCE EOCHG => ");
    //app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    //app_uart_iso_send_data_wait();
#endif    
}



void kirana_diagnosis(void) {
    uint16_t cks;
    uint8_t i;

#ifdef NO_EEPROM
    charger.error2.invalid_op_id = 0;
    charger.error2.invalid_session_id = 0;

#else
    match_success = match_operator_id(&ble_data.payload[0]);
    if (match_success == 0) {
        charger.error2.invalid_op_id = 1;
        app_uart_iso_print("\n Operator ID mismatch\n");
    } else {
        charger.error2.invalid_op_id = 0;

    }
    match_success = match_session_id(&ble_data.payload[4]);
    if (match_success == 0) {
        charger.error2.invalid_session_id = 1;
        app_uart_iso_print("\n Session ID mismatch\n");

    } else {
        charger.error2.invalid_session_id = 0;

    }

#endif


    charger.operator_id[0] = ble_data.payload[0];
    charger.operator_id[1] = ble_data.payload[1];

    charger.sessionID[0] = ble_data.payload[2];
    charger.sessionID[1] = ble_data.payload[3];



    app_uart_ble_data.tx_buff[0] = K_FRAME_START_BYTE; //PKT TYPE

    app_uart_ble_data.tx_buff[1] = PKT_DIAG; //PKT TYPE
    app_uart_ble_data.tx_buff[2] = 12; //BYTES OF DATA

    //session id

    app_uart_ble_data.tx_buff[3] = charger.sessionID[0];
    app_uart_ble_data.tx_buff[4] = charger.sessionID[1];

    //error code 1
    app_uart_ble_data.tx_buff[5] = charger.error2.bytes;
    //error code 2
    app_uart_ble_data.tx_buff[6] = charger.error1.bytes;

    //state
    app_uart_ble_data.tx_buff[7] = charger.arai_status;


    app_uart_ble_data.tx_buff[8] = charger.session_active; //session active 
      
    //voltage
    uint16_type j;
    j.int_value = (uint16_t) (meter_at_energy*1000);
    app_uart_ble_data.tx_buff[9] = j.byteH;
    app_uart_ble_data.tx_buff[10] = j.byteL;
   
   //current
    j.int_value = (uint16_t)(meter_data.irms.float_var*1000) ;
    app_uart_ble_data.tx_buff[11] = j.byteH;
    app_uart_ble_data.tx_buff[12] = j.byteL;
    
    j.int_value = (uint16_t) meter_data.at_energy.float_var;
    app_uart_ble_data.tx_buff[13] = j.byteH;
    app_uart_ble_data.tx_buff[14] = j.byteL;
    cks = 0;
    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 12); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }
    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 12;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;
    app_uart_ble_send_data_wait(); 

    
#ifdef DEBUG_L1
    //app_uart_iso_print("\n\r LEV RESPONCE DIAG => ");
    //app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    //app_uart_iso_send_data_wait();
#endif   

}

void kirana_timesync(void) {
    uint16_t cks;

    charger.operator_id[0] = ble_data.payload[1];
    charger.operator_id[1] = ble_data.payload[2];

    charger.app_time.tm_mday = ble_data.payload[3]; //DD
    charger.app_time.tm_mon = ble_data.payload[4]; //MM
    charger.app_time.tm_year = ble_data.payload[5]; //YY
    charger.app_time.tm_hour = ble_data.payload[6]; //HH
    charger.app_time.tm_min = ble_data.payload[7]; //MM
    charger.app_time.tm_sec = ble_data.payload[8]; //SS

    app_uart_ble_data.tx_buff[0] = K_FRAME_START_BYTE; //PKT TYPE

    app_uart_ble_data.tx_buff[1] = PKT_TIME; //PKT TYPE
    app_uart_ble_data.tx_buff[2] = 9; //BYTES OF DATA

    //error code 1
    app_uart_ble_data.tx_buff[3] = ble_data.payload[0];

    app_uart_ble_data.tx_buff[4] = charger.operator_id[0];
    app_uart_ble_data.tx_buff[5] = charger.operator_id[1];

    app_uart_ble_data.tx_buff[6] = charger.app_time.tm_mday;
    app_uart_ble_data.tx_buff[7] = charger.app_time.tm_mon;
    app_uart_ble_data.tx_buff[8] = charger.app_time.tm_year;// - YEAR_OFFSET;
    app_uart_ble_data.tx_buff[9] = charger.app_time.tm_hour;
    app_uart_ble_data.tx_buff[10] = charger.app_time.tm_min;
    app_uart_ble_data.tx_buff[11] = charger.app_time.tm_sec;


    cks = 0;
    uint8_t i;
    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 9); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }
    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 9;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;
    app_uart_ble_send_data_wait();        
       

#ifdef DEBUG_L1
    //app_uart_iso_print("\n\r LEV RESPONCE TIME SYNC => ");
    //app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    //app_uart_iso_send_data_wait();
#endif   
}

void kirana_schdule_charge_request(void) {
    uint16_t cks;
    uint8_t i;

    charger.operator_id[0] = ble_data.payload[0];
    charger.operator_id[1] = ble_data.payload[1];

    //start time
    charger_sch_time.tostart[0] = ble_data.payload[2];
    charger_sch_time.tostart[1] = ble_data.payload[3];
    charger_sch_time.tostart[2] = ble_data.payload[4];
    charger_sch_time.tostart[3] = ble_data.payload[5];
    charger_sch_time.tostart[4] = ble_data.payload[6];
    charger_sch_time.tostart[5] = ble_data.payload[7];

    //stop time
    charger_sch_time.tostop[0] = ble_data.payload[8];
    charger_sch_time.tostop[1] = ble_data.payload[9];
    charger_sch_time.tostop[2] = ble_data.payload[10];
    charger_sch_time.tostop[3] = ble_data.payload[11];
    charger_sch_time.tostop[4] = ble_data.payload[12];
    charger_sch_time.tostop[5] = ble_data.payload[13];

    fill_header_with_time();

    app_uart_ble_data.tx_buff[0] = K_FRAME_START_BYTE;
    app_uart_ble_data.tx_buff[1] = PKT_SCHCHG; //PKT TYPE
    app_uart_ble_data.tx_buff[2] = 1; //BYTES OF DATA

    app_uart_ble_data.tx_buff[3] = 1;

    cks = 0;
    for (i = K_FRAME_DATA_LOC; i < (K_FRAME_DATA_LOC + 1); i++) {
        cks = cks + app_uart_ble_data.tx_buff[i];
    }
    app_uart_ble_data.txbufflen = K_FRAME_DATA_LOC + 1;
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks / 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = (uint8_t) (cks % 100);
    app_uart_ble_data.tx_buff[app_uart_ble_data.txbufflen++] = K_FRAME_END_BYTE;
    app_uart_ble_send_data_wait();

#ifdef DEBUG_L1
    app_uart_iso_print("\n\r LEV RESPONCE SCH CHARGE => ");
    app_uart_iso_data.txbufflen = to_ascii(app_uart_ble_data.tx_buff, app_uart_iso_data.tx_buff, app_uart_ble_data.txbufflen);
    app_uart_iso_send_data_wait();
#endif   
}


uint8_t to_ascii(uint8_t *src, uint8_t *dest, uint8_t len) {
    uint8_t i;
    uint8_t j;
    uint8_t b[4];
    j = 0;
    for (i = 0; i < len; i++) {
        sprintf((char *) b, "%02d", src[i]);
        dest[j++] = b[0];
        dest[j++] = b[1];       
        dest[j++] = ' ';

    }
    return j;
}


uint8_t to_ascii_(uint8_t *src, uint8_t *dest, uint8_t start, uint8_t end) {
    uint8_t i;
    uint8_t j;
    uint8_t b[4];
    j = 0;
    for (i = start; i <= end; i++) {
        sprintf((char *) b, "%02d", src[i]);
        dest[j++] = b[0];
        dest[j++] = b[1];       
        dest[j++] = ' ';

    }
    return j;
}

uint8_t hextoascii_(uint8_t *src, uint8_t *dest, uint8_t start, uint8_t end) {
    uint8_t i;
    uint8_t j;
    uint8_t b[4];
    j = 0;
    for (i = start; i <= end; i++) {
        sprintf((char *) b, "%02X", src[i]);
        dest[j++] = b[0];
        dest[j++] = b[1];       
        dest[j++] = ' ';

    }
    return j;
}

void init_charger(void)
{
    charger.arai_status = ARAI_IDLE;
    charger.error1.bytes = 0;
    charger.error2.bytes = 0;
    charger.session_active = 0;
    charger.time_out_1min = 0;
}

void reg_operator_id(uint8_t *id) {
    uint8_t i;
    uint8_t sta;

    sta = 0;
    if(sta==0)
    {
        
    }

    for (i = 0; i < 2; i++) {
        charger.operator_id[i] = *id++;
    }


}

void update_operator_id(uint8_t *id) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        charger.operator_id[i] = *id++;
    }
}

void reg_owner_id(uint8_t *id) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        charger.owner_id[i] = *id++;
    }


}

uint8_t is_charging_point_ready(void) {
    if (is_afe_ready)
        return 1;
    else
        return 0;
}

void time_of_1min_end_of_charging(void) {
    if (charger.time_out_1min > 0) {
        charger.time_out_1min--;
        if (charger.time_out_1min == 0) {
            if (charger.arai_status == ARAI_EOCHARGING) {
                charger.arai_status = ARAI_IDLE;
            }

        }

    }

}

/* *****************************************************************************
 End of File
 */
