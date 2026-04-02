#include "llc_trace.h"
#include "llc_control.h"
#include "llc_cr_proto.h"
#include "llc_log_dump.h"
#include "pwm_llc.h"
#include "debug_printf.h"
#include "math.h"
#if LLC_COLLAPSE_TRACE_ENABLE

typedef struct
{
    llc_trace_sample_t buf[LLC_COLLAPSE_TRACE_PRE_MS + LLC_COLLAPSE_TRACE_POST_MS + 2U];
    uint16_t wr;
    uint16_t size;
    uint8_t active;
    uint8_t post_left;
    uint32_t last_trigger_ms;
    uint8_t trigger_inhibit;
    float last_vout_v;
} llc_collapse_trace_ctx_t;

typedef struct
{
    uint8_t pending;
    uint8_t emit_idx;
    llc_trace_sample_t pre;
    llc_trace_sample_t trig;
    llc_trace_sample_t post;
    uint8_t verdict;
    uint8_t reason_state;
    float dv_v;
    float df_cmd_hz;
    float df_act_hz;
} llc_collapse_emit_ctx_t;

static llc_collapse_trace_ctx_t s_collapse_trace;
static llc_collapse_emit_ctx_t s_collapse_emit;

static void llc_collapse_trace_push(const llc_trace_sample_t *s)
{
    uint16_t cap = (uint16_t)(sizeof(s_collapse_trace.buf) / sizeof(s_collapse_trace.buf[0]));
    s_collapse_trace.buf[s_collapse_trace.wr] = *s;
    s_collapse_trace.wr = (uint16_t)((s_collapse_trace.wr + 1U) % cap);
    if (s_collapse_trace.size < cap) {
        s_collapse_trace.size++;
    }
}

static void llc_collapse_trace_dump(uint8_t reason_state)
{
    uint16_t cap = (uint16_t)(sizeof(s_collapse_trace.buf) / sizeof(s_collapse_trace.buf[0]));
    uint16_t cnt = s_collapse_trace.size;
    uint16_t start_idx;
    uint16_t trig_off;
    uint16_t idx_pre;
    uint16_t idx_trig;
    uint16_t idx_post;
    const llc_trace_sample_t *pre;
    const llc_trace_sample_t *trig;
    const llc_trace_sample_t *post;
    float dv_v;
    float df_cmd_hz;
    float df_act_hz;
    uint8_t verdict = 0U;

    if (cnt == 0U) {
        return;
    }

    start_idx = (uint16_t)((s_collapse_trace.wr + cap - cnt) % cap);
    trig_off = (cnt > (LLC_COLLAPSE_TRACE_POST_MS + 1U)) ?
               (uint16_t)(cnt - LLC_COLLAPSE_TRACE_POST_MS - 1U) : 0U;

    idx_pre = start_idx;
    idx_trig = (uint16_t)((start_idx + trig_off) % cap);
    idx_post = (uint16_t)((start_idx + cnt - 1U) % cap);

    pre = &s_collapse_trace.buf[idx_pre];
    trig = &s_collapse_trace.buf[idx_trig];
    post = &s_collapse_trace.buf[idx_post];

    dv_v = trig->vout_v - pre->vout_v;
    df_cmd_hz = trig->f_cmd_hz - pre->f_cmd_hz;
    df_act_hz = (float)trig->f_act_hz - (float)pre->f_act_hz;

    if ((pre->state != trig->state) || (fabsf(df_cmd_hz) > 3000.0f)) {
        verdict = 1U;
    } else if (fabsf(trig->f_cmd_hz - (float)trig->f_act_hz) < 2500.0f) {
        verdict = 2U;
    }

    s_collapse_emit.pre = *pre;
    s_collapse_emit.trig = *trig;
    s_collapse_emit.post = *post;
    s_collapse_emit.verdict = verdict;
    s_collapse_emit.reason_state = reason_state;
    s_collapse_emit.dv_v = dv_v;
    s_collapse_emit.df_cmd_hz = df_cmd_hz;
    s_collapse_emit.df_act_hz = df_act_hz;
    s_collapse_emit.emit_idx = 0U;
    s_collapse_emit.pending = 1U;
}

static void llc_collapse_trace_drain_budget(uint8_t budget)
{
    while ((budget > 0U) && s_collapse_emit.pending) {
        if (llc_log_dump_busy()) {
            break;
        }

        if (s_collapse_emit.emit_idx <= 2U) {
            if (debug_tx_available() < 18) {
                break;
            }

            if (s_collapse_emit.emit_idx == 0U) {
                llc_cr_proto_collapse_emit_bin(0U,
                                               s_collapse_emit.pre.t_ms,
                                               s_collapse_emit.pre.state,
                                               s_collapse_emit.pre.vout_v,
                                               s_collapse_emit.pre.f_cmd_hz,
                                               s_collapse_emit.pre.f_act_hz);
            } else if (s_collapse_emit.emit_idx == 1U) {
                llc_cr_proto_collapse_emit_bin(1U,
                                               s_collapse_emit.trig.t_ms,
                                               s_collapse_emit.trig.state,
                                               s_collapse_emit.trig.vout_v,
                                               s_collapse_emit.trig.f_cmd_hz,
                                               s_collapse_emit.trig.f_act_hz);
            } else {
                llc_cr_proto_collapse_emit_bin(2U,
                                               s_collapse_emit.post.t_ms,
                                               s_collapse_emit.post.state,
                                               s_collapse_emit.post.vout_v,
                                               s_collapse_emit.post.f_cmd_hz,
                                               s_collapse_emit.post.f_act_hz);
            }
            s_collapse_emit.emit_idx++;
        } else {
            if (debug_tx_available() < 14) {
                break;
            }
            llc_cr_proto_collapse_diag_emit_bin(s_collapse_emit.verdict,
                                                s_collapse_emit.reason_state,
                                                s_collapse_emit.dv_v,
                                                s_collapse_emit.df_cmd_hz,
                                                s_collapse_emit.df_act_hz);
            s_collapse_emit.pending = 0U;
            s_collapse_emit.emit_idx = 0U;
        }

        budget--;
    }
}

#endif

void llc_trace_init(void)
{
    llc_trace_reset();
}

void llc_trace_reset(void)
{
#if LLC_COLLAPSE_TRACE_ENABLE
    s_collapse_trace.wr = 0U;
    s_collapse_trace.size = 0U;
    s_collapse_trace.active = 0U;
    s_collapse_trace.post_left = 0U;
    s_collapse_trace.trigger_inhibit = 0U;
    s_collapse_trace.last_trigger_ms = 0U;
    s_collapse_trace.last_vout_v = 0.0f;

    s_collapse_emit.pending = 0U;
    s_collapse_emit.emit_idx = 0U;
#endif
}

void llc_trace_fast_tick(const llc_trace_sample_t *s)
{
    (void)s;
}

void llc_trace_slow_tick(const llc_trace_sample_t *s)
{
#if LLC_COLLAPSE_TRACE_ENABLE
    if (s == NULL) {
        return;
    }

    if ((s->state != ST_LLC_RUN) && (s->state != ST_BURST_MODE)) {
        s_collapse_trace.active = 0U;
        s_collapse_trace.post_left = 0U;
        s_collapse_trace.trigger_inhibit = 0U;
        s_collapse_trace.last_vout_v = s->vout_v;
        return;
    }

    llc_collapse_trace_push(s);

    if (s_collapse_trace.active) {
        if (s_collapse_trace.post_left > 0U) {
            s_collapse_trace.post_left--;
        }
        if (s_collapse_trace.post_left == 0U) {
            llc_collapse_trace_dump((uint8_t)s->state);
            s_collapse_trace.active = 0U;
            s_collapse_trace.trigger_inhibit = 1U;
            s_collapse_trace.last_trigger_ms = s->t_ms;
        }
    } else if (s_collapse_trace.trigger_inhibit == 0U) {
        float dv = s_collapse_trace.last_vout_v - s->vout_v;
        uint8_t abs_hit = (s->vout_v <= LLC_COLLAPSE_VOUT_ABS_MIN_V) ? 1U : 0U;
        uint8_t drop_hit = (dv >= LLC_COLLAPSE_VOUT_DROP_V) ? 1U : 0U;
        uint8_t rearmed = (s_collapse_trace.last_trigger_ms == 0U) ||
                          elapsed_reached(s_collapse_trace.last_trigger_ms, LLC_COLLAPSE_TRACE_REARM_MS);

        if (rearmed && (abs_hit || drop_hit)) {
            s_collapse_trace.active = 1U;
            s_collapse_trace.post_left = (uint8_t)LLC_COLLAPSE_TRACE_POST_MS;
        }
    }

    s_collapse_trace.last_vout_v = s->vout_v;
    llc_collapse_trace_drain_budget(1U);
#else
    (void)s;
#endif
}

void llc_trace_on_state_change(uint8_t prev, uint8_t next, uint32_t t_ms)
{
    (void)prev;
    (void)next;
    (void)t_ms;
}

uint8_t llc_trace_cr_pending(void)
{
    return llc_log_dump_busy();
}

uint16_t llc_trace_cr_pending_count(void)
{
    return llc_log_dump_pending_count();
}

uint8_t llc_trace_cr_peek(llc_trace_cr_item_t *out)
{
    (void)out;
    return false;
}

uint8_t llc_trace_cr_pop(llc_trace_cr_item_t *out)
{
    (void)out;
    return false;
}

uint32_t llc_trace_cr_overflow_count(void)
{
    return 0U;
}

uint8_t llc_trace_tx_pending(void)
{
#if LLC_COLLAPSE_TRACE_ENABLE
    return (s_collapse_emit.pending != 0U);
#else
    return false;
#endif
}

uint8_t llc_trace_tx_peek(llc_trace_tx_item_t *out)
{
    (void)out;
    return false;
}

void llc_trace_tx_consume(void)
{
}