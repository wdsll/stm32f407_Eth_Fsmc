#include "charge_ctrl.h"
#include "llc_control.h"
#include "add_dma.h"

#ifndef CHARGE_OUT_RELAY_ACTIVE_LEVEL
#define CHARGE_OUT_RELAY_ACTIVE_LEVEL   1   /* 1: GPIO置位吸合; 0: GPIO复位吸合 */
#endif

typedef struct
{
    charge_state_t state;
    uint32_t entry_ms;
    uint32_t detect_ms;
    uint32_t term_ms;
    float vbat_v;
    float vout_v;
    float iout_a;
} charge_rt_t;

static charge_cfg_t s_chg_cfg =
{
    .cv_target_v         = 54.75f,
    .cc_target_a         = 40.0f,
    .precharge_current_a = 2.0f,
    .precharge_margin_v  = 1.5f,
    .v_match_window_v    = 1.0f,
    .vbat_present_min_v  = 20.0f,
    .vbat_absent_max_v   = 5.0f,
    .cv_enter_margin_v   = 0.5f,
    .term_current_a      = 2.0f,
    .detect_debounce_ms  = 200U,
    .precharge_hold_ms   = 200U,
    .relay_settle_ms     = 100U,
    .term_hold_ms        = 3000U,
};

static charge_rt_t s_chg_rt;

static inline float charge_conv_adc_to_v(uint16_t raw, float rtop, float rbot)
{
    float v = ((float)raw * VREF_ADC) / 4095.0f;
    return v * ((rtop + rbot) / rbot);
}

static void charge_out_relay_set(bool on)
{
    static bool init = false;

    if (!init) {
        rcu_periph_clock_enable(OUT_RELAY_RCU);
        gpio_init(OUT_RELAY, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, OUT_RELAY_PIN);

#if CHARGE_OUT_RELAY_ACTIVE_LEVEL
        gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
#else
        gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
#endif
        init = true;
    }

#if CHARGE_OUT_RELAY_ACTIVE_LEVEL
    if (on) 
			gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
    else    
			gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
#else
    if (on) gpio_bit_reset(OUT_RELAY, OUT_RELAY_PIN);
    else    gpio_bit_set(OUT_RELAY, OUT_RELAY_PIN);
#endif
}

static void charge_enter(charge_state_t st)
{
    s_chg_rt.state = st;
    s_chg_rt.entry_ms = g_ms;
    s_chg_rt.term_ms = 0U;
}

static void charge_update_meas(void)
{
    llc_status_t llc_st = {0};

    llc_get_status(&llc_st);

    s_chg_rt.vbat_v = charge_conv_adc_to_v(g_adc_multi.vbt_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
    s_chg_rt.vout_v = llc_st.vout_v;
    s_chg_rt.iout_a = llc_st.iout_a;
}

static void charge_apply(bool llc_run, bool relay_on, float vref, float iref)
{
    llc_set_vref(vref);
    llc_set_iref(iref);
    llc_set_run_request(llc_run);
    charge_out_relay_set(relay_on);
}

void charge_ctrl_init(void)
{
    s_chg_rt.state = CHG_ST_IDLE;
    s_chg_rt.entry_ms = 0U;
    s_chg_rt.detect_ms = 0U;
    s_chg_rt.term_ms = 0U;
    s_chg_rt.vbat_v = 0.0f;
    s_chg_rt.vout_v = 0.0f;
    s_chg_rt.iout_a = 0.0f;

    charge_out_relay_set(false);
    llc_set_run_request(false);
    llc_set_vref(s_chg_cfg.cv_target_v);
    llc_set_iref(s_chg_cfg.precharge_current_a);
}

void charge_ctrl_set_cfg(const charge_cfg_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    s_chg_cfg = *cfg;
}

void charge_ctrl_set_target(float cv_v, float cc_a)
{
    s_chg_cfg.cv_target_v = cv_v;
    s_chg_cfg.cc_target_a = cc_a;
}

charge_state_t charge_ctrl_state(void)
{
    return s_chg_rt.state;
}

void charge_ctrl_get_status(charge_status_t *st)
{
    if (st == NULL) {
        return;
    }

    st->state  = s_chg_rt.state;
    st->vbat_v = s_chg_rt.vbat_v;
    st->vout_v = s_chg_rt.vout_v;
    st->iout_a = s_chg_rt.iout_a;
}

void charge_ctrl_tick_1khz(void)
{
    charge_update_meas();

    /* LLC自己已经故障，直接转FAULT */
    if (llc_is_fault_state()) {
        charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);
        charge_enter(CHG_ST_FAULT);
        return;
    }

    switch (s_chg_rt.state)
    {
    default:
    case CHG_ST_IDLE:
    {
        charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);

        if (s_chg_rt.vbat_v >= s_chg_cfg.vbat_present_min_v) {
            if (s_chg_rt.detect_ms == 0U) {
                s_chg_rt.detect_ms = g_ms;
            } else if (elapsed_reached(s_chg_rt.detect_ms, s_chg_cfg.detect_debounce_ms)) {
                charge_enter(CHG_ST_PRECHARGE);
                s_chg_rt.detect_ms = 0U;
            }
        } else {
            s_chg_rt.detect_ms = 0U;
        }
        break;
    }

    case CHG_ST_PRECHARGE:
    {
        float vref = s_chg_rt.vbat_v + s_chg_cfg.precharge_margin_v;
        if (vref > s_chg_cfg.cv_target_v) {
            vref = s_chg_cfg.cv_target_v;
        }

        charge_apply(true, false, vref, s_chg_cfg.precharge_current_a);

        if (s_chg_rt.vbat_v < s_chg_cfg.vbat_absent_max_v) {
            charge_enter(CHG_ST_IDLE);
            break;
        }

        if (fabsf(s_chg_rt.vout_v - s_chg_rt.vbat_v) <= s_chg_cfg.v_match_window_v) {
            if (s_chg_rt.term_ms == 0U) {
                s_chg_rt.term_ms = g_ms;
            } else if (elapsed_reached(s_chg_rt.term_ms, s_chg_cfg.precharge_hold_ms)) {
                charge_enter(CHG_ST_RELAY_ON);
                s_chg_rt.term_ms = 0U;
            }
        } else {
            s_chg_rt.term_ms = 0U;
        }
        break;
    }

    case CHG_ST_RELAY_ON:
    {
        float vref = s_chg_rt.vbat_v + s_chg_cfg.precharge_margin_v;
        if (vref > s_chg_cfg.cv_target_v) {
            vref = s_chg_cfg.cv_target_v;
        }

        charge_apply(true, true, vref, s_chg_cfg.precharge_current_a);

        if (elapsed_reached(s_chg_rt.entry_ms, s_chg_cfg.relay_settle_ms)) {
            charge_enter(CHG_ST_CC);
        }
        break;
    }

    case CHG_ST_CC:
    {
        charge_apply(true, true, s_chg_cfg.cv_target_v, s_chg_cfg.cc_target_a);

        if (s_chg_rt.vbat_v >= (s_chg_cfg.cv_target_v - s_chg_cfg.cv_enter_margin_v)) {
            charge_enter(CHG_ST_CV);
        }
        break;
    }

    case CHG_ST_CV:
    {
        charge_apply(true, true, s_chg_cfg.cv_target_v, s_chg_cfg.cc_target_a);

        if (s_chg_rt.iout_a <= s_chg_cfg.term_current_a) {
            if (s_chg_rt.term_ms == 0U) {
                s_chg_rt.term_ms = g_ms;
            } else if (elapsed_reached(s_chg_rt.term_ms, s_chg_cfg.term_hold_ms)) {
                charge_enter(CHG_ST_DONE);
                s_chg_rt.term_ms = 0U;
            }
        } else {
            s_chg_rt.term_ms = 0U;
        }
        break;
    }

    case CHG_ST_DONE:
    {
        charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);

        if (s_chg_rt.vbat_v < s_chg_cfg.vbat_absent_max_v) {
            charge_enter(CHG_ST_IDLE);
        }
        break;
    }

    case CHG_ST_FAULT:
    {
        charge_apply(false, false, s_chg_cfg.cv_target_v, s_chg_cfg.precharge_current_a);
        break;
    }
    }
}