#include "temp_control.h"
#include "main.h"

#define TEMP_PULLUP_OHM        (10000.0f)
#define TEMP_ADC_MAX_COUNTS    (4095.0f)
#define TEMP_OVERLIMIT_C       (90.0f)

typedef struct {
    int8_t temp_c;
    float resistance_ohm;
} ntc_table_entry_t;

static const ntc_table_entry_t k_ntc_table[] = {
    { 7, 20384.0f },
    { 8, 19555.0f },
    { 9, 18764.0f },
    { 10, 18010.0f },
    { 11, 17290.0f },
    { 12, 16602.0f },
    { 13, 15946.0f },
    { 14, 15319.0f },
    { 15, 14720.0f },
    { 16, 14148.0f },
    { 17, 13601.0f },
    { 18, 13078.0f },
    { 19, 12578.0f },
    { 20, 12099.0f },
    { 21, 11642.0f },
    { 22, 11204.0f },
    { 23, 10785.0f },
    { 24, 10384.0f },
    { 25, 10000.0f },
    { 26, 9632.0f },
    { 27, 9280.0f },
    { 28, 8943.0f },
    { 29, 8619.0f },
    { 30, 8309.0f },
    { 31, 8012.0f },
    { 32, 7727.0f },
    { 33, 7453.0f },
    { 34, 7191.0f },
    { 35, 6939.0f },
    { 36, 6698.0f },
    { 37, 6466.0f },
    { 38, 6243.0f },
    { 39, 6029.0f },
    { 40, 5824.0f },
    { 41, 5627.0f },
    { 42, 5437.0f },
    { 43, 5255.0f },
    { 44, 5080.0f },
    { 45, 4911.0f },
    { 46, 4749.0f },
    { 47, 4593.0f },
    { 48, 4443.0f },
    { 49, 4299.0f },
    { 50, 4160.0f },
    { 51, 4027.0f },
    { 52, 3898.0f },
    { 53, 3774.0f }
};

static temp_sensor_data_t s_pfc_temp;
static temp_sensor_data_t s_llc_temp;

/*********************************************************************************************************
* 函数名称：ntc_resistance_from_adc
* 函数功能：温度控制模块 中的 NTC（负温度系数）热敏电阻阻值计算函数,它的核心作用是将 ADC 原始采样值 转换为 热敏电阻的实际阻值
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年02月06日
* 注    意：
*********************************************************************************************************/
static bool ntc_resistance_from_adc(uint16_t raw, float *resistance_out)
{
	  //无效原始值检测
    if (raw == 0xFFFFU) {
        return false;
    }
		//ADC 值转电压以及电压值合理性校验
    float v = ((float)raw * VREF_ADC) / TEMP_ADC_MAX_COUNTS;
    if (v <= 0.0f || v >= (VREF_ADC - 0.001f)) {
        return false;
    }
		//阻值计算与输出 电阻 = v*10k欧姆/（3.3-v）
    *resistance_out = (v * TEMP_PULLUP_OHM) / (VREF_ADC - v);
    return true;
}
/*********************************************************************************************************
* 函数名称：ntc_temperature_from_resistance
* 函数功能：它的作用是将热敏电阻的阻值（单位：Ω）转换为对应的温度值（单位：℃），采用查表法（LUT）结合线性插值
* 输入参数：resistance_ohm
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年02月06日
* 注    意：
*********************************************************************************************************/
static float ntc_temperature_from_resistance(float resistance_ohm)
{
	  //动态计算表大小
    const size_t table_size = sizeof(k_ntc_table) / sizeof(k_ntc_table[0]);
	  /*表格范围外处理 这是 NTC 的特性：温度越低，电阻越高。
			边界处理：
			如果电阻大于等于表格最大值（即温度低于表格最低温度 7°C），则返回表格最低温度（7°C）。
			如果电阻小于等于表格最小值（即温度高于表格最高温度 53°C），则返回表格最高温度（53°C）。
	  */
    if (resistance_ohm >= k_ntc_table[0].resistance_ohm) {
        return (float)k_ntc_table[0].temp_c;
    }
    if (resistance_ohm <= k_ntc_table[table_size - 1U].resistance_ohm) {
        return (float)k_ntc_table[table_size - 1U].temp_c;
    }
	  //线性插值 遍历相邻表项，找到电阻值所在的区间 [r_low, r_high]，其中 r_high > r_low（电阻降序）。
    for (size_t i = 0U; i + 1U < table_size; i++) {
        float r_high = k_ntc_table[i].resistance_ohm;
        float r_low = k_ntc_table[i + 1U].resistance_ohm;
        if (resistance_ohm <= r_high && resistance_ohm >= r_low) {
            float t_high = (float)k_ntc_table[i].temp_c;
            float t_low = (float)k_ntc_table[i + 1U].temp_c;
            float ratio = (r_high - resistance_ohm) / (r_high - r_low);
            return t_high + (t_low - t_high) * ratio;
        }
    }
    //理论上不会执行：因为电阻在表格范围内时，循环必定会找到匹配区间（除非浮点误差极大）。
		//但作为防御性编程，提供一个默认返回值（表格最高温度），避免未定义行为
    return (float)k_ntc_table[table_size - 1U].temp_c;
}
/*********************************************************************************************************
* 函数名称：temp_sensor_update
* 函数功能：它扮演了温度测量信号链的整合者角色，将 ADC 原始采样值 通过一系列转换和校验，
最终更新为完整的传感器状态（电阻、温度、有效性、超限标志）。
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年02月06日
* 注    意：
*********************************************************************************************************/
static void temp_sensor_update(temp_sensor_data_t *sensor, uint16_t raw)
{
	//原始数据存档：将 ADC 采样的原始整数值（uint16_t）直接存入传感器结构体
    sensor->raw = raw;
	//函数返回 bool 表示计算是否成功（即原始值是否有效、电压是否在合理范围内）通过指针参数 &sensor->resistance_ohm 输出计算出的电阻值（单位：Ω）
    sensor->valid = ntc_resistance_from_adc(raw, &sensor->resistance_ohm);
    if (sensor->valid) {
			  //ntc_temperature_from_resistance：调用查表插值函数，将电阻值转换为温度（℃）。
        sensor->temperature_c = ntc_temperature_from_resistance(sensor->resistance_ohm);
			  //这个标志可用于过温保护，如降频、关机或报警。
        sensor->over_limit = (sensor->temperature_c > TEMP_OVERLIMIT_C);
    } else {
        sensor->temperature_c = 0.0f;
        sensor->over_limit = false;
    }
}

void temp_control_init(void)
{
    s_pfc_temp.raw = 0xFFFFU;
    s_pfc_temp.resistance_ohm = 0.0f;
    s_pfc_temp.temperature_c = 0.0f;
    s_pfc_temp.valid = false;
    s_pfc_temp.over_limit = false;

    s_llc_temp = s_pfc_temp;
}

void temp_control_tick_1khz(void)
{
    temp_sensor_update(&s_pfc_temp, g_adc_multi.tsense_pfc_raw);
    temp_sensor_update(&s_llc_temp, g_adc_multi.tsense_llc_raw);
}

void temp_control_get_pfc(temp_sensor_data_t *out)
{
    if (out != NULL) {
        *out = s_pfc_temp;
    }
}

void temp_control_get_llc(temp_sensor_data_t *out)
{
    if (out != NULL) {
        *out = s_llc_temp;
    }
}