#ifndef AUX_POWER_H
#define AUX_POWER_H

#include "main.h"
typedef struct
{
	float v3v3_v;
	float vbat_v;
	float v3v3_min_v;
	float vbat_min_v;
	uint32_t last_update_ms;
	uint32_t drop_detected_ms; //电压下降被检测到的时间戳
	uint32_t restore_detected_ms; //电压恢复被检测到的时间戳
	bool power_ok;
} aux_power_monitor_t;

void aux_power_monitor_update(float v3v3_v, float vbat_v);
bool aux_power_ok_now(void);
bool aux_power_ok_stable_since(uint32_t ms);
bool aux_power_brownout_stable(uint32_t ms);
const aux_power_monitor_t *aux_power_monitor_state(void);

#endif /* AUX_POWER_H */