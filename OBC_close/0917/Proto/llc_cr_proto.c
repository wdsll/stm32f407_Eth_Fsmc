#include "main.h"
#include "llc_cr_proto.h"

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

static inline uint16_t u16_sat_from_u32(uint32_t x)
{
    return (x > 65535UL) ? 65535U : (uint16_t)x;
}

static inline void put_u16_le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
}

static inline void put_i16_le(uint8_t *p, int16_t v)
{
    put_u16_le(p, (uint16_t)v);
}

static void llc_cr_proto_putc_blocking(uint8_t ch)
{
    while (RESET == usart_flag_get(USART2, USART_FLAG_TBE)) {
    }
    usart_data_transmit(USART2, ch);
}

static void llc_cr_proto_write_binary(const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if ((buf == NULL) || (len == 0U)) {
        return;
    }

    for (i = 0U; i < len; i++) {
        llc_cr_proto_putc_blocking(buf[i]);
    }
}

static uint8_t s_cr_uart_seq = 0U;

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
    frame[idx++] = s_cr_uart_seq++;

    for (i = 0U; i < len; i++) {
        frame[idx++] = payload[i];
    }

    for (i = 0U; i < idx; i++) {
        chk ^= frame[i];
    }

    frame[idx++] = chk;
    frame[idx++] = 0x5AU;

    llc_cr_proto_write_binary(frame, idx);
}

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
