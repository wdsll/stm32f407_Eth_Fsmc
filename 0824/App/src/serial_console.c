/* USART2 commissioning console for the first software/hardware bench test. */
#include "serial_console.h"
#include "debug_printf.h"
#include "llc_control.h"
#include "adc_dma.h"
#include "protect.h"
#include <stdio.h>
#include <string.h>

#define CONSOLE_LINE_SIZE   (64U)
#define CONSOLE_STATUS_PERIOD_MS   (500U)
#define CONSOLE_WATCHDOG_MS        (5000U)

static char s_line[CONSOLE_LINE_SIZE];
static uint32_t s_line_length;
static uint32_t s_last_status_ms;
static uint32_t s_last_command_ms;

static void print_help(void)
{
    debug_printf("Commands: HELP | STATUS | SET <volt> <amp> | START | STOP | CLEAR | PING\r\n");
    debug_printf("Safety: SET/START/CLEAR only while stopped; enabled output needs PING within 5s.\r\n");
}

static void print_status(void)
{
    debug_printf("STAT ms=%lu state=%u fault=%u en=%u ref=%.1fV/%.1fA "
                 "ac=%.1fV bus=%.1fV out=%.1fV bat=%.1fV i=%.1fA\r\n",
                 (unsigned long)g_ms, (unsigned int)g_charger_state,
                 (unsigned int)g_fault, power_supervisor_requested() ? 1U : 0U,
                 power_supervisor_voltage_reference(),
                 power_supervisor_current_reference(), g_adc_multi.ac_vol_v,
                 g_adc_multi.bus_vol_v, g_adc_multi.vout_v, g_adc_multi.vbat_v,
                 g_adc_multi.iout_a);
}

static void execute_command(char *line)
{
	float voltage_v;
	float current_a;
	
	s_last_command_ms = g_ms;
	if((strcmp(line,"HELP") == 0)||(strcmp(line,"?")==0))
	{
		print_help();
	}
	else if(strcmp(line,"STATUS") == 0)
	{
		print_status();
	}
	else if(strcmp(line,"PING") == 0)
	{
		debug_printf("OK PING\r\n");
	}
	else if(strcmp(line,"STOP") == 0)
	{
		power_supervisor_request(false);
		debug_printf("OK STOP\r\n");
	}
	else if(sscanf(line,"SET %f %f",&voltage_v,&current_a) == 2)
	{
		if(power_supervisor_requested())
		{
			debug_printf("ERR stop before SET\r\n");
		}
		else if((voltage_v < 10.0f)||(voltage_v > 80.0f)||(current_a <= 0.0f)||(current_a > 20.0f))
		{
			debug_printf("ERR range: voltage=10..80V current=0..20A\r\n");
		}
		else
		{
			power_supervisor_set_references(voltage_v,current_a);
			debug_printf("OK SET %.1fV %.1fA\r\n", voltage_v, current_a);
		}	
	}
	else if(strcmp(line,"START") == 0)
	{
		if((g_charger_state == MAIN_STEP_FAULT)||protect_fault_latched()||protect_fault_active_hw())
		{
			debug_printf("ERR fault active; STOP then CLEAR\r\n");
		}
		else if((power_supervisor_voltage_reference() < 10.0f)||(power_supervisor_current_reference() <= 0))
		{
			debug_printf("ERR SET reference first\r\n");
		}
		else
		{
			power_supervisor_request(true);
			debug_printf("OK START\r\n");
		}
	}
	else if(strcmp(line,"CLEAR") == 0)
	{
		if(power_supervisor_requested())
		{
			debug_printf("ERR STOP before CLEAR\r\n");
		}
		//允许 CLEAR 释放外部硬件锁存，采用非阻塞 10ms 清除脉冲和 5ms 释放等待；
		//只有硬件故障输入确实解除后，才清除软件锁存和故障码，从而兼顾故障恢复与防止误复位。
		//else if(protect_fault_active_hw())
		//{
		//	debug_printf("ERR hardware fault still active\r\n");
		//}
		else
		{
			protect_clear_fault_async();
			debug_printf("OK CLEAR requested\r\n");
		}
	}
	else
	{
		debug_printf("ERR unknown command; type HELP\r\n");
	}
}

void serial_console_init(void)
{
    s_line_length = 0U;
    s_last_status_ms = g_ms;
    s_last_command_ms = g_ms;
    debug_printf("[COMM] USART2 commissioning mode, CAN disabled\r\n");
    print_help();
}

void serial_console_task(void)
{
    char ch;
    while (debug_getchar(&ch)) {
        if ((ch == '\r') || (ch == '\n')) {
            if (s_line_length > 0U) {
                s_line[s_line_length] = '\0';
                execute_command(s_line);
                s_line_length = 0U;
            }
        } else if ((ch == '\b') || ((unsigned char)ch == 0x7FU)) {
            if (s_line_length > 0U) s_line_length--;
        } else if ((ch >= 'a') && (ch <= 'z')) {
            if (s_line_length < (CONSOLE_LINE_SIZE - 1U))
                s_line[s_line_length++] = (char)(ch - 'a' + 'A');
        } else if (s_line_length < (CONSOLE_LINE_SIZE - 1U)) {
            s_line[s_line_length++] = ch;
        }
    }

    if (power_supervisor_requested() &&
        elapsed_reached(s_last_command_ms, CONSOLE_WATCHDOG_MS)) {
        power_supervisor_request(false);
        debug_printf("WARN command watchdog: STOP\r\n");
    }
    if (elapsed_reached(s_last_status_ms, CONSOLE_STATUS_PERIOD_MS)) {
        s_last_status_ms = g_ms;
        print_status();
    }
}
