/*********************************************************************************************************
* 模块名称：pfc_control.c
* 摘    要：
* 作    者：Rengar
* 内    容：PFC 控制 (NCP1654 外置控制器, MCU 继电器控制+监控) 
* 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
*          
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "pfc_control.h"
#include "adc_dma.h"
#include "protect.h"
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static pfc_state_t s_pfc_state = PFC_STATE_OFF;
static uint32_t s_vbus_ovp_ms = 0U;  //过压计数
static uint32_t s_vbus_uvp_ms = 0U;  //欠压计数

/*********************************************************************************************************
*                                              函数实现
*********************************************************************************************************/
void pfc_init(void)
{
    s_pfc_state = PFC_STATE_OFF;
    gpio_bit_reset(PFC_RELAY_PORT, PFC_RELAY_PIN);  /* 初始断开 */
}

void pfc_enable(void)
{
    if (s_pfc_state == PFC_STATE_OFF) {
			  if ((g_adc_multi.ac_vol_v < PFC_AC_INPUT_MIN_V) ||
            (g_adc_multi.ac_vol_v > PFC_AC_INPUT_MAX_V)) {
            protect_set_fault(FAULT_AC_INPUT_RANGE);
            return;
        }
        /* 闭合 PFC 继电器, NCP1654 开始工作 */
        gpio_bit_set(PFC_RELAY_PORT, PFC_RELAY_PIN);
        s_pfc_state = PFC_STATE_RELAY_ON;
				
        s_vbus_ovp_ms = 0U;
        s_vbus_uvp_ms = 0U;
    }
}

void pfc_disable(void)
{
    gpio_bit_reset(PFC_RELAY_PORT, PFC_RELAY_PIN);
    s_pfc_state = PFC_STATE_OFF;
}
/*********************************************************************************************************
* 函数名称：pfc_tick_1khz
* 函数功能：pfc状态机
| PFC 状态     | 主要行为               | 退出条件        |
| ---------- | ------------------ | ----------- |
| `OFF`      | 撤销 PFC 工作请求对应的控制输出 | 收到有效启动请求    |
| `STARTING` | 执行启动动作，等待母线连续满足条件  | 稳定就绪、超时或故障  |
| `READY`    | 持续监控 AC、母线及采样有效性   | 停止请求或运行异常   |
| `FAULT`    | 保持停止状态，保留故障原因      | 撤销请求且故障确认清除 |
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2026年08月14日
* 注    意：
*********************************************************************************************************/
void pfc_tick_1khz(void)
{
    switch (s_pfc_state) {
    case PFC_STATE_OFF:
        break;

    case PFC_STATE_RELAY_ON:
        /* 等待母线电压建立 (NCP1654 软启动 ~50ms) */
        if (g_adc_multi.bus_vol_v >= VBUS_MIN_START_V) {
            s_pfc_state = PFC_STATE_RUN;
        }
        break;

    case PFC_STATE_RUN:
        /* 监控母线电压: NCP1654 外置控制, MCU 只做保护监控 加入OVP UVP的保护*/
				if(g_adc_multi.bus_vol_v > VBUS_OVP_V)
				{
					s_vbus_ovp_ms = 0U;
					if (++s_vbus_ovp_ms >= VBUS_OVP_DEBOUNCE_MS) {
							protect_set_fault(FAULT_BUS_OVP);
							s_pfc_state = PFC_STATE_FAULT;
					}
				}else if(g_adc_multi.bus_vol_v<VBUS_UVP_V)
				{
					s_vbus_ovp_ms = 0U;
					if(++s_vbus_uvp_ms >= VBUS_UVP_DEBOUNCE_MS)
					{
						protect_set_fault(FAULT_BUS_UVP);
						s_pfc_state = PFC_STATE_FAULT;
					}
				}
				else
				{
					s_vbus_ovp_ms = 0U;
					s_vbus_uvp_ms = 0U;
				}
        break;

    case PFC_STATE_FAULT:
        pfc_disable();
        break;
    }
}

pfc_state_t pfc_get_state(void) { return s_pfc_state; }

bool pfc_is_ready(void)
{
    const bool ac_ok = (g_adc_multi.ac_vol_v >= PFC_AC_INPUT_MIN_V) &&
                       (g_adc_multi.ac_vol_v <= PFC_AC_INPUT_MAX_V);
    const bool vbus_ok = (g_adc_multi.bus_vol_v >= LLC_ENTRY_V) &&
                         (g_adc_multi.bus_vol_v <= VBUS_OVP_V);

    return (s_pfc_state == PFC_STATE_RUN) && ac_ok && vbus_ok;
}










