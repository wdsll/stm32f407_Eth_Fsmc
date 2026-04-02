#include "llc_log_dump.h"
#include "main.h"
#include "debug_printf.h"

#ifndef LLC_CR_RESP_LOG_ENABLE
#define LLC_CR_RESP_LOG_ENABLE        (1U)
#endif

#ifndef LLC_CR_RESP_LOG_CACHE_MAX
#define LLC_CR_RESP_LOG_CACHE_MAX      (256U)
#endif

static llc_cr_log_item_t s_cr_log_buf[LLC_CR_RESP_LOG_CACHE_MAX];
static uint16_t s_cr_log_w = 0U;
static uint16_t s_cr_log_r = 0U;
static uint16_t s_cr_log_cnt = 0U;
static uint8_t  s_cr_log_pending_dump = 0U;
static uint32_t s_cr_log_overflow_cnt = 0U;
static uint32_t s_cr_log_overflow_reported = 0U;

static uint16_t llc_cr_log_frame_bytes(const llc_cr_log_item_t *item)
{
    uint8_t payload_len = 0U;

    if (item == NULL) {
        return 0U;
    }

    switch (item->type) {
    case CR_LOG_MON:
        payload_len = 10U;
        break;
    case CR_LOG_EVT_BEGIN:
        payload_len = 8U;
        break;
    case CR_LOG_EVT_END:
        payload_len = 14U;
        break;
    default:
        return 0U;
    }

    return (uint16_t)(6U + payload_len);
}

static void llc_cr_resp_log_dump_limited(uint8_t max_items, uint8_t allow_mon)
{
#if LLC_CR_RESP_LOG_ENABLE
    uint8_t dumped = 0U;

    if (max_items == 0U) {
        return;
    }

    while (dumped < max_items) {
        llc_cr_log_item_t item;
        uint32_t primask;
        uint16_t frame_len;
        uint8_t can_pop = 0U;

        primask = __get_PRIMASK();
        __disable_irq();

        if (s_cr_log_cnt == 0U) {
            s_cr_log_pending_dump = 0U;
            __set_PRIMASK(primask);
            break;
        }

        item = s_cr_log_buf[s_cr_log_r];
        __set_PRIMASK(primask);

        if ((item.type == CR_LOG_MON) && (allow_mon == 0U)) {
            primask = __get_PRIMASK();
            __disable_irq();

            if (s_cr_log_cnt > 0U) {
                s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
                s_cr_log_cnt--;
                s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;
            } else {
                s_cr_log_pending_dump = 0U;
            }

            __set_PRIMASK(primask);
            dumped++;
            continue;
        }

        frame_len = llc_cr_log_frame_bytes(&item);

        if ((item.type == CR_LOG_EVT_BEGIN) || (item.type == CR_LOG_EVT_END)) {
            if (debug_tx_available() < (int)frame_len) {
                break;
            }
        }

        if (item.type == CR_LOG_MON) {
            if (debug_tx_available() < (int)frame_len) {
                primask = __get_PRIMASK();
                __disable_irq();

                if (s_cr_log_cnt > 0U) {
                    s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
                    s_cr_log_cnt--;
                    s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;
                } else {
                    s_cr_log_pending_dump = 0U;
                }

                __set_PRIMASK(primask);
                dumped++;
                continue;
            }
        }

        primask = __get_PRIMASK();
        __disable_irq();

        if (s_cr_log_cnt > 0U) {
            item = s_cr_log_buf[s_cr_log_r];
            s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
            s_cr_log_cnt--;
            s_cr_log_pending_dump = (s_cr_log_cnt > 0U) ? 1U : 0U;
            can_pop = 1U;
        } else {
            s_cr_log_pending_dump = 0U;
        }

        __set_PRIMASK(primask);

        if (can_pop) {
            llc_cr_proto_log_emit_bin(&item);
            dumped++;
        } else {
            break;
        }
    }
#else
    (void)max_items;
    (void)allow_mon;
#endif
}

void llc_log_dump_init(void)
{
    llc_log_dump_reset();
}

void llc_log_dump_reset(void)
{
    s_cr_log_w = 0U;
    s_cr_log_r = 0U;
    s_cr_log_cnt = 0U;
    s_cr_log_pending_dump = 0U;
    s_cr_log_overflow_cnt = 0U;
    s_cr_log_overflow_reported = 0U;
}

void llc_log_dump_push(const llc_cr_log_item_t *item)
{
#if LLC_CR_RESP_LOG_ENABLE
    uint16_t idx;
    uint32_t primask;

    if (item == NULL) {
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    idx = s_cr_log_w;
    s_cr_log_buf[idx] = *item;
    s_cr_log_w = (uint16_t)((s_cr_log_w + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);

    if (s_cr_log_cnt < LLC_CR_RESP_LOG_CACHE_MAX) {
        s_cr_log_cnt++;
    } else {
        s_cr_log_r = (uint16_t)((s_cr_log_r + 1U) % LLC_CR_RESP_LOG_CACHE_MAX);
        s_cr_log_overflow_cnt++;
    }

    s_cr_log_pending_dump = 1U;
    __set_PRIMASK(primask);
#else
    (void)item;
#endif
}

void llc_log_dump_service(uint8_t  run_state, uint8_t  power_stage_idle)
{
#if LLC_CR_RESP_LOG_ENABLE
    static uint32_t s_cr_dump_last_ms = 0U;
    static uint32_t s_cr_mon_last_ms = 0U;

    if (s_cr_log_pending_dump &&
        (s_cr_dump_last_ms == 0U || elapsed_reached(s_cr_dump_last_ms, run_state ? 2U : 5U))) {
        uint8_t allow_mon = 1U;
        uint8_t dump_n = 4U;

        if (run_state) {
            allow_mon = (s_cr_mon_last_ms == 0U || elapsed_reached(s_cr_mon_last_ms, 100U)) ? 1U : 0U;
            if (allow_mon) {
                s_cr_mon_last_ms = g_ms;
            }
            dump_n = (s_cr_log_cnt > (LLC_CR_RESP_LOG_CACHE_MAX / 2U)) ? 12U : 6U;
        } else if (power_stage_idle) {
            dump_n = (s_cr_log_cnt > (LLC_CR_RESP_LOG_CACHE_MAX / 2U)) ? 10U : 5U;
        }

        s_cr_dump_last_ms = g_ms;
        llc_cr_resp_log_dump_limited(dump_n, allow_mon);
    }
#else
    (void)run_state;
    (void)power_stage_idle;
#endif
}

void llc_log_dump_flush_all(void)
{
#if LLC_CR_RESP_LOG_ENABLE
    while (s_cr_log_pending_dump || (s_cr_log_cnt > 0U)) {
        llc_cr_resp_log_dump_limited(16U, 1U);
    }
#endif
}

uint8_t  llc_log_dump_busy(void)
{
    return (s_cr_log_pending_dump != 0U) || (s_cr_log_cnt > 0U);
}

uint16_t llc_log_dump_pending_count(void)
{
    return s_cr_log_cnt;
}