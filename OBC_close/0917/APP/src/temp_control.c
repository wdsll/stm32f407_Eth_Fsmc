#include "temp_control.h"
#include "main.h"

#define TEMP_PULLUP_OHM        (10000.0f)
#define TEMP_ADC_MAX_COUNTS    (4095.0f)
#define TEMP_OVERLIMIT_C       (90.0f)
#ifndef DEBUG_PRINTF_TEMP_CONTROL
#define DEBUG_PRINTF_TEMP_CONTROL 0
#endif
typedef struct {
    int8_t temp_c;
    float resistance_ohm;
} ntc_table_entry_t;

static const ntc_table_entry_t k_ntc_table[] = {
	  { -40, 197390.0f },
	  { -39, 186540.0f },
		{ -38, 176350.0f },
	  { -37, 166800.0f },
		{ -36, 157820.0f },
	  { -35, 149390.0f },
		{ -34, 141510.0f },
	  { -33, 134090.0f },
		{ -32, 127110.0f },
		{ -31, 120530.0f },
	  { -30, 114340.0f },
		
		{ -29, 108530.0f },
	  { -28, 103040.0f },
		{ -27, 97870.0f },
	  { -26, 92989.0f },
		{ -25, 88381.0f },
	  { -24, 84036.0f },
		{ -23, 79931.0f },
	  { -22, 76052.0f },
		{ -21, 72384.0f },
		{ -20, 68915.0f },
	  { -19, 65634.0f },
		
		{ -18, 62529.0f },
	  { -17, 59589.0f },
		{ -16, 56804.0f },
	  { -15, 54166.0f },
		{ -14, 51665.0f },
	  { -13, 49294.0f },
		{ -12, 47046.0f },
	  { -11, 44913.0f },
		{ -10, 42889.0f },
		{ -9, 40967.0f },
	  { -8, 39142.0f },
		
		{ -7, 37408.0f },
	  { -6, 35761.0f },
		{ -5, 34196.0f },
	  { -4, 32707.0f },
		{ -3, 31291.0f },
	  { -2, 29945.0f },
		{ -1, 28664.0f },
	  { 0, 27445.0f },
		{ 1, 26283.0f },
		{ 2, 25177.0f },
	  { 3, 24124.0f },
		{ 4, 23121.0f },
		{ 5, 22165.0f },
		{ 6, 21253.0f },
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
    { 53, 3774.0f },
		{ 54, 3654.0f },
    { 55, 3539.0f },
    { 56, 3429.0f },
    { 57, 3322.0f },
		{ 58, 3219.0f },
    { 59, 3119.0f },
    { 60, 3024.0f },
    { 61, 2931.0f },
    { 62, 2842.0f },
    { 63, 2756.0f },
    { 64, 2673.0f },
    { 65, 2593.0f },
    { 66, 2516.0f },
    { 67, 2441.0f },
    { 68, 2369.0f },
    { 69, 2300.0f },
    { 70, 2233.0f },
    { 71, 2168.0f },
    { 72, 2105.0f },
    { 73, 2044.0f },
    { 74, 1985.0f },
    { 75, 1929.0f },
    { 76, 1874.0f },
    { 77, 1821.0f },
    { 78, 1770.0f },
    { 79, 1720.0f },
    { 80, 1672.0f },
    { 81, 1626.0f },
    { 82, 1581.0f },
    { 83, 1538.0f },
    { 84, 1496.0f },
    { 85, 1455.0f },
    { 86, 1416.0f },
    { 87, 1377.0f },
    { 88, 1340.0f },
    { 89, 1304.0f },
    { 90, 1270.0f },
    { 91, 1236.0f },
    { 92, 1204.0f },
    { 93, 1172.0f },
    { 94, 1141.0f },
    { 95, 1112.0f },
    { 96, 1083.0f },
    { 97, 1055.0f },
    { 98, 1028.0f },
    { 99, 1002.0f },
    { 100, 976.0f },
    { 101, 951.0f },
    { 102, 927.0f },
    { 103, 904.0f },
    { 104, 882.0f },
    { 105, 860.0f },
    { 106, 838.0f },
    { 107, 818.0f },
    { 108, 798.0f },
    { 109, 778.0f },
    { 110, 759.0f },
    { 111, 741.0f },
    { 112, 723.0f },
    { 113, 706.0f },
    { 114, 689.0f },
    { 115, 673.0f },
    { 116, 657.0f },
    { 117, 641.0f },
    { 118, 626.0f },
    { 119, 612.0f },
    { 120, 598.0f },
    { 121, 584.0f },
    { 122, 570.0f },
    { 123, 557.0f },
    { 124, 545.0f },
    { 125, 532.0f }
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
#if DEBUG_PRINTF_TEMP_CONTROL
    static uint32_t s_last_print_ms = 0U;
    if (elapsed_reached(s_last_print_ms, 200U)) {
        s_last_print_ms = g_ms;
        int32_t pfc_c10 = (int32_t)(s_pfc_temp.temperature_c * 10.0f);
        int32_t llc_c10 = (int32_t)(s_llc_temp.temperature_c * 10.0f);
        debug_printf("[TEMP] PFC=%ld.%ldC(%s) LLC=%ld.%ldC(%s)\n",
            (long)(pfc_c10 / 10), (long)(pfc_c10 % 10 < 0 ? -(pfc_c10 % 10) : pfc_c10 % 10),
            s_pfc_temp.over_limit ? "OVR" : "OK",
            (long)(llc_c10 / 10), (long)(llc_c10 % 10 < 0 ? -(llc_c10 % 10) : llc_c10 % 10),
            s_llc_temp.over_limit ? "OVR" : "OK");
    }
#endif
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