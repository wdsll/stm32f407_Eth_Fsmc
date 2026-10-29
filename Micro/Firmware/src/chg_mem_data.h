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

#ifndef _CHG_MEM_DATA_H    /* Guard against multiple inclusion */
#define _CHG_MEM_DATA_H


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

typedef union
{
    uint8_t array[28];
    struct{
    uint8_t log_idseq;        //no int use 1B
    uint8_t sessionID[10];    // 10B
    uint8_t timeofstart[6];   // 6B
    uint8_t timeofstop[6];    // 6B
    uint16_t unitconsume;     // 2B
    uint8_t reason_for_stp; //1B 
    uint16_t cks;
    };
}session_log_data;  //28bytes

typedef union
{
    uint8_t array[7];
    struct{
    uint16_t start_session_addr;
    uint16_t last_session_addr;
    uint8_t total_session;
    uint16_t cks;
    };
}session_info_t; //7 bytes

typedef union
{
    uint8_t array[8];
    struct{
    uint8_t op_id[4];
    uint8_t is_reg;
    uint8_t dummy;
    uint16_t cks;
    };
}operator_reg_data; //8 bytes

        
void write_session_log(session_log_data *session);
uint8_t check_session_info(session_info_t *sinfo);
uint8_t read_session_info(session_info_t *sinfo);
uint8_t read_session_log(session_log_data *session,uint8_t session_no);
void test_chg_memory(void);
uint8_t read_operator_id(operator_reg_data *opid, uint8_t opid_number);
uint8_t write_opid(uint8_t *opid);
uint8_t match_op_id(uint8_t *opid);
void test_op_id(void);
    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
