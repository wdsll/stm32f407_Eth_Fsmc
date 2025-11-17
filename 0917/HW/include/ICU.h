#ifndef ICU_H
#define ICU_H

#include "main.h"
#ifdef __cplusplus
extern "C" { 
#endif
void cap_pa01_init(void);
//int cap_pa0_read_period(uint32_t* ticks);
//int cap_pa1_read_period(uint32_t* ticks);

int cap_pa0_read_duty(float* duty);  
int cap_pa1_read_duty(float* duty);

int   cap_pa3_read_bus_voltage(float *vbus);
float bus_voltage_from_pwm(uint32_t high, uint32_t period);

#ifdef __cplusplus
}
#endif
#endif

