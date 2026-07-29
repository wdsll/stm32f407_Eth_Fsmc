#include <assert.h>
#include "charger_config.h"
#include "charger_control.h"

int main(void)
{
    charger_control_t c;
    charger_inputs_t in = {0};
    charger_control_init(&c);
    assert(c.state == CHARGER_OFF && !c.outputs.llc_enable);
    in.start_request = true;
    charger_control_step(&c, &in, 1u);
    assert(c.state == CHARGER_PRECHARGE && c.outputs.pfc_relay);
    charger_control_step(&c, &in, CHARGER_PRECHARGE_TIME_MS);
    assert(c.state == CHARGER_PFC_WAIT && !c.outputs.llc_enable);
    in.bus_mv = CHARGER_BUS_MIN_MV;
    charger_control_step(&c, &in, CHARGER_RELAY_SETTLE_TIME_MS);
    assert(c.state == CHARGER_LLC_SOFTSTART && c.outputs.llc_enable);
    charger_control_step(&c, &in, CHARGER_SOFTSTART_TIME_MS);
    assert(c.state == CHARGER_RUNNING && c.outputs.output_relay);
    in.current_ma = CHARGER_CURRENT_MAX_MA + 1u;
    charger_control_step(&c, &in, 1u);
    assert(c.state == CHARGER_FAULT && !c.outputs.llc_enable && !c.outputs.pfc_relay);
    in.current_ma = 0u;
    in.start_request = false;
    charger_control_clear_fault(&c, &in);
    assert(c.state == CHARGER_OFF);
    return 0;
}
