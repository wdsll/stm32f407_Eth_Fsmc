#ifndef AUX_POWER_H
#define AUX_POWER_H

#include "main.h"

void aux_power_monitor_update(float v3v3_v, float vbat_v);
bool aux_power_ok_now(void);
bool aux_power_ok_stable_since(uint32_t ms);
bool aux_power_brownout_stable(uint32_t ms);
const aux_power_monitor_t *aux_power_monitor_state(void);

#endif /* AUX_POWER_H */