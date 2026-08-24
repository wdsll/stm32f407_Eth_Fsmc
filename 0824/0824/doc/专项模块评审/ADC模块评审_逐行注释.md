# ADC 模块评审报告（逐行注释）

> 评审时间：2026-08-24
> 评审对象（0824 工程，GD32F303RE）：
> - `HW/src/adc_dma.c` + `HW/include/adc_dma.h` —— ADC0 多通道 DMA 采样（5 通道：AC/BUS/VOUT/I_SENSE/VBT）+ 物理量换算 + AC RMS
> - 关键依赖宏（已核对 `App/include/main.h`）：`VREF_ADC=3.30f`、`ADC_RESOLUTION`、`AC_RMS_WINDOW_SAMPLES=1000`、`VBUS_RTOP=1.32M/RBOT=8.6k`、`VOUT/VBT_RTOP=240k/RBOT=4.7k`、`AC_RTOP=1.32M/RBOT=5.1k`、`ISHUNT=3.33mΩ`、`IAMP_GAIN=20`、各通道映射（`AC=CH1/PA1`、`BUS=CH3/PA3`、`VOUT=CH5/PA5`、`I=CH6/PA6`、`VBT=CH15/PC5`）

接线/调用核对（已确认）：
- `main.c:245` `adc_multi_init_dma(ADC0_1_2_EXTTRIG_REGULAR_NONE)` → **外部触发源 = NONE（纯软件触发）**。
- `main.c:246` `adc_multi_start()` 软件触发首轮；主循环 `if(run_adc_monitor_tick)` 块内 `adc_multi_copy()` → `adc_ac_sample_fast(raw)` → `adc_multi_trigger_fast()`（每处理一次 pending 软件触发一轮）。
- TIMER3 仅作 5kHz pending 节拍源，**不直接触发 ADC**（因 exttrig=NONE）。
- 物理量转换在 `monitor_protect_tick_1khz()` 开头调 `adc_multi_sample_aux_1khz()`（main.c:144）。

总体评价：**模块结构清晰、通道映射与结构体顺序一致、AC RMS 数学正确（全波整流用 √(E[x²])）**，是驱动层里写得较稳的一份。但存在两个 P1 级架构/标定风险：**① ADC 采样率由主循环驱动而非固定 5kHz，导致 AC RMS 窗口实际时长不确定；② 电流标定（NSI1312 失调点/量程）依赖硬件实测，当前 `V_offset=1.65V` 假设可能错误**。这两点初版联调用稳压源+电子负载可先跑，但接真电池前必须落实。

---

## 一、adc_dma.h 逐行注释

| 行 | 代码 | 评审注释 |
|---|---|---|
| 1-7 | 文件头（版本 0.1.0，Rengar） | 摘要写得是"主头文件"，其实是 ADC 模块头，**摘要文案过时**（P3）。 |
| 8 | `/* ADC 多通道 DMA 采样 (10 通道) */` | **注释过时**：实际 `ADC_CHANNEL_QTY=5`，仅 5 通道在用（另 5 个 fan/t 通道被注释预留）。"10 通道"会误导维护者（P3）。 |
| 9-12 | include guard + `main.h` | OK。 |
| 14-40 | `adc_multi_t` 结构体 | raw 字段顺序 `ac_vol/bus_vol/vout/isense/vbt`（与 c 文件 ch_list 顺序一致 ✓）；被注释掉的 `fan_cs/t_*` 成员不占内存，不影响布局。 |
| 16/18/20/21/24 | raw 字段引脚注释 | PA1/PA3/PA5/PA6/PC5，与 `main.h` 通道映射（`CH1/CH3/CH5/CH6/CH15`）一致 ✓。注意 `VBT=CH15=PC5` 正确（GD32F30x CH15 即 PC5）。 |
| 28-38 | 物理量字段 `ac_vol_v/ac_vol_inst_v/bus_vol_v/vout_v/iout_a/vbat_v` | 与 raw 一一对应，缺 `t_*`（温度，按你"温度先不管"指令已注释关闭，合理）。 |
| 42 | `extern adc_multi_t g_adc_multi;` | 全局采样结果，供保护/状态机/串口读取。 |
| 44-50 | 函数声明 | 齐全。 |
| 52-56 | 转换函数声明（含 `#if 0` 注释掉的 fan/ntc） | `adc_raw_to_fan_current`/`ntc_raw_to_temp_c` 已被 `#if 0` 关停，声明也注释掉，一致 ✓；温度恢复时需一并启用。 |

---

## 二、adc_dma.c 逐行注释

| 行 | 代码 | 评审注释 |
|---|---|---|
| 1 | `/* ADC 多通道 DMA 采样 (5 通道, ADC0 only) */` | 准确（实际 5 通道）✓。 |
| 4-5 | `AC_DIV_RATIO = 5100/(4*330000+5100)` | AC 分压比 ≈ 0.003849，即 1/259.8。与 main.h `AC_RTOP=1.32M/AC_RBOT=5.1k` 的分压比一致 ✓。 |
| 6 | `adc_multi_t g_adc_multi;` | 全局，零初始化（初值 0，首 RMS 窗口前 `ac_vol_v=0`）。 |
| 8 | `s_adc0_dma_buf[ADC_CHANNEL_QTY]` | DMA 缓冲 5 字，与通道数一致 ✓。 |
| 11-12 | `s_ac_raw_square_sum / s_ac_sample_count` | AC RMS 累加器。 |
| 15-20 | `adc_multi_init_dma` 开时钟 | GPIOA/GPIOC/ADC0/DMA0 时钟，必需 ✓。 |
| 23-25 | `gpio_init(..., GPIO_MODE_AIN, ...)` | PA1/PA3/PA5/PA6 + PC5 模拟输入，正确（无 PUPD 参数，SPL 风格 ✓）。 |
| 27 | `rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8)` | APB2(≤120MHz)/8 ≤ 15MHz，GD32F30x ADC 上限 40MHz，**安全** ✓。 |
| 29-34 | ADC 配置 | `SCAN_MODE=ENABLE`、`CONTINUOUS_MODE=DISABLE`、`exttrig 源=NONE`、`exttrig 使能`、`RIGHT 对齐` —— **注意**：continuous 关 + exttrig=NONE 意味着每次转换必须**软件触发**，见 P1-①。 |
| 37-43 | `ch_list[]` + `adc_regular_channel_config(ADC0, i, ch, 55POINT5)` | 扫描序列与结构体顺序一致 ✓；采样时间 55.5 周期。 |
| 44 | `adc_channel_length_config(..., ADC_CHANNEL_QTY)` | 长度=5 ✓。 |
| 47-59 | DMA 配置 | periph=ADC_RDATA、mem=buf、number=5、periph 不增、mem 增、16bit、P2M、HIGH 优先级 ✓。 |
| 60 | `dma_circulation_enable` | 循环模式 ✓（持续刷新最新 5 通道）。 |
| 61 | `dma_flag_clear(DMA0, DMA_CH0, DMA_FLAG_G)` | 清全局标志，多余但无害；建议明确用 `DMA_FLAG_FTF`（P3）。 |
| 63-66 | `adc_enable / adc_calibration_enable / dma_channel_enable / adc_dma_mode_enable` | 顺序合理；校准在 enable 后 ✓。 |
| 68-69 | 复位累加器 | OK。 |
| 72-75 | `adc_multi_start()` | 软件触发一次（启动时首轮）。 |
| 77-80 | `adc_multi_trigger_fast()` | 软件触发一轮 —— **P1-① 核心**：ADC 由主循环每次调用时触发，非硬件定时。 |
| 81-101 | `adc_multi_copy_if_ready()` | **亮点**：以 `DMA_FLAG_FTF`（整轮传输完成）为同步点，避免半帧拷贝；拷贝后清 FTF ✓。注释解释了"DMA 不受 CPU 中断影响，故用 FTF 同步" ✓。 |
| 102-105 | `adc_multi_copy()` | 直接 `(void)adc_multi_copy_if_ready()`，忽略返回值（主循环不在意是否刚好完成）。 |
| 107-116 | `adc_ac_sample_fast` 注释 | **数学正确**：全波整流波形无 DC offset，RMS=√(E[x²])，**不能用** √(E[x²]−E[x]²)（那会变成标准差）。这点写得对 ✓。 |
| 117-140 | `adc_ac_sample_fast(raw)` | 累加 `raw²`（119-121）；满 `AC_RMS_WINDOW_SAMPLES(1000)` 样本算 RMS（123-137）：`rms_raw=√mean_sq` → `adc_rms_v` → `/AC_DIV_RATIO*AC_RMS_CALIBRATION` 还原到 AC 输入端（130-133）；同时更新瞬时值 `ac_vol_inst_v`（139）。 |
| 142-150 | `adc_multi_sample_aux_1khz()` | 把 raw 转物理量：bus/vout/vbat 用 `adc_raw_to_voltage`，iout 用 `adc_raw_to_current`；ac_vol_v 的换算被注释（146，改由 RMS 路径算）✓ 一致。 |
| 152-157 | `adc_raw_to_voltage(raw, rtop, rbot)` | 通用分压反算 `v_adc*(rtop+rbot)/rbot` ✓。 |
| 159-167 | `adc_raw_to_current(raw)` | NSI1312：`I=(v_adc−V_offset)/(R_shunt*Gain)`，**负电流 clamp 0**（充电单向，合理）✓。但 `V_offset=VREF/2=1.65V` 是**待标定假设**（见 P1-②）。 |
| 168-188 | `#if 0` fan/ntc 函数 | 已关停，初版不编译，合理；恢复温度时需启用并核对 NTC 公式。 |

---

## 三、标定参数核对（基于 main.h 实测值）

| 参数 | 值 | 核对结论 |
|---|---|---|
| 母线分压比 | (1.32M+8.6k)/8.6k ≈ 154.6 | 400V→2.59V（3.3V 量程内 ✓）；最高母线需确认整流峰值不超 3.3V（P2-①）。 |
| 输出/电池分压比 | (240k+4.7k)/4.7k ≈ 52.0 | 58.4V→1.12V、64V→1.23V（OVP）均安全 ✓。 |
| AC 分压比 | 1/0.003849 ≈ 259.8 | 220V RMS→0.85V RMS、峰值 1.2V（安全 ✓）；264V 峰值 1.44V（安全 ✓）。 |
| 电流传感器 | ISHUNT=3.33mΩ, GAIN=20 → 摆幅 66.6mV/A | **P1-②**：40A→摆幅 2.66V，叠加 V_offset 后可能超 3.3V 量程（饱和）；且 V_offset=1.65V 假设 NSI1312 由 3.3V 供电，**实际 NSI1312 常 5V 供电→中点 2.5V**。需实测点校，否则电流读数系统性偏差甚至符号错。 |
| AC_RMS_CALIBRATION | 1.0（待标定） | 初版占位，需用标准源点校（P2-②）。 |

---

## 四、问题清单（按严重度）

### P1（建议初版内确认/改进，接真电池前必须落实）
1. **① ADC 采样率不可控（软件触发 + 主循环驱动）**：`exttrig=NONE` + 主循环 `adc_multi_trigger_fast()`，使 ADC 实际采样率 = **super-loop 频率**（受 `monitor_protect_tick_1khz` 等耗时影响，通常约 1kHz 量级），并非命名/注释假设的 5kHz。后果：AC RMS 注释假定"5kHz / 200ms / 1000 样本"，但实际窗口 = (主循环周期)×1000 样本 ≈ **1 秒而非 200ms**，且采样非均匀。`AC_RMS_WINDOW_SAMPLES=5000/5=1000` 这个"5000/5"隐含了 5kHz 假设，与主循环实际频率脱钩。**建议修复**：改用 TIMER3（或 TIMERx）**硬件触发** ADC 规则组（`exttrig` 设为对应 `TIMERx_TRGO`），DMA 循环自动搬运，主循环只 copy+转换。这样采样率锁定 5kHz，RMS 窗口真正 200ms。
2. **② 电流标定风险（NSI1312）**：`adc_raw_to_current` 的 `V_offset=VREF_ADC/2=1.65V` 假设传感器由 3.3V 供电；NSI1312 典型供电 5V、失调点 2.5V。且 `(R_shunt*Gain)=0.0666` 对应 40A 摆幅 2.66V，叠加后可能超 ADC 量程导致饱和（约 24.8A 即满量程）。**建议**：核对硬件供电与失调点、用标准电流源点校 `IAMP_GAIN`/`V_offset`/`ISHUNT`，确认 40A 满量程落在 ADC 线性区。

### P2（初版可调/核对）
3. **① 母线最高电压量程**：确认整流后母线峰值（含浪涌）经 154.6 分压后仍 < 3.3V，避免 ADC 饱和导致母线读数失真/保护误判。
4. **② 标定系数待定**：`AC_RMS_CALIBRATION=1.0`、`VOUT/VBT/VBUS` 分压依赖电阻精度，初版用标准源点校一次。
5. **③ AC RMS 窗口/首值**：首 1000 样本前 `ac_vol_v=0`，PFC/保护若早期依赖 `ac_vol_v` 可能误判；建议启动后等首窗口完成再用，或在 startup_check 里加 AC 有效性判断。

### P3（代码整洁/注释）
6. **头文件注释"10 通道"过时**（adc_dma.h:8）→ 改为 5 通道。
7. **摘要文案错误**（adc_dma.h:3 "主头文件"）→ 改为 ADC 模块头。
8. **`DMA_FLAG_G` 全局清**（adc_dma.c:61）→ 建议明确为 `DMA_FLAG_FTF`。
9. **`IAMP_GAIN` 注释乱码**（main.h:68）→ 重填"放大器增益，待定"。
10. `#if 0` 的 fan/ntc 函数可保留（温度恢复用），但建议加一句"启用时需同步启用结构体 t_* 字段"。

---

## 五、亮点（保留）

- AC RMS 用 √(E[x²]) 正确处理全波整流无 offset 波形 ✓。
- DMA 以 FTF 为同步点，避免半帧拷贝 ✓。
- 扫描序列与 `adc_multi_t` 字段顺序严格一致，copy 映射无错位 ✓。
- 负电流 clamp 0（充电单向）✓。
- `adc_raw_to_voltage` 通用分压函数被多路复用 ✓。

---

## 六、上板前检查清单（ADC 模块）
- [ ] 用标准源/可调电源点校 VOUT/VBAT 分压（58.4V 应读 ~1.12V ADC→换算 58.4V）。
- [ ] **电流点校**：标准电流源测 NSI1312 实际失调点与灵敏度，修正 `V_offset`/`IAMP_GAIN`/`ISHUNT`，确认 40A 不超量程（P1-②）。
- [ ] 确认 ADC 采样率：用示波器/定时器测实际采样节拍，验证 AC RMS 窗口是否为预期 200ms；若不满足，按 P1-① 改硬件触发。
- [ ] 母线最高电压 ADC 读数不超 3.3V（P2-①）。
- [ ] 清理过时注释（"10 通道"、摘要文案、乱码）（P3）。
- [ ] 温度恢复时同步启用 ntc 函数 + 结构体 t_* 字段。
