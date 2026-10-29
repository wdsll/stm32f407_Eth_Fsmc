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

#ifndef _BLE_APP_H    /* Guard against multiple inclusion */
#define _BLE_APP_H


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
    
#define K_FRAME_START_BYTE 24
#define K_FRAME_END_BYTE 23
#define K_FRAME_START_BYTE_LOC 0
#define K_FRAME_DATE_LOC 1
#define K_FRAME_DATE_SIZE 6
#define K_FRAME_PACKETTYPE_LOC 1//7
#define K_FRAME_DATA_LEN_LOC     2//8
#define K_FRAME_DATA_LOC        3//9
#define K_FRAME_MIN_LEN    6//12    

#define ACLEV_ID_NUMBER 0xAABBCCDD
    
typedef enum{
            BLE_POWER_ON,
            BLE_WAIT_MAC,
            BLE_WAIT_NAME_ACK,
            BLE_WAIT_UUID_ACK,
            BLE_NORMAL,
            BLE_IDLE
    
}ble_task_sta_t;


typedef enum{

            ERR_CODE_1_INVALID_OP_ID=0,
            ERR_CODE_1_POWER_FAIL,
            ERR_CODE_1_INVALID_SESSION,
            ERR_CODE_1_CKS_ERROR,
            ERR_CODE_1_LESS_MIN_CURRENT

}ERROR_CODE1;

typedef enum{
            ERR_CODE_2_MAINS_LOW,
            ERR_CODE_2_MAINS_HIGH,
            ERR_CODE_2_OP_CURRENT_H,
            ERR_CODE_2_EARTH_DISCONNECT,
            ERR_CODE_2_EARTHLEAKAGE,
            ERR_CODE_2_EMERGENCY,
            ERR_CODE_2_TEMP_FAIL
            
}ERROR_CODE2;

typedef enum{
RFS_Normal=0,
RFS_SuspendedbyCharger,
RFS_SuspendedbyUser,
RFS_LEVChargNotReady,
RFS_Timeout,
RFS_PowerFail,
RFS_MainsLow,
RFS_MainsHi,
RFS_OutCurrHi,
RFS_EarthLeakage,
RFS_EarthDisconnect,
RFS_Nutralearth,
RFS_Emergancy,
RFS_TemperatureFail

}REASON_for_Stop;


typedef enum{
    ARAI_IDLE =0,
    ARAI_INTIALIZATION,
    ARAI_AUTHORIZATION,
    ARAI_CHARGING,
    ARAI_EOCHARGING,
    ARAI_ERROR
}arai_status_t;



typedef enum{
    PKT_INIT= 0x01,
    PKT_AUTH,
    PKT_CHG,
    PKT_EOCHG,
    PKT_DIAG,
    PKT_SLOG,
    PKT_TIME,
    PKT_OPR,
    PKT_THC,
    PKT_OWR,
    PKT_SCHCHG,
    PKT_OPIDREQ
}PKT_TYPE;

typedef enum{
    EV_TYPE_2W =1,
            EV_TYPE_3W,
            EV_TYPE_CYCLE,
            EV_TYPE_4W
}EV_TYPE;

typedef enum{
    PAYMENT_TYPE_CASH = 0x01,
    PAYMENT_TYPE_CONTRACT,
    PAYMENT_TYPE_PREPAID
            
}PAYMENT_TYPE;

typedef union
{
    uint8_t bytes;
    struct{
        unsigned mains_low:1;
        unsigned mains_high:1;
        unsigned output_current_high:1;
        unsigned earth_leakage:1;
        unsigned earth_disconnect:1;
        unsigned neutral_earth_detect:1;
        unsigned emergency:1;
        unsigned temperature_falure:1;
    };

}err1_type;

typedef union
{
    uint8_t bytes;
    struct{
        unsigned invalid_op_id:1;
        unsigned power_fail:1;
        unsigned invalid_session_id:1;
        unsigned checksum_mismatch:1;
        unsigned less_than_min_current:1;
        unsigned :3;
    
    };

}err2_type;

typedef struct
{
    uint8_t tx_buff[64];
    uint8_t rx_buff[64];
    uint8_t mac[6];
    uint8_t uuid[16];
    uint8_t name[12];
    uint8_t payload[32];
    uint8_t payload_len;
    volatile uint8_t ble_led;
    volatile uint8_t txbufflen;
    volatile uint8_t rxbufflen;
    ble_task_sta_t ble_status;
    volatile uint8_t process_ble_data;
    volatile uint16_t time_out;
}ble_params_t;

typedef struct
{
    uint16_type max_volt;
    uint16_type min_volt;
    uint16_type max_curr;
    uint16_type min_curr;
    uint8_t ne_volt_detect;
    uint8_t earth_detect;

}charger_th_vaule;

typedef struct
{
    uint16_t lev_ver;
    uint16_t total_unit_req;
    EV_TYPE ev_type;
    PAYMENT_TYPE pay_type;
    uint8_t operator_id[4];
    uint8_t owner_id[4];
    uint8_t realtime[6];
    uint8_t timeofstart[6];
    uint8_t timeofstop[6];
    uint8_t sessionID[2];
    uint8_t charging_on;
    uint8_t charging_event;
    uint16_type metervalue;
    arai_status_t arai_status;
    err1_type error1;
    err2_type error2;
    REASON_for_Stop reason_for_stp;
    struct tm app_time;
    uint8_t session_active;
    uint16_t time_out_1min;

}kirana_params_t;

typedef struct
{
    uint8_t tostart[6];
    uint8_t tostop[6];
    uint8_t issch_enable;

}schedule_charging_param;




extern ble_params_t ble_data;
extern kirana_params_t charger;
extern charger_th_vaule charger_th;
extern schedule_charging_param charger_sch_time;
void init_charger(void);
void init_ble_data(void);
void ble_task(void);
void send_name(void);
void send_uuid(void);
void send_metering_data(void);
void load_on(void);
void load_off(void);
void k_buff_play(void);
void responce_to_packet(PKT_TYPE packet_type);
uint16_t calculate_cks(uint8_t len,uint8_t *buff);

void kirana_operator_registration(void);
void kirana_owner_registration(void);
void kirana_initialization(void);
void kirana_authorization(void);
void kirana_charging(void);
void kirana_endofcharging(void);
void kirana_sessionlog(void);
void kirana_timesync(void);
void kirana_diagnosis(void);
void kirana_threshold_config(void);
void kirana_schdule_charge_request(void);
void kirana_op_id_request(void);
void update_operator_id(uint8_t *id);
void fill_header_with_time(void);
uint8_t match_operator_id(uint8_t *buff);
uint8_t match_owner_id(uint8_t *buff);
uint8_t match_session_id(uint8_t *buff);
uint8_t to_ascii(uint8_t *src,uint8_t *dest,uint8_t len);
uint8_t to_ascii_(uint8_t *src, uint8_t *dest, uint8_t start, uint8_t end);
uint8_t hextoascii_(uint8_t *src,uint8_t *dest,uint8_t start, uint8_t end);
void reg_operator_id(uint8_t *id);
void reg_owner_id(uint8_t *id);
uint8_t is_charging_point_ready(void);
void time_of_1min_end_of_charging(void);

void ble_init(void);        


    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
