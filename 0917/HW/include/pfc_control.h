#include "main.h"
typedef enum {
        PFC_ST_IDLE = 0,
        //PFC_ST_CHARGING,
        PFC_ST_READY,
        PFC_ST_FAULT
} pfc_state_t;




void pfc_app_init(void);
void pfc_app_request_start(void);
void pfc_app_force_off(void);
void pfc_app_tick_1khz(float vbus_v);
pfc_state_t pfc_app_state(void);
bool pfc_app_ready(void);
float pfc_bus_voltage(void);
