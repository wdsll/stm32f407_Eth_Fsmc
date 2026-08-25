#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include "main.h"

void serial_console_init(void);
void serial_console_task(void);

bool serial_console_pfc_test_active(void);

#endif