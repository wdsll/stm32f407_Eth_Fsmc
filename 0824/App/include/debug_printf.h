/* debug_printf.h - µ÷ÊÔ´®¿Ú (·Ç×èÈû) */
#ifndef _DEBUG_PRINTF_H_
#define _DEBUG_PRINTF_H_
#include "main.h"

void debug_printf_init(uint32_t baudrate);
void debug_printf(const char *fmt, ...);
void debug_tx_task(void);
uint32_t debug_buffer_used(void);

#endif
