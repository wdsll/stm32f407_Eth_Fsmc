#include "gd32f30x.h"
#include <stdint.h>
//#include <stdbool.h>


typedef struct { 
	uint32_t pwm_hz;
	uint32_t deadtime_ns;
	float duty;
	} llc_pwm_cfg_t;

void llc_pwm_init(const llc_pwm_cfg_t* cfg);
void llc_pwm_set_duty(float duty);
void llc_pwm_outputs_enable(bool en);
void llc_pwm_break(bool en);
void llc_pwm_set_freq(uint32_t f_hz);
