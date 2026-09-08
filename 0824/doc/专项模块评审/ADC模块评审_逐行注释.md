# ADC 模块评审报告（逐行注释）

> 评审时间：2026-09-03（v3，结合 0824 项目 0917 迭代后的代码状态刷新）
> 修订记录：
> - 2026-08-24 v1 初版（当时误带 48V 项目参数 40A/58.4V/64V，本版已全部换为 0824 的 72V/20A/94V）
> - 2026-09-03 v3：① 确认 P0-1 uint16_t 下溢补丁已落实（且 bus 通道改为取消偏置，处置方式与 v1 预期不同）；② OCP 阈值 24A 贴边 ADC 满量程 24.8A 的风险首次点出；③ 参数表全部换为 0824 规格；④ 补 PA6 引脚冲突对本模块的影响；⑤ AC 采样率风险按当前代码重述。
> 评审对象（0824 工程，GD32F303RE，1.5kW 72V/20A Li-ion 充电器）：
> - `HW/src/adc_dma.c`（当前 202 行）+ `HW/include/adc_dma.h` —— ADC0 多通道 DMA 采样（5 通道：AC/BUS/VOUT/I_SENSE/VBT）+ 物理量换算 + AC RMS
> - 关键依赖宏（已核对 `App/include/main.h` 当前版，354 行）：`VREF_ADC=3.30f`、`ADC_RESOLUTION=4095`、`AC_RMS_WINDOW_SAMPLES=1000`、`VBUS_RTOP=1.32M/RBOT=8.6k`、`AC_RTOP=1.32M/RBOT=5.1k`、`VOUT/VBT_RTOP=240k/RBOT=4.7k`、`ISHUNT=3.33mΩ`、`IAMP_GAIN=20`、`VOUT_OVP_V=94`、`IOUT_OCP_A=24`、`VBUS_OVP_V=420`、各通道映射（`AC=CH1/PA1`、`BUS=CH3/PA3`、`VOUT=CH5/PA5`、`I=CH6/PA6`、`VBT=CH15/PC5`）

接线/调用核对（已确认，main.c 当前版）：
- `main.c:267` `adc_multi_init_dma(ADC0_1_2_EXTTRIG_REGULAR_NONE)` → **外部触发源 = NONE（纯软件触发）**。
- `main.c:268` `adc_multi_start()` 软件触发首轮；主循环 `if(run_adc_monitor_tick)` 块内 `adc_multi_copy()` → `adc_ac_sample_fast(raw)` → `adc_multi_trigger_fast()`（main.c:334-336）。
- TIMER3（main.c:102-125，5kHz）仅作 pending 节拍源，**不直接触发 ADC**（因 exttrig=NONE）。
- 物理量转换在 `monitor_protect_tick_1khz()` 开头调 `adc_multi_sample_aux_1khz()`（main.c:145）。
- 启动自检 `adc_startup_check()`（main.c:160-177）：软件触发 8 次 + 每次 2ms 延时后再判饱和。

总体评价：**模块结构清晰、通道映射与结构体顺序一致、AC RMS 数学正确（全波整流用 √(E[x²])）、下溢补丁已正确落地**。当前最需要盯的两件事：**① OCP=24A 与 ADC 电流满量程 24.8A 只差 0.8A，保护裕度被标定误差直接吃掉；② ADC 实际采样率由主循环消费速率决定（≤5kHz），AC RMS 的 200ms 整周期窗口只在满速时成立**。

---

## 一、0824 规格换算核对（v3 重算，替换 v1 的 48V 参数表）

| 参数 | 值 | 核对结论 |
|---|---|---|
| 母线分压比 | (1.32M+8.6k)/8.6k ≈ **154.65** | 380V 母线→2.46V；`VBUS_OVP_V=420V`→2.72V（3.3V 量程内 ✓）。整流峰值+浪涌仍需确认不超 3.3V（P2-①）。 |
| 输出/电池分压比 | (240k+4.7k)/4.7k ≈ **52.06** | 72V 满电（20串 Li-ion ≈84V）→1.61V；`VOUT_OVP_V=94V`→1.81V ✓。 |
| AC 分压比 | 5100/(4×330k+5100) = 1/259.8 | 220V RMS→0.85V RMS；264V RMS→1.02V，峰值 1.44V（安全 ✓）。 |
| 电流通道 | ISHUNT=3.33mΩ × GAIN=20 → **66.6mV/A** | 20A 额定→1.33V 摆幅，叠加 V_offset 1.65V→2.98V ✓；**满量程 3.3V 对应 24.8A**。 |
| **OCP 阈值** | `IOUT_OCP_A=24A` | ⚠ **新发现（P1-③）**：24A 距 ADC 满量程 24.8A 仅 0.8A。V_offset/增益若有 ±2% 标定误差（≈±0.5A@20A），OCP 判据可能永远够不到或在额定附近误判。20A 额定已用量程 81%，标定必须精。 |
| AC_RMS_CALIBRATION | 1.0（待标定） | 初版占位，需用标准源点校（P2-②）。 |

> v1 表中"40A→摆幅 2.66V 超量程、OCP 30A 死代码"的结论**作废**——那是 48V/40A 项目参数。0824 规格下 OCP=24A 落在可测区间内，死代码问题不存在；取而代之的是上面 P1-③ 的"贴边"问题。

---

## 二、adc_dma.c 逐行注释（当前 202 行，v3 刷新）

| 行 | 代码 | 评审注释 |
|---|---|---|
| 1 | `/* ADC 多通道 DMA 采样 (5 通道, ADC0 only) */` | 准确 ✓。 |
| 4-5 | `AC_DIV_RATIO = 5100/(4*330000+5100)` | ≈0.003849（1/259.8），与 main.h `AC_RTOP/RBOT` 一致 ✓。 |
| 7-10 | `ADC_OFFSET_COUNTS (73U)` 台架标定值 + 注释 | ✅ **P0-1 下溢补丁已落实**。注释把"raw<offset 时 uint16_t 下溢成 65463 → 假 8149V 污染 AC RMS"的机理写清楚了，可维护性好。 |
| 20-24 | `adc_sub_offset_sat()` 饱和减法 | ✅ `(raw > offset) ? raw-offset : 0`，正确堵死下溢路径。 |
| 35-37 | `gpio_init(..., GPIO_MODE_AIN, ...)` PA1/PA3/PA5/PA6 + PC5 | 模拟输入正确。**注意 PA6 双重身份**：此处配 AIN（I_SENSE/CH6），而 `bus_vol_adj_pwm_init`（pwm_llc.c:168）会把 PA6 配成 AF_PP——两者互斥，绝不能在同一固件里同时调用（详见 pwm_llc 评审 v2 的 B1 撤回说明）。当前正常固件只调本函数，PA6 归 ADC ✓。 |
| 39 | `rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV8)` | 120/8=15MHz。注释"GD32F30x ADC 上限 40MHz"正确（GD32F303 数据手册 ADCCLK max=40MHz），**15MHz 合规** ✅。（早前"踩 14MHz 上限"的结论是拿 STM32F1 的限制套了 GD32，已撤回。） |
| 41-46 | deinit/SCAN/CONTINUOUS 关/exttrig=NONE/右对齐 | continuous 关 + exttrig=NONE → 每轮必须软件触发，见 P1-①。 |
| 49-56 | `ch_list[]` 5 通道 + rank 0..4 | 与 `adc_multi_t` 字段顺序一致 ✓。**温度/FAN 通道仍未加入**（main.h 定义了 10 通道，`ADC_CHANNEL_QTY=5` 只扫 5 个：AC/BUS/VOUT/I/VBT；T_CASE=PA4、T_PFC=PA7、T_TR=PC4、T_LLC=PB1、FAN_CS=PA2 全部不在序列）→ 过温保护数据源仍断（T1，与保护评审联动）。 |
| 58-77 | DMA0_CH0 循环模式 + 使能链 | 配置正确 ✓。`dma_flag_clear(..., DMA_FLAG_G)`（L73）清全局标志属多余但无害（P3）。 |
| 75-76 | `adc_enable` 后 `adc_calibration_enable` | 顺序正确 ✓。 |
| 84-92 | `adc_multi_start` / `adc_multi_trigger_fast` | 均为软件触发一轮；后者由主循环每次 pending 消费时调用——采样率取决于主循环（见 P1-①）。 |
| 99-114 | `adc_multi_copy_if_ready()` | ✅ 亮点：以 `DMA_FLAG_FTF` 为同步点，拷贝后清 FTF。**L105**：`ac_vol_raw = adc_sub_offset_sat(buf[0], 73)`——AC 通道去偏置 + 饱和减法 ✓。**L106-107（v3 关键更新）**：`bus_vol_raw = s_adc0_dma_buf[1]` **直接赋值，偏置已取消**，注释"Vbus加偏置0V准 但是上高压不行还是取消偏置"。处置合理：bus 通道不加偏置就无下溢可能，0V 附近的固定小偏差改由上层阈值（350/360V）自然吸收；**代价是母线低压读数含 ~73 counts 固定偏差**（≈0.06V ADC 端 ×154.65 ≈ 9V 母线侧），对 350V 门槛无实质影响，但若日后要做母线低压精细判据需记得补回标定。 |
| 115-118 | `adc_multi_copy()` | `(void)` 丢弃返回值。主循环 E 项（main.c:334-336 未判 ready 仍 sample+trigger）仍在，见 main.c 评审。 |
| 120-153 | `adc_ac_sample_fast` | ✅ 数学正确：全波整流无 DC offset，RMS=√(E[x²])，非标准差。200ms 窗口=1000 样本**隐含 5kHz 采样率假设**（main.h:45-46 注释"10 cycles at 50Hz"），与 P1-① 的实际采样率风险绑定。`ac_vol_inst_v`（L152）**无 `AC_RMS_CALIBRATION` 标定系数**——RMS 与瞬时值标定不一致，用于 PFC 逻辑判据前需统一（见 0917 项目对 ac_vol_inst_v 的喂入方式）。 |
| 155-163 | `adc_multi_sample_aux_1khz()` | raw→物理量 ✓；AC 行已注释（改由 RMS 路径算）✓。 |
| 166-170 | `adc_raw_to_voltage` | 通用分压反算 ✓。 |
| 172-180 | `adc_raw_to_current` | NSI1312：`I=(V_adc−V_offset)/(R_shunt×Gain)`，V_offset=VREF/2=1.65V **仍是待标定假设**（P1-②）；负电流钳 0 ✓。 |
| 181-201 | `#if 0` fan/ntc 函数 | 温度恢复时需同步：启用本函数 + ch_list 加通道 + 结构体 t_* 字段 + protect 解注释，四处一起动。 |

---

## 三、问题清单（v3）

### P1（接真电池/上功率前必须落实）
1. **① ADC 实际采样率 = 主循环消费速率（≤5kHz，非硬定时）**：exttrig=NONE + 主循环 copy→trigger。主循环每圈若能在 200µs 内跑完，采样率=5kHz、RMS 窗口=200ms；一旦被串口打印/故障处理拖慢（debug_printf 一帧状态 ~150 字节 @115200 ≈ 13ms 发送时间，虽然发送在环形缓冲、不阻塞，但主循环每圈调用 `serial_console_task` + `debug_tx_task` 仍耗时），pending 丢弃计数增加，**实际采样率 <5kHz，RMS 窗口时长=1000/实际速率>200ms，且不再是 50/60Hz 整数周期**→ RMS 混入周期泄漏误差。修复方向：改 TIMER3 TRGO 硬件触发（exttrig 设 `ADC0_1_2_EXTTRIG_REGULAR_T3_CH0` 类），DMA 循环搬运，主循环只 copy。0917 项目已按"10kHz ADC + 200ms 整周期 RMS"方案重构，0824 建议跟进同一思路。
2. **② 电流标定（NSI1312 V_offset=1.65V 假设）**：需台架实测失调点与增益（0824 用 NSI1311 做的是 AC 电压隔离放大，电流通道是 NSI1312，两者别混）；未标定前 `iout_a` 可能系统性偏差甚至恒 0，直接连累 OCP（protect）与完成判据（llc_control C1）。
3. **③ OCP=24A 贴边满量程 24.8A（新发现）**：0.8A 裕度 < 标定误差量级。建议：①精确点校后确认 24A 读数稳定落在 ~3.19V；②在 protect 加"raw 接近满量程即判饱和过流"兜底（如 `isense_raw >= 4000` 直接 FAULT_OCP），把"量程耗尽"变成可判故障。
### P2（初版可调/核对）
4. **① 母线峰值量程**：确认整流峰值（含浪涌）×(1/154.65) < 3.3V。
5. **② AC_RMS_CALIBRATION=1.0 与 ac_vol_inst_v 无标定系数**：两路输出标定要统一，`pfc_is_ready` 用的是 `ac_vol_v`（RMS 路径），AC 欠压/过压门槛（180/264V）有效性取决于这次标定。另注意 `PFC_AC_INPUT_MIN_V=180V` 会拒绝实测可用的 165V 输入（PFC 板已验证 165V/1300W），需要的话把下限改 ~150V。
6. **③ AC RMS 首窗口**：上电后前 200ms `ac_vol_v=0`，启动自检（`adc_startup_check`）不覆盖 RMS 有效性；PFC 若在首窗口内就绪不受影响（门槛是 bus 不是 ac），可接受，记录即可。
7. **④ bus 通道偏置取消后的低压读数偏差**：~73 counts 固定偏差（≈母线侧 9V），350/360V 门槛不受影响；若后续做母线 UVP 精细判据（<304V 一类）时记得补标定或恢复饱和减法路径。

### P3（代码整洁/注释）
8. 头文件注释"10 通道"过时（adc_dma.h）→ 改 5 通道。
9. `DMA_FLAG_G` → 建议明确 `DMA_FLAG_FTF`。
10. `adc_multi_copy()` 丢返回值的用法（联动 main.c E 项）。
11. `#if 0` 的 ntc/fan 建议加一句"启用时需同步启用 ch_list + t_* 字段"。

---

## 四、亮点（v3 保留 + 新增）

- ✅ **P0-1 下溢补丁正确落地**：饱和减法 + 注释写明机理，且按通道特性差异化处置（AC 去偏置、BUS 取消偏置），是"修 bug 顺手把标定策略理顺"的好例子。
- ✅ AC RMS 用 √(E[x²]) 正确处理全波整流无 offset 波形。
- ✅ DMA 以 FTF 为同步点，避免半帧拷贝。
- ✅ 扫描序列与 `adc_multi_t` 字段顺序严格一致，copy 映射无错位。
- ✅ 负电流 clamp 0（充电单向）。
- ✅ ADC 时钟 15MHz，落在 GD32F30x 40MHz 规格内（v1 的"超 14MHz"结论已按 GD32 手册撤回）。

---

## 五、上板前检查清单（ADC 模块，v3）

- [ ] **电流点校**（P1-②/P1-③）：标准源实测 NSI1312 失调点/增益，确认 20A 读数 ~2.98V、24A 判据稳定；加"raw≥4000 饱和即过流"兜底。
- [ ] **AC 点校**：标准 AC 源标 `AC_RMS_CALIBRATION`，同步给 `ac_vol_inst_v` 补标定系数；确认 165V 输入是否要放开 `PFC_AC_INPUT_MIN_V`。
- [ ] **采样率确认**（P1-①）：用示波器/丢帧计数（`s_adc_monitor_tick_drop_count`）验证满载打印时实际采样率；考虑改硬件触发或对齐 0917 的 10kHz 方案。
- [ ] 母线峰值不超 ADC 量程（P2-①）。
- [ ] bus 通道取消偏置后，低压段读数偏差记录在案（P2-④）。
- [ ] 温度恢复时四处同步启用（ntc 函数 + ch_list + t_* 字段 + protect 解注释）。
- [ ] 清理过时注释（"10 通道"、DMA_FLAG_G）。
