/*********************************************************************************************************
* 模块名称：can_test.c
* 摘    要：CAN 测试模块（带 CAN 中断）
* 当前版本：1.0.0
* 作    者：Rengar
* 创建日期：2026年02月04日
* 备    注：
**********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "can_test.h"
#include "can_driver.h"
#include "debug_printf.h"
#include "main.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

#define CAN_TEST_STD_ID          (0x321U)
#define CAN_TEST_PERIOD_TICKS    (1000U)

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static volatile uint32_t s_can_test_rx_count = 0U;
static volatile uint32_t s_can_test_tx_count = 0U;
static volatile uint32_t s_can_test_tx_fail_count = 0U;
static volatile can_receive_message_struct s_can_test_last_msg;

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void can_test_irq_init(void);
static void can_test_pack_payload(uint8_t *payload, uint32_t counter);

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
//核心作用是为 CAN（Controller Area Network）通信配置硬件中断，确保收到 CAN 报文时能及时触发中断处理程序
static void can_test_irq_init(void)
{
	/*CAN_DRIVER_CAN_PERIPH：指向具体的 CAN 外设（如 CAN0 或 CAN1），通过驱动层的宏定义，实现硬件抽象。
	CAN_INTEN_RFNEIE0：中断使能标志，表示“接收 FIFO 0 非空中断”（Receive FIFO 0 Not Empty Interrupt Enable）。
当 CAN 控制器接收到报文并存入 FIFO 0 时，该中断会被触发。
	*/
    can_interrupt_enable(CAN_DRIVER_CAN_PERIPH, CAN_INTEN_RFNEIE0);
    nvic_irq_enable(USBD_LP_CAN0_RX0_IRQn,irq_priority_encode(IRQ_PRIO_BACKGROUND_PREEMPT, IRQ_PRIO_BACKGROUND_SUB),0U);
}

static void can_test_pack_payload(uint8_t *payload, uint32_t counter)
{
    if (payload == NULL) {
        return;
    }

    payload[0] = (uint8_t)(counter & 0xFFU);
    payload[1] = (uint8_t)((counter >> 8U) & 0xFFU);
    payload[2] = (uint8_t)((counter >> 16U) & 0xFFU);
    payload[3] = (uint8_t)((counter >> 24U) & 0xFFU);
    payload[4] = (uint8_t)(s_can_test_rx_count & 0xFFU);
    payload[5] = (uint8_t)((s_can_test_rx_count >> 8U) & 0xFFU);
    payload[6] = (uint8_t)((s_can_test_rx_count >> 16U) & 0xFFU);
    payload[7] = (uint8_t)((s_can_test_rx_count >> 24U) & 0xFFU);
}

/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/

void can_test_init(void)
{
    can_driver_init();
    can_test_irq_init();
    debug_printf("CAN test init done.\r\n");
}
/*********************************************************************************************************
* 函数名称：can_test_tick_1khz
* 函数功能：
		实现了周期性的 CAN 报文发送任务，以 1?kHz 的调用频率被触发，但实际发送周期为 1?秒（每 1000 次调用发送一次）。
这种设计是嵌入式系统中“高频调用、低频执行”的典型模式，既能保证定时精度，又避免不必要的 CPU 开销
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年07月01日
* 注    意：
*********************************************************************************************************/
void can_test_tick_1khz(void)
{
    static uint32_t tick_count = 0U;
    uint8_t payload[8] = {0U};  //在栈上分配 8 字节数组，对应 CAN 标准帧的最大数据长度。

		//每次调用增加 tick_count，若未达到周期值（CAN_TEST_PERIOD_TICKS = 1000），则直接返回
//将高频调用（1kHz） 转换为低频执行（1Hz）。这种“快速检查、快速返回”的模式在实时操作系统的定时任务中极为常见，既能保证严格的周期定时，又避免在非触发时刻执行耗时操作。
    tick_count++;
    if (tick_count < CAN_TEST_PERIOD_TICKS) {
        return;
    }
//重置计数器 周期复位：达到周期后重置 tick_count，开始下一个发送周期。这保证了严格的等间隔发送，不受函数执行时间微小波动的影响。
    tick_count = 0U;
//打包待发送数据,将当前发送计数器 s_can_test_tx_count 的值打包到 payload 中（同时也会包含接收计数器 s_can_test_rx_count）。
    can_test_pack_payload(payload, s_can_test_tx_count);
//错误统计而非立即重试：在通信失败时记录错误而非盲目重试，避免阻塞或恶化总线状态，同时为诊断保留数据。
    if (can_driver_send_standard(CAN_TEST_STD_ID, payload, sizeof(payload)) == SUCCESS) {
        s_can_test_tx_count++;
    } else {
        s_can_test_tx_fail_count++;
    }
}

uint32_t can_test_get_rx_count(void)
{
    return s_can_test_rx_count;
}

uint32_t can_test_get_tx_count(void)
{
    return s_can_test_tx_count;
}

uint32_t can_test_get_tx_fail_count(void)
{
    return s_can_test_tx_fail_count;
}

/*********************************************************************************************************
*                                              中断处理函数
*********************************************************************************************************/
void USBD_LP_CAN0_RX0_IRQHandler(void)
{
    can_receive_message_struct msg;
//驱动层函数，尝试从指定的 FIFO（可能是 FIFO 0 或 FIFO 1）读取一条报文。
    while (can_driver_receive(&msg, CAN_DRIVER_FIFO))  //关键设计：持续读取直到 FIFO 为空，确保一次性处理所有待处理报文，避免因中断响应延迟而丢失报文。
		{
        s_can_test_last_msg = msg; //为调试或诊断保留最近一条报文的内容，便于在需要时检查报文细节（如 ID、数据负载）
        s_can_test_rx_count++; //递增接收计数器，统计总共收到的报文数量,在 can_test_pack_payload 中被打包进发送报文，形成双向通信的闭环验证。
    }
}















