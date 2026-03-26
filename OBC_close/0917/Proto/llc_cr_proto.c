#include "main.h"
#include "llc_cr_proto.h"
#include "debug_printf.h"
/**
 * @brief 将浮点数转换为Q10定点数格式
 * 
 * Q10格式表示：10位小数，数值范围为[-3276.8, 3276.7]
 * 
 * @param x 待转换的浮点数
 * @return int16_t 转换后的Q10格式定点数
 * 
 * @note 当输入值超出Q10格式表示范围时，进行饱和截断处理
 *       - 正溢出：返回32767
 *       - 负溢出：返回-32768
 */
static inline int16_t q10_from_float(float x)
{
    if (x >= 3276.7f) {
        return 32767;
    }
    if (x <= -3276.8f) {
        return -32768;
    }
    return (int16_t)(x * 10.0f);
}

/**
 * @brief 将32位无符号整数饱和转换为16位无符号整数
 * @param x 待转换的32位无符号整数
 * @return 转换后的16位无符号整数，若输入值超过65535则返回65535
 */
static inline uint16_t u16_sat_from_u32(uint32_t x)
{
    return (x > 65535UL) ? 65535U : (uint16_t)x;
}
/**
 * @brief 将16位无符号整数以小端序格式写入字节数组
 * @param p 目标字节数组指针，用于存储转换后的数据
 * @param v 要转换的16位无符号整数值
 * 
 * @note 小端序：低字节存储在低地址，高字节存储在高地址
 * @note 该函数为内联函数，适合高频调用的场景
 */
static inline void put_u16_le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
}
/**
 * @brief 将16位有符号整数以小端字节序写入缓冲区
 * @param p 目标缓冲区指针
 * @param v 要写入的16位有符号整数值
 * @note 内部调用 put_u16_le 函数实现，将值转换为无符号类型后写入
 */
static inline void put_i16_le(uint8_t *p, int16_t v)
{
    put_u16_le(p, (uint16_t)v);
}

/**
 * @brief 写入二进制数据到调试串口
 * @note 复用 debug 非阻塞环形队列，避免 CR 发送阻塞串口
 * 
 * @param buf 待写入的数据缓冲区指针
 * @param len 待写入的数据长度
 * 
 * @return 无
 */
static void llc_cr_proto_write_binary(const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U)) {
        return;
    }

    /* 复用 debug 非阻塞环形队列，避免 CR 发送阻塞串口。 */
    debug_write_raw(buf, (size_t)len);
}

static uint8_t s_cr_uart_seq = 0U;
/**
 * @brief 发送LLC电流响应(CR)协议二进制帧
 * @details 构造包含帧头、类型、长度、序列号、有效载荷和校验和的完整二进制帧，
 *          并通过UART发送。帧格式为：[0xA5][类型][长度][序列号][有效载荷][校验和][0x5A]
 * @param type 帧类型标识符
 * @param payload 有效载荷数据指针，可为NULL（当len=0时）
 * @param len 有效载荷数据长度（字节）
 * @note 校验和采用XOR异或算法计算，覆盖从帧头到有效载荷的所有字节
 * @note 序列号s_cr_uart_seq在每次发送后自增
	 [0xA5][类型][长度][序列号][有效载荷][校验和][0x5A]
 */
static void llc_cr_proto_send_bin_frame(uint8_t type, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[40];
    uint8_t i;
    uint8_t chk = 0U;
    uint8_t idx = 0U;

    if ((payload == NULL) && (len > 0U)) {
        return;
    }

    frame[idx++] = 0xA5U;
    frame[idx++] = type;
    frame[idx++] = len;
    frame[idx++] = s_cr_uart_seq++; // 自动递增序列号

    for (i = 0U; i < len; i++) {
        frame[idx++] = payload[i];
    }

    for (i = 0U; i < idx; i++) {
        chk ^= frame[i];  // XOR校验
    }

    frame[idx++] = chk;
    frame[idx++] = 0x5AU;

    llc_cr_proto_write_binary(frame, idx);
}
/**
 * @brief 发送LLC电流响应日志项的二进制帧
 * 
 * 根据日志项的类型（监控数据、事件开始、事件结束）将数据打包成二进制格式，
 * 并通过LLC电流响应协议发送。支持三种日志类型的编码和传输：
 * - CR_LOG_MON: 监控数据（时间戳、输出电压、输出电流、误差、频率）
 * - CR_LOG_EVT_BEGIN: 事件开始（事件ID、电流跳变范围、预置电压）
 * - CR_LOG_EVT_END: 事件结束（事件ID、测试结果、耗时、稳定时间、电压统计）
 * 
 * @param item 指向要发送的日志项结构体指针，若为NULL则直接返回
 * 
 * @note 函数内部使用20字节固定缓冲区进行数据打包
 * @note 数据采用小端序编码，浮点数转换为Q10定点数格式
 * @note 时间戳等32位数据会进行饱和截断到16位范围
 */
void llc_cr_proto_log_emit_bin(const llc_cr_log_item_t *item)
{
    uint8_t payload[20];
    uint8_t len = 0U;

    if (item == NULL) {
        return;
    }

    switch (item->type) {
    case CR_LOG_MON:
        put_u16_le(&payload[0], u16_sat_from_u32(item->t_ms));
        put_i16_le(&payload[2], q10_from_float(item->vout_v));
        put_i16_le(&payload[4], q10_from_float(item->iout_a));
        put_i16_le(&payload[6], q10_from_float(item->err_v));
        put_u16_le(&payload[8], u16_sat_from_u32((uint32_t)(item->f_cmd_hz / 10.0f)));
        len = 10U;
        llc_cr_proto_send_bin_frame(0x01U, payload, len);
        break;

    case CR_LOG_EVT_BEGIN:
        put_u16_le(&payload[0], u16_sat_from_u32(item->id));
        put_i16_le(&payload[2], q10_from_float(item->iout_from_a));
        put_i16_le(&payload[4], q10_from_float(item->iout_to_a));
        put_i16_le(&payload[6], q10_from_float(item->vout_pre_v));
        len = 8U;
        llc_cr_proto_send_bin_frame(0x02U, payload, len);
        break;

    case CR_LOG_EVT_END:
        put_u16_le(&payload[0], u16_sat_from_u32(item->id));
        payload[2] = item->pass;
        payload[3] = 0U;
        put_u16_le(&payload[4], u16_sat_from_u32(item->dt_ms));
        put_u16_le(&payload[6], u16_sat_from_u32(item->settle_ms));
        put_i16_le(&payload[8], q10_from_float(item->vmin_v));
        put_i16_le(&payload[10], q10_from_float(item->vmax_v));
        put_i16_le(&payload[12], q10_from_float(item->maxerr_v));
        len = 14U;
        llc_cr_proto_send_bin_frame(0x03U, payload, len);
        break;

    default:
        break;
    }
}
