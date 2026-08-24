/*********************************************************************************************************
* 模块名称：can_comm.c
* 摘    要：
* 作    者：Rengar
* 内    容：
* 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
*          
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
/* can_comm.c - CAN 通信 (ISO1050 隔离, 500kbps, PA11/PA12) */
#include "can_comm.h"
#include "adc_dma.h"
#include <string.h>
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/

/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/


static can_rx_data_t s_rx_cmd;
static volatile bool s_rx_flag = false;
static volatile uint32_t s_last_rx_ms = 0U;
static volatile bool s_can_timeout = false;

void can_comm_init(uint32_t baudrate)
{
    rcu_periph_clock_enable(CAN0_RCU);
    rcu_periph_clock_enable(CAN0_GPIO_RCU);
    rcu_periph_clock_enable(RCU_AF);

    /* PA11 = CAN_RX, PA12 = CAN_TX (默认映射, 无需 remap) */
    gpio_init(CAN0_RX_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, CAN0_RX_PIN);
    gpio_init(CAN0_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, CAN0_TX_PIN);

    can_deinit(CAN0);
    can_parameter_struct can_param;
    memset(&can_param, 0, sizeof(can_param));
    can_param.working_mode = CAN_NORMAL_MODE;
    can_param.resync_jump_width = CAN_BT_SJW_1TQ;
    can_param.time_segment_1 = CAN_BT_BS1_15TQ;
    can_param.time_segment_2 = CAN_BT_BS2_4TQ;
    /* (1+15+4)=20 TQ; APB1=60MHz(120MHz/2), 60M/prescaler/20 = baudrate */
    can_param.prescaler = (rcu_clock_freq_get(CK_APB1) / (baudrate * 20U));
    can_init(CAN0, &can_param);

    /* 接收过滤器: 接收 BMS 控制帧 */
    can_filter_parameter_struct filter;
    filter.filter_number = 0;
    filter.filter_mode = CAN_FILTERMODE_MASK;
    filter.filter_bits = CAN_FILTERBITS_32BIT;
    filter.filter_list_high = (CAN_ID_BMS_CONTROL << 5) & 0xFFFF;
    filter.filter_list_low = 0x0000;
    filter.filter_mask_high = 0xFFFF;
    filter.filter_mask_low = 0x0000;
    filter.filter_fifo_number = CAN_FIFO0;
    filter.filter_enable = ENABLE;
    can_filter_init(&filter);

    /* 使能接收中断 */
    can_interrupt_enable(CAN0, CAN_INT_RFNE0);
    nvic_irq_enable(USBD_LP_CAN0_RX0_IRQn,
        irq_priority_encode(IRQ_PRIO_CAN_PREEMPT, IRQ_PRIO_CAN_SUB), 0U);

    s_last_rx_ms = g_ms;
}

/* CAN0 RX0 中断 */
void USBD_LP_CAN0_RX0_IRQHandler(void)
{
    if (can_receive_message_length_get(CAN0, CAN_FIFO0) > 0U) {
        can_receive_message_struct rx_msg;
        can_message_receive(CAN0, CAN_FIFO0, &rx_msg);
        if (rx_msg.rx_sfid == CAN_ID_BMS_CONTROL && rx_msg.rx_dlen >= 6) {
            s_rx_cmd.vout_set_x10 = (uint16_t)(rx_msg.rx_data[0] | (rx_msg.rx_data[1] << 8));
            s_rx_cmd.iout_set_x10 = (uint16_t)(rx_msg.rx_data[2] | (rx_msg.rx_data[3] << 8));
            s_rx_cmd.enable = rx_msg.rx_data[4];
            s_rx_flag = true;
            s_last_rx_ms = g_ms;
            s_can_timeout = false;
        }
    }
    can_interrupt_flag_clear(CAN0, CAN_INT_RFNE0);
}

void can_comm_poll(void)
{
    /* 超时检测: 3 秒无 BMS 报文 */
    if (!s_can_timeout && elapsed_reached(s_last_rx_ms, 3000U)) {
        s_can_timeout = true;
    }
}

void can_comm_tx_status(void)
{
    can_trasnmit_message_struct tx_msg;
    memset(&tx_msg, 0, sizeof(tx_msg));
    tx_msg.tx_sfid = CAN_ID_CHARGER_STATUS;
    tx_msg.tx_ff = CAN_FF_STANDARD;
    tx_msg.tx_ft = CAN_FT_DATA;
    tx_msg.tx_dlen = 8;
    tx_msg.tx_data[0] = (uint8_t)(g_adc_multi.vout_v * 10.0f);
    tx_msg.tx_data[1] = (uint8_t)((uint16_t)(g_adc_multi.vout_v * 10.0f) >> 8);
    tx_msg.tx_data[2] = (uint8_t)(g_adc_multi.iout_a * 10.0f);
    tx_msg.tx_data[3] = (uint8_t)((uint16_t)(g_adc_multi.iout_a * 10.0f) >> 8);
    tx_msg.tx_data[4] = (uint8_t)g_charger_state;
    tx_msg.tx_data[5] = (uint8_t)g_fault;
    tx_msg.tx_data[6] = (uint8_t)(g_adc_multi.vbat_v * 10.0f);
    tx_msg.tx_data[7] = (uint8_t)((uint16_t)(g_adc_multi.vbat_v * 10.0f) >> 8);
    can_message_transmit(CAN0, &tx_msg);
}

bool can_comm_get_cmd(can_rx_data_t *out)
{
    if (s_rx_flag) {
        *out = s_rx_cmd;
        s_rx_flag = false;
        return true;
    }
    return false;
}

bool can_comm_timeout(void) { return s_can_timeout; }
