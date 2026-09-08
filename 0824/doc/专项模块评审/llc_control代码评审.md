# llc_control.c 专项评审（v3 更新版）

> 文件: `HW/src/llc_control.c`（当前 370 行）+ `HW/include/llc_control.h` + 依赖宏见 `App/include/main.h`（354 行）
> 修订记录：
> - 2026-08-12 v1 初版评审
> - 2026-08-24 v2 更新（A1~C6 处置状态、N1~N4 新发现、第七节验证分级）
> - 2026-09-03 v3（本版）：① **B2 结论再次修正**——FAULT 恢复分支含恒真条件，纯"自动恢复"仍是死代码，实际恢复语义是"CLEAR 后自动回 STANDBY"（承接 09-02 状态机 v2 复审）；② C1 结论修正——`iout_a>=0` 不是"恒真无害"，iout 未标定时完成判据可能提前误触发 FINISHED；③ 新增 `llc_enable()/llc_disable()` 封装与 **LLC_EN 低有效极性反转**的核对；④ CV_REFERENCE_MAX_V 80→85；⑤ 行号全部按 370 行当前版重对；⑥ 过时注释清单更新。
> 范围: 仅 `llc_control` 模块（充电功率时序状态机）

---

## 〇、模块定位（v3）

本文件是充电状态机核心：STANDBY → PRECHARGE → CC → CV → FINISHED，FAULT 在 `tick` 顶部统一收敛。MCU 仅通过 `CV_PWM` / `CC_PWM` 给模拟 LLC IC 设定**开环电压/电流基准**，模拟 IC 内部完成双环闭环；MCU 不做 PI。

**v2 之后代码的三个实质变化：**
1. **`llc_enable()/llc_disable()` 封装新增（:55-65），LLC_EN 极性反转为低有效**：`llc_enable()`=拉低（使能，注释 "LLC_EN is active low"）、`llc_disable()`=拉高（关断）。旧版裸 GPIO 的 `set=开/reset=关` 已全部替换。与 `main.c:203` `hw_gpio_init` 里的 `llc_disable()`（初始关断）核对一致 ✅。这是好重构：极性收口到两个函数，消除散布的裸 GPIO 极性。
2. `CV_REFERENCE_MAX_V` 80→**85**（:35）。
3. `main.c` 侧 standalone PFC 测试门控被注释（`serial_console_pfc_test_active` 已随 PFC ON/OFF 命令移除），`power_supervisor_tick_1khz()` 无条件执行。

---

## 一、问题处置状态总表（v3 刷新）

| 编号 | 原严重级 | 描述 | v3 状态 |
| -- | -- | -- | -- |
| A1 | 🔴 | PRECHARGE 无超时永久卡死 | ✅ **已修** — `:311-313` 3s 超时 → `FAULT_PRECHARGE_TIMEOUT`；且 `pfc_control.c:62-66` RELAY_ON 态也有同款 3s 超时（双层，pfc 层先触发，见 N5） |
| A2 | 🔴 | CV 不切基准、头注释矛盾 | ✅ **已修（注释侧）** — 头注释 `:9` 已注明矛盾，MCU 仅下发固定 setpoint，符合设计意图 |
| B1 | 🟠 | 安全断电无时序，继电器带载分断 | ❌ **仍开放** — `outputs_force_off()`(`:121-128`) 仍同 tick 齐断（见 N-B1，最高剩余风险） |
| B2 | 🟠 | FAULT 后无法恢复 | 🟡 **结论再次修正（见第二节 B2 专节）**：恢复链路实际为"STOP→CLEAR→下一 tick 自动回 STANDBY"；分支里的 `!protect_fault_active_hw()` 因 protect.c `#if 1` 恒真，**纯无人干预的"自动恢复"仍是死代码** |
| B3 | 🟠 | CC/CV 超时误用调试值 | ✅ **已修** — `main.h:104-105` 已为 8h/3h。**但 `:324/:345` 代码注释仍写"8s(调试值)/3s(调试值)"，过时未更新** |
| C1 | 🟡 | 完成判据 `iout_a>=0` 冗余 | 🟡 **结论修正**：冗余本身无害（负值已被钳 0，恒真），**真正风险在 `iout_a<=1A` 这半句**——iout 未标定时可能恒 0 → 充电中提前误判 FINISHED 断输出（见第二节 C1 专节） |
| C2 | 🟡 | 启动超时名实不符 | ✅ **已修** — `OUTPUT_START_TIMEOUT_MS`(`:30`)、`FAULT_OUTPUT_START_TIMEOUT`(`:333`)、vout 判定(`:331`)。`:333` 行内注释"名实不符(报母线欠压实则查输出)"已过时，可删 |
| C3 | 🟡 | STANDBY 同 tick PFC off→on | ✅ **已修** — `pfc_enable()` 在 `enter_state(PRECHARGE)`(`:167`) |
| C4 | 🟡 | CV 进入判据用 vbat 非 vout | 🟡 维持 — `:327/:349` 仍用 `vbat_v`（初版可接受，长线损场景后续优化） |
| C5 | 🟡 | outputs_off 不复位 PWM | ✅ **已修** — `:124-125` 置 `PWM_DUTY_SAFE`。**函数头注释 `:119` "C5：未复位…"已过时，应删** |
| C6 | 🟡 | TRICKLE/INVALID 兜底 | 🟡 维持 — `:366` default→STANDBY，可接受 |
| N1 | 🟡 | CC 进入即闭合输出继电器 | ❌ **仍开放（P1-A）** — `:174` CC 进入瞬间 `llc_enable()+outputs_on()`，先建输出再接电池的软起保护缺失（修法见第二节） |
| N2 | 🟡 | CAN 整体禁用（设计意图） | 🟡 维持 — `:21` `can_comm.h` 注释、main.c `can_comm_init/poll` 注释；进 CAN 集成阶段需恢复并喂命令给 supervisor API |
| N3 | 🟡 | 死注释 `#define PFC_READY_TIMEOUT_MS` | ❌ 仍在 `:32`，建议删除 |
| N4 | 🟢 | `g_fault` 与锁存自洽 | 🟢 正确 — `:289` 清 `g_fault` 与 protect.c 一致（active_hw 恒真后此语义更简单） |
| N5 | 🆕 | PRECHARGE 双层 3s 超时并存 | 🆕 pfc 层（RELAY_ON 等 bus≥350V）与 llc 层（PRECHARGE 等 pfc_is_ready）同为 3s、计时起点相同；pfc_tick 先于 supervisor_tick 执行 → pfc 层先报，llc 层超时实际不可达。功能上无冲突（都报 `FAULT_PRECHARGE_TIMEOUT`），但双层冗余建议择一保留，且 pfc 层触发后故障码"超时"可能掩盖真实原因（见 N6） |
| N6 | 🆕 | 预充故障码无诊断性 | 🆕 `pfc_is_ready()` 三重门（RUN ∧ AC∈[180,264] ∧ bus∈[360,420]）任一不满足 + 3s 超时，都报 `FAULT_PRECHARGE_TIMEOUT`：AC 越限、母线建不起、母线过压（>420V 反而使 vbus_ok=false）三种不同故障共用一个码。建议超时分支内按 `ac_vol_v`/`bus_vol_v` 实际值分流故障码（方案见第四节补丁 B） |

---

## 二、v3 三个结论修正（详细）

### B2 专节 — FAULT 恢复的真实语义（承接 09-02 状态机 v2 复审）

当前代码（`:284-293`）：
```c
if (g_charger_state == MAIN_STEP_FAULT) {
    outputs_force_off();
    if(!s_enable_requested && !protect_fault_latched() && !protect_fault_active_hw())
    {
        g_fault = FAULT_NONE;
        enter_state(MAIN_STEP_STANDBY);
    }
    return;
}
```
- `protect_fault_active_hw()` 当前 `#if 1 return false`（protect.c:55-56）**恒假** → `!...` 恒真，此条件形同虚设；
- `protect_fault_latched()` 进 FAULT 必为 true（`protect_set_fault` 置位），只有串口 `CLEAR` → `protect_clear_fault()` 能清它；
- 因此该分支的**实际语义**："用户 STOP（清 `s_enable_requested`）→ CLEAR（清锁存）→ 下一 tick 自动回 STANDBY"。恢复链路是**通的**，v1"永久锁死"的旧结论不成立；
- 但它**不是**"无人干预的自动恢复"：只要没人 CLEAR，`latched` 恒 true，分支永远走不到。若设计意图含"过流/过压类软故障延时自恢复"，当前实现是死代码。
- **修法（二选一）**：
  A. 明确"人工确认"语义——保持现状，把注释写清楚"FAULT 需 STOP+CLEAR 才能恢复"，删除恒真的 `!protect_fault_active_hw()` 或在 protect.c 恢复引脚读取后它才有意义；
  B. 增加 `protect_clear_latch()` 接口并区分故障类别——对 OVP/OCP/LCC_CHECK 等硬故障保持人工确认，对 PRECHARGE_TIMEOUT 等时序类故障允许 tick 内延时自恢复。
- ⚠ 关联风险：`protect_clear_fault()` 内 `if (!protect_fault_active_hw())` 守卫因 active_hw 恒真而**无条件清锁存**——将来 protect.c 恢复读引脚后，CLEAR 才重新具备"硬件故障未解除则拒绝清"的语义（见保护模块评审 v3）。

### C1 专节 — 完成判据的真实风险（v2"恒真冗余无害"结论修正）

`:349-350`：
```c
if(condition_held(&s_qualification, s_voltage_reference_v>BATTERY_PRESENT_V
    && g_adc_multi.vbat_v>=(s_voltage_reference_v - CHARGE_FINISH_VOLTAGE_MARGIN_V)
    && g_adc_multi.iout_a >= 0 && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A, ...))
```
- `iout_a >= 0`：`adc_raw_to_current()` 对负值钳 0，此条件恒真，冗余（删不删均可）；
- **真风险**：`iout_a <= CHARGE_FINISH_CURRENT_A(1A)`。电流通道未标定时（NSI1312 V_offset=1.65V 是假设值），`iout_a` 可能恒 0 或严重偏低 → CV 态进入后 30s 去抖一过就误判"充满"→ `FINISHED` 断输出，**充电提前终止且无报错**。
- **要求**：接真电池前必须完成电流点校（ADC 评审 P1-②）；联调阶段若标定未完成，建议临时把 `CHARGE_FINISH_CURRENT_A` 判据旁路或打印 iout 原始值人工确认。

### N1/P1-A 专节 — CC 进入即闭合输出继电器（延后合闸方案）

`:170-175` CC 进入同时 `apply_references + llc_enable + outputs_on`。LLC 刚使能、输出尚未建立就把电池接上，靠模拟 IC 的 CC 限流兜底浪涌。
**建议修法（最小改动）**：enter 里只 `apply_references() + llc_enable()`，把 `outputs_on()` 挪到 tick 内输出建起守卫之后：
```c
case MAIN_STEP_CC:
    if (!outputs_closed &&
        g_adc_multi.vout_v >= OUTPUT_RELAY_MIN_V) {   /* 输出已建立 */
        outputs_on();
        outputs_closed = true;
    }
    ...
```
（`outputs_closed` 随状态 reset；`enter_state` 离开 CC 时复位。）接真电池前 bench 验证一次浪涌波形，二选一：接受现状（CC 限流兜底）或落地延后合闸。

---

## 三、逐行中文注释（当前 370 行，v3 重对）

> 标记：`✅` 亮点 / `⚠` 问题 / `🟢` 确认正确 / `🆕` v3 新发现。

```c
  18  #include "llc_control.h"      // 本模块接口/枚举/宏
  19  #include "adc_dma.h"          // g_adc_multi
  20  #include "condition_held.h"   // 去抖重构（condition_qualification_t）
  21  //#include "can_comm.h"       // N2：CAN 命令改由外部 supervisor API 注入
  22  #include "pfc_control.h"      // pfc_enable/disable/is_ready
  23  #include "protect.h"          // protect_set_fault/latched/active_hw
  24  #include "pwm_llc.h"          // cv/cc_pwm_set_duty

  28  #define BATTERY_PRESENT_V     (10.0f)   // 电池已接入最低电压
  29  #define OUTPUT_RELAY_MIN_V    (10.0f)   // 输出建起门限（兼输出启动超时判据）
  30  #define OUTPUT_START_TIMEOUT_MS (1500U) // ✅ C2 已改名
  32  //#define PFC_READY_TIMEOUT_MS (3000U) // ⚠ N3 死注释：真实定义 main.h:89，建议删
  35  #define CV_REFERENCE_MAX_V    (85.0f)   // 🆕 80→85；注释"344/80V"与 85 不一致，建议重写
  37  #define CC_REFERENCE_MAX_A    (20.0f)   // ⚠ 行尾注释"OCP=30A 超量程"过时（现 OCP=24A 且在量程内）

  55  void llc_enable(void)  { gpio_bit_reset(...); }  // 🆕 低有效：拉低=使能
  61  void llc_disable(void) { gpio_bit_set(...);   }  // 🆕 拉高=关断（fail-safe 极性）
        // ✅ 极性收口到封装；与 main.c:203 初始 llc_disable() 一致

  75  voltage_to_duty()  // 电压→占空比，钳 [CV_MIN, CV_MAX]
  90  current_to_duty()  // 电流→占空比，钳 [CC_MIN, CC_MAX]
 104  apply_references() // 开环设定点：cv/cc_pwm_set_duty 各一次 ✅

 121  static void outputs_force_off(void)
 123     llc_disable();                    // 🆕 封装调用（拉高关 LLC）
 124     cv_pwm_set_duty(PWM_DUTY_SAFE);   // ✅ C5 已修
 125     cc_pwm_set_duty(PWM_DUTY_SAFE);
 126     gpio_bit_reset(OUT_RELAY...);     // 断输出继电器
 127     pfc_disable();                    // 断 PFC
        // ⚠ B1/N-B1：同 tick 齐断，无"先关 LLC→泄放→再断继电器"缓冲
        // ⚠ 函数头注释 :119 "C5：未复位占空比"过时应删
 130  outputs_on()   // 闭继电器+绿亮红灭

 146  static void enter_state(charger_state_t state)
 148     同态幂等守卫 ✅
 152-154  切状态/重置计时/condition_qualification_reset ✅ 防跨状态去抖残留
 158  STANDBY:   outputs_force_off + 红亮绿灭
 165  PRECHARGE: pfc_enable()                      // ✅ C3 已修
 170  CC:        apply_references + llc_enable + outputs_on   // ⚠ N1/P1-A 立即合继电器
 177  CV:        outputs_on()
 182  FINISHED:  outputs_force_off + 绿亮
 189  FAULT:     outputs_force_off + 红亮          // ✅ fail-safe 收敛点

 216  power_supervisor_init()  // 清基准/PWM 置初值/enter(STANDBY)
 236  power_supervisor_request(bool)   // 外部置使能
 262  power_supervisor_set_references(v, a)
 264     f_clampf(0, CV_REFERENCE_MAX_V) / f_clampf(0, CC_REFERENCE_MAX_A)  // 限幅 ✅
 266     apply_references()  // 立即下发（CC 中在线改 setpoint）✅

 281  void power_supervisor_tick_1khz(void)
 284  FAULT 顶部收敛 ✅（B2 专节见上）
 294  switch(g_charger_state):
 296  STANDBY:   en && vbat≥10V → PRECHARGE
 304  PRECHARGE: !en→STANDBY | pfc_is_ready→CC | 3s 超时→FAULT_PRECHARGE_TIMEOUT ✅A1
        // N5：pfc 层同款 3s 超时先触发；N6：故障码无诊断性
 316  CC:  !en→STANDBY | 8h 超时→FAULT_CHARGE_TIMEOUT ✅B3（注释"8s"过时）
        | vbat≥vref−0.5V 去抖 1s→CV（C4 用 vbat）
        | vout≤10V 持续到 1.5s→FAULT_OUTPUT_START_TIMEOUT ✅C2（行尾注释过时）
 337  CV:  !en→STANDBY | 3h 超时→FAULT_CHARGE_TIMEOUT ✅B3（注释"3s"过时）
        | vbat≥vref−0.5V && iout∈[0,1A] 去抖 30s→FINISHED  // ⚠ C1 专节：iout 未标定时风险
 357  FINISHED: !en→STANDBY
 365  default:  →STANDBY  // C6 兜底
```

---

## 四、补丁建议（v3 收敛为两条）

### 补丁 A（N-B1，安全断电时序）——最高剩余风险
`outputs_force_off()` 改为子状态机：立即 `llc_disable()` + PWM 置安全 → 延时 20–50ms 或 `iout_a` 降阈值 → 断 `OUT_RELAY` → `pfc_disable()`。模拟架构下 MCU 是停功率的唯一软件路径，1.5kW 感性负载下继电器带载分断会拉弧/粘连。

### 补丁 B（N6，预充故障细分）——提高联调诊断效率
PRECHARGE 超时分支内按实测值分流：
```c
else if (elapsed_reached(s_state_started_ms, PFC_READY_TIMEOUT_MS)) {
    if (g_adc_multi.ac_vol_v < PFC_AC_INPUT_MIN_V || g_adc_multi.ac_vol_v > PFC_AC_INPUT_MAX_V)
        protect_set_fault(FAULT_AC_RANGE);        /* 建议新增故障码 */
    else if (g_adc_multi.bus_vol_v > VBUS_OVP_V)
        protect_set_fault(FAULT_BUS_OVP);
    else
        protect_set_fault(FAULT_PRECHARGE_TIMEOUT); /* 真超时 */
}
```
pfc 层同款超时建议删除（保留 llc 层一处即可，消除 N5 双层冗余）。

> 旧 v2 的"补丁 1（B1 撤回后不再有 main.c 侧改动）"已不适用；其余 C4/C1/N3 均为小改，随版本节奏清理。

---

## 五、v3 过时注释清单（一次清理）

| 行 | 现状 | 应改为 |
| -- | -- | -- |
| `:32` | 死注释 `#define PFC_READY_TIMEOUT_MS` | 删除 |
| `:35` | 注释"344/80V"与 85.0 不一致 | 写明当前标定依据 |
| `:37` | "OCP=30A 超量程" | OCP 现 24A 且在量程内，删除或更新 |
| `:119` | 函数头"C5：未复位 CV/CC PWM" | 删除（:124-125 已复位） |
| `:324` | "8s(调试值)" | 8h（main.h:104） |
| `:333` | "C2：名实不符…" | 已修复，删除 |
| `:345` | "3s(调试值)" | 3h（main.h:105） |

---

## 六、亮点（v3 保留）

1. **`llc_enable/llc_disable` 极性封装（🆕）**：低有效语义收口到两个函数，main.c 初始关断、CAL 固件强制关断、状态机全部走封装，消除裸 GPIO 极性散布。
2. **`enter_state` 同态幂等守卫 + 一次性动作集中**：enter 范式落地，condition_qualification_reset 防跨状态残留。
3. **FAULT 顶部优先收敛**：fail-safe 到位，配合 STOP+CLEAR 可恢复待机。
4. **PWM 安全复位进 outputs_force_off**（C5）。
5. **去抖重构为 condition_qualification_t**：CC→CV 与完成判据共用、按状态 reset。
6. **CAN 与 supervisor 解耦**：transport-independent API，为 CAN 集成阶段留好接口。

---

## 七、CC/CV 验证分级（v2 内容，v3 更新数值）

> 结论不变：**状态机结构上必须保留 CC/CV；初版联调先跑通 CC + 验证 CV 进入钳位，FINISHED 可暂缓**。数值按 0824 更新：OVP=94V、OCP=24A、CV_REFERENCE_MAX=85V。

| 阶段 | 验证目标 | 配置 | 通过标准 |
| -- | -- | -- | -- |
| ① 先 CC | 功率链路/PFC 建压/保护/采样标定 | 输出接 ≥10V 稳压源，电子负载吸流 | 母线 ≥360V（且 AC∈[180,264]，见 ADC 评审 165V 备注）、CC 进入、输出建起、串口读数与万用表对上 |
| ② 再 CV | 电压钳位 + CV 进入判据 | 负载减流让 vout 顶到 v_ref | 自动切 CV 且钳住，无 OVP 误触发 |
| ③ FINISHED（可暂缓） | 充满判定 | 负载电流 ≤1A | 30s 去抖后 FINISHED 断输出。**前置：电流点校必须完成（C1 专节）** |

**两条红线不变**：CC 硬限流（进入 CC 时给的是电流参考而非电压参考）、CV 进入判据能通（否则电压冲到 94V OVP → 假故障循环）。

---

> v3 总结：模块已解决 A1/A2/B3/C2/C3/C5 共 6 项，极性封装与状态机骨架健壮。**剩余最高风险是 N-B1 断电无缓冲时序**；**接真电池前两个必须项是电流点校（C1/ADC P1-②③）与 N1 浪涌确认**。FAULT 恢复按"STOP→CLEAR→自动回 STANDBY"语义使用，不要期待无人干预自恢复（B2 专节）。
