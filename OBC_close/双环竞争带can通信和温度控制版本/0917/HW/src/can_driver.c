/*********************************************************************************************************
* 模块名称：can_driver.c
* 摘    要：CAN驱动模块
* 当前版本：1.0.0
* 作    者：Rengar
* 创建日期：2026年02月4日
* 备    注：
* 注    意：
**********************************************************************************************************
* 版本变更：
* 作    者：
* 修改日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "can_driver.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void can_driver_gpio_init(void);
static void can_driver_filter_accept_all(void);

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
static void can_driver_gpio_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_init(CAN_DRIVER_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, CAN_DRIVER_TX_PIN);
    gpio_init(CAN_DRIVER_RX_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, CAN_DRIVER_RX_PIN);
}

static void can_driver_filter_accept_all(void)
{
    can_filter_parameter_struct filter;  // 声明一个 CAN 过滤器参数结构体
    filter.filter_number = 0U;  // 使用过滤器编号 0（GD32 提供多个过滤器）
	  //CAN 过滤器通常有两种模式：列表模式（List Mode，精确匹配）和掩码模式（Mask Mode，按位屏蔽）。
	  //掩码模式原理：每个标识符位对应一个掩码位。掩码位 = 1 表示“必须匹配”，掩码位 = 0 表示“不关心”。
    filter.filter_mode = CAN_FILTERMODE_MASK;  // 设置为“掩码模式”（Mask Mode）
    filter.filter_bits = CAN_FILTERBITS_32BIT; // 使用 32 位标识符格式（标准帧+扩展帧统一处理）
  	//将 filter_mask_high 和 filter_mask_low 全部设为 0，意味着对所有标识符位都“不关心”，因此任何标识符的消息都会通过过滤器。
    filter.filter_list_high = 0U; // 标识符列表高 16 位（此处未使用）
    filter.filter_list_low = 0U; // 标识符列表低 16 位（此处未使用）// 标识符列表低 16 位（此处未使用）
    filter.filter_mask_high = 0U; // **关键**：掩码高 16 位设为 0，表示不检查对应位
    filter.filter_mask_low = 0U; // **关键**：掩码低 16 位设为 0，表示不检查对应位
    filter.filter_fifo_number = CAN_DRIVER_FIFO; // 指定接收 FIFO（在头文件中定义为 CAN_FIFO0）
    filter.filter_enable = ENABLE; // 启用该过滤器
    can_filter_init(&filter);  // 调用 GD32 库函数，将配置写入硬件寄存器
}


/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：can_driver_init
* 函数功能：
				硬件初始化：配置CAN控制器的工作模式、时序参数和GPIO引脚
				通信准备：设置过滤器以接收所有CAN消息（全通模式）
				模块化设计：作为上层应用与底层硬件的接口，遵循“初始化-使用”的嵌入式驱动模型
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年02月04日
* 注    意：
*********************************************************************************************************/
void can_driver_init(void)
{
    can_parameter_struct can_init_parameter;  

    can_driver_gpio_init(); 
    rcu_periph_clock_enable(RCU_CAN0);
    can_deinit(CAN_DRIVER_CAN_PERIPH);

    can_init_parameter.working_mode = CAN_NORMAL_MODE;  //正常通信模式（非静默/环回）
    can_init_parameter.resync_jump_width = CAN_DRIVER_SJW; //重同步跳跃宽度（1TQ）
    can_init_parameter.time_segment_1 = CAN_DRIVER_BS1; //时间段1（12TQ）
    can_init_parameter.time_segment_2 = CAN_DRIVER_BS2; //时间段2（2TQ）
    can_init_parameter.time_triggered = DISABLE; //禁用时间触发通信模式
    can_init_parameter.auto_bus_off_recovery = ENABLE; //	关键安全特性：总线关闭后自动恢复
  	can_init_parameter.auto_wake_up = DISABLE; //禁用自动唤醒
    can_init_parameter.auto_retrans = ENABLE; //确保可靠性：发送失败时自动重传
    can_init_parameter.rec_fifo_overwrite = DISABLE; //防止新消息覆盖未读消息
    can_init_parameter.trans_fifo_order = DISABLE; //按发送顺序处理FIFO
    can_init_parameter.prescaler = CAN_DRIVER_PRESCALER; //	预分频系数（8）
    //强制转换：(void)显式忽略返回值，表明开发者已确认函数不会失败
    (void)can_init(CAN_DRIVER_CAN_PERIPH, &can_init_parameter);
    can_driver_filter_accept_all(); //接收所有CAN消息（掩码全0的掩码模式）
}

/*********************************************************************************************************
* 函数名称：can_driver_send_standard
* 函数功能：			
* 输入参数：
			std_id 标准帧ID（11位，范围0-0x7FF）
			data  只读数据指针，确保发送过程不修改源数据
      len		数据长度（0-8字节，符合CAN协议限制）
* 输出参数：void
* 返 回 值：枚举类型（SUCCESS/ERROR），提供明确状态反馈
* 创建日期：2026年02月04日
* 注    意：
*********************************************************************************************************/
ErrStatus can_driver_send_standard(uint32_t std_id, const uint8_t *data, uint8_t len)
{
    can_trasnmit_message_struct msg;  // CAN消息结构体 集中管理消息所有字段，符合"数据聚合"原则
		uint8_t mailbox;  // 硬件邮箱编号
    uint32_t timeout = CAN_TIMEOUT; // 超时计数器（宏定义，可配置） 防御性编程体现，防止硬件死锁导致系统卡死
    //空指针检查：防止解引用无效指针导致系统崩溃
	  //这里没有检查len=0的情况，因为CAN协议允许空数据帧（用于网络管理、心跳等场景）
    if ((data == NULL) || (len > 8U)) {
        return ERROR;
    }

    msg.tx_sfid = std_id & CAN_SFID_MASK;  // 标准帧ID掩码处理  CAN_SFID_MASK 通常为0x7ff确保11ID不会越界
    msg.tx_efid = 0U; // 扩展帧ID清零
    msg.tx_ff = CAN_FF_STANDARD;  // 帧格式：标准帧
    msg.tx_ft = CAN_FT_DATA; // 帧类型：数据帧（非远程帧）
    msg.tx_dlen = len; // 数据长度

    for (uint8_t i = 0U; i < 8U; i++) {
        msg.tx_data[i] = (i < len) ? data[i] : 0U;
    }
    //硬件邮箱分配机制 这种机制实现了非阻塞尝试发送，上层应用需根据返回错误决定重试策略。
    mailbox = can_message_transmit(CAN_DRIVER_CAN_PERIPH, &msg);
		//CAN_NOMAILBOX：所有邮箱都在忙（发送中），硬件资源耗尽
    if (mailbox == CAN_NOMAILBOX) {
        return ERROR;
    }
    // 状态轮询与超时保护
		/*
			CAN_TRANSMIT_PENDING  发送中（轮询状态）
		  CAN_TRANSMIT_OK  发送成功（返回成功）
		  CAN_TRANSMIT_FAILED 发送失败（返回错误）
		*/
    while ((can_transmit_states(CAN_DRIVER_CAN_PERIPH, mailbox) == CAN_TRANSMIT_PENDING) && (timeout > 0U)) {
        timeout--;
    }
		//函数在超时退出后没有立即返回错误，而是再次检查状态。这是因为：超时期间可能刚好完成发送,避免竞态条件导致误判
    if (timeout == 0U) {
        return ERROR;
    }

    return (can_transmit_states(CAN_DRIVER_CAN_PERIPH, mailbox) == CAN_TRANSMIT_OK) ? SUCCESS : ERROR;
}

bool can_driver_receive(can_receive_message_struct *message, uint8_t fifo)
{
    if (message == NULL) {
        return false;
    }

    if (can_receive_message_length_get(CAN_DRIVER_CAN_PERIPH, fifo) == 0U) {
        return false;
    }

    can_message_receive(CAN_DRIVER_CAN_PERIPH, fifo, message);
    return true;
}













