/*********************************************************************************************************
* 模块名称：can_driver.h
* 摘    要：CAN驱动模块
* 当前版本：1.0.0
* 作    者：Rengar
* 创建日期：202年2月4日
* 备    注：
* 注    意：
**********************************************************************************************************
* 版本变更：
* 作    者：
* 修改日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/
#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "gd32f30x_conf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define CAN_DRIVER_CAN_PERIPH        CAN0
#define CAN_DRIVER_TX_PORT           GPIOA
#define CAN_DRIVER_TX_PIN            GPIO_PIN_12
#define CAN_DRIVER_RX_PORT           GPIOA
#define CAN_DRIVER_RX_PIN            GPIO_PIN_11
#define CAN_DRIVER_FIFO              CAN_FIFO0

/* APB1=60MHz, prescaler=8, (1+12+2)TQ -> 500kbps */
#define CAN_DRIVER_PRESCALER         8U
#define CAN_DRIVER_SJW               CAN_BT_SJW_1TQ
#define CAN_DRIVER_BS1               CAN_BT_BS1_12TQ
#define CAN_DRIVER_BS2               CAN_BT_BS2_2TQ

/*********************************************************************************************************
*                                              API声明
*********************************************************************************************************/
void can_driver_init(void);
ErrStatus can_driver_send_standard(uint32_t std_id, const uint8_t *data, uint8_t len);
bool can_driver_receive(can_receive_message_struct *message, uint8_t fifo);

#endif
