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

#ifndef _APP_ATM90E26_H    /* Guard against multiple inclusion */
#define _APP_ATM90E26_H


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

#define METER_CONST        14400//10000 4 PULSES AT EVERY SECONDS
#define PM_CALIBRATION          0x00804010//0x80001040// 
#define PLCONSTH 0x7B//0x00BE//0x02DB//0x0023
#define PLCONSTL 0XDEF7//0xFBF0//0x6DB6//0xE666//0xB88F
#define LGAIN 0x0000//0x0909
#define UGAIN 0x9AA0// 0X9CCE//0x9C3A//0x6720//0x6629//0x6720
#define IGAINL 0X4104//0x40FB//0x5576//0x4350//0x9561
#define LPHI   0x0000//0x0095
#define POFFSETL   0x0000//0x207A// 207a r=fbe7
#define QOFFSETL  0x0000//0xFBE7
#define MMODE_VALUE 0x1422//0x7422//0x7422  //Current Gain 4x, L Line 1,All zero Crossing, pth = 3.125%
//0001 0100 0010 0010 
    
typedef	enum
{
    //status and special register
	SoftReset =0,        //write 0x789A to reset device
	SysStatus,          //checksum alert & metering direction status 
	FuncEn,             //enable disable voltage sag and direction change int
	SagTH,              //Formate is XXX.XX 
	SmallPMod,
	LastData =6,     //Last transacted data
    //metering calibration and configuration
	Plsb     = 8,     //16 LSB of RMS power Register low 8 bit are always 0
	CalStart =0x20,     //default = 6886h calibration start = 5678, check= 8675  
	PLconstH =0x21,     //
	PLconstL =0x22,
	Lgain =0x23,
	Lphi =0x24,
	Ngain =0x25,
	Nphi =0x26,
	PStartTH =0x27,
	PNolTH =0x28,
	QStartTH =0x29,
	QNolTH =0x2A,
	MMode =0x2B,
	CS1 =0x2C,		//1-13
	AdjStart =0x30,
	Ugain =0x31,
	IgainL =0x32,
	IgainN =0x33,
	Uoffset =0x34,
	IoffsetL =0x35,
	IoffsetN =0x36,
	PoffsetL =0x37,
	PoffsetN =0x38,
	QoffsetL =0x39,
	QoffsetN =0x3A,
	CS2 =0x3B,			//1-12
            //enregy registers
	APenegy =0x40,
	ANenergy =0x41,
	ATenergy =0x42,
	RPenergy =0x43,
	RNenergy =0x44,
	RTenergy =0x45,
	Enstatus =0x46,		//1-7
            //
	Irms =0x48,
	Urms =0x49,
	Pmean =0x4A,
	Qmean =0x4B,
	Freq =0x4C,
	PowerF =0x4D,
	Pangle =0x4E,
	Smean =0x4F,
	Irms2 =0x68,
	Pmean2 =0x6A,
	Qmean2 =0x6B,
	PowerF2 =0x6D,
	Pangle2 =0x6E,
	Smean2 =0x6F		//1-14
} AFE_90E26_REG;
    
typedef enum
{
    MEAS_APenergy =0, // 22  forward active energy
    MEAS_ANenergy, // 23 reverse active energy
    MEAS_ATenergy, // 24 absolute active energy        
    MEAS_RPenergy, // 25 Forward Reactive (Inductive) Energy
    MEAS_RNenergy, // 26 Reverse Capacitive Reactive Energy
    MEAS_RTenergy, // 27 Absolute Reactive Energy

    MEAS_sysstatus,
    
    MEAS_PLconstH,  //1      
    MEAS_PLconstL,  //2
    MEAS_Lgain,     //3   
    MEAS_Lphi,      //4
    MEAS_Mmode,     //5   
    MEAS_Ugain,     //6
    MEAS_IgainL,    //7
    MEAS_PoffsetL,  //8
    MEAS_QoffsetL,  //9
            
            
            
    MEAS_ENstatus, // 10
    MEAS_Irms, // 11
    MEAS_Urms, // 12
    MEAS_Pmean, // 13 Active Power KW
    MEAS_Qmean, // 14 Reactive Power //
    MEAS_Freq, //15
    MEAS_PowerF, //16
    MEAS_Pangle, //17
    MEAS_Smean, //18  Mean Apparent Power //KVA

    MEAS_Irms2, //19
    MEAS_Pmean2, //20
    MEAS_Qmean2, //21

}AFE_MEASURMENT_TABLE;


typedef	enum
{
    AFE_INIT=0,
            AFE_RESET,
            AFE_CALIB,
            AFE_READ,
            AFE_WRITE,
            AFE_WACK_WAIT,
            AFE_READ_WAIT,
            AFE_IDLE  //all AFE TASK would be taken here
}AFE_task_sta_;
    
    



typedef enum{
            AFE_CALIB_START,    //START CALIBRATION
            AFE_CALIB_CALSTART, //WRITE CALIBRATION
            AFE_CALIB_ADSTART,
            AFE_CALIB_MMODE,          
            AFE_CALIB_PLCONSTH,
            AFE_CALIB_PLCONSTL,
            AFE_CALIB_POFFSET,  //POFFSET      
            AFE_CALIB_QOFFSET,  //POFFSET      
            AFE_CALIB_LGAIN,   
            AFE_CALIB_IGAINL,
            AFE_CALIB_UGAIN,
            AFE_CALIB_LPHI,
            AFE_CALIB_ALL,
            AFE_CALIB_WAIT,
            AFE_CALIB_STOP,
           
    
}afe_calib_sta_;



    typedef struct{
    uint8_t tx_buff[5];
    uint8_t rx_buff[3];
    volatile uint8_t txbufflen;
    volatile uint8_t txdone;
    volatile uint8_t rxbufflen;
    volatile rxbuff_sta rxstatus;
    volatile uint16_t rx_overflow;
    }afeuart_params;


    typedef struct{
    afe_calib_sta_ afe_calib_start_sta;
    afe_calib_sta_ afe_calib_sta_next;
    afe_calib_sta_ afe_calib_sta_last;
    
    uint8_t last_write_checksum;
    uint8_t fail_counter;
    
    
    }afe_params;


/*
 Data format is XXXX.X pulses. Resolution is 0.1 pulse. Maximum is 6553.5 pulses.
When the accumulation of this register has achieved FFFFH, the continuation accumulation will return to
0000H.
 * 
 */
typedef struct
{
    type_float_n_long ap_energy; //active forward energy 
    type_float_n_long at_energy; //absolute Active Energy //Kwh
    type_float_n_long rt_energy; //absolute Active Energy //KvarH
    type_float_n_long irms;      
    type_float_n_long urms;
    type_float_n_long pmean;
    type_float_n_long qmean;
    type_float_n_long freq;
    type_float_n_long pf;
    type_float_n_long smean;
    type_float_n_long enstatus;
    
}afe_meas_results;
    

typedef struct 
{
 //   uint16_t PLconstH;
 //   uint16_t plconstL;
    uint16_t Lgain;
    uint16_t Lphi;
    uint16_t Ugain;
    uint16_t IgainL;
//    uint16_t IgainN;
    uint16_t PoffsetL;
    uint16_t QoffsetL;
    uint16_t cks;
}afe_calibration_reg_;

typedef struct
{
    uint16_t eeprom_Lgain;
    uint16_t eeprom_Lphi;
    uint16_t eeprom_Ugain;
    uint16_t eeprom_IgainL;
//    uint16_t IgainN;
    uint16_t eeprom_PoffsetL;
    uint16_t eeprom_QoffsetL;
    uint16_t eeprom_cks;
}afe_calibration_reg_eeprom;



 #define AFE_MS_OVF_WAIT 2
 #define READ_PARAM_NUM 28
   
    
    
void AFE_uart_rx_overflow_fun(void);    
void AFE_uart_receive_data_start(void);
void AFE_uart_send_data(void);
void AFE_uart_send_data_wait(void);

void AFE_calibration(void);
void AFE_Load_Cal_Data(void);

void AFE_set_default_calib_values(void);
void AFE_uart_init(void);
void AFE_Task(void);
void AFE_write_command(uint8_t addr, uint16_t value);
void AFE_init(void);
void AFE_read_command(uint8_t addr);
void update_metering_data(void);
void process_calib_cmd(void);
void AFE_process_calibration(void);
void offset_calibration(void);
uint8_t check_write_checksum(void);
uint8_t read_register(uint8_t reg_addr);
void afe_print_read_registers(void);
void afe_print_read_registers_buff(void);
void AFE_write_calibration_register(void);
uint8_t afe_uart_rx_error_check(void);
void AFE_via_autocalibration(void);
void AFE_setcalibration_mode(void);
void AFE_ugain_calibration();
void AFE_igain_calibration();
//extern uart_params afe_uart_data;
   
    
extern volatile uint16_t afe_wait_for_read;    
extern volatile uint8_t afe_read_register_ready;
extern volatile uint16_t afe_wait_for_read_energy;
extern uint16_type afe_read_param[READ_PARAM_NUM];    
extern afe_meas_results meter_data;    
extern float meter_at_energy;
extern float meter_rt_energy;
extern volatile uint8_t is_afe_ready;
extern buffer_t calib_buff;
extern afe_calibration_reg_ afe_cal_reg;    
    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
