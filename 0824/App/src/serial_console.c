/* USART2 commissioning console for the first software/hardware bench test. */
#include "serial_console.h"
#include "debug_printf.h"
#include "llc_control.h"
#include "adc_dma.h"
#include "protect.h"
#include <stdio.h>
#include <string.h>

#define CONSOLE_LINE_SIZE   (64U)  //命令行缓冲 64B，命令都很短，够用。
#define CONSOLE_STATUS_PERIOD_MS   (1000U)
#define CONSOLE_WATCHDOG_MS        (5000U) //	控制台看门狗 5s：使能输出后若 5s 无命令则自动 STOP（终端失联保护）。
 
static char s_line[CONSOLE_LINE_SIZE];
static uint32_t s_line_length;
static uint32_t s_last_status_ms;
static uint32_t s_last_command_ms;

static void print_help(void) //	帮助函数
{
    debug_printf("Commands: HELP | STATUS | SET <volt> <amp> | START | STOP | CLEAR | PING\r\n"); //命令清单。
    debug_printf("Safety: SET/START/CLEAR only while stopped; enabled output needs PING within 5s.\r\n"); //安全提示，清楚。
}

static void print_status_test(void) //	状态打印。
{
	//一帧把关键量全打出来，联调非常实用。g_ms→%lu、g_charger_state/g_fault→%u 的强转正确。但 %f 浮点打印需要 printf 浮点支持（见下方 P1）。
    debug_printf("STAT ms=%lu state=%u fault=%u en=%u ref=%.1fV/%.1fA "
                 "ac=%.1fV bus=%.1fV out=%.1fV bat=%.1fV i=%.1fA\r\n"
								 "raw_ac=%u raw_bus=%u raw_out=%u raw_bat=%u raw_i=%u\r\n",
	
                 (unsigned long)g_ms, (unsigned int)g_charger_state,
                 (unsigned int)g_fault, power_supervisor_requested() ? 1U : 0U,
                 power_supervisor_voltage_reference(),
                 power_supervisor_current_reference(), g_adc_multi.ac_vol_v,
                 g_adc_multi.bus_vol_v, g_adc_multi.vout_v, g_adc_multi.vbat_v,
                 g_adc_multi.iout_a,
								 (unsigned int)g_adc_multi.ac_vol_raw,
                 (unsigned int)g_adc_multi.bus_vol_raw,
                 (unsigned int)g_adc_multi.vout_raw,
                 (unsigned int)g_adc_multi.vbt_raw,
                 (unsigned int)g_adc_multi.isense_raw);
}
static void print_status(bool include_raw)
{
    debug_printf("STAT ms=%lu state=%u fault=%u en=%u ref=%.1fV/%.1fA "
                 "ac=%.1fV bus=%.1fV out=%.1fV bat=%.1fV i=%.1fA\r\n",
                 (unsigned long)g_ms, (unsigned int)g_charger_state,
                 (unsigned int)g_fault, power_supervisor_requested() ? 1U : 0U,
                 power_supervisor_voltage_reference(),
                 power_supervisor_current_reference(), g_adc_multi.ac_vol_v,
                 g_adc_multi.bus_vol_v, g_adc_multi.vout_v, g_adc_multi.vbat_v,
                 g_adc_multi.iout_a);

    if (include_raw) {
        debug_printf("RAW ac=%u bus=%u out=%u bat=%u i=%u\r\n",
                     (unsigned int)g_adc_multi.ac_vol_raw,
                     (unsigned int)g_adc_multi.bus_vol_raw,
                     (unsigned int)g_adc_multi.vout_raw,
                     (unsigned int)g_adc_multi.vbt_raw,
                     (unsigned int)g_adc_multi.isense_raw);
    }
}
static void execute_command(char *line) //	命令分发
{
	float voltage_v; //目标电压
	float current_a; //限流值
	
	s_last_command_ms = g_ms; //	任何命令（含 STATUS）都刷新看门狗计时——这点重要，避免误触发自停。
	if((strcmp(line,"HELP") == 0)||(strcmp(line,"?")==0))  //支持 HELP 与 ?，容错好。
	{
		print_help();
	}
	else if(strcmp(line,"STATUS") == 0)
	{
		print_status(true);
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
	else if(sscanf(line,"SET %f %f",&voltage_v,&current_a) == 2) //	用 sscanf 解析字符串（不是 scanf 从 stdin 读），正确；要求恰好 2 个浮点才匹配。
	{
		if(power_supervisor_requested())
		{
			debug_printf("ERR stop before SET\r\n"); 	//运行中禁止改设定值，合理。
		}
		//设定值范围校验。注意上限 80V 与保护侧 OVP(64V) 不一致（见 P2）：设 80V 会让 VOUT 目标冲过 OVP 阈值而触发故障。建议上限收到 ≤60V。
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
	else if(strcmp(line,"START") == 0) //启动分支。
	{
		//故障态/硬件故障未解除时拒绝启动，这是关键安全门，正确
		if((g_charger_state == MAIN_STEP_FAULT)||protect_fault_latched()||protect_fault_active_hw())
		{
			debug_printf("ERR fault active; STOP then CLEAR\r\n");
		}
		//未设参考值拒绝启动，正确。
		else if((power_supervisor_voltage_reference() < 10.0f)||(power_supervisor_current_reference() <= 0))
		{
			debug_printf("ERR SET reference first\r\n");
		}
		else
		{
			power_supervisor_request(true); //启动请求，正确。
			debug_printf("OK START\r\n");
		}
	}
	else if(strcmp(line,"CLEAR") == 0)
	{
		//	运行中禁止清故障，合理。
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
			protect_clear_fault();
			debug_printf("OK CLEAR\r\n");
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
    print_help(); //上电即打印帮助，联调方便
}

void serial_console_task(void) //每轮主循环调用
{
    char ch;
    while (debug_getchar(&ch)) { //轮询收，有则循环读完（一次调用可排空 RDR），非阻塞。
		//回车或换行都触发执行。注意：若终端发 \r\n，第一个 \r 执行命令，第二个 \n 时 s_line_length==0 不执行——不会重复执行，处理正确。	
        if ((ch == '\r') || (ch == '\n')) { 
            if (s_line_length > 0U) {
                s_line[s_line_length] = '\0';
                execute_command(s_line);
                s_line_length = 0U;
            }
        } else if ((ch == '\b') || ((unsigned char)ch == 0x7FU)) { //退格与 DEL 都回删，好。
            if (s_line_length > 0U) s_line_length--;
        } else if ((ch >= 'a') && (ch <= 'z')) { //	小写自动转大写，容错好
            if (s_line_length < (CONSOLE_LINE_SIZE - 1U))
                s_line[s_line_length++] = (char)(ch - 'a' + 'A');
				//	入队前都判断 s_line_length < CONSOLE_LINE_SIZE-1，且执行时 s_line[len]='\0' 最大下标 63<64，无溢出，处理正确。
        } else if (s_line_length < (CONSOLE_LINE_SIZE - 1U)) { 
            s_line[s_line_length++] = ch;
        }
    }
//控制台看门狗：使能后 5s 无命令自动停功率。安全问题见下（P2 设计考量）：自动状态打印(150行)不刷新 s_last_command_ms，
//所以「START 后只看自动状态、5s 不敲键」会被自停；联调长充电时需周期性 PING/STATUS 或放宽超时。
    if (power_supervisor_requested() &&
        elapsed_reached(s_last_command_ms, CONSOLE_WATCHDOG_MS)) {
        power_supervisor_request(false);
        debug_printf("WARN command watchdog: STOP\r\n");
    }
		//每 500ms 自动打状态，联调友好
    if (elapsed_reached(s_last_status_ms, CONSOLE_STATUS_PERIOD_MS)) {
        s_last_status_ms = g_ms;
        print_status(false);
    }
}
