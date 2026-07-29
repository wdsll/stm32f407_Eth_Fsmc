#include "charger_control.h"
#include "charger_config.h"

static charger_outputs_t safe_outputs(void)
{
    charger_outputs_t out = {0};
    out.cv_pwm_permille = CHARGER_CV_START_PERMILLE;
    out.current_pwm_permille = CHARGER_CURRENT_LIMIT_PERMILLE;
    return out;
}

static bool unsafe(const charger_inputs_t *in)
{
    return in->hardware_fault || in->llc_fault ||
           in->bus_mv > CHARGER_BUS_MAX_MV ||
           in->output_mv > CHARGER_OUTPUT_MAX_MV ||
           in->current_ma > CHARGER_CURRENT_MAX_MA ||
           in->hottest_temperature_dc > CHARGER_TEMPERATURE_MAX_DC;
}

static void enter(charger_control_t *control, charger_state_t state)
{
    control->state = state;
    control->state_time_ms = 0u;
}

void charger_control_init(charger_control_t *control)
{
    control->state = CHARGER_OFF;
    control->state_time_ms = 0u;
    control->outputs = safe_outputs();
}

void charger_control_step(charger_control_t *control,
                          const charger_inputs_t *in,
                          uint32_t elapsed_ms)
{
    if (unsafe(in)) {
        control->outputs = safe_outputs();
        enter(control, CHARGER_FAULT);
        return;
    }
    if (!in->start_request && control->state != CHARGER_FAULT) {
        control->outputs = safe_outputs();
        enter(control, CHARGER_OFF);
        return;
    }

    control->state_time_ms += elapsed_ms;
    switch (control->state) {
    case CHARGER_OFF:
        control->outputs.pfc_relay = true;
        enter(control, CHARGER_PRECHARGE);
        break;
    case CHARGER_PRECHARGE:
        if (control->state_time_ms >= CHARGER_PRECHARGE_TIME_MS)
            enter(control, CHARGER_PFC_WAIT);
        break;
    case CHARGER_PFC_WAIT:
        if (in->bus_mv >= CHARGER_BUS_MIN_MV &&
            control->state_time_ms >= CHARGER_RELAY_SETTLE_TIME_MS) {
            control->outputs.llc_enable = true;
            enter(control, CHARGER_LLC_SOFTSTART);
        }
        break;
    case CHARGER_LLC_SOFTSTART: {
        uint32_t span = CHARGER_CV_START_PERMILLE - CHARGER_CV_RUN_PERMILLE;
        uint32_t time = control->state_time_ms;
        if (time > CHARGER_SOFTSTART_TIME_MS)
            time = CHARGER_SOFTSTART_TIME_MS;
        control->outputs.cv_pwm_permille = (uint16_t)(
            CHARGER_CV_START_PERMILLE - span * time / CHARGER_SOFTSTART_TIME_MS);
        if (control->state_time_ms >= CHARGER_SOFTSTART_TIME_MS) {
            control->outputs.output_relay = true;
            enter(control, CHARGER_RUNNING);
        }
        break;
    }
    case CHARGER_RUNNING:
        break;
    case CHARGER_FAULT:
    default:
        control->outputs = safe_outputs();
        break;
    }
}

void charger_control_clear_fault(charger_control_t *control,
                                 const charger_inputs_t *in)
{
    if (control->state == CHARGER_FAULT && !unsafe(in) && !in->start_request)
        charger_control_init(control);
}
