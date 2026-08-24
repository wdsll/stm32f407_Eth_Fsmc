# llc_control.c 专项评审

> 文件: `HW/src/llc_control.c` (243 行) + `HW/include/llc_control.h`  
> 评审日期: 2026-08-12  
> 范围: 仅 `llc_control` 模块（充电功率时序状态机）

---

## 〇、模块定位

本文件是充电状态机核心：STANDBY → PRECHARGE → CC → CV → FINISHED，外加 FAULT（在进入 switch 前处理）。MCU 通过 `CV_PWM`(PA8) / `CC_PWM`(PA0) 给模拟 LLC IC 设定电压/电流基准，模拟 IC 内部完成闭环。

完整状态转移:

```
STANDBY ──enable & vbat≥10V──► PRECHARGE ──pfc_is_ready──► CC ──vbat≈vref──► CV ──iout≤1A/30s──► FINISHED
   ▲                                  │                       │                        │              │
   └────────── !enable / default ─────┘  └──!enable──────────┘  └──!enable──────────┘  └──!enable────┘
   FAULT (任意故障) ── outputs_off + 红灯, 无恢复 ──► (永久锁死)
```

---

## 一、严重问题 (Critical)

### 🔴 A1 — PRECHARGE 状态无超时保护（会永久卡死）

| 项  | 内容                                       |
| -- | ---------------------------------------- |
| 位置 | `llc_control.c:161-172` (PRECHARGE case) |

PRECHARGE 只有两个分支：`!s_enable_requested → STANDBY` 和 `pfc_is_ready() → CC`。**没有超时判据。**

- 若 AC 输入异常 / PFC 硬件故障导致母线永远建不到 `LLC_ENTRY_V`(360V)，`pfc_is_ready()` 永远为 false。
- 后果: 状态机**永久卡在 PRECHARGE**——不进 CC、不回 STANDBY、不报故障、输出继电器一直断开。系统表现为"使能了但没反应，也不报错"，极难排查。

**修复建议:** 增加 PRECHARGE 超时（如 2~3s 内 `pfc_is_ready()` 否则 `protect_set_fault(FAULT_BUS_UVP)` 或新增 `FAULT_PFC_TIMEOUT`）。可复用 `s_state_started_ms`（PRECHARGE 由 `enter_state` 记录了进入时刻）。

```c
case MAIN_STEP_PRECHARGE:
    if (!s_enable_requested) {
        outputs_off();
        enter_state(MAIN_STEP_STANDBY);
    } else if (pfc_is_ready()) {
        apply_references();
        gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);
        enter_state(MAIN_STEP_CC);
    } else if (elapsed_reached(s_state_started_ms, PFC_READY_TIMEOUT_MS)) {
        protect_set_fault(FAULT_PFC_TIMEOUT);   // 新增或复用 BUS_UVP
    }
    break;
```

---

### 🔴 A2 — CV 状态不切换 PWM 基准，文件头注释与实现矛盾

| 项  | 内容                                              |
| -- | ----------------------------------------------- |
| 位置 | 头注释 `llc_control.c:6-8` vs 实现 `:167` `:200-225` |

文件头明确写：

```
* CC 模式: CC_PWM=目标电流, CV_PWM=电压上限
* CV 模式: CV_PWM=目标电压, CC_PWM=电流上限
```

但代码里：

- `apply_references()` 仅在 **PRECHARGE→CC 转移时调用一次**（`:167`）
- CC 状态和 CV 状态**都使用这一组固定设定点**（来自 CAN 命令的 `s_voltage_reference_v` / `s_current_reference_a`）
- CV 状态（`:200-225`）内**没有任何 `apply_references()` / 重设基准**的逻辑，只做"闭继电器 + 判完成"

即：MCU 的 CC→CV 切换**不改变任何硬件基准**，纯粹是状态指示。真正 CC/CV 切换由模拟 IC 双环（电压环+电流环谁饱和谁主导）自动完成。

- **风险1（误导）**: 头注释描述的设计（CC/CV 用不同 PWM 组合）与实现不符，维护者按注释改会出错。
- **风险2（功能）**: 如果设计意图确实是"CV 模式把 CV_PWM 设到目标电压、CC_PWM 设到电流上限"以收紧限流，那这部分功能**缺失**，依赖模拟 IC 默认行为。

**修复建议:** 二选一，并修正注释保持一致：

1. 若确认由模拟 IC 自动 CC/CV（推荐，符合设计意图）：**删除头注释中 CC/CV 模式 PWM 切换的描述**，改为"MCU 仅下发固定 setpoint，CC/CV 由模拟 IC 决定"。
2. 若确需 MCU 切换基准：在 `enter_state(MAIN_STEP_CV)` 处按 CV 模式重算并 `apply_references()`。

---

## 二、主要问题 (Major)

### 🟠 B1 — 安全断电序列缺失（继电器带载分断风险）

| 项  | 内容                                              |
| -- | ----------------------------------------------- |
| 位置 | `llc_control.c:63-68` (`outputs_off`)；所有状态转移调用点 |

`outputs_off()` 在同一函数内同步复位三路：

```c
gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);   // 输出继电器
gpio_bit_reset(LLC_EN_PORT,   LLC_EN_PIN);        // LLC 使能
pfc_disable();                                     // PFC 继电器
```

- 没有"先关 LLC_EN → 等待母线电容放电 → 再关 PFC/继电器"的时序。
- 对 LLC 拓扑，突然同时断开 LLC_EN 和输出继电器，母线电容能量无处释放，输出继电器**可能带载分断（拉弧）**，影响继电器寿命和安全性。
- 参考项目 LIMA48V 有专门的三步断电状态机（关负载 → 延时 300ms → 关 SCR）。

**修复建议:** 将 `outputs_off()` 改为带时序的关断（可加一个 `off_sequence_t` 子状态机，或在保护/PRECHARGE 序列里先拉低 LLC_EN、延时、再断继电器）。至少保证：关 LLC_EN 后留几 ms~几十 ms 让电流自然衰减，再断 OUT_RELAY，最后断 PFC_RELAY。

---

### 🟠 B2 — FAULT 后无法恢复 + CAN 命令处理在 FAULT 检查之前

| 项  | 内容                                                        |
| -- | --------------------------------------------------------- |
| 位置 | `llc_control.c:128-133` (CAN 处理) vs `:141-146` (FAULT 检查) |

- `power_supervisor_tick_1khz()` 先处理 CAN 命令（`:128-133`，会调 `apply_references()` 改 PWM），**之后**才在 `:141` 检查 FAULT 并 `return`。
- 一旦进入 `MAIN_STEP_FAULT`，`:141` 直接 `outputs_off()` + return，**永不处理 CAN 命令**（`can_comm_get_cmd` 不再被调用）。
- 后果: 即使 BMS 重新下发 `enable=0` 或专用清故障命令，也无法清除锁存 → **永久锁死，必须断电复位**（与全局评审 M1 一致，此处是本文件的具体表现）。

**修复建议:**

1. 在 FAULT 分支增加恢复条件，例如 `s_rx_cmd.enable==0` 或收到清故障命令时调用 `protect_clear_fault()` 并安全回到 STANDBY（需先满足 B1 的时序断电）。
2. 明确"CAN 命令处理"在 FAULT 态是否允许：至少应允许接收"清故障/停止"命令，否则连远程恢复都不可能。

---

### 🟠 B3 — CC 充电超时误用 8s（调试值残留）

| 项  | 内容                                                              |
| -- | --------------------------------------------------------------- |
| 位置 | `llc_control.c:180` + `main.h:100-106` (`CHARGE_CC_TIMEOUT_MS`) |

`CHARGE_CC_TIMEOUT_MS` 因为 `#if test_mode` 未定义，实际取值 **8000ms（8 秒）**。正常充电 8 秒即报 `FAULT_CHARGE_TIMEOUT`。

- 这是**阻断级**问题（全局评审 C2），在 llc_control 里表现为 `:180` 这一行必然在 8 秒后触发故障。
- 同文件 `:207` 的 CV 超时（3000ms）同理。

**修复建议:** 见全局评审 C2——把量产值改为 8h/3h，不要依赖未定义的 `test_mode` 宏。

---

## 三、次要问题 (Minor)

### 🟡 C1 — 完成判据 `iout_a >= 0` 恒真冗余

| 项  | 内容                      |
| -- | ----------------------- |
| 位置 | `llc_control.c:217-218` |

```c
&& g_adc_multi.iout_a >= 0 && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A
```

`adc_raw_to_current()`（`adc_dma.c:187`）末尾 `(i < 0.0f) ? 0.0f : i` 已把负值钳到 0，所以 `iout_a >= 0` **永远为真**，纯冗余，易误导读者以为有负电流判定。

**修复建议:** 删除 `&& iout_a >= 0`，只保留 `iout_a <= CHARGE_FINISH_CURRENT_A`。

---

### 🟡 C2 — `LLC_START_TIMEOUT_MS` 命名/语义不符

| 项  | 内容                                    |
| -- | ------------------------------------- |
| 位置 | `llc_control.c:27` 定义 + `:194-196` 使用 |

该宏名是"LLC 启动超时"，但实际判据是"进入 CC 后 1.5s 内 `vout_v > OUTPUT_RELAY_MIN_V`(10V) 是否成立"，且报的是 **`FAULT_BUS_UVP`（母线欠压）**——而硬件查的是 `vout`（输出），不是母线。

- 名实不符：测的是输出建起，报的是母线欠压。
- 该超时**只覆盖 CC 状态**（进入 CC 后 1.5s）；PRECHARGE 状态无等效保护（见 A1），形成保护盲区。

**修复建议:** 重命名为 `CC_OUTPUT_BUILD_TIMEOUT_MS`，报错改用更准确的故障码（如新增 `FAULT_LLC_START` 或 `FAULT_OUTPUT_UVP`）；PRECHARGE 超时单独处理（见 A1）。

---

### 🟡 C3 — STANDBY 每 tick 先 `outputs_off()` 再可能 `pfc_enable()`

| 项  | 内容                            |
| -- | ----------------------------- |
| 位置 | `llc_control.c:151` vs `:156` |

STANDBY case 顶端无条件 `outputs_off()`（其中包含 `pfc_disable()`），随后若 `s_enable_requested` 为真则 `pfc_enable()` → `enter_state(PRECHARGE)`。

- 在状态切换的**那一个 tick** 内，PFC 继电器经历 off→on。仅发生一次（之后就不在 STANDBY），无重复抖动，风险低。
- 但逻辑不优雅，且若将来 STANDBY 每 tick 都满足条件（理论上不会），会继电器抖振。

**修复建议:** 把 `pfc_enable()` 移到进入 PRECHARGE 的瞬间一次性执行，STANDBY 主体只做 `outputs_off()` + LED 指示，不在同一分支内反复开关 PFC。

---

### 🟡 C4 — CV 进入判据用 `vbat` 而非 `vout`

| 项  | 内容                  |
| -- | ------------------- |
| 位置 | `llc_control.c:184` |

```c
condition_held(s_voltage_reference_v > BATTERY_PRESENT_V
    && g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_CV_ENTRY_MARGIN_V), ...)
```

进入 CV 的判据用的是 `vbat`（电池电压）。继电器未闭合时 `vbat ≈ vout`，闭合后 `vbat = vout - 线损`（近似相等），可接受。但严格说应比较 `vout`（充电端口电压），否则长线损时判据偏差。

**修复建议:** 改用 `g_adc_multi.vout_v`（输出端口采样）做 CV 进入判据，与 C2 的 `FAULT_BUS_UVP` 错用同理，统一用输出侧量。

---

### 🟡 C5 — `outputs_off()` 不复位 CV/CC PWM 占空比

| 项  | 内容                    |
| -- | --------------------- |
| 位置 | `llc_control.c:63-68` |

`outputs_off()` 关了三路功率输出，但 **`CV_PWM` / `CC_PWM` 占空比保持上次值**。`power_supervisor_init()` 里设了 INIT 值（5%/2%），但运行期 `apply_references()` 改过之后，故障/待机时不会清零。

- 因 `LLC_EN` 已拉低，PWM 不影响输出，当前无害。
- 但从 FAULT 恢复或重新充电时：若某路径进入 PRECHARGE→CC 但未调 `apply_references()`（正常会调），可能带着旧占空比上电，有浪涌风险。

**修复建议:** 在 `outputs_off()` 末尾把 `cv_pwm_set_duty(CV_PWM_DUTY_INIT)` / `cc_pwm_set_duty(CC_PWM_DUTY_INIT)` 复位，确保重新充电从安全占空比起步。

---

### 🟡 C6 — 未实现状态靠 default 兜底，TRICKLE 逻辑缺失

| 项  | 内容                                |
| -- | --------------------------------- |
| 位置 | `llc_control.c:237-240` (default) |

`MAIN_STEP_INIT` / `MAIN_STEP_TRICKLE` / `MAIN_STEP_INVALID` 在 switch 中**没有对应 case**，全部落入 `default: outputs_off() + enter_state(STANDBY)`。

- `TRICKLE` 枚举定义了但状态机无实现，若被外部置位会直接回待机，丢失涓流逻辑（与全局评审一致，TRICKLE 本就空壳）。
- `INIT` 由 main.c 自检后直接进 STANDBY，不会进 tick，OK。
- `INVALID` 兜底回 STANDBY 可接受。

**修复建议:** 若暂不实现 TRICKLE，建议在枚举或注释明确标记"未实现"，避免维护者误以为有涓流；或显式 `case MAIN_STEP_TRICKLE: /* TODO */ break;`。

---

## 四、亮点（做对的地方）

1. **`condition_held()` 去抖封装干净** (`:70-84`)：首次满足记录时间戳、中途失败重置、持续 `duration_ms` 才返回真。CV 进入（1s）和完成（30s）判据都复用，避免误触发，思路正确。
2. **`enter_state()` 集中重置** (`:86-92`)：状态计时 `s_state_started_ms` 与去抖标志 `s_qualification_active` 一并重置，避免跨状态残留。
3. **FAULT 前即时断电 + LED 指示完整** (`:141-146`)：FAULT 态红亮绿灭 + `outputs_off()`，符合 fail-safe。
4. **引用清晰**：`g_charcger_state`、`g_adc_multi` 通过头文件 extern，状态机与外部模块解耦良好。
5. **`MAIN_STEP_CC` 注释 "Legacy CAN state value: running"** (`:169`)：诚实标注了 CC 本质是"运行态"，不掩盖状态机名义化的事实。

---

## 五、本文件问题清单速查

| 编号 | 严重级   | 描述                                 | 位置                           |
| -- | ----- | ---------------------------------- | ---------------------------- |
| A1 | 🔴 严重 | PRECHARGE 无超时，PFC 不 ready 永久卡死     | `:161-172`                   |
| A2 | 🔴 严重 | CV 状态不切基准，头注释与实现矛盾                 | `:6-8` / `:167` / `:200-225` |
| B1 | 🟠 主要 | 安全断电无时序，继电器带载分断风险                  | `:63-68`                     |
| B2 | 🟠 主要 | FAULT 后无法恢复，CAN 命令处理在其前            | `:128-133` / `:141-146`      |
| B3 | 🟠 主要 | CC 超时误用 8s（调试值）                    | `:180` + `main.h:104`        |
| C1 | 🟡 次要 | 完成判据 `iout_a>=0` 恒真冗余              | `:217-218`                   |
| C2 | 🟡 次要 | `LLC_START_TIMEOUT_MS` 名实不符，仅覆盖 CC | `:27` / `:194-196`           |
| C3 | 🟡 次要 | STANDBY 同 tick PFC off→on          | `:151` / `:156`              |
| C4 | 🟡 次要 | CV 进入判据应用 vout 非 vbat              | `:184`                       |
| C5 | 🟡 次要 | outputs_off 不复位 PWM 占空比            | `:63-68`                     |
| C6 | 🟡 次要 | TRICKLE/INVALID 靠 default 兜底       | `:237-240`                   |

---

## 六、建议修改优先级

```
1. A1  PRECHARGE 加超时          —— 防永久卡死（最优先，纯逻辑加几行）
2. B3  充电超时改 8h/3h          —— 否则 8 秒锁死（全局 C2）
3. B2  FAULT 恢复路径            —— 否则只能断电复位
4. A2  统一 CV 基准切换的注释/实现 —— 消除设计歧义
5. B1  安全断电时序              —— 继电器寿命/安全
6. C1~C6 清理与边界修正          —— 量产前
```

---

## 附：llc_control.c 逐行中文注释

> 以下为源码逐行解读（仅注释，不改源码）。行号对应原文件。

```c
   1  /*****************************************************************/   // 文件头横幅：开始
   2  * 模块名称：llc_control.c
   3  * 摘    要：                                          // 摘要为空，建议补一句
   4  * 作    者：Rengar
   5  * 内    容：LLC 模拟控制状态机 (CV_PWM + CC_PWM 双基准)
   6  *           MCU 仅设定 CV_PWM(电压基准) + CC_PWM(电流基准), 模拟IC 内部闭环
   7  *           CC 模式: CC_PWM=目标电流, CV_PWM=电压上限
   8  *           CV 模式: CV_PWM=目标电压, CC_PWM=电流上限
      // ⚠ A2：此注释与实现矛盾——代码里 CV 状态并没切换基准，见 A2
   9  * 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
  10  *         
  11  *****************************************************************/   // 文件头横幅：结束

  13  /***************** 包含头文件 *****************/            // 段横幅
  16  #include "llc_control.h"      // 本模块接口/枚举/宏
  17  #include "adc_dma.h"          // g_adc_multi（电压/电流采样值）
  18  #include "can_comm.h"         // CAN 命令接收、状态发送
  19  #include "pfc_control.h"      // pfc_enable()/pfc_disable()/pfc_is_ready()
  20  #include "protect.h"          // protect_set_fault() 故障锁存
  21  #include "pwm_llc.h"          // cv_pwm_set_duty()/cc_pwm_set_duty() 基准输出

  25  #define BATTERY_PRESENT_V       (10.0f)   // 判定"电池已接入"的最低电压
  26  #define OUTPUT_RELAY_MIN_V      (10.0f)   // CC 态判定输出已建起的电压门限
  27  #define LLC_START_TIMEOUT_MS    (1500U)   // ⚠ C2：名实不符，实际用于"CC 后 1.5s 输出未建起→报 BUS_UVP"
  28  #define CAN_STATUS_PERIOD_MS    (100U)    // CAN 状态帧上报周期 100ms（10Hz）
  29  #define CV_REFERENCE_MAX_V      (80.0f)   // CV_PWM 占空比换算满量程电压（344/80V）
  30  #define CC_REFERENCE_MAX_A      (20.0f)   // CC_PWM 占空比换算满量程电流（⚠ 全局 C3：满量程≈24.8A，OCP=30A 超量程）

  34  static bool s_enable_requested;     // 充电使能请求（来自 CAN）
  35  static float s_voltage_reference_v; // 当前电压基准(V)，来自 CAN 命令
  36  static float s_current_reference_a; // 当前电流基准(A)，来自 CAN 命令
  37  static uint32_t s_state_started_ms;      // 进入当前状态的时间戳（状态内超时用）
  38  static uint32_t s_last_status_ms;       // 上次 CAN 状态上报时间戳
  39  static uint32_t s_qualification_started_ms; // 去抖计时起点（condition_held 用）
  40  static bool s_qualification_active;        // 去抖是否已激活（首次满足标记）

  44  static float voltage_to_duty(float voltage_v)
  45  {
  46      return f_clampf(voltage_v / CV_REFERENCE_MAX_V,     // 电压→占空比线性映射
  47                      CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX);  // 钳在 PWM 合法区间
  48  }

  50  static float current_to_duty(float current_a)
  51  {
  52      return f_clampf(current_a / CC_REFERENCE_MAX_A,     // 电流→占空比线性映射
  53                      CC_PWM_DUTY_MIN, CC_PWM_DUTY_MAX);
  54  }

  56  static void apply_references(void)
  57  {
  58      /* Open-loop set-point conversion only; ADC feedback is not used here. */
  59      cv_pwm_set_duty(voltage_to_duty(s_voltage_reference_v)); // 设电压基准
  60      cc_pwm_set_duty(current_to_duty(s_current_reference_a)); // 设电流基准
        // ⚠ 注释诚实：开环设定点，反馈在模拟 IC 内完成；MCU 不读 ADC 修正
  61  }

  63  static void outputs_off(void)
  64  {
  65      gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);  // 断输出继电器
  66      gpio_bit_reset(LLC_EN_PORT,   LLC_EN_PIN);     // 关 LLC 使能
  67      pfc_disable();                                  // 断 PFC
        // ⚠ B1：三路同 tick 同步断开，无"先关 LLC→放电→再断继电器"时序
        // ⚠ C5：未复位 CV_PWM/CC_PWM 占空比，重新充电可能带旧值
  68  }

  70  static bool condition_held(bool condition, uint32_t duration_ms)
  71  {
  72      if (!condition) {                       // 条件不满足→复位去抖状态
  73          s_qualification_active = false;
  74          return false;
  75      }
  76      if (!s_qualification_active) {          // 首次满足→记录起点，本次返回假
  77          s_qualification_active = true;
  78          s_qualification_started_ms = g_ms;
  79          return false;
  80      }
  81      return elapsed_reached(s_qualification_started_ms, duration_ms); // 持续够久才真
        // ✅ 亮点：去抖封装干净，避免误触发
  82  }

  86  static void enter_state(charger_state_t state)
  87  {
  88      g_charger_state = state;          // 切换全局状态
  89      s_state_started_ms = g_ms;        // 重置状态计时
  90      s_qualification_active = false;   // 重置去抖标志（防跨状态残留）
        // ✅ 亮点：集中重置
  91  }

  94  void power_supervisor_init(void)
  95  {
  96      s_enable_requested = false;
  97      s_voltage_reference_v = 0.0f;
  98      s_current_reference_a = 0.0f;
  99      s_last_status_ms = g_ms;
 100      cv_pwm_set_duty(CV_PWM_DUTY_INIT);   // 初始安全占空比
 101      cc_pwm_set_duty(CC_PWM_DUTY_INIT);
 102      outputs_off();                        // 默认全部关断
 103      enter_state(MAIN_STEP_STANDBY);       // 初始进待机
 104  }

 106  void power_supervisor_request(bool enable)
 107  { s_enable_requested = enable; }          // CAN 命令设使能

 111  bool power_supervisor_requested(void)
 112  { return s_enable_requested; }            // 供其他模块查询使能

 116  void power_supervisor_set_references(float voltage_v, float current_a)
 117  {
 118      s_voltage_reference_v = f_clampf(voltage_v, 0.0f, CV_REFERENCE_MAX_V);
 119      s_current_reference_a = f_clampf(current_a, 0.0f, CC_REFERENCE_MAX_A);
 120      apply_references();                    // 立即下发新基准
 121  }

 123  float power_supervisor_voltage_reference(void) { return s_voltage_reference_v; }
 124  float power_supervisor_current_reference(void) { return s_current_reference_a; }

 126  void power_supervisor_tick_1khz(void)        // 1kHz 主节拍（来自 systick）
 127  {
 128      can_rx_data_t command;
 129      if (can_comm_get_cmd(&command)) {        // 收到 CAN 命令
 130          power_supervisor_set_references((float)command.vout_set_x10 * 0.1f,
 131                                          (float)command.iout_set_x10 * 0.1f); // ×0.1 还原 V/A
 132          power_supervisor_request(command.enable != 0U);
 133      }
 134      if (elapsed_reached(s_last_status_ms, CAN_STATUS_PERIOD_MS)) {
 135          s_last_status_ms = g_ms;
 136          can_comm_tx_status();               // 周期上报状态
 137      }
 138      if (s_enable_requested && can_comm_timeout()) {
 139          protect_set_fault(FAULT_CAN_TIMEOUT);  // CAN 超时→锁故障
 140      }
 141      if (g_charger_state == MAIN_STEP_FAULT) {  // ⚠ B2：FAULT 在 CAN 处理后才判
 142          outputs_off();                        // 即时 fail-safe
 143          gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
 144          gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
 145          return;                               // ⚠ B2：return 后不再收 CAN，无法远程清故障
 146      }
 147      switch(g_charger_state)
 148      {
 149          case MAIN_STEP_STANDBY:
 150          {
 151              outputs_off();                    // ⚠ C3：每 tick 先全关
 152              gpio_bit_set(LED_RED_PORT,LED_RED_PIN);
 153              gpio_bit_reset(LED_GREEN_PORT,LED_GREEN_PIN);
 154              if(s_enable_requested && g_adc_multi.vbat_v >= BATTERY_PRESENT_V)
 155              {                                 // 使能且电池已接入(≥10V)
 156                  pfc_enable();                 // ⚠ C3：同一 tick 又开 PFC
 157                  enter_state(MAIN_STEP_PRECHARGE);
 158              }
 159              break;
 160          }
 161          case MAIN_STEP_PRECHARGE:
 162          {
 163              if (!s_enable_requested) {        // 使能撤销→回待机
 164                  outputs_off();
 165                  enter_state(MAIN_STEP_STANDBY);
 166              } else if (pfc_is_ready()) {      // 母线建起(360V)→进 CC
 167                  apply_references();           // 仅此处下发一次基准（见 A2）
 168                  gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN); // 开 LLC
 169                  enter_state(MAIN_STEP_CC);    // 注释: Legacy CAN state value: running
 170              }
        // ⚠ A1：无第三个分支——pfc 永远不 ready 则永久卡死，不报错不回待机
 171              break;
 172          }
 173          case MAIN_STEP_CC:
 174          {
 175              if(!s_enable_requested) {         // 使能撤销→待机
 176                  outputs_off();
 177                  enter_state(MAIN_STEP_STANDBY);
 178              }
 179              else if(elapsed_reached(s_state_started_ms,CHARGE_CC_TIMEOUT_MS)) {
 180                  protect_set_fault(FAULT_CHARGE_TIMEOUT); // ⚠ B3：CC 超时=8s(调试值)，正常应 8h
 181              }
 182              else if(condition_held(s_voltage_reference_v > BATTERY_PRESENT_V
 183                      && g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_CV_ENTRY_MARGIN_V),
 184                      CHARGE_CV_ENTRY_DEBOUNCE_MS)) {  // ⚠ C4：判据用 vbat 非 vout
 185                  enter_state(MAIN_STEP_CV);    // 电压到达→进 CV
 186              }
 187              else if(g_adc_multi.vout_v>OUTPUT_RELAY_MIN_V) { // 输出已建起
 188                  gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN); // 闭合输出继电器
 189                  gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
 190                  gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
 191              }
 192              else if (elapsed_reached(s_state_started_ms, LLC_START_TIMEOUT_MS)) {
 193                  protect_set_fault(FAULT_BUS_UVP); // ⚠ C2：名实不符(报母线欠压实则查输出)
 194              }
 195              break;
 196          }
 197          case MAIN_STEP_CV:
 198          {
 199              if(!s_enable_requested) {         // 使能撤销→待机
 200                  outputs_off();
 201                  enter_state(MAIN_STEP_STANDBY);
 202              }
 203              else if(elapsed_reached(s_state_started_ms,CHARGE_CV_TIMEOUT_MS)) {
 204                  protect_set_fault(FAULT_CHARGE_TIMEOUT); // ⚠ B3：CV 超时=3s(调试值)，正常应 3h
 205              }
 206              else
 207              {
 208              /* The analog IC owns the CV loop; the MCU only qualifies completion. */
 209                  gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
 210                  gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
 211                  gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
 212                  if(condition_held(s_voltage_reference_v>BATTERY_PRESENT_V
 213                      && g_adc_multi.vbat_v>=(s_voltage_reference_v - CHARGE_FINISH_VOLTAGE_MARGIN_V)
 214                      && g_adc_multi.iout_a >= 0                    // ⚠ C1：恒真冗余
 215                      && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A, // 电流≤阈值
 216                      CHARGE_FINISH_DEBOUNCE_MS)) {
 217                      outputs_off();               // 充满→断输出
 218                      enter_state(MAIN_STEP_FINISHED);
 219                  }
 220              }
 221              break;
 222          }
 223          case MAIN_STEP_FINISHED:
 224          {
 225              outputs_off();                    // 保持断开输出
 226              gpio_bit_reset(LED_RED_PORT,LED_RED_PIN);
 227              gpio_bit_set(LED_GREEN_PORT,LED_GREEN_PIN); // 绿灯常亮=充满
 228              if(!s_enable_requested) {
 229                  enter_state(MAIN_STEP_STANDBY); // 撤销使能→回待机
 230              }
 231              break;
 232          }
 233          default:
 234              outputs_off();
 235              enter_state(MAIN_STEP_STANDBY);   // ⚠ C6：TRICKLE/INVALID 全靠此处兜底
 236              break;
 237      }
 238      // （函数结束）
 239  }   // 第 243 行：文件结束
```

> 行内 `⚠` 标记对应前文 A/B/C 各问题编号；`✅` 为亮点。完整问题清单见第五节。
