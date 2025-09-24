#ifndef PROTECT_EXTI_H
#define PROTECT_EXTI_H
#include "main.h"

void protect_exti_init(void);
int  protect_fault_latched(void);
void protect_clear_fault(void);

#endif

