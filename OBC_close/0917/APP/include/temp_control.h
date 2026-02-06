#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t raw;
    float resistance_ohm; 
    float temperature_c;
    bool valid;
    bool over_limit;
} temp_sensor_data_t;

void temp_control_init(void);
void temp_control_tick_1khz(void);
void temp_control_get_pfc(temp_sensor_data_t *out);
void temp_control_get_llc(temp_sensor_data_t *out);

#ifdef __cplusplus
}
#endif

#endif