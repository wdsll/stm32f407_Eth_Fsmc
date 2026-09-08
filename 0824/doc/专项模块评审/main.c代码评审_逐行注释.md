# main.c 代码评审（逐行注释）v2

> 评审对象：`0824/App/src/main.c`（v0.2.2，当前 356 行）
> 修订记录：
> - 2026-08-27 v1 初版
> - 2026-09-03 v2（本版）：① **C 项已修**——自检失败现在置 FAULT+红灯+状态码（用户已按补丁 3 落实且更完整）；② **B1 性质修正并撤回补丁 1**——`bus_vol_adj_pwm_init` 缺失不是缺陷，是为规避 PA6 双重定义的有意设计（调用会毁掉 I_SENSE 电流采样）；③ **LLC_EN 极性反转核对**——`hw_gpio_init` 已改为 `llc_disable()` 初始关断（低有效），与 llc_control.c 新封装一致；④ standalone PFC 测试门控已被注释（PFC ON/OFF 命令移除）；⑤ 行号按 356 行当前版重对。
> 上下文：1.5kW OBC（72V/20A Li-ion），GD32F303RE；模拟 PFC(NCP1654)+模拟 LLC，MCU 只做监控/保护/时序/通信。

---

## 一、文件结构概览（v2，356 行）

| 区段 | 行号 | 作用 |
|---|---|---|
| 头文件 | 14-23 | 依赖声明 |
| 宏 / 内部变量 | 27-52 | ADC 采样率、`g_ms`、tick pending 标志 |
| delay / SysTick | 56-93 | 1ms 节拍中断，只置 pending 标志 |
| ADC 监控定时器 | 95-140 | TIMER3 5kHz 采样节拍，只置 pending 标志 |
| 1kHz 监控任务 | 141-158 | 串起 ADC 换算 / 保护 / PFC / 功率时序 |
| 启动自检 | 159-190 | ADC 饱和检查 + 保护自检 |
| GPIO 初始化 | 191-225 | 所有功率/指示灯引脚初始为安全态 |
| main() | 229-355 | 时钟→串口→GPIO→SysTick→CAL 或正常分支→自检→init→主循环 |

调度骨架不变：**中断只置「最新任务」pending、不补跑、主循环每圈最多各执行一次**，丢节拍只计数。✓

---

## 二、v2 重点变化逐条核对

### 1. 启动自检失败处理（285-292）—— ✅ C 项已修（v1 补丁 3 落实，且比建议更完整）
```c
285  if (!protect_ok || !adc_ok) {
286      debug_printf("[STARTUP] Preflight FAIL: protect=%d adc=%d\r\n", ...);
287      protect_set_fault(FAULT_HARDWARE_PRO);   // 走统一故障入口
288      g_charger_state = MAIN_STEP_FAULT;       // 显式置位（幂等冗余，确保可见）
289      g_fault = FAULT_HARDWARE_PRO;
290      gpio_bit_set(LED_RED_PORT, LED_RED_PIN); // 红灯
291      while (1) { __NOP(); }                   // 安全停机
292  }
```
- v1 的三点要求（`g_fault` 可见、红灯、安全停机）全部落实，且额外走 `protect_set_fault()` 统一入口（内部会 `power_supervisor_enter_fault()`，虽然此处主状态机尚未 init，`enter_state` 直接写 `g_charger_state` 也无副作用）。
- `:288-289` 与 `protect_set_fault` 的内部置位重复，属"宁可冗余也要可见"的防御写法，可接受。
- 注：此处死循环时功率 GPIO 全为初始安全态（`hw_gpio_init` 在 240 行已跑完，PWM/ADC 尚未 init），安全 ✓。

### 2. LLC_EN 初始电平（200-203）—— ✅ 极性反转已对齐
```c
202  gpio_init(LLC_EN_PORT, GPIO_MODE_OUT_PP, ...);
203  llc_disable();  /* 初始关闭 低电平使能，高电平关断 */
```
- v1 时代这里是 `gpio_bit_reset()` 注释"初始关"；llc_control.c 引入低有效封装后，**若沿用 reset 就变成上电即开 LLC**。当前已改为 `llc_disable()`（拉高），注释同步更新 ✅。
- 提醒：`gpio_init` 配置为输出后的瞬时电平取决于 ODR 复位值（低），`llc_disable()` 在其后立即拉高——上电到拉高之间有数百 µs 低电平窗口，若 LLC 控制器使能脚无外部下拉/上拉约定，需在原理图确认这个窗口无害（LLC 功率级此时无供电，通常无害，记录即可）。

### 3. PFC standalone 测试门控（153-157）—— 已注释
```c
153  /* PFC standalone test owns the PFC relay and must never advance into LLC. */
154  //if (!serial_console_pfc_test_active()) {
156      power_supervisor_tick_1khz();
157  //}
```
- `PFC ON/OFF` 串口命令已从 serial_console.c 移除，`serial_console_pfc_test_active()` 已无定义，仅剩此处注释引用。
- `power_supervisor_tick_1khz()` 现在**无条件执行**——正确：PFC 板级调试已由硬件手段（交流源+示波器）完成，固件回归"统一状态机"模式。建议后续删掉 153-154 注释残留。

### 4. CAL 模式分支（245-260）—— 结构清晰
```c
245  #if PWM_ANALOG_CALIBRATION_MODE
247      cv_pwm_init(PWM_BASE_HZ, PWM_DUTY_SAFE);
248      cc_pwm_init(PWM_BASE_HZ, PWM_DUTY_SAFE);
249      bus_vol_adj_pwm_init(PWM_BASE_HZ, PWM_DUTY_SAFE);  // 仅 CAL 分支
250      serial_console_init();
253  while (1) {
255      llc_disable();                             // 每圈强制关 LLC（低有效拉高）
256-257  gpio_bit_reset(PFC_RELAY/OUT_RELAY);       // 每圈强制断双继电器
258      serial_console_task();
```
- CAL 固件三路 PWM 全开、功率三路每圈锁断——**这是 PA6 冲突下唯一合法使用 `bus_vol_adj_pwm_init` 的场景**（CAL 分支不调 `adc_multi_init_dma`，无 ADC 采样，PA6 归 TIMER2；正常分支相反，PA6 归 ADC）。互斥成立 ✓。
- 正常分支（261-353）只 `cv_pwm_init`+`cc_pwm_init`，**不调 `bus_vol_adj_pwm_init`**——见下节 B1 性质修正。

---

## 三、B1 性质修正（撤回 v1 补丁 1，重要）

v1 结论："正常固件漏 `bus_vol_adj_pwm_init`，母线参考浮空，必须补"——**此结论错误，撤回**。

事实链（已逐处核实）：
1. `main.h:134-136` 定义 `I_SENSE_PIN = GPIO_PIN_6`（ADC CH6，电流采样）；
2. `main.h:168-173` 定义 `BUS_VOL_ADJ_PIN = GPIO_PIN_6`（TIMER2_CH0，母线参考 PWM）——**同一物理引脚 PA6**；
3. `main.h:174-175` 用户已加标记宏 `CC_CV_PWM_REQUIRE_INDEPENDENT=1U` / `CC_CV_PWM_PIN_CONFLICT=1U`（自行承认冲突存在）；
4. `adc_dma.c:35-36` 把 PA6 配成 `GPIO_MODE_AIN`；`pwm_llc.c:168` 会把 PA6 配成 `GPIO_MODE_AF_PP`——**后配置者获胜，互斥**；
5. 若按 v1 补丁 1 在正常分支补 `bus_vol_adj_pwm_init`，PA6 变 AF_PP，**I_SENSE 电流采样立即失效**，OCP/电流显示/完成判据全部瘫痪——照改会毁掉电流采样。

**正确认知**：正常固件不初始化 BUS_VOL_ADJ 是有意规避；NCP1654 的母线电压参考由其外围电阻网络设定（需对照 Main wf3.pdf 原理图最终确认，MCU 不参与母线目标控制）。长期解法是硬件改版把 BUS_VOL_ADJ 换脚或彻底删掉该网络。

---

## 四、遗留问题清单（v2 状态）

| 编号 | 级别 | 位置 | 问题 | v2 状态 |
|---|---|---|---|---|
| ~~B1~~ | ~~高~~ | main.c:263-264 | ~~正常固件漏 bus_vol_adj_pwm_init~~ | ✅ **结论撤回**（有意规避 PA6 冲突，见第三节） |
| **A** | **中(P1)** | main.c:150 + pfc_control.c:69-72 | RUN 态母线 OVP 缺失：`pfc_tick` 的 `PFC_STATE_RUN` 仍空壳（仅注释"加入OVP的保护"）；`VBUS_OVP_V=420` 仅被 `pfc_is_ready()` 用作 is_ready 上限（pfc_control.c:87），**从未作为故障触发** | ❌ 仍开放（0917 版本已在 tick 顶部加 400V OVP，0824 建议移植同款：RUN 态 `bus_vol_v>=VBUS_OVP_V` → `protect_set_fault(FAULT_BUS_OVP)`） |
| ~~C~~ | ~~中~~ | main.c:285-292 | ~~自检失败无指示~~ | ✅ **已修**（protect_set_fault+状态+红灯，见第二节 1） |
| **D** | **中(P1)** | 全工程 | 无 IWDG；保护在 1kHz 任务级；CPU 异常只死循环（gd32f30x_it.c） | ❌ 仍开放（与保护评审 P0 两项同源：补 `fault_safe_off()` + `fwdgt_config/enable`） |
| **E** | **低(P2)** | main.c:334-336 | `adc_multi_copy()` 丢返回值，仍 `adc_ac_sample_fast+trigger_fast`。下溢补丁后旧值重复累加的后果已减轻（raw 值合法），但"DMA 未完成仍触发新帧"逻辑仍不严谨 | 🟡 建议改 `if (adc_multi_copy_if_ready()) { sample; trigger; }` |
| **F** | **低(P2)** | main.c:302-307 / 321-326 | `__disable_irq()` 全关清统计/取标志 | 🟡 维持建议：临界区极短可接受，严格应只屏蔽 SysTick/TIMER3 |
| **G** | **低(P3)** | main.c:154/269/272/278-279/352 | 裸注释残留（pfc_test_active、adc_multi_trigger_fast、can_comm 等） | 🟡 建议统一用宏或删除 |
| **H** | **重写** | main.c（不转移状态） | v1 表述"`PFC ON` 命令永远被拒"已失效——PFC ON/OFF 命令已移除，状态转移完全依赖 `power_supervisor_tick`（串口 START/STOP 经 supervisor API），语义闭环成立 | ✅ 语义闭环，仅剩 153-154 注释残留待清理 |
| **I** | **建议** | main.c:170-175 | 启动 ADC 自检只查上溢（`>=ADC_RESOLUTION`），未查下溢/开路（raw≈0）；且 main.h 定义的 3V3 自检参数（`ADC_STARTUP_V3V3_*`）与分压宏（`V3V3_RTOP/BOT`）全工程零引用——3V3 监测通道未进扫描序列，自检未实现 | 🟡 下溢在无偏置通道上无危害（bus 已取消偏置）；有偏置的 ac 通道已被饱和减法钳 0，同样无下溢假值。3V3 自检属预留未实现，要么实现要么删宏 |
| **J** | 🆕 低(P3) | main.c:237 | 横幅 "FW v0.2.2" 与实际迭代进度（0917 评审、多轮补丁）脱节，建议版本号递进或加日期 | 🆕 记录 |

---

## 五、与 0917 对照（v2 更新）

| 维度 | 0824 (main.c 体现) | 0917 |
|---|---|---|
| 功率使能触发 | 串口 START→supervisor API（PFC ON 已移除） | 上电自动（安全闸门后） |
| 母线 OVP | ❌ RUN 空壳，`FAULT_BUS_OVP` 未用作故障 | ✅ tick 顶部 400V 全状态覆盖 |
| 故障断继电器 | ⚠ 故障路径走 `outputs_force_off`（齐断无缓冲） | ✅ 每个故障路径先 `pfc_outputs_off()` |
| 看门狗/快关 | ❌ 无 IWDG，保护在任务级 | ✅ EXTI + BKIN 刹车自检 |
| 状态机转移 | main.c 不转移，依赖 supervisor | main.c 内 pfc 状态机自洽 |
| AC 采样 | 5kHz 软件触发 + 200ms RMS（窗口随主循环漂移） | 10kHz ADC + 200ms 整周期 RMS（NSI1311 链路） |

---

## 六、结论（v2）

`main.c` 的**调度骨架与 GPIO 安全默认依然稳健**；v1 的 B1/C 两大项已经Closure：C 项用户已修（自检失败可见），B1 经 PA6 冲突核实后撤回（"缺失"实为"有意规避"）。

剩余按优先级：
1. **A（母线 OVP）**——从 0917 移植 tick 顶部 OVP 判据，工作量小、收益大；
2. **D（IWDG + CPU 异常关功率）**——无人值守前必须补；
3. E/F/G/H/I/J 按版本节奏清理。

> 落实状态：本报告为评审文档，**未改动源码**。v1 补丁 1 作废；补丁 2（母线 OVP）与补丁 4（copy 返回值）代码建议仍有效，见上文对应条目。
