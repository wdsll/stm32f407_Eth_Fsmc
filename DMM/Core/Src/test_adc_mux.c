/*
 * test_adc_mux.c
 *
 *  Created on: Sep 26, 2025
 *      Author: huyan
 */

#ifndef SRC_TEST_ADC_MUX_C_
#define SRC_TEST_ADC_MUX_C_

#include "test_adc_mux.h"
#include "adc_mux.h"
#include "mux_hw.h"
#include <stdint.h>

int tests_run = 0; // 记录运行的测试用例数量

static uint8_t last_mux_i = 0xff; // 记录最后一次选择的电流多路复用器通道
static uint8_t last_mux_r = 0xff;   // 记录最后一次选择的电阻多路复用器通道

void MUX_FUN_Select_test(uint8_t ch) // 测试函数：模拟选择功能通道（未实现具体功能
{
	(void)ch;
}
void MUX_I_Select_test(uint8_t ch) // 测试函数：模拟选择电流多路复用器通道
{
	last_mux_i = ch;
}
void MUX_R_Select_test(uint8_t ch) // 测试函数：模拟选择电阻多路复用器通道
{
	last_mux_r = ch;
}
// 测试函数：验证ADC码值转换为电压值的逻辑
static const char *test_code_to_voltage(void)
{
    const float expected = 2.5f;
    const float actual = MEAS_CodeToVoltage(16777215u, 2.5f, 1.0f); // 调用转换函数
    mu_assert_float("code to voltage", expected, actual, 1e-4f);  // 断言比较结果
    return NULL;
}
// 测试函数：验证电阻范围边界设置的逻辑
static const char *test_resistance_range_bounds(void)
{
    MEAS_SetResistanceRangeIndex(100u); // 设置电阻范围索引为100（超出范围）
    mu_check(MEAS_GetResistanceRangeIndex() == MEAS_Range_R_Count() - 1u);  
    const size_t last_index = MEAS_Range_R_Count() - 1u;  // 获取最大索引值 
    const RRangeCfg *cfg = MEAS_Range_R_Config(last_index); // 获取配置信息
    mu_check(MEAS_GetResistanceRangeIndex() == last_index);
    // 计算预期地址值
    const uint8_t expected_addr = ((uint8_t)((cfg->u16_sel - 1u) & 0x7u)) |
                                  ((uint8_t)(cfg->u20_sel & 0x7u) << 3);
    mu_check(last_mux_r == expected_addr);
    return NULL;
}
// 测试函数：验证初始化设置默认值的逻辑
static const char *test_init_sets_defaults(void)
{
    last_mux_i = 0xff; // 重置电流多路复用器通道记录
    last_mux_r = 0xff; // 重置电阻多路复用器通道记录
    MEAS_Init();   // 调用初始化函数
    mu_check(last_mux_i == 1u); // 断言电流多路复用器通道设置为1
    mu_check(last_mux_r != 0xffu); // 断言电阻多路复用器通道已设置
    return NULL;
}

static const char *all_tests(void)
{
    mu_run_test(test_code_to_voltage); // 运行电压转换测试
    mu_run_test(test_resistance_range_bounds); // 运行电阻范围测试
    mu_run_test(test_init_sets_defaults); // 运行初始化测试
    return NULL;
}

int main_test(void)
{
    const char *result = all_tests();
    if (result != NULL)
    {
        printf("%s\n", result);
        return 1;
    }

    printf("ALL TESTS PASSED (%d)\n", tests_run);
    return 0;
}













#endif /* SRC_TEST_ADC_MUX_C_ */
