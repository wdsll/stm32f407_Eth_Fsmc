# llc_control.c 专项评审（更新版）

> 文件: `HW/src/llc_control.c` (359 行) + `HW/include/llc_control.h` + 依赖宏见 `App/include/main.h`  
> 初版评审: 2026-08-12　|　本更新: 2026-08-24  
> 范围: 仅 `llc_control` 模块（充电功率时序状态机）  
> 对比基准: `doc/专项模块评审/llc_control代码评审.md`（08-12 初版）

---

## 〇、模块定位（更新）

本文件是充电状态机核心：STANDBY → PRECHARGE → CC → CV → FINISHED，FAULT 在 `tick` 顶部统一收敛。MCU 仅通过 `CV_PWM` / `CC_PWM` 给模拟 LLC IC 设定**开环电压/电流基准**，模拟 IC 内部完成双环闭环；MCU 不做 PI。

**与 08-12 初版最大的架构变化：**
1. `enter_state()` 增加了**同态幂等守卫** + **按状态的一次性动作**（apply_references / LLC_EN / 继电器 / LED 全收进 enter），符合我们此前评审建议的「一次性动作进 enter」范式。
2. `can_comm.h` 被注释，CAN 命令**不再在 tick 内处理**——改由外部（串口 console）通过「transport-independent 的 supervisor API」(`power_supervisor_request/set_references`) 注入。CAN 整体在 `main.c` 里被禁用（`can_comm_init` / `can_comm_poll` 注释），**这是为初版串口联调有意预留的「CAN 集成阶段」**，不是 bug。
3. FAULT 增加了**远程恢复路径**（需 `!enable && 锁存已清 && 硬件无故障`）。

---

## 一、相比 08-12 初版：问题处置状态

| 编号 | 原严重级 | 描述 | 现状 |
| -- | -- | -- | -- |
| A1 | 🔴 | PRECHARGE 无超时永久卡死 | ✅ **已修** — `:300-302` 新增 `PFC_READY_TIMEOUT_MS`(3s) 超时 → `FAULT_PRECHARGE_TIMEOUT` |
| A2 | 🔴 | CV 不切基准、头注释与实现矛盾 | ✅ **已修（注释侧）** — 头注释第 9 行已加 `// A2：此注释与实现矛盾`，明确 MCU 仅下发固定 setpoint；实现未改（仍由模拟 IC 自动 CC/CV，符合设计意图） |
| B1 | 🟠 | 安全断电无时序，继电器带载分断 | ❌ **仍开放** — `outputs_force_off()`(`:110-117`) 仍同 tick 齐断 LLC_EN/继电器/PFC，无「先关 LLC→泄放→再断继电器」缓冲（见下 N-B1） |
| B2 | 🟠 | FAULT 后无法恢复 | ✅ **已修** — `:273-282` 新增 FAULT 态恢复：满足 `!enable && !protect_fault_latched() && !protect_fault_active_hw()` 即回 STANDBY |
| B3 | 🟠 | CC/CV 超时误用 8s/3s 调试值 | ✅ **已修** — `main.h:109-110` 已为 `8h / 3h`（量产值） |
| C1 | 🟡 | 完成判据 `iout_a>=0` 恒真冗余 | ❌ **仍开放** — `:340` 仍保留 `g_adc_multi.iout_a >= 0` |
| C2 | 🟡 | `LLC_START_TIMEOUT_MS` 名实不符 | ✅ **已修（命名+故障码）** — 改名 `OUTPUT_START_TIMEOUT_MS`(`:30`)、报错 `FAULT_OUTPUT_START_TIMEOUT`(`:309`)，并改用 `vout` 判定(`:321`) |
| C3 | 🟡 | STANDBY 同 tick PFC off→on | ✅ **已修** — STANDBY 不再每 tick 调 `outputs_off/pfc_enable`；`pfc_enable()` 移入 `enter_state(PRECHARGE)`(`:156`) |
| C4 | 🟡 | CV 进入判据用 vbat 非 vout | 🟡 **部分修** — 输出建起超时(`:321`)已改用 `vout`；但 CV 进入(`:317/:339`)仍用 `vbat` |
| C5 | 🟡 | outputs_off 不复位 PWM 占空比 | ✅ **已修** — `outputs_force_off()`(`:113-114`) 先把 `CV/CC_PWM` 置 `PWM_DUTY_SAFE`(=0) |
| C6 | 🟡 | TRICKLE/INVALID 靠 default 兜底 | 🟡 维持 — `:356` default→STANDBY，可接受（TRICKLE 本就空壳） |

---

## 二、本次新发现的问题

### 🟠 N-B1 — 安全断电仍无时序（B1 未落实，最高剩余风险）
`outputs_force_off()`(`:110-117`) 顺序是：关 `LLC_EN` → PWM 置安全 → 断 `OUT_RELAY` → `pfc_disable()`，**全部同一 tick 完成**。对 1.5kW 感性输出，继电器在仍有电流时断开会产生拉弧、触点粘连。模拟 PFC/LLC 架构下 MCU 是停功率的**唯一软件路径**（除非原理图有 BKIN 硬件直断，需核对）。建议改成：立刻关 `LLC_EN` → 延时 ~20–50ms 或等 `iout_a` 降到阈值 → 再断 `OUT_RELAY` → 最后 `pfc_disable()`。可加一个 `off_sequence` 子状态机实现。

### 🟡 N1 — CC 进入即闭合输出继电器（行为变化，需验证）
08-12 初版：CC 内 `vout>10V` 后才闭合继电器（先让 LLC 输出建立再接电池）。**现版**：`enter_state(MAIN_STEP_CC)`(`:159-164`) 在 CC 进入瞬间同时 `apply_references + 置 LLC_EN + outputs_on`（继电器立即闭合）。意味 LLC 刚使能、输出尚在爬升时就把电池（≥10V）接上。
- 由于模拟 IC 有 40A CC 限流，电池反向给输出电容充电的浪涌被限流钳住，**初版联调大概率可接受**；
- 但相比「先建输出再接电池」少了一次软起保护。建议：接真电池/大电流前，在 bench 上确认无异常冲击电流；或恢复「`vout` 建起后再闭合继电器」的守卫。

### 🟡 N2 — CAN 被整体禁用（设计意图，需记备忘）
`can_comm_init`(`main.c:257`) 与 `can_comm_poll`(`main.c:329-330`) 均注释，`llc_control.c:21` 的 `can_comm.h` 也注释。当前**唯一控制入口是串口 console**（`serial_console.c` 调 `power_supervisor_request/set_references`）。**这是为初版联调有意预留 CAN 阶段，不是缺陷**。但进入 CAN 集成阶段时，需恢复 `can_comm_poll` 并在其中把命令喂给 supervisor API（否则 CAN 无法控制/上报）。建议在代码或文档标注此 TODO。

### 🟡 N3 — 死注释 `#define PFC_READY_TIMEOUT_MS`(`:32`)
本文件第 32 行把该宏注释掉了，但实际定义在 `main.h:90`。属冗余死代码，不影响编译，建议删除本行以免误导「宏未定义」。

### 🟢 N4 — `g_fault` 与锁存自洽（已正确）
`:278` FAULT 恢复里 `g_fault = FAULT_NONE`，与 `protect.c` 的 `protect_set_fault`(写 `g_fault`+锁存) / `protect_clear_fault`(清锁存+`g_fault`) 一致。`g_fault` 是统一全局量(`main.h:322`)，且恢复前置条件含 `!protect_fault_latched()`，不会与硬件锁存冲突。**正确**。

---

## 三、逐行中文注释（当前 359 行）

> 行号对应 `HW/src/llc_control.c` 当前版本。标记：`✅` 亮点 / `⚠` 问题(编号见上) / `🟢` 确认正确 / `🆕` 本次新发现。

```c
   1  /*****************************************************************/   // 文件头横幅
   2  * 模块名称：llc_control.c
   3  * 摘    要：
   4  * 作    者：Rengar
   5  * 内    容：llc_control.c - LLC 模拟控制状态机 (CV_PWM + CC_PWM 双基准)
   6  *           MCU 仅设定 CV_PWM(电压基准) + CC_PWM(电流基准), 模拟IC 内部闭环
   7  *           CC 模式: CC_PWM=目标电流, CV_PWM=电压上限
   8  *           CV 模式: CV_PWM=目标电压, CC_PWM=电流上限
   9  *           // ⚠ A2：此注释与实现矛盾——CV 状态并没切换基准，已加注说明
  10  * 注    意：需在 Options->Target 勾选 Use MicroLIB，否则 printf 不会输出
  11  *****************************************************************/

  15  /***************** 包含头文件 *****************/
  18  #include "llc_control.h"      // 本模块接口/枚举/宏
  19  #include "adc_dma.h"          // g_adc_multi（电压/电流采样值）
  20  #include "condition_held.h"   // 🆕 condition_qualification_t / condition_held / reset（去抖重构）
  21  //#include "can_comm.h"       // 🆕 CAN 类型已不用；命令改由外部 supervisor API 注入
  22  #include "pfc_control.h"      // pfc_enable()/pfc_disable()/pfc_is_ready()
  23  #include "protect.h"          // protect_set_fault()/protect_fault_latched()/protect_fault_active_hw()
  24  #include "pwm_llc.h"          // cv_pwm_set_duty()/cc_pwm_set_duty()

  28  #define BATTERY_PRESENT_V       (10.0f)   // 判定"电池已接入"的最低电压
  29  #define OUTPUT_RELAY_MIN_V      (10.0f)   // CC 态判定输出建起的电压门限（亦用于输出启动超时）
  30  #define OUTPUT_START_TIMEOUT_MS (1500U)   // ✅ C2 已改名：CC 后 1.5s 输出未建起→FAULT_OUTPUT_START_TIMEOUT
  32  //#define PFC_READY_TIMEOUT_MS (3000U)     // 🟡 N3 死注释：真实定义在 main.h:90，建议删除本行
  33  //#define CAN_STATUS_PERIOD_MS (100U)      // CAN 周期上报已随 CAN 禁用一并注释
  35  #define CV_REFERENCE_MAX_V      (80.0f)   // CV_PWM 占空比换算满量程电压
  37  #define CC_REFERENCE_MAX_A      (20.0f)   // CC_PWM 满量程电流（注意 OCP 真实~24.8A，见全局 C3）

  42  static bool s_enable_requested;        // 充电使能请求
  44  static float s_voltage_reference_v;    // 当前电压基准(V)，来自外部 set_references
  46  static float s_current_reference_a;    // 当前电流基准(A)
  48  static uint32_t s_state_started_ms;    // 进入当前状态时间戳（状态内超时用）
  50  static condition_qualification_t s_qualification;  // 🆕 去抖状态（CC→CV / 完成判据共用，按状态 reset）

  64  static float voltage_to_duty(float voltage_v)
  66      return f_clampf(voltage_v / CV_REFERENCE_MAX_V, CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX); // 电压→占空比，钳合法区间

  79  static float current_to_duty(float current_a)
  81      return f_clampf(current_a / CC_REFERENCE_MAX_A, CC_PWM_DUTY_MIN, CC_PWM_DUTY_MAX); // 电流→占空比

  93  static void apply_references(void)
  95      /* Open-loop set-point conversion only; ADC feedback is not used here. */
  96      cv_pwm_set_duty(voltage_to_duty(s_voltage_reference_v)); // 设电压基准
  97      cc_pwm_set_duty(current_to_duty(s_current_reference_a)); // 设电流基准
        // ✅ 注释诚实：开环设定点，反馈在模拟 IC 内；MCU 不读 ADC 修正

 110  static void outputs_force_off(void)    // ❌ B1/N-B1：同 tick 齐断，无缓冲时序
 112     gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);   // 关 LLC 使能
 113     cv_pwm_set_duty(PWM_DUTY_SAFE);            // ✅ C5 已修：PWM 复位到安全占空比(=0)
 114     cc_pwm_set_duty(PWM_DUTY_SAFE);
 115     gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN); // 断输出继电器
 116     pfc_disable();                             // 断 PFC
        // ⚠ B1：LLC_EN 关与 OUT_RELAY 断无延时，继电器带载分断风险

 119  static void outputs_on(void)           // 🆕 CC/CV/FINISHED 共用：闭合继电器+绿亮红灭
 121     gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
 122     gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
 123     gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);

 135  static void enter_state(charger_state_t state)
 137     if (g_charger_state == state) { return; }   // ✅ 同态幂等守卫：重复进入不重复执行一次性动作
 141     g_charger_state = state;                    // 切换全局状态
 142     s_state_started_ms = g_ms;                 // 重置状态计时
 143     condition_qualification_reset(&s_qualification); // ✅ 重置去抖，防跨状态残留
 145     switch(state) {                            // ✅ 一次性动作集中在此（enter 范式）
 147         case MAIN_STEP_STANDBY: outputs_force_off(); 红灯; 绿灯灭; break;
 154         case MAIN_STEP_PRECHARGE: pfc_enable(); break;   // ✅ C3 已修：PFC 使能挪到此处一次性
 159         case MAIN_STEP_CC:
 161             apply_references();                // 仅此处下发一次基准
 162             gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN); // 开 LLC
 163             outputs_on();                      // 🟡 N1：此处立即闭合继电器（行为变化，见 N1）
 164             break;
 166         case MAIN_STEP_CV:  outputs_on(); break;  // CV 已闭合，保持即可
 171         case MAIN_STEP_FINISHED: outputs_force_off(); 红灯灭; 绿灯亮; break;
 178         case MAIN_STEP_FAULT: outputs_force_off(); 红灯; 绿灯灭; break;  // ✅ fail-safe 收敛点
 185         default: break;
 188  }

 190  void power_supervisor_enter_fault(void) { enter_state(MAIN_STEP_FAULT); } // protect 调此进 FAULT

 205  void power_supervisor_init(void)
 207     s_enable_requested = false;
 208     s_voltage_reference_v = 0.0f;
 209     s_current_reference_a = 0.0f;
 211     cv_pwm_set_duty(CV_PWM_DUTY_INIT);   // 初始安全占空比
 212     cc_pwm_set_duty(CC_PWM_DUTY_INIT);
 214     enter_state(MAIN_STEP_STANDBY);      // 初始进待机（enter 内已 force_off + LED）

 225  void power_supervisor_request(bool enable) { s_enable_requested = enable; }  // 外部置使能
 238  bool power_supervisor_requested(void) { return s_enable_requested; }        // 供查询
 251  void power_supervisor_set_references(float voltage_v, float current_a)
 253     s_voltage_reference_v = f_clampf(voltage_v, 0.0f, CV_REFERENCE_MAX_V);
 254     s_current_reference_a = f_clampf(current_a, 0.0f, CC_REFERENCE_MAX_A);
 255     apply_references();   // 立即下发新基准（CC 中调此即可在线改 setpoint）
 258  float power_supervisor_voltage_reference(void) { return s_voltage_reference_v; }
 259  float power_supervisor_current_reference(void) { return s_current_reference_a; }

 270  void power_supervisor_tick_1khz(void)   // 1kHz 主节拍（来自 systick）
 272     /* Commands arrive through the transport-independent supervisor API. */
 273     if (g_charger_state == MAIN_STEP_FAULT) {        // ✅ B2 已修：FAULT 顶部优先处理
 274         outputs_force_off();                        // 持续强制断电
 276         if (!s_enable_requested && !protect_fault_latched() && !protect_fault_active_hw()) {
 278             g_fault = FAULT_NONE;                    // 🟢 N4 与锁存自洽
 279             enter_state(MAIN_STEP_STANDBY);          // 满足恢复条件→回待机
 281         return;                                      // FAULT 态不再跑下方 switch
 282     }
 283     switch(g_charger_state) {
 285         case MAIN_STEP_STANDBY:
 287             if (s_enable_requested && g_adc_multi.vbat_v >= BATTERY_PRESENT_V) // 使能且电池已接入(≥10V)
 289                 enter_state(MAIN_STEP_PRECHARGE);    // → PRECHARGE（pfc_enable 在 enter 内）
 291             break;
 293         case MAIN_STEP_PRECHARGE:
 295             if (!s_enable_requested) enter_state(MAIN_STEP_STANDBY);  // 使能撤销→待机
 297             else if (pfc_is_ready()) enter_state(MAIN_STEP_CC);       // 母线≥360V→CC
 300             else if (elapsed_reached(s_state_started_ms, PFC_READY_TIMEOUT_MS))  // ✅ A1 已修：3s 超时
 301                 protect_set_fault(FAULT_PRECHARGE_TIMEOUT);          // 母线建不起→故障，不再永久卡死
 304             break;
 306         case MAIN_STEP_CC:
 308             if (!s_enable_requested) enter_state(MAIN_STEP_STANDBY); // 使能撤销→待机
 312             else if (elapsed_reached(s_state_started_ms, CHARGE_CC_TIMEOUT_MS))  // ✅ B3 已修：8h
 314                 protect_set_fault(FAULT_CHARGE_TIMEOUT);
 317             else if (condition_held(&s_qualification,
                       s_voltage_reference_v > BATTERY_PRESENT_V
                       && g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_CV_ENTRY_MARGIN_V),
                       CHARGE_CV_ENTRY_DEBOUNCE_MS))   // 🟡 C4 仍用 vbat 判 CV 进入（非 vout）
 319                 enter_state(MAIN_STEP_CV);          // 电压到达→进 CV
 321             else if (g_adc_multi.vout_v <= OUTPUT_RELAY_MIN_V
                       && elapsed_reached(s_state_started_ms, OUTPUT_START_TIMEOUT_MS))  // ✅ C2 已用 vout
 323                 protect_set_fault(FAULT_OUTPUT_START_TIMEOUT);  // 1.5s 内输出未建起→故障
 325             break;
 327         case MAIN_STEP_CV:
 329             if (!s_enable_requested) enter_state(MAIN_STEP_STANDBY);
 333             else if (elapsed_reached(s_state_started_ms, CHARGE_CV_TIMEOUT_MS))  // ✅ B3 已修：3h
 335                 protect_set_fault(FAULT_CHARGE_TIMEOUT);
 339             else if (condition_held(&s_qualification,
                       s_voltage_reference_v > BATTERY_PRESENT_V
                       && g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_FINISH_VOLTAGE_MARGIN_V)
                       && g_adc_multi.iout_a >= 0                    // ❌ C1 仍冗余：adc 已将负值钳 0
                       && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A,  // 电流≤1A
                       CHARGE_FINISH_DEBOUNCE_MS))   // 30s 持续
 342                 enter_state(MAIN_STEP_FINISHED);  // 充满→断输出
 345             break;
 347         case MAIN_STEP_FINISHED:
 349             if (!s_enable_requested) enter_state(MAIN_STEP_STANDBY);  // 撤销使能→回待机
 353             break;
 355         default: enter_state(MAIN_STEP_STANDBY);   // 🟡 C6：TRICKLE/INVALID 兜底
 358     }
 359  }
```

---

## 四、本文件问题清单（含状态）

| 编号 | 严重级 | 描述 | 位置 | 状态 |
| -- | -- | -- | -- | -- |
| A1 | 🔴 | PRECHARGE 无超时永久卡死 | `:300-302` | ✅ 已修 |
| A2 | 🔴 | CV 不切基准、注释矛盾 | `:9` | ✅ 已修(注释) |
| B1/N-B1 | 🟠 | 安全断电无时序，继电器带载分断 | `:110-117` | ❌ 仍开放（最高剩余） |
| B2 | 🟠 | FAULT 后无法恢复 | `:273-282` | ✅ 已修 |
| B3 | 🟠 | CC/CV 超时误用 8s/3s | `main.h:109-110` | ✅ 已修 |
| C1 | 🟡 | 完成判据 `iout_a>=0` 恒真冗余 | `:340` | ❌ 仍开放 |
| C2 | 🟡 | 启动超时名实不符 | `:30/:321` | ✅ 已修 |
| C3 | 🟡 | STANDBY 同 tick PFC off→on | `:154` | ✅ 已修 |
| C4 | 🟡 | CV 进入判据用 vbat 非 vout | `:317/:339` | 🟡 部分修 |
| C5 | 🟡 | outputs_off 不复位 PWM | `:113-114` | ✅ 已修 |
| C6 | 🟡 | TRICKLE/INVALID 兜底 | `:355` | 🟡 维持 |
| N1 | 🟡 | CC 进入即闭合继电器（行为变化） | `:163` | 🆕 需验证 |
| N2 | 🟡 | CAN 整体禁用（设计意图） | `main.c:257/329` | 🆕 记备忘 |
| N3 | 🟡 | 死注释 `#define PFC_READY_TIMEOUT_MS` | `:32` | 🆕 清垃圾 |
| N4 | 🟢 | `g_fault` 与锁存自洽 | `:278` | 🟢 已正确 |

---

## 五、修改优先级建议

```
1. N-B1  安全断电时序        —— 继电器寿命/安全，最高剩余风险（纯逻辑加子状态机）
2. N1    CC 进入即闭继电器    —— 接真电池前 bench 验证浪涌；或恢复"vout 建起后闭继电器"
3. C4    CV 进入判据改用 vout —— 长线损时判据更准确
4. C1    删 iout_a>=0 冗余    —— 一行清理
5. N3    删死注释 #define     —— 一行清理
6. N2    标注 CAN 阶段 TODO   —— 进 CAN 集成时恢复 can_comm_poll 喂命令
```

---

## 六、亮点（做对的地方）

1. **`enter_state` 同态幂等守卫 + 一次性动作集中**（`:137/:145-188`）：状态切换干净，避免重复动作与跨状态残留，是我们此前「enter 范式」建议的落地。
2. **FAULT 顶部优先收敛 + 远程恢复**（`:273-282`）：fail-safe 到位，且允许「STOP + CLEAR」后自动回待机，不再只能断电复位（B2 修复）。
3. **PWM 安全复位进 `outputs_force_off`**（`:113-114`）：重新充电必从 `PWM_DUTY_SAFE` 起步，消除旧占空比浪涌（C5 修复）。
4. **去抖重构为 `condition_qualification_t`**（`:50/:317/:339`）：CC→CV 与完成判据共用、按状态 reset，封装干净。
5. **CAN 与 supervisor 解耦**：`can_comm.h` 注释 + 外部 API 注入，为初版串口联调扫清依赖，且便于后续接 CAN/其他 transport。
6. **`g_fault` 与保护锁存自洽**（N4）：恢复逻辑不会与硬件锁存冲突。

---

> 总结：相对 08-12 初版，本模块已解决 7 项（A1/A2/B2/B3/C2/C3/C5），架构明显更健壮。**初版软硬联调可用**：状态机逻辑闭合、FAULT 可恢复、超时防卡死均已具备。剩余最高风险是 **N-B1 断电无缓冲时序**（接真电池/大电流前建议补），以及 **N1 的继电器闭合时机**需在 bench 上确认无冲击。CAN 禁用是设计意图，记得在 CAN 集成阶段恢复。

---

## 七、初版 CC/CV 验证分级（2026-08-24 补）

> 结论先行：**状态机结构上必须保留 CC/CV（代码已分，别删）；但初版联调不必以"完整跑完 CV 并判定 FINISHED"作为成功标准**，分阶段验证即可。

### 1. 为什么必须分 CC/CV

本拓扑是**模拟 LLC（NCP4390 内部闭环）**，MCU 只给开环设定点：CC 下发电流参考、CV 下发电压参考（见 `:93-110` `apply_references`）。分段对 MCU 只是一行切换，边际成本极低，但**不分有真隐患**：

- 若一直停在 CC 不进 CV → 电池电压持续冲高 → 触发 OVP 64V 保护进 FAULT。等于"只要充电就必故障"，联调反而更乱。
- `STANDBY→PRECHARGE→CC→CV→FINISHED` 骨架已齐全，**保留即可**，删除反而是倒退。

### 2. 两个初版就必须通的红线的（否则变"假故障循环"）

| 红线 | 对应代码 | 失败后果 |
| -- | -- | -- |
| **CC 硬限流** | CC 进入 `:159-164` 下发 `s_current_reference_a`（40A 钳位） | 电池低压/短路时灌爆；需确认进入 CC 时给的是电流参考而非电压参考 |
| **CV 进入判据能通** | `:317-319` `vbat_v ≥ v_ref − CHARGE_CV_ENTRY_MARGIN_V`（去抖后切 CV） | 不进 CV → 电压一路冲到 OVP → 必然 FAULT |

> 注：C4（CV 进入用 `vbat` 而非 `vout`）初版可接受，长线损场景后续再优化。

### 3. 验证分级（按优先级，不必一次到位）

| 阶段 | 验证目标 | 负载/源配置（稳压源+电子负载模拟） | 通过标准 |
| -- | -- | -- | -- |
| **① 先 CC** | 功率链路 / PFC 建压 / 保护 / 采样标定 | 输出端子接 ≥10V 稳压源（满足 `BATTERY_PRESENT_V`），电子负载吸流（恒流或定阻） | 母线 ≥360V、CC 进入、输出能建起、串口电压/电流读数跟万用表对得上 |
| **② 再 CV** | 电压钳位 + CV 进入判据 | 电子负载减小电流，让 vout 顶到 `v_ref`(≈58.4V) | 自动切到 CV 且电压钳住（不再上涨），无 OVP 误触发 |
| **③ FINISHED（可暂缓）** | 充满判定（电流衰减到阈值） | 电子负载电流调到 ≤`CHARGE_FINISH_CURRENT_A`(1A) | 30s 去抖后进入 FINISHED 并断输出 |

### 4. 初版可放宽 / 临时处置点

- **FINISHED 自动停机判定（③）初版可放宽或手动 STOP**：避免"充不满自动停不了"干扰联调。手动 `STOP` 命令（串口 console）即可安全回 STANDBY（`:347-353`）。
- **标定没把握时临时调宽保护门槛**：OCP 24A、OVP 64V 在采样标定未稳前，若因读数偏差误触发，调试阶段可临时放宽，联调通过再收紧。
- **无电池进不了预充是设计意图**：端子完全开路（vbat < 10V）卡在 STANDBY 是充电器有意的安全拦截，不是 bug；接 ≥10V 稳压源即可解锁（见 N1 之前的 STANDBY 出口 `:287`）。

### 5. 一句话

CC/CV **结构上要分（已有，留着）**，初版联调**先跑通 CC + 验证 CV 进入钳位**即可，FINISHED 自动停机初版可放宽或手动停。核心红线：**CC 限流 + CV 能进**，否则退化为"过充触发保护"的假故障循环。
