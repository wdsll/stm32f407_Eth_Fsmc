#ifndef CHARGER_CONTROL_H
#define CHARGER_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CHARGER_OFF = 0,
    CHARGER_PRECHARGE,
    CHARGER_PFC_WAIT,
    CHARGER_LLC_SOFTSTART,
    CHARGER_RUNNING,
    CHARGER_FAULT
} charger_state_t;

typedef struct {
    uint32_t bus_mv;
    uint32_t output_mv;
    uint32_t current_ma;
    int16_t hottest_temperature_dc;
    bool hardware_fault;
    bool llc_fault;
    bool start_request;
} charger_inputs_t;

typedef struct {
    bool pfc_relay;
    bool output_relay;
    bool llc_enable;
    uint16_t cv_pwm_permille;
    uint16_t current_pwm_permille;
} charger_outputs_t;

typedef struct {
    charger_state_t state;
    uint32_t state_time_ms;
    charger_outputs_t outputs;
} charger_control_t;

void charger_control_init(charger_control_t *control);
void charger_control_step(charger_control_t *control,
                          const charger_inputs_t *inputs,
                          uint32_t elapsed_ms);
void charger_control_clear_fault(charger_control_t *control,
                                 const charger_inputs_t *inputs);

#endif
