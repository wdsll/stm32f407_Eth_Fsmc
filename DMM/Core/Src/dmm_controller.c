/*
 * dmm_controller.c
 *
 *  Created on: Sep 27, 2025
 *      Author: huyan
 */

#include "dmm_controller.h"

/* ---------------------------------------------------------------
 * DMM_VOLTAGE_ONLY: 单路复用采样 — 只测电压工况
 *   1 = 锁定电压模式，屏蔽电流/电阻切换
 *   0 = 恢复完整 V/I/R 功能
 * --------------------------------------------------------------- */
#ifndef DMM_VOLTAGE_ONLY
#define DMM_VOLTAGE_ONLY  1
#endif



/* LCD 每一行文字的 Y 坐标换算宏 */
#define LCD_LINE_Y(n)     ((n) * 20U + 10U)


/* ---------------------------------------------------------------
 * 模块内部状态变量
 * --------------------------------------------------------------- */
/* 当前工作模式，默认上电进入电压模式 */
static DMM_Mode s_activeMode = DMM_MODE_VOLTAGE;
/* 上一次串口打印测量值的时间戳，用于限频打印 */
static uint32_t s_lastPrintTick = 0U;
/* 上一次模式切换时间戳，可用于调试或后续增加切换延时 */
static uint32_t s_lastModeChangeTick = 0U;
/* 最近一次测得的电压值 */
static float s_latestVoltage_V = 0.0f;
/* 最近一次测得的电流值 */
static float s_latestCurrent_A = 0.0f;
/* 最近一次测得的电阻值 */
static float s_latestResistance_Ohm = 0.0f;
/* 按键锁存，防止 KEY_UP 长按时连续触发 */
static uint8_t s_keyLatch = 0U;
/**
 * @brief  非阻塞读取调试串口 1 个字符
 * @retval >=0 读到的字符
 * @retval -1  当前没有收到数据
 *
 * @note
 * 这里通过 HAL_UART_Receive(..., timeout=0) 实现“轮询且不阻塞”。
 * 上层可根据返回值判断是否收到串口命令，例如 'v' / 'i' / 'r' / 'm'。
 */
static int Debug_ReadCharNonBlocking(void)
{
	uint8_t ch = 0;
	if(HAL_UART_Receive(&husart_debug,&ch,1U,0U))
	{
		return (int)ch;
	}
	return -1;
}
//指针函数
/**
 * @brief  将模式枚举值转换成字符串
 * @param  mode 当前模式枚举
 * @retval 对应的字符串常量
 *
 * @note
 * 该函数主要用于 LCD 显示和串口打印。
 */
static const char *DMM_ModeLable(DMM_Mode mode)
{
	switch(mode)
	{
		case DMM_MODE_VOLTAGE:
			return "Voltage";
		case DMM_MODE_CURRENT:
			return "Current";
		case DMM_MODE_RESISTANCE:
			return "Resistance";
		default:
			break;
	}
    return "Unknown";
}
/**
 * @brief  按 SI 工程单位格式化数值字符串
 * @param  out   输出缓冲区
 * @param  size  输出缓冲区长度
 * @param  value 原始数值
 * @param  unit  单位字符串，例如 "V" / "A" / "Ohm"
 *
 * @note
 * 会根据数值大小自动添加前缀：
 *   M / k / m / u / n
 * 例如：
 *   0.0012 V -> 1.200 mV
 *   1200 Ohm -> 1.200 kOhm
 */
static void format_si_value(char *out,size_t size,float value,const char *unit)
{
    const float abs_value=fabsf(value);
    const char *prefix = "";
    float scaled= value;

    if(abs_value>=1000000.0f)
    {
        prefix = "M";
        scaled = value/1000000.0f;
    }
    else if(abs_value>=1000.0f)
    {
        prefix = "k";
        scaled = value/1000.0f;
    }
    else if(abs_value>=1.0f)
    {
        prefix = "";
        scaled = value;
    }
    else if(abs_value>=0.001f)
    {
        prefix = "m";
        scaled = value*1000.0f;
    }
    else if(abs_value>=0.000001f)
    {
        prefix = "u";
        scaled = value*1000000.0f;
    }
    else if(abs_value>=0.000000001f)
    {
        prefix = "n";
        scaled = value*1000000000.0f;
    }
    (void)snprintf(out, size, "%+8.3f %s%s", (double)scaled, prefix, unit);
}
/**
 * @brief  在 LCD 指定逻辑行显示一行文本
 * @param  line_index 逻辑行号
 * @param  text       待显示的字符串
 *
 * @note
 * 这里统一用蓝色字体显示。
 * LCD 的实际 Y 坐标由 LCD_LINE_Y(n) 宏换算。
 */
static void DMM_DrawText(uint16_t line_index,const char *text)
{
    char buffer[32];
    (void)snprintf(buffer,sizeof(buffer),"%s",text);
    lcd_show_str(10,LCD_LINE_Y(line_index),12,buffer,BLUE);
}
/**
 * @brief  在 LCD 指定逻辑行显示一行文本
 * @param  line_index 逻辑行号
 * @param  text       待显示的字符串
 *
 * @note
 * 这里统一用蓝色字体显示。
 * LCD 的实际 Y 坐标由 LCD_LINE_Y(n) 宏换算。
 */
static void DMM_AnnounceMode(DMM_Mode mode)
{
    printf("\r\n[MODE] %s\r\n", DMM_ModeLable(mode));
    printf("Commands: 'v','i','r' select mode, 'm' or KEY_UP cycles.\r\n");
}
/**
 * @brief  根据当前模式刷新 LCD 显示内容
 *
 * @note
 * - 第 0 行显示当前模式
 * - 第 1~3 行显示电压/电流/电阻
 * - 未启用的量统一显示 (n/a)
 * - 第 4~5 行显示操作提示
 */
static void DMM_UpdateDisplay(void)
{
    char value[32];
    char line[48];

    (void)snprintf(line, sizeof(line), "Mode: %s", DMM_ModeLable(s_activeMode));
    DMM_DrawText(0U, line);

    switch (s_activeMode)
    {
        case DMM_MODE_VOLTAGE:
            format_si_value(value, sizeof(value), s_latestVoltage_V, "V");
            (void)snprintf(line, sizeof(line), "Voltage: %22s", value);
            DMM_DrawText(1U, line);
            DMM_DrawText(2U, "Current: (n/a)");
            DMM_DrawText(3U, "Resistance: (n/a)");
            break;
        case DMM_MODE_CURRENT:
            DMM_DrawText(1U, "Voltage: (n/a)");
            format_si_value(value, sizeof(value), s_latestCurrent_A, "A");
            (void)snprintf(line, sizeof(line), "Current: %s", value);
            DMM_DrawText(2U, line);
            DMM_DrawText(3U, "Resistance: (n/a)");
            break;
        case DMM_MODE_RESISTANCE:
            format_si_value(value, sizeof(value), s_latestResistance_Ohm, "Ohm");
            (void)snprintf(line, sizeof(line), "Resistance: %s", value);
            DMM_DrawText(1U, "Voltage: (n/a)");
            DMM_DrawText(2U, "Current: (n/a)");
            DMM_DrawText(3U, line);
            break;
        default:
            DMM_DrawText(1U, "Voltage: (n/a)");
            DMM_DrawText(2U, "Current: (n/a)");
            DMM_DrawText(3U, "Resistance: (n/a)");
            // Debug log for invalid mode
            (void)snprintf(line, sizeof(line), "Invalid mode: %d", s_activeMode);
            DMM_DrawText(6U, line);
            break;
    }
    DMM_DrawText(4U, "KEY_UP/m : next mode");
    DMM_DrawText(5U, "UART v/i/r : select");
}
//这段代码的主要功能是定期打印数字万用表（DMM）的测量值（电压、电流或电阻），
//并根据当前的工作模式（ s_activeMode ）选择性地输出对应的测量结果
static void DMM_PrintMeasurements(void)
{
	const uint32_t now = HAL_GetTick();
	if((now-s_lastPrintTick)<250U)
	{
		return;
	}
	s_lastPrintTick = now;

	char vbuf[24];
	char ibuf[24];
	char rbuf[24];

    format_si_value(vbuf, sizeof(vbuf), s_latestVoltage_V, "V");
    format_si_value(ibuf, sizeof(ibuf), s_latestCurrent_A, "A");
    format_si_value(rbuf, sizeof(rbuf), s_latestResistance_Ohm, "Ohm");

    switch (s_activeMode)
    {
        case DMM_MODE_VOLTAGE:
            printf("[Voltage] %s\r\n", vbuf);
            break;
        case DMM_MODE_CURRENT:
            printf("[Current] %s\r\n", ibuf);
            break;
        case DMM_MODE_RESISTANCE:
            printf("[Resistance] %s, %s, %s\r\n", rbuf, vbuf, ibuf);
            break;
        default:
            break;
    }
}
/**
 * @brief  根据当前模式处理一次新的测量结果
 * @param  channel 本次采样对应的 ADC 通道号
 *
 * @note
 * 这是“测量值分发”的核心函数。
 * - 电压模式：只有 channel==0 时才更新电压值
 * - 电流模式：只有采样通道等于当前电流通道时才更新
 * - 电阻模式：当采样通道命中电压/电流任一通道时，重新计算一次 R/V/I
 *
 * @warning
 * 你当前项目正往“单路复用采样、只测电压”方向收敛时，
 * 这里后续大概率还要再简化。
 */
static void DMM_ProcessMeasurement(int8_t channel)
{
	if(channel < 0)
	{
		return;
	}
	switch(s_activeMode)
	{
		case DMM_MODE_VOLTAGE:

			if((uint8_t)channel == 0)
			{
				s_latestVoltage_V = MEAS_ReadVoltage_V_Single((uint8_t)channel, 1U);
				DMM_UpdateDisplay();
				DMM_PrintMeasurements();
			}
        break;
		case DMM_MODE_CURRENT:
			/* 电流模式下，只有命中当前电流绑定通道才更新 */
            if((uint8_t)channel == DMM_Measurement_GetCurrentChannel())
            {
                s_latestCurrent_A = DMM_ReadCurrent_A(1U);
                DMM_UpdateDisplay();
                DMM_PrintMeasurements();    
            }
        break;
        case DMM_MODE_RESISTANCE:
        {
        	/* 电阻模式下，分别取电压采样通道和电流采样通道 */
            uint8_t v_ch = DMM_Measurement_GetResistanceVoltageChannel();
            uint8_t i_ch = DMM_Measurement_GetResistanceCurrentChannel();
            /* 只要任一相关通道更新，就重新计算一次电阻 */
            if(((uint8_t)channel == v_ch) || ((uint8_t)channel == i_ch))
            {
                float r = 0.0f;
                float v = 0.0f;
                float i = 0.0f;

                MEAS_ReadResistance_Ohm(&r,&v,&i);
                s_latestResistance_Ohm = r;
                s_latestCurrent_A = i;
                s_latestVoltage_V = v;
                DMM_UpdateDisplay();
                DMM_PrintMeasurements();
            }
            break;
        }
        default:
            break;
	}
}
/**
 * @brief  切换到指定测量模式
 * @param  mode 目标模式
 *
 * @note
 * 该函数除了更新 s_activeMode 外，还负责：
 * - 切换前端测量路径
 * - 清零无关测量量
 * - 电压模式下控制继电器状态
 * - 刷新显示并打印模式提示
 *
 * 当前版本中：
 * - 电压模式会先关掉所有继电器，再打开 RELAY_ID_3
 * - 电流/电阻模式不控制继电器
 */
void DMM_Controller_SelectMode(DMM_Mode mode)
{
    if(mode >= DMM_MODE_COUNT)
    {
        return;
    }   
    DMM_Mode previous = s_activeMode;
    s_activeMode = mode;

    switch(mode)
    {
        case DMM_MODE_VOLTAGE:
            /* 进入电压模式：
             * 1) 先关闭全部继电器，避免多档同时吸合
             * 2) 打开电压路径需要的继电器档位（当前代码选 RELAY_ID_3）
             * 3) 切换功能 MUX 到电压路径
             */
            RELAY_AllOff();
            RELAY_Set(RELAY_ID_3, 1);
            DMM_Measurement_SelectVoltagePath();
            s_latestCurrent_A = 0.0f;
            s_latestResistance_Ohm = 0.0f;
            break;
        case DMM_MODE_CURRENT:
            /* 进入电流模式：
             * 1) 切 MUX 到电流路径
             * 2) 使用当前配置的电流采样通道
             * 3) 清空无关显示量
             */
            DMM_Measurement_SelectCurrentPath();
            DMM_SetCurrentChannel(DMM_Measurement_GetCurrentChannel());
            s_latestVoltage_V = 0.0f;
            s_latestResistance_Ohm = 0.0f;
            break;
        case DMM_MODE_RESISTANCE:
            /* 进入电阻模式：
             * 1) 切 MUX 到电阻路径
             * 2) 清空上次残留的 V/I/R 显示
             */
            DMM_Measurement_SelectResistancePath();
			s_latestVoltage_V = 0.0f;
			s_latestCurrent_A = 0.0f;
			s_latestResistance_Ohm = 0.0f;
            break;
        default:
            break;
    }
    /* 如果模式变化，或者这是第一次初始化，则记录时间并打印提示 */
    if((mode != previous) || (s_lastModeChangeTick == 0U))
    {
        s_lastModeChangeTick = HAL_GetTick();

        DMM_AnnounceMode(mode);
    } 
    DMM_UpdateDisplay();
}
/**
 * @brief  按方向循环切换模式
 * @param  direction >=0 表示向后循环，<0 表示向前循环
 *
 * @note
 * 这个函数本质上是“模式枚举循环器”：
 * Voltage -> Current -> Resistance -> Voltage ...
 */
void DMM_Controller_CycleMode(int direction)
{
    int next = (int)s_activeMode + direction;
    if (direction >= 0)
    {
        next %= (int)DMM_MODE_COUNT;
    }
    else
    {
        while (next < 0)
        {
            next += (int)DMM_MODE_COUNT;
        }
        next %= (int)DMM_MODE_COUNT;
    }

    DMM_Controller_SelectMode((DMM_Mode)next);
}
/**
 * @brief  处理模式切换输入（串口 + 按键）
 *
 * @note
 * 支持两种输入源：
 * 1) 串口命令：
 *    - 'v' 进入电压模式
 *    - 'i' 进入电流模式
 *    - 'r' 进入电阻模式
 *    - 'm' 循环切换
 * 2) 板上按键 KEY_UP：
 *    - 短按切换到下一个模式
 *
 * 当 DMM_VOLTAGE_ONLY == 1 时：
 * - 只允许 'v'
 * - 'i'/'r'/'m' 会给出“仅电压模式”的提示
 * - KEY_UP 不再执行循环切换
 */
void DMM_Controller_HandleModeSwitch(void)
{
    int ch = Debug_ReadCharNonBlocking();
    if (ch >= 0)
    {
        switch ((char)ch)
        {
            case 'v':
            case 'V':
                DMM_Controller_SelectMode(DMM_MODE_VOLTAGE);
                break;
#if DMM_VOLTAGE_ONLY == 0
            case 'i':
            case 'I':
                DMM_Controller_SelectMode(DMM_MODE_CURRENT);
                break;
            case 'r':
            case 'R':
                DMM_Controller_SelectMode(DMM_MODE_RESISTANCE);
                break;
            case 'm':
            case 'M':
                DMM_Controller_CycleMode(1);
                break;
#else
            case 'i': case 'I':
            case 'r': case 'R':
            case 'm': case 'M':
                printf("[VOLTAGE_ONLY] Mode locked to Voltage. Set DMM_VOLTAGE_ONLY=0 to unlock.\r\n");
                break;
#endif
            default:
                break;
        }
    }

    KEYS key = ScanPressedKey(1U);
    if (key == KEY_UP)
    {
        if (s_keyLatch == 0U)
        {
#if DMM_VOLTAGE_ONLY == 0
            DMM_Controller_CycleMode(1);
#endif
            s_keyLatch = 1U;
        }
    }
    else if (key == KEY_NONE)
    {
        s_keyLatch = 0U;
    }
}
/**
 * @brief  处理一次新到达的采样结果，并在 LCD 上显示调试信息
 * @param  channel 本次采样对应通道号
 *
 * @note
 * 该函数做两件事：
 * 1) 在 LCD 上显示当前通道的原始偏置或换算电压
 * 2) 调用 DMM_ProcessMeasurement() 进一步按模式更新内部测量值
 *
 * ZERO_MODE == 1：
 * - 显示当前通道的零点偏置码
 *
 * ZERO_MODE == 0：
 * - 显示当前通道换算后的电压值
 */
void DMM_Controller_HandleMeasurement(int8_t channel)
{
    if (channel < 0)
    {
        return;
    }

#if ZERO_MODE == 1U
    int32_t bias = DMM_Measurement_GetBias((uint8_t)channel);
    int32_t filtered_bias = AD7190_Filter(channel, bias);
    char disp[20];
    (void)snprintf(disp, sizeof(disp), "IN%d. 0x%05X", channel, filtered_bias);
    lcd_show_str(10, (uint16_t)((channel + 1) * 20 + 10), 16, disp, RED);
#else
    float voltage = MEAS_ReadVoltage_V_Single((uint8_t)channel, 0U);
    char disp[40];
    (void)snprintf(disp, sizeof(disp), "CH%d:%6.6fV", channel + 1, (double)voltage);
    lcd_show_str(10, (uint16_t)((channel + 1) * 10 + 10), 12, disp, RED);
#endif

    DMM_ProcessMeasurement(channel);
}
/**
 * @brief  DMM 控制器初始化
 *
 * @note
 * 该函数完成控制层状态清零，并强制进入默认模式（电压模式）。
 * 典型调用时机：系统上电完成、底层 ADC/显示初始化之后。
 */
void DMM_Controller_Init(void)
{
    s_activeMode = DMM_MODE_VOLTAGE;
    s_lastPrintTick = 0U;
    s_lastModeChangeTick = 0U;
    s_latestVoltage_V = 0.0f;
    s_latestCurrent_A = 0.0f;
    s_latestResistance_Ohm = 0.0f;
    s_keyLatch = 0U;

    DMM_Controller_SelectMode(DMM_MODE_VOLTAGE);
}














