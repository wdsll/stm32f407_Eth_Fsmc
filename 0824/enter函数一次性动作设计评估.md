# enter 函数「一次性动作」设计评估 —— 0824 vs 0917 vs CAN_LIMA48V10A

> 评估主题：是否应遵循「**只需执行一次的动作放进 enter/transition，不要每个 tick 都执行**」的状态机设计范式。
> 评估对象：0824（当前版本）、0917、CAN_LIMA48V10A 三项目对比。
> 结论一句话：**0824 部分符合（基准下发已正确只做一次），但输出类 GPIO 仍在每个 tick 重复写，应把一次性动作收进 enter_state。功能无害，属设计质量改进，强烈建议对齐 0917 的「幂等 enter + 按状态初始化」范式。**

---

## 1. 设计原则

状态机里每类动作分两种性质：

| 性质 | 例子 | 应该放哪 |
|---|---|---|
| **边沿动作（edge）**：进入某状态才发生一次 | 闭合继电器、使能 LLC_EN、下发 PWM 基准、清零计时器、清积分器、亮 LED | **enter_state / transition 钩子** |
| **电平动作（level）：周期性监控** | 读 ADC、判越限、去抖、超时计时 | **tick 内** |

把边沿动作误放进 tick，会导致：① 每 tick 重复写 GPIO 寄存器（浪费、且让"瞬时诊断动作"无法插入）；② 状态机可读性差，转移逻辑和动作搅在一起；③ 若将来加"进状态先短暂关再开"的消抖/缓冲，会和每 tick 重断言打架。

**注意**：GPIO 置位/复位对同一个值重复写是**幂等**的（写已设的位无副作用），所以 0824 现在的写法**不会引发功能 bug**，只是工程规范性问题 + 埋下将来改动雷。

---

## 2. 0824 现状（当前代码）

### 2.1 enter_state 太瘦（llc_control.c:197-203）
```c
static void enter_state(charger_state_t state)
{
    g_charger_state = state;
    s_state_started_ms = g_ms;      // 只重置了超时用的时间戳
    s_qualification_active = false; // 只清了去抖标志
}
```
**没有**做任何硬件动作（继电器/LLC_EN/PWM/LED）。一次性硬件动作要么散在转移分支里，要么散在每个 tick 里。

### 2.2 已经做对的（只执行一次）✅
- `apply_references()`（下发 PWM 基准）：只在 **PRECHARGE→CC 转移**（:316）和命令 `power_supervisor_set_references()`（:264）调用，**不在 tick 重复**。这点完全正确——模拟 IC 的设定点下发一次即锁存，绝不该每 tick 重写。
- `pfc_enable()`（合 PFC 继电器）：只在 `enable` 为真那一 tick 调一次（:302），之后 STANDBY 的 else 分支只调 `outputs_off()`，不重复合继电器 → PFC 继电器不抖。

### 2.3 每个 tick 重复执行的边沿动作（应改进）⚠
| 位置 | 每 tick 重复写的动作 | 性质 | 危害 |
|---|---|---|---|
| FAULT 分支 :282-292 | `outputs_off()`（断输出继电器+LLC_EN+pfc_disable）每 tick | 边沿动作 | 幂等无害；但 FAULT 失败保护应一次性触发，不该靠"每 tick 兜底" |
| STANDBY else :306 | `outputs_off()` 每 tick（未使能时） | 边沿动作 | 冗余写，PFC/输出已在 OFF，无意义 |
| CC :342-347 | `gpio_bit_set(OUT_RELAY)` + LED 绿/红 每 tick | 边沿动作 | 输出建起后每 tick 重断言继电器引脚；正常无害，但难加"瞬时断开"诊断 |
| CV :368-370 | `gpio_bit_set(OUT_RELAY)` + LED 每 tick | 边沿动作 | 同上 |
| FINISHED :382 | `outputs_off()` 每 tick | 边沿动作 | 冗余写 |
| default :392 | `outputs_off()` 每 tick | 边沿动作 | 冗余写 |

> 作者其实已意识到：`:297` 那行 `//outputs_off();  //C3：每 tick 先全关` 被注释掉了，说明已经发现 STANDBY 每 tick 全关是冗余；但 else 分支（:306）仍在每 tick 调 `outputs_off()`，只是挪了位置，没根治。

### 2.4 关键隐患：FAULT 不是经 enter_state 进入的
`g_charger_state = MAIN_STEP_FAULT` 是 **protect 模块直接置位**的（见 protect.c），**没有调用 `enter_state(FAULT)`**。所以 FAULT 没有"进入钩子"，只能靠 FAULT 分支每 tick 的 `outputs_off()` 兜底断电。
- 这是**安全设计上的双刃剑**：兜底断电保证电源肯定被切断（好），但意味着"一次性失败动作"被迫退化成"每 tick 兜底"。
- 正确做法：让故障入口统一走 `enter_state(FAULT)`（或专用 `enter_fault()`），由 enter 里一次性 `outputs_off()` 切断，tick 只负责"故障解除后回待机"。

---

## 3. 0917 的范式（推荐的对标对象）

### 3.1 pfc_state_enter（pfc_control.c:201-246）—— 幂等 + 按状态一次性初始化
```c
static void pfc_state_enter(pfc_state_t next)
{
    if (s_pfc.state == next) return;     // ★幂等守卫：同状态重复进入无副作用
    s_pfc.state = next;
    s_pfc.entry_ms = g_ms;               // 统一时间戳锚点
    if(next == PFC_ST_IDLE)       { /* 清所有计时器 */ pfc_outputs_off(); }
    else if(next == PFC_ST_RAMP)  { /* 清 vbus/dropout/ac 计时 */ }
    else if(next == PFC_ST_READY) { /* 清 dropout/ac 计时，硬件保持开(由tick控) */ }
    else if(next == PFC_ST_FAULT) { s_pfc.fault_since_ms=g_ms; s_pfc.fault_latched=true;
                                    pfc_outputs_off(); }  // ★故障一次性断电
}
```
要点：
1. **幂等守卫** `if(state==next) return;` —— 防止同一状态被反复"进入"时重复跑初始化（这正是 0824 缺的，0824 的 enter_state 没这守卫）。
2. **一次性硬件动作只在 enter**：IDLE/FAULT 进状态时 `pfc_outputs_off()` 调一次，tick 里不重复。
3. 注释明确"硬件策略：硬件状态由 tick 函数控制，而非状态进入函数"——即**使能类**动作可由 tick 触发，但**关断/清零类**边沿动作进 enter。0824 的 outputs_off 恰好属于"关断类"，应进 enter。

### 3.2 llc_state_enter（llc_control.c:1119-1175）—— 进入即初始化闭环
```c
static void llc_state_enter(llc_state_t next)
{
    if (s_llc_rt.app.state == next) return;   // 幂等守卫
    s_llc_rt.app.state = next;
    s_llc_rt.app.entry_ms = g_ms;
    switch(next){
      case ST_IDLE:        llc_softstart_on_fault(); llc_softstop_reset();
                           llc_pwm_outputs_enable(0); llc_driver_en_set(false);
                           llc_set_freq(s_llc.f_max, true); /* 所有运行时计时器清零 */ break;
      case ST_PRECHECK:    llc_driver_en_set(true); llc_pwm_outputs_enable(0);
                           llc_set_freq(s_llc.f_max, true); break;
      case ST_SOFTSTART:   s_llc_rt.softstart_begin_ms=g_ms; llc_driver_en_set(true);
                           llc_pwm_outputs_enable(1); llc_softstart_start(...); break;
      case ST_LLC_RUN:     llc_driver_en_set(true); s_llc_rt.hold_last_adjust_ms=g_ms;
                           llc_ctrl_bumpless_init(); /* 无扰切换初始化 */ break;
      ...
    }
}
```
要点：**每个状态的硬件使能、频率设定、无扰切换初值、计时器清零，全部在 enter 一次性完成**，tick 只做闭环调节和转段判定。这才是该范式的标准形态。

---

## 4. CAN_LIMA 的范式（边沿检测进入钩子）

无统一 `enter_state()`，靠每个状态块顶部的边沿检测手工模拟：
```c
// charging_Subprocess.c:141-151  PRE 进入
if (nowState != lastState) {   // ★边沿检测，等价 enter 钩子
    lastState = nowState;
    PWM_set(电压基准); PWM_set(电流基准);  // 一次性下发
    preChargingTime = 0; crosscvFlag = 0;   // 一次性清零
}
```
- 一次性 PWM 基准下发、计时清零都在 `if(边沿)` 里跑一次 ✅
- 缺陷：进入钩子是各块重复粘贴的 `if(now!=last)`，易遗漏；SCR/风扇引脚仍在 tick 里重复写（幂等无害）。
- 总体**符合**范式，但工程整洁度不如 0917 的统一 `enter_state`。

---

## 5. 三项目横向对照

| 维度 | 0824 | 0917 | CAN_LIMA |
|---|---|---|---|
| 是否有统一 enter 钩子 | ⚠ 有但极瘦（只存时间戳） | ✅ `pfc_state_enter`/`llc_state_enter` | ⚠ 无，靠每块 `if(边沿)` |
| 幂等守卫 `if(state==next)return` | ❌ 无 | ✅ 有 | ⚠ 边沿检测等价 |
| PWM 基准下发时机 | ✅ 只转移时一次 | ✅ enter 一次 | ✅ 边沿一次 |
| 关断/清零类动作进 enter | ❌ 散在每 tick | ✅ IDLE/FAULT 进 enter | ✅ PRE 进 enter |
| 输出继电器/LED 每 tick 重写 | ⚠ 是（STANDBY/FAULT/FINISHED/CC/CV） | ✅ 否（enter 一次） | ⚠ SCR/风扇引脚每 tick |
| FAULT 失败动作 | ⚠ 每 tick 兜底（FAULT 不经 enter） | ✅ enter 一次性断电 | ✅ 边沿一次 |

---

## 6. 给 0824 的更新方向（P0 设计改进）

### 6.1 强化 enter_state（加幂等守卫 + 按状态一次性硬件动作）
```c
static void enter_state(charger_state_t state)
{
    if (g_charger_state == state) return;   // ★幂等守卫（学 0917）
    g_charger_state = state;
    s_state_started_ms = g_ms;
    s_qualification_active = false;

    switch (state) {
    case MAIN_STEP_STANDBY:
        outputs_off();                      // 进待机：一次性全关
        gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
        gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
        break;
    case MAIN_STEP_PRECHARGE:
        /* PFC 继电器已在 pfc_enable() 合上；此处只清计时 */
        break;
    case MAIN_STEP_CC:
        apply_references();                 // 一次性下发基准（原 :316）
        gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);
        outputs_on();                      // ★新增：闭合输出继电器+绿灯
        break;
    case MAIN_STEP_CV:
        outputs_on();                      // 输出保持合；若修 A2 可在此重下发基准
        break;
    case MAIN_STEP_FINISHED:
        outputs_off();                     // 一次性断开输出
        gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
        gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
        break;
    case MAIN_STEP_FAULT:
        outputs_off();                     // ★故障一次性失败断电（治本：不再靠每 tick 兜底）
        gpio_bit_set(LED_RED_PORT, LED_RED_PIN);
        gpio_bit_reset(LED_GREEN_PORT, LED_GREEN_PIN);
        break;
    default: break;
    }
}
/* 配套新增 */
static void outputs_on(void) {
    gpio_bit_set(OUT_RELAY_PORT, OUT_RELAY_PIN);
    gpio_bit_reset(LED_RED_PORT, LED_RED_PIN);
    gpio_bit_set(LED_GREEN_PORT, LED_GREEN_PIN);
}
```

### 6.2 tick 只留监控 + 转段（清空冗余 GPIO 写）
- **STANDBY**：删 `else { outputs_off(); }`（进 STANDBY 已一次性关）。只判 `s_enable_requested && vbat>=10V → pfc_enable()+enter_state(PRECHARGE)`。
- **CC/CV**：删 `gpio_bit_set(OUT_RELAY)` + LED 那几行（已在 enter 一次性做）。tick 只做超时判据、CC→CV 去抖、充满去抖、使能撤销。
- **FAULT 分支**：删每 tick `outputs_off()`，改由 `enter_state(FAULT)` 一次性断电；tick 只判"故障清除 → 回 STANDBY"。
- **FINISHED**：删每 tick `outputs_off()`（enter 已做）。

### 6.3 让故障统一走 enter（治本关键）
当前 `protect.c` 直接置 `g_charger_state = FAULT` 而不调 `enter_state(FAULT)`，导致无进入钩子。建议：
- 在 `protect_set_fault()` 里（或 charger 模块订阅故障时）调用 `enter_state(MAIN_STEP_FAULT)`，把"失败断电"收敛到 enter 一次性执行；
- 这样 FAULT 分支的每 tick `outputs_off()` 可删，且失败动作语义更清晰（与 0917 的 `pfc_state_enter(FAULT)→pfc_outputs_off()` 完全对齐）。

---

## 7. 收益与风险

**收益**
- 与 0917/CAN_LIMA 工程范式一致，可维护、可移植；
- 转移逻辑与硬件动作解耦，将来加"断电缓冲时序"（P0 安全项）、"瞬断诊断"不会和每 tick 重断言冲突；
- 消除每 tick 无意义的 GPIO 寄存器写（1kHz 下量极小，但规范即收益）。

**风险/注意**
- ⚠ **不要**把"失败保护"从兜底改成纯 enter 后丢失冗余：功率产品建议保留 `enter_state(FAULT)` 一次性断电 **+** 在 FAULT tick 里保留一句廉价的 `outputs_off()` 兜底（双保险）。0917 靠 `pfc_state_enter(FAULT)` 一次性断电，但 0824 的故障来源更多（protect 模块多处），统一收口前保留兜底更安全。
- `apply_references()` 保持"只转移时一次 + 命令时一次"，**绝不要**挪进 tick（模拟 IC 设定点已锁存，重复写反而可能引入扰动）。

---

## 8. 结论
0824 当前**基准下发已正确只做一次**（符合范式），但**输出继电器 / LLC_EN / LED 这些 GPIO 仍在每个 tick 重复写**。功能上幂等无害，但不符合「一次性动作进 enter」的规范，且 FAULT 因不经 enter 只能靠每 tick 兜底断电。

建议按 6.1–6.3 强化 `enter_state`（加幂等守卫 + 按状态一次性硬件动作），并让故障统一经 enter 收敛——这正好对齐 0917 的 `pfc_state_enter`/`llc_state_enter` 范式，也是 CAN_LIMA 边沿检测进入钩子的同义实现。属**设计质量改进，非功能 bug**，初版联调不受影响，可作为下一轮代码清理项。
