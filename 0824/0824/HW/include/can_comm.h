/* can_comm.h - CAN 通信 (ISO1050 隔离, J1939/标准帧) */
#ifndef _CAN_COMM_H_
#define _CAN_COMM_H_
#include "main.h"

#define CAN_ID_CHARGER_STATUS   (0x1801U)   /* 充电机状态帧 */
#define CAN_ID_BMS_CONTROL      (0x1802U)   /* BMS 控制帧 */
#define CAN_ID_CHARGER_CMD      (0x1800U)   /* 充电机命令帧 */

typedef struct {
    uint16_t vout_x10;      /* 输出电压 ×10 (0.1V) */
    uint16_t iout_x10;      /* 输出电流 ×10 (0.1A) */
    uint16_t vbus_x10;      /* 母线电压 ×10 */
    uint16_t temp_x10;      /* 最高温度 ×10 (°C) */
    uint8_t  charger_state; /* charger_state_t */
    uint8_t  fault_code;    /* fault_type_t */
    uint16_t vbat_x10;      /* 电池电压 ×10 */
} can_tx_data_t;

typedef struct {
    uint16_t vout_set_x10;  /* 目标电压 ×10 */
    uint16_t iout_set_x10;  /* 目标电流 ×10 */
    uint8_t  enable;        /* 1=使能充电 0=停止 */
    uint8_t  reserved;
} can_rx_data_t;

void can_comm_init(uint32_t baudrate);
void can_comm_poll(void);
void can_comm_tx_status(void);
bool can_comm_get_cmd(can_rx_data_t *out);
bool can_comm_timeout(void);

#endif
