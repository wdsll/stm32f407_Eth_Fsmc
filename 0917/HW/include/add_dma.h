#ifndef ADD_DMA_H
#define ADD_DMA_H
#include "gd32f30x.h"

typedef struct {
    uint16_t vout_raw;    /* PA5 */ 
    uint16_t isense_raw;  /* PA6 */
    uint16_t tsense_raw;  /* PA7 */
    uint16_t v3v3_raw;    /* PC4 */
    uint16_t vbt_raw;     /* PC5 */
    uint16_t t_llc_raw;   /* PB1 */
} adc_multi_frame_t;

enum { 
	ADC_MULTI_CHANNEL_COUNT = 6U,
	ADC_TIM0_TRIGGERED_COUNT = 2U
};
//static uint16_t s_buf[ADC_MULTI_CHANNEL_COUNT * 2U];
static uint16_t s_buf[ADC_TIM0_TRIGGERED_COUNT * 2U];
static volatile adc_multi_frame_t s_latched;

extern volatile adc_multi_frame_t g_adc_multi;

void adc_multi_init_dma(uint32_t trig_src /* e.g. ADC_EXTTRIG_REGULAR_T0_CH0 */);
void adc_multi_start(void);
void adc_multi_copy(void);

/*
# ADC 采集方案评估

## 硬件资源与管脚配置
- `adc_multi_init_dma` 在使能 DMA 之前调用 `analog_pins`，为 PA5/PA6/PA7、PB1、PC4/PC5 配置为模拟输入，覆盖 6 路采样点（Vout、电流、两路温度和两路电压检测）。
- 采样通道次序固定，配合 `adc_multi_store_frame` 将 DMA 缓冲的每 6 个采样值按顺序锁存，确保上层读取时的语义一致。

## 触发与速率控制
- 常规组长度设置为 6，采样时间统一为 55.5 周期（约 20μs），并通过 `adc_external_trigger_source_config` 配置为 `TIMER0 CH0` 触发。
- `adc_multi_start` 通过向 `LLC_PWM_TIMER` 触发软件事件（`TIMER_EVENT_SRC_CH0G`）启动首次转换，与 LLC PWM 控制保持时序一致。

## DMA 架构
- DMA0 通道 0 工作在循环模式，目标为 `ADC_RDATA(ADC0)`，内存缓冲 `s_buf` 长度为 12（6 路 * 双缓冲），半传输/全传输中断分别写入缓存，形成“乒乓”行为。
- 中断服务在清除标志后调用 `adc_multi_store_frame`，把有效半缓冲复制到 `s_latched`，随后 `adc_multi_copy` 在关中断的临界区内将其拷贝到全局 `g_adc_multi`，避免读写竞争。

## 数值转换与上层使用
- 上层在 `main.c` 中通过 `g_adc_multi` 获取原始值，并借助电阻分压/采样函数完成电压、电流换算，在调试模式下可直接输出原始与换算结果。

## 发现的问题与改进建议
1. **触发与同步**：ADC 外部触发源由 `trig_src` 参数传入，调用方当前固定为 `ADC0_1_EXTTRIG_REGULAR_T0_CH0`，确保和定时器同步。如果后续需要支持其它触发源，可考虑在接口文档中明确时钟约束以及定时器配置要求。
2. **缓冲管理**：`s_buf` 和 `s_latched` 定义在头文件里（`static`），每个包含者都会生成独立副本。目前只有 `adc_dma.c` 使用，但为避免误用，建议迁移到实现文件或改为 `extern` 声明。
3. **误差与校准**：流程在 `adc_enable` 后执行 `adc_calibration_enable` 完成一次性校准，缺乏对 Vref 波动或温度的动态补偿。若对测量精度有更高要求，可引入定期重新校准或内部温度/参考电压的检测。
4. **上层数据完整性**：`adc_multi_copy` 在调用者的上下文中执行，不带返回值指示新数据是否可用。若需要区分新旧数据，可增加帧计数或时间戳，避免重复处理旧样本。

## 总体结论
ADC 采集链路以 DMA 双缓冲配合定时器触发实现了 6 通道同步采样，数据搬运和互斥机制设计合理。需要关注的是头文件中静态缓冲的可见性以及未来精度和数据状态管理的扩展需求。
*/
#endif
