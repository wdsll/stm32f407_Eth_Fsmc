# main.c 代码评审（逐行注释）

> 评审对象：`0824/App/src/main.c`（v0.2.2，354 行）
> 评审日期：2026-08-27
> 上下文：1.5kW OBC，GD32F303RE；模拟 PFC(NCP1654)+模拟 LLC(NCP4390)，MCU 只做监控/保护/时序/通信。
> 配套已评审：`pfc_control.c`、`protect.c`、`serial_console.c`、`adc_dma.c`、`pwm_llc.c`

---

## 一、文件结构概览

| 区段 | 行号 | 作用 |
|---|---|---|
| 头文件 | 14-23 | 依赖声明 |
| 宏 / 内部变量 | 27-52 | ADC 采样率、全局时间 `g_ms`、tick pending 标志 |
| delay / SysTick | 56-92 | 1ms 节拍中断，只置 pending 标志 |
| ADC 监控定时器 | 94-139 | TIMER3 产生 2~5kHz 采样节拍，只置 pending 标志 |
| 1kHz 监控任务 | 141-157 | 串起 ADC 采样 / 保护 / PFC 状态机 / 功率时序 |
| 启动自检 | 159-189 | ADC 饱和检查 + 保护自检 |
| GPIO 初始化 | 191-223 | 所有功率/指示灯引脚初始为复位（关） |
| main() | 227-353 | 时钟→串口→GPIO→SysTick→PWM/ADC→CAN(注释)→自检→init→主循环 |

整体架构清晰：**中断只置「最新任务」pending 标志、不补跑、主循环每圈最多各执行一次**，丢节拍只计数不阻塞。这个调度骨架是合理的，下面挑出真实问题。

---

## 二、逐段注释（关键行）

### 1. 时间基准与 pending 标志（41-52）
```
41  volatile uint32_t g_ms = 0U;          // SysTick 累加，全局时间基准，OK
42  charger_state_t g_charger_state = MAIN_STEP_INIT;  // 初始 INIT
43  fault_type_t    g_fault = FAULT_NONE;
49  static volatile uint32_t s_control_tick_pending = 0U;     // 1kHz 任务标志（只存 0/1）
50  static volatile uint32_t s_adc_monitor_tick_pending = 0U;// 5kHz 采样标志
51  static volatile uint32_t s_tick_drop_count = 0U;          // 1kHz 丢节拍计数
52  static volatile uint32_t s_adc_monitor_tick_drop_count = 0U;
```
注释 44-48 已说明设计意图（「待执行标志，不累计历史」）。**设计正确**：中断里 `if(pending==0) pending=1; else drop++`，避免主循环卡死时背靠背补跑导致节拍雪崩。✓

### 2. SysTick 1ms（66-92）
```
83  void SysTick_Handler(void){
85      g_ms++;
87      if (s_control_tick_pending == 0U) s_control_tick_pending = 1U;
89      else s_tick_drop_count++;
```
轻量、正确。注意 SysTick 和 TIMER3 都在中断里只置标志，真正的重活在任务级，所以中断本身极短，不会阻塞其他中断。✓

### 3. ADC 监控定时器 TIMER3（94-139）
```
107     uint32_t period = tclk / ADC_MONITOR_SAMPLE_HZ;  // 5kHz→period= tclk/5000
108     if (period == 0U) period = 1U;
110     period -= 1U;
125  void TIMER3_IRQHandler(void){
133      if (s_adc_monitor_tick_pending == 0U) s_adc_monitor_tick_pending = 1U;
135      else s_adc_monitor_tick_drop_count++;
```
- `period==0` 保护合理（低频时钟下不至于，但防御性 OK）。
- 与 SysTick 同一套「只存最新」逻辑，一致。✓

### 4. 1kHz 监控任务（141-157）★ 缺口 A 的根因在此调用链
```
144     adc_multi_sample_aux_1khz();   // raw→电压换算（不触发 ADC，OK）
147     protect_tick_1khz();           // 软件保护
150     pfc_tick_1khz();               // PFC 状态机（RUN 态空壳，见下）
153     if (!serial_console_pfc_test_active()) {
155         power_supervisor_tick_1khz();  // 正常充电时序；PFC 测试中跳过
```
- `adc_multi_sample_aux_1khz` 仅做换算、不重新触发 ADC，**与 TIMER3 主循环里的 trigger 不冲突**。✓
- `pfc_tick_1khz` 的 `PFC_STATE_RUN` 体是空壳（仅注释「加入OVP」）——母线过压/欠压监控在此缺失。
- `protect_tick_1khz` 只查 vout OVP / iout OCP / LLC_FAULT_CHECK，**从不查 `bus_vol_v`**，所以 `FAULT_BUS_OVP`（枚举已定义）**全工程从未被 set**，`VBUS_OVP_V=420` 宏定义了却零使用。→ **缺口 A**。

### 5. 启动自检（159-189）★ 状态指示缺陷
```
159  static bool adc_startup_check(void){
170      if (g_adc_multi.ac_vol_raw  >= ADC_RESOLUTION) return false;  // 满量程=异常
171      if (g_adc_multi.bus_vol_raw >= ADC_RESOLUTION) return false;
172      if (g_adc_multi.vout_raw    >= ADC_RESOLUTION) return false;
173      if (g_adc_multi.isense_raw  >= ADC_RESOLUTION) return false;
174      if (g_adc_multi.vbt_raw     >= ADC_RESOLUTION) return false;
```
- 饱和判据合理（前端短路到 VREF 会满量程）。
- **不足**：只查上溢，未查下溢/开路（raw≈0 可能引脚断开）。次要。
```
178  static bool protect_startup_check(void){
180      if (protect_fault_active_hw()) return false;   // 硬件故障→失败
184      if (protect_fault_latched()) protect_clear_fault();  // 锁存→先清
188      return !protect_fault_active_hw();
```
逻辑正确：硬件故障持续则启动失败；软锁存可清。✓

### 6. GPIO 初始化（191-223）★ 全部初始复位（关），这是正确的安全默认
```
196     gpio_bit_reset(PFC_RELAY_PORT, PFC_RELAY_PIN);   // 继电器初始断开
201     gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);         // LLC 初始关
206     gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);   // 输出继电器初始关
211     gpio_bit_reset(FAN_CTL_PORT, FAN_CTL_PIN);       // 风扇初始关
216     gpio_bit_reset(HARD_FAULT_CLR_PORT, HARD_FAULT_CLR_PIN);
221     gpio_bit_set(LED_RED_PORT, LED_RED_PIN);         // 红灯亮=待机
```
- 所有功率引脚初始为**关**，上电即安全态。✓（这正回答了之前「上电会不会自动预充」：不会，继电器初始断开）
- LED 红/绿同在 GPIOC（已核对 main.h 235/239），`gpio_init(LED_RED_PORT,...,RED_PIN|GREEN_PIN)` 合法。✓ 无端口 bug。

### 7. main() 主体（227-353）★ 见问题清单

**PWM 参考初始化（243-262）—— ★ B1 真实缺陷**
```
243  #if PWM_ANALOG_CALIBRATION_MODE          // =0U（见 main.h:185），编译走 #else
244      cv_pwm_init(PWM_BASE_HZ, PWM_DUTY_SAFE);
245      cc_pwm_init(PWM_BASE_HZ, PWM_DUTY_SAFE);
247      bus_vol_adj_pwm_init(PWM_BASE_HZ, PWM_DUTY_SAFE);  // ← 仅在 CAL 分支
...
259  #else
261      cv_pwm_init(PWM_BASE_HZ, CV_PWM_DUTY_INIT);
262      cc_pwm_init(PWM_BASE_HZ, CC_PWM_DUTY_INIT);
         // ★★★ 正常固件漏了 bus_vol_adj_pwm_init ★★★
```
- `PWM_ANALOG_CALIBRATION_MODE` 在 `main.h:185` 定义为 `(0U)`，所以**实际编译的是 #else 分支**。
- 因此 `bus_vol_adj_pwm_init`（给 NCP1654 设定**母线电压参考**的 PWM 通道）**在正常固件里从未被调用** → 该 GPIO 未配为 AF、定时器通道未初始化 → 母线参考引脚浮空/不确定 → 母线电压目标不可控。
- CAL 分支(=1)反而有它，这就是为什么「校准版本能跑、正常版本出怪问题」。
- **必须修**（详见第四节补丁）。

**CAN 集成（276-277, 349-350）—— 初版可接受但需标注**
```
276  /* CAN is intentionally left uninitialized during first bench integration. */
277  //can_comm_init(CAN_BAUDRATE);
349  //can_comm_poll();
```
- 初版 bench 联调注释掉 CAN 合理。**风险**：若日后只把 `can_comm_poll()` 打开而忘记 `can_comm_init()`，会空轮询。建议用宏统一开关，而非裸注释。

**启动自检失败处理（283-290）—— ★ 状态指示缺陷（P1）**
```
283  if (!protect_ok || !adc_ok) {
284      debug_printf("[STARTUP] Preflight FAIL: protect=%d adc=%d\r\n", ...);
285      protect_set_fault(FAULT_HARDWARE_PRO);
286      // g_charger_state = MAIN_STEP_FAULT;   // ← 被注释
287      // g_fault = FAULT_HARDWARE_PRO;        // ← 被注释
288      // gpio_bit_set(LED_RED_PORT, LED_RED_PIN);  // ← 被注释
289      while (1) { __NOP(); }   // 安全停机
```
- **安全性 OK**：此时功率输出全是复位态（pfc_init 在 294 行才调用），死循环不会带功率。
- **可维护性差**：注释掉 `g_fault`/`g_charger_state`/红灯后，外部（串口 STATUS、CAN）完全看不到「自检失败」状态，只会「板子卡死无响应」。调试时极难判断是硬件坏了还是固件卡在 preflight。
- **建议**：至少保留 `g_fault = FAULT_HARDWARE_PRO;` 和红灯点亮，让故障可见。

**主循环调度（300-351）—— 临界区写法与 ADC 帧同步**
```
300  __disable_irq();
301  s_control_tick_pending = 0U; ... s_tick_drop_count = 0U;
305  __enable_irq();
```
- 用 `__disable_irq()` 关**全部**中断来清统计，临界区极短（几条赋值）尚可接受，但严格说应只屏蔽 SysTick/TIMER3，避免扩大关中断窗口。小问题。

```
312  while (1) {
319      __disable_irq();                          // 原子取走 pending
320      run_control_tick = s_control_tick_pending; s_control_tick_pending = 0U;
322      run_adc_monitor_tick = s_adc_monitor_tick_pending; s_adc_monitor_tick_pending = 0U;
324      __enable_irq();
326      if(run_adc_monitor_tick != 0){
332          adc_multi_copy();                     // ← 丢弃是否成功
333          adc_ac_sample_fast(g_adc_multi.ac_vol_raw);  // 累加 AC RMS
334          adc_multi_trigger_fast();             // 触发下一帧
```
- **潜在健壮性缺陷（P2）**：`adc_multi_copy()` 内部 `adc_multi_copy_if_ready()` 若 DMA 未完成（FTF 未置）会直接返回、**不更新 `g_adc_multi`**。但下一行 `adc_ac_sample_fast` 仍用旧 `ac_vol_raw` 累加、且 `trigger_fast` 又启动新帧。
  - 在当前参数下（ADC 帧 ~22.7µs ≪ TIMER3 200µs 周期）主循环每圈必已等 FT F 置位，实际**不会**重复累加，所以只是理论风险。
  - 但代码逻辑不严谨：**copy 成功才应 sample+trigger**。建议改为用 `adc_multi_copy_if_ready()` 的返回值判断，或把 sample/trigger 包进 `if(adc_multi_copy_if_ready())`。

```
337      if (run_control_tick != 0U) {
338          monitor_protect_tick_1khz();
```
- 1kHz 任务在任务级执行，意味着**硬件故障（LLC_FAULT_CHECK 轮询）最坏响应 = 1ms tick + 主循环调度延迟**；且全工程**无独立硬件看门狗（IWDG）**，CPU 跑飞无法自恢复。
- 这是贯穿多轮的**系统级 P0**：`main.c` 体现了「异常只进死循环/无 IWDG/保护在任务级」的架构，建议后续补 IWDG + 关键故障 EXTI/BKIN 快关。

---

## 三、问题清单（按严重度）

| 编号 | 级别 | 位置 | 问题 | 影响 |
|---|---|---|---|---|
| **B1** | **高(P0/P1)** | main.c:261-262 | 正常固件漏 `bus_vol_adj_pwm_init`，母线参考 PWM 未初始化 | 母线电压目标不可控，NCP1654 参考浮空 |
| **A** | **中(P1)** | main.c:147/150 + protect.c | RUN 态母线 OVP 缺失，`FAULT_BUS_OVP` 永不触发，`VBUS_OVP_V=420` 零使用 | 母线过压仅靠板级模拟 OVP，缺固件最后一道闸 |
| **C** | **中(P1)** | main.c:283-290 | 启动自检失败死循环，但注释掉 `g_fault`/状态/红灯 | 故障不可见，调试难判读（功率关，安全） |
| **D** | **中(P1)** | 全工程 | 无 IWDG；保护在 1kHz 任务级；CPU 异常只死循环 | 跑飞不自恢复，硬件故障最坏 ~1ms 响应 |
| **E** | **低(P2)** | main.c:332-334 | `adc_multi_copy()` 丢弃成功与否，仍 sample+trigger | 参数安全但逻辑不严谨，建议改判返回值 |
| **F** | **低(P2)** | main.c:300-324 | `__disable_irq()` 关全部中断清统计/取标志 | 临界区扩大风险，建议收窄 |
| **G** | **低(P3)** | main.c:267/270/277/349 | 多处裸注释残留代码 | 建议用宏统一开关 |
| **H** | **依赖** | main.c（自身不转移状态） | `g_charger_state` 从 INIT→STANDBY 依赖 `power_supervisor_tick` | 若该模块未推送 STANDBY，`PFC ON` 命令永远被拒 |
| **I** | **建议** | main.c:170-174 | 启动 ADC 自检只查上溢，未查下溢/开路 | 引脚断开可能漏检 |

---

## 四、关键补丁建议

### 补丁 1（修复 B1，必须做）
在 `main.c` 的 `#else` 分支补上 `bus_vol_adj_pwm_init`：
```c
#else
    /* 5. PWM-to-DC references. Analog ICs close the CC/CV loops. */
    cv_pwm_init(PWM_BASE_HZ, CV_PWM_DUTY_INIT);
    cc_pwm_init(PWM_BASE_HZ, CC_PWM_DUTY_INIT);
    /* ★ 修复 B1：正常固件也必须初始化母线参考 PWM，否则 NCP1654 目标电压浮空 */
    bus_vol_adj_pwm_init(PWM_BASE_HZ, BUS_VOL_ADJ_PWM_DUTY_INIT);  // 初值建议 PWM_DUTY_SAFE(0)
```
> 注：还需确认 `pfc_control.c` 在 RUN 态是否真正**设定** `bus_vol_adj` 目标占空比（之前评审发现 `PFC_STATE_RUN` 是空壳）。若只初始化不设定、或根本不驱动该 PWM，则母线仍无受控目标——这是更深一层待办，需结合 PFC 原理图/芯片手册确认母线参考来源。

### 补丁 2（修复 A）
在 `pfc_tick_1khz` 的 `PFC_STATE_RUN` 体（pfc_control.c）顶部加：
```c
if (g_adc_multi.bus_vol_v >= VBUS_OVP_V) {
    protect_set_fault(FAULT_BUS_OVP);
    pfc_outputs_off();          // 先断继电器再进故障
    s_pfc_state = PFC_STATE_FAULT;
}
```
（main.c 侧无需改，根因在 pfc_tick；此处列出以便闭环）

### 补丁 3（修复 C，最小改动）
```c
if (!protect_ok || !adc_ok) {
    debug_printf("[STARTUP] Preflight FAIL: protect=%d adc=%d\r\n", protect_ok, adc_ok);
    g_fault = FAULT_HARDWARE_PRO;                 // 让故障可见
    gpio_bit_set(LED_RED_PORT, LED_RED_PIN);     // 红灯指示
    while (1) { __NOP(); }
}
```

### 补丁 4（修复 E，健壮性）
```c
if (run_adc_monitor_tick != 0) {
    if (adc_multi_copy_if_ready()) {              // 仅当 DMA 整帧完成才处理
        adc_ac_sample_fast(g_adc_multi.ac_vol_raw);
        adc_multi_trigger_fast();
    }
}
```

---

## 五、与 0917 对照（PFC 主线差异）

| 维度 | 0824 (main.c 体现) | 0917 |
|---|---|---|
| 功率使能触发 | 手动串口 `PFC ON` | 上电自动（安全闸门后） |
| 母线 OVP | ❌ RUN 空壳，`FAULT_BUS_OVP` 零使用 | ✅ tick 顶部 400V 全状态覆盖 |
| 故障断继电器 | ⚠️ standalone 不自断 | ✅ 每个故障路径先 `pfc_outputs_off()` |
| 看门狗/快关 | ❌ 无 IWDG，保护在任务级 | ✅ EXTI + BKIN 刹车自检 |
| 状态机转移 | main.c 不转移，依赖 `power_supervisor_tick` | main.c 内 pfc 状态机自洽 |

---

## 六、结论

`main.c` 的**调度骨架（中断只置 pending、不补跑、主循环每圈各执行一次）是合理且稳健的**，GPIO 安全默认也正确。但有两类必须处理的问题：

1. **功能缺陷 B1（高）**：正常固件漏初始化 `bus_vol_adj_pwm_init`，直接动摇「母线电压受控」这一前提，需立即补。
2. **安全/可维护性缺口 A/C/D（中）**：母线 OVP 未落地、自检失败无指示、无独立看门狗——这些在实验室有人盯守时可用，但不应作为长期/无人值守方案。

建议优先级：B1 → A → C/D（看门狗）→ E/F（健壮性）。

> 落实状态：本报告为评审文档，**未改动源码**。B1 补丁可直接套用第四节代码。
