#include "aux_power.h"

static aux_power_monitor_t s_aux_power = {
        .v3v3_v = 0.0f,
        .vbat_v = 0.0f,
        .v3v3_min_v = AUX_V3V3_OK_MIN_V,
        .vbat_min_v = AUX_VBAT_OK_MIN_V,
        .last_update_ms = 0U,
        .drop_detected_ms = 0U,
        .restore_detected_ms = 0U,
        .power_ok =
#if AUX_POWER_MONITOR_ENABLE
        false
#else
        true
#endif
};

void aux_power_monitor_update(float v3v3_v, float vbat_v)
{
#if AUX_POWER_MONITOR_ENABLE
        s_aux_power.v3v3_v = v3v3_v;
        s_aux_power.vbat_v = vbat_v;

        if (v3v3_v < s_aux_power.v3v3_min_v) {
                s_aux_power.v3v3_min_v = v3v3_v;
        }

        if (vbat_v < s_aux_power.vbat_min_v) {
                s_aux_power.vbat_min_v = vbat_v;
        }

        bool aux_ok = (v3v3_v >= AUX_V3V3_OK_MIN_V) && (vbat_v >= AUX_VBAT_OK_MIN_V);

        if (aux_ok) {
                if (!s_aux_power.power_ok) {
                        s_aux_power.restore_detected_ms = g_ms;
                }
        } else {
                if (s_aux_power.power_ok) {
                        s_aux_power.drop_detected_ms = g_ms;
                }
        }
        s_aux_power.power_ok = aux_ok;
        s_aux_power.last_update_ms = g_ms;
#else
        (void)v3v3_v;
        (void)vbat_v;
#endif
}

bool aux_power_ok_now(void)
{
#if AUX_POWER_MONITOR_ENABLE
        return s_aux_power.power_ok;
#else
        return true;
#endif
}

/*********************************************************************************************************
* 函数名称：aux_power_ok_stable_since
* 函数功能：用于判断辅助电源是否在指定的时间范围内稳定恢复。
* 输入参数：ms
* 输出参数：void
* 返 回 值：bool
* 创建日期：2025年10月21日
* 注    意： 
*********************************************************************************************************/
bool aux_power_ok_stable_since(uint32_t ms)
{
	#if AUX_POWER_MONITOR_ENABLE
    /* 恢复去抖：要求 power_ok=true 且恢复时间记过阈值 */
    if (!s_aux_power.power_ok) return false;
    if (s_aux_power.restore_detected_ms == 0U) return false;
    return (uint32_t)(g_ms - s_aux_power.restore_detected_ms) >= ms;
	#else
		(void)ms;
		return false;
	#endif
}

/*********************************************************************************************************
* 函数名称：aux_power_brownout_stable
* 函数功能：检查辅助电源掉电是否稳定
* 输入参数：ms
* 输出参数：void
* 返 回 值：bool
* 创建日期：2025年10月21日
* 注    意：返回true表示掉电稳定，false表示不稳定 
*********************************************************************************************************/
bool aux_power_brownout_stable(uint32_t ms)
{
    /* 掉电去抖：要求 power_ok=false 且掉电时间记过阈值 */
    if (s_aux_power.power_ok) return false;
    if (s_aux_power.drop_detected_ms == 0U) return false;
    return (uint32_t)(g_ms - s_aux_power.drop_detected_ms) >= ms;
}

const aux_power_monitor_t *aux_power_monitor_state(void)
{
        return &s_aux_power;
}