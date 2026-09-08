# power_supervisor_tick_1khz 状态机评估 v2

> 评估对象：`0824/HW/src/llc_control.c` 中的充电主状态机
> （`power_supervisor_tick_1khz()` + `enter_state()` + `outputs_force_off()`，1kHz 由主循环调用）
> 交叉核对：`protect.c`、`pfc_control.c`、`serial_console.c`、`main.c`、`adc_dma.c`、`condition_held.h`
> **v1：2026-08-19 ｜ v2 刷新：2026-09-02**

---

## 0. 修订记录（v1 → v2）

| 编号 | v1（08-19）结论 | v2（09-02）复核结果 |
|---|---|---|
| **2.4 FAULT 恢复** | ✅ "已修复，恢复条件完整" | ❌ **v1 结论错误**。恢复条件存在循环依赖，自动恢复是**死代码**，见 P0-A |
| **B3 超时注释** | 🔴 必须改 | ⚠️ **仍未改**，且新增一处过期注释（C3/OCP 30A） |
| **C1 `iout_a >= 0`** | "恒真冗余，无害" | ⚠️ **判断错误**。iout 未标定时该判据既可能恒假也可能误真，见 P1-C |
| **standalone 隔离** | v1 未涉及 | 🆕 `main.c:154-157` 的跳过逻辑**已被注释掉**，行为改变，见 P1-D |
| S3 / S4 / A1 / B2 | ✅ 已修 | ✅ 复核仍成立（`enter_state` 范式确实解决了同 tick 抖动问题） |
| C4 vbat 判据 | 设计选择 | ✅ 维持：模拟 IC 方案下用电池端电压是正确选择 |

---

## 1. 一句话结论（v2）

**骨架质量比 v1 时期更好**（`enter_state` 一次性动作范式 + 幂等守卫 + 去抖自动重置，写得干净），但 **FAULT 态的自动恢复分支永远进不去**——故障只能靠串口 `CLEAR` 人工解除，且这段代码会给人"已有自动恢复"的错觉。此外 `pfc_is_ready()` 是三重门，母线/AC 任一越界都会表现为"预充 3s 超时"，**故障码不具备诊断性**。初版联调可用，但 P0-A 建议先修。

---

## 2. 逐行注释

### 2.1 `power_supervisor_tick_1khz()`（L270-358）

```c
void power_supervisor_tick_1khz(void)
{
    if (g_charger_state == MAIN_STEP_FAULT) {          /* L273 FAULT 前置处理，置于 switch 之前 */
        outputs_force_off();                            /* L274 每 1ms tick 全关（兜底，但浪费，见 P2-A） */

        if(!s_enable_requested && !protect_fault_latched() && !protect_fault_active_hw())
        {                                               /* L276 ❌ P0-A：循环依赖，永不成立 */
            g_fault = FAULT_NONE;                       /* L278 实际永不执行 */
            enter_state(MAIN_STEP_STANDBY);             /* L279 实际永不执行 */
        }
        return;                                         /* L281 FAULT 态不再走 switch，正确 */
    }
```

**P0-A 详解**：`protect_fault_latched()` 返回 `s_fault_latched`（`protect.c:63`），该标志：
- 置 true：仅在 `protect_set_fault()`（`protect.c:80`）
- 置 false：仅在 `protect_clear_fault()`（`protect.c:73`），且需外部调用

而**进入 FAULT 态的必经路径就是 `protect_set_fault()` → `power_supervisor_enter_fault()`**。所以只要进了 FAULT，`s_fault_latched` 必为 true，L276 的 `!protect_fault_latched()` 恒 false → **整个恢复分支是死代码**。
唯一出口：串口 `CLEAR` 命令（`serial_console.c:241`）或上电自检（`main.c:187`）调 `protect_clear_fault()`。

```c
    switch(g_charger_state)
    {
        case MAIN_STEP_STANDBY:                         /* L285 */
        {
            if(s_enable_requested && g_adc_multi.vbat_v >= BATTERY_PRESENT_V)  /* L287 10V */
            {
                enter_state(MAIN_STEP_PRECHARGE);       /* L289 pfc_enable() 在 enter 内一次性执行 */
            }
            break;                                      /* L291 不满足则停留，无副作用（正确） */
        }
```
✅ 判据合理：没接电池（vbat < 10V）永不启动。⚠️ 只接电子负载会一直卡这里，联调需用"稳压源+负载"模拟电池。

```c
        case MAIN_STEP_PRECHARGE:                       /* L293 */
        {
            if (!s_enable_requested) {                  /* L295 撤使能 */
                 enter_state(MAIN_STEP_STANDBY);        /* L296 → enter 内 outputs_force_off() 断 PFC */
             } else if (pfc_is_ready()) {               /* L297 三重门，见 P0-B */
                 enter_state(MAIN_STEP_CC);
             }
            else if (elapsed_reached(s_state_started_ms, PFC_READY_TIMEOUT_MS)) {  /* L300 3s */
                protect_set_fault(FAULT_PRECHARGE_TIMEOUT);  /* L301 A1 已修：不再永久卡死 */
             }
             break;
        }
```
✅ A1（v1 的"永久卡死"）确认已修。
❌ **P0-B**：`pfc_is_ready()` 要求同时满足三件事（`pfc_control.c:82-90`）：
```c
(s_pfc_state == PFC_STATE_RUN)          /* 需 bus>=350 由 pfc_tick 推进 */
&& (180V <= ac_vol_v <= 264V)           /* AC 窗口 */
&& (360V <= bus_vol_v <= 420V)          /* 母线窗口 */
```
任一不满足 → `is_ready=false` → 卡在 PRECHARGE → 3s 后一律报 `FAULT_PRECHARGE_TIMEOUT`。
**后果**：母线真的过压（>420V）、AC 真的超限（>264V 或 <180V）、母线停在 350~360V 之间——三种完全不同的故障，报的是同一个码，**无法区分**。而 `FAULT_BUS_OVP` 全工程从未被 set（见 main.h 评审 P1-2）。

```c
        case MAIN_STEP_CC:                              /* L305 */
        {
            if(!s_enable_requested) { enter_state(MAIN_STEP_STANDBY); }        /* L307-310 */
            else if(elapsed_reached(s_state_started_ms,CHARGE_CC_TIMEOUT_MS))  /* L311 */
            {
                protect_set_fault(FAULT_CHARGE_TIMEOUT);  /* L313 ⚠️ 注释写"8s(调试值)"已过时，实际 8h */
            }
            else if(condition_held(&s_qualification,
                    s_voltage_reference_v > BATTERY_PRESENT_V &&
                    g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_CV_ENTRY_MARGIN_V),
                    CHARGE_CV_ENTRY_DEBOUNCE_MS))       /* L316 去抖 1s，判据用 vbat（C4，设计选择） */
            {
                enter_state(MAIN_STEP_CV);              /* L318 */
            }
            else if (g_adc_multi.vout_v <= OUTPUT_RELAY_MIN_V &&
                     elapsed_reached(s_state_started_ms, OUTPUT_START_TIMEOUT_MS))  /* L320 1.5s */
            {
                protect_set_fault(FAULT_OUTPUT_START_TIMEOUT);  /* L322 S4 已修：专用故障码 */
            }
            break;
        }
```
✅ S4（v1 的"名实不符"）确认已修。
⚠️ **P1-B**：`s_voltage_reference_v > BATTERY_PRESENT_V(10V)` 这个前置条件依赖外部先 `SET`。`serial_console.c:216` 的 START 守卫已保证 `voltage_reference >= 10V` 才允许启动 ✅，所以实际不会卡死——但这是靠命令层守卫兜的，状态机本身没有防御。
⚠️ **P1-A**：L320 的输出建立检测在**继电器已闭合之后**执行（`enter_state(CC)` 里立即 `outputs_on()`），若电池反灌使 `vout_v > 10V`，此超时检测失效。

```c
        case MAIN_STEP_CV:                              /* L326 */
        {
            if(!s_enable_requested) { enter_state(MAIN_STEP_STANDBY); }        /* L328-331 */
            else if(elapsed_reached(s_state_started_ms,CHARGE_CV_TIMEOUT_MS))  /* L332 */
            {
                protect_set_fault(FAULT_CHARGE_TIMEOUT);  /* L334 ⚠️ 注释写"3s"已过时，实际 3h */
            }
            else
            {
                if(condition_held(&s_qualification,
                    s_voltage_reference_v > BATTERY_PRESENT_V &&
                    g_adc_multi.vbat_v >= (s_voltage_reference_v - CHARGE_FINISH_VOLTAGE_MARGIN_V)
                    && g_adc_multi.iout_a >= 0 && g_adc_multi.iout_a <= CHARGE_FINISH_CURRENT_A,
                    CHARGE_FINISH_DEBOUNCE_MS))          /* L338-339 30s 去抖，截止电流 1A */
                {
                    enter_state(MAIN_STEP_FINISHED);     /* L341 */
                }
            }
            break;
        }
```
⚠️ **P1-C**：`g_adc_multi.iout_a >= 0` 这个条件在 v1 里被判为"恒真冗余、无害"，**v2 认为判断有误**。`iout_a` 来自 NSI1312 隔离运放，输出带 Vref 偏置，`adc_raw_to_current()` 需减去偏置才能得到真值。在运放偏置**未标定**的情况下：
- 若偏置偏大 → 真实 5A 时读数可能是负值 → `iout >= 0` 为假 → **永不进 FINISHED**；
- 若偏置偏小 → 真实 5A 时读数可能是 0.2A → 误判 `< 1A` → **提前误报充满**。
两种情况在初版联调都会出现，且第二种有安全隐患（电池未充满就结束）。建议先标定 NSI1312 偏置，再启用 FINISHED 判据。

```c
        case MAIN_STEP_FINISHED:                        /* L346 */
        {
            if(!s_enable_requested) { enter_state(MAIN_STEP_STANDBY); }  /* L348-351 */
            break;
        }
        default:
            enter_state(MAIN_STEP_STANDBY);             /* L355 C6：TRICKLE/INVALID 兜底 */
            break;
    }
}
```
✅ `MAIN_STEP_INIT` 与 `MAIN_STEP_TRICKLE` 都靠 default 收敛到 STANDBY，无悬空态。

### 2.2 `enter_state()`（L135-188）——本次评估中写得最好的部分

```c
static void enter_state(charger_state_t state)
{
    if (g_charger_state == state) { return; }   /* L137-139 幂等守卫：重复进入不重复执行一次性动作 */
    g_charger_state = state;                    /* L141 */
    s_state_started_ms = g_ms;                  /* L142 状态计时重置 */
    condition_qualification_reset(&s_qualification);  /* L143 去抖重置，防跨状态残留 ✅ */
```
✅ **这是正确的设计**：把"一次性动作"与"每 tick 轮询判据"分离，解决了 v1 里的 S3 同 tick 断-合抖动问题。`condition_qualification_reset` 在每个状态入口重置，避免上一个状态的去抖计数污染下一个状态——这点很多项目会漏。

```c
    switch(state)
    {
        case MAIN_STEP_STANDBY:  outputs_force_off(); 红灯亮/绿灯灭; break;   /* L147-153 */
        case MAIN_STEP_PRECHARGE: pfc_enable(); break;                        /* L154-158 C3 已修 */
        case MAIN_STEP_CC:
            apply_references();                         /* L161 仅此处下发一次基准 */
            gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);    /* L162 开 LLC（低电平使能） */
            outputs_on();                               /* L163 ❌ P1-A：立即闭合输出继电器 */
            break;
        case MAIN_STEP_CV: outputs_on(); break;         /* L166-170 保持 */
        case MAIN_STEP_FINISHED: outputs_force_off(); 绿亮; break;  /* L171-177 */
        case MAIN_STEP_FAULT: outputs_force_off(); 红亮; break;     /* L178-184 fail-safe 收敛点 */
    }
}
```
⚠️ **P1-A（N1）**：`enter_state(CC)` 里三个动作在同一 tick 完成——下发基准 → 开 LLC → 合输出继电器。**LLC 输出尚未建立就闭合继电器**，若此时电池电压高于 LLC 输出，会有反灌电流；且 L320 的输出建立超时检测因此失效（继电器已合，vout 被电池钳住）。
建议：CC 入口只开 LLC，等 `vout_v >= OUTPUT_RELAY_MIN_V` 持续若干 ms 后再 `outputs_on()`。

⚠️ **A2 注释与实现不符**（v1 已指出，仍未改）：文件头 L7-8 写"CV 模式: CV_PWM=目标电压, CC_PWM=电流上限"，但 `enter_state(CV)` **不调 `apply_references()`**，CC/CV 共用进入 CC 时下发的一组基准。对模拟 IC 方案这是正确做法（NCP4390 内部自己切环），但注释会误导维护者去"补"一个不存在的切换。

### 2.3 `outputs_force_off()`（L110-117）

```c
static void outputs_force_off(void)
{
    gpio_bit_set(LLC_EN_PORT, LLC_EN_PIN);      /* L112 关 LLC 使能（高=关） */
    cv_pwm_set_duty(PWM_DUTY_SAFE);             /* L113 C5 已修：PWM 归零 */
    cc_pwm_set_duty(PWM_DUTY_SAFE);             /* L114 */
    gpio_bit_reset(OUT_RELAY_PORT, OUT_RELAY_PIN);  /* L115 断输出继电器 */
    pfc_disable();                              /* L116 断 PFC 继电器 */
}
```
✅ C5（v1 的"断电不复位 PWM"）已修。
⚠️ **B1（v1 已记录）**：五路动作同 tick 齐发，无"先关 LLC → 延时放电 → 再断输出继电器"的缓冲时序。初版低压小电流可接受，量产前需加。

### 2.4 `power_supervisor_init()`（L205-215）

```c
s_enable_requested = false;         /* ✅ 上电不自动充电 */
s_voltage_reference_v = 0.0f;       /* ✅ 无默认目标，必须外部 SET */
s_current_reference_a = 0.0f;
cv_pwm_set_duty(CV_PWM_DUTY_INIT);  /* 5% */
cc_pwm_set_duty(CC_PWM_DUTY_INIT);  /* 2% */
enter_state(MAIN_STEP_STANDBY);     /* ✅ 进待机并执行 outputs_force_off() */
```
✅ **上电默认安全**：不自动请求充电、基准为 0、进 STANDBY 即关断全部功率。这比 0917 的"上电无条件 `pfc_enable()`"更适合 bench。

---

## 3. 状态转移表（v2 更新）

| 当前态 | 转移条件 | 入口一次性动作 | 目标态 |
|---|---|---|---|
| STANDBY | `enable && vbat ≥ 10V` | `pfc_enable()` | PRECHARGE |
| STANDBY | 否则 | — | 停留 |
| PRECHARGE | `!enable` | `outputs_force_off()` | STANDBY |
| PRECHARGE | `pfc_is_ready()`（RUN ∧ AC∈[180,264] ∧ bus∈[360,420]） | `apply_ref` + 开 LLC + **合输出继电器** | CC |
| PRECHARGE | 3s 超时 | `set_fault(PRECHARGE_TIMEOUT)` | FAULT |
| CC | `!enable` | `outputs_force_off()` | STANDBY |
| CC | 8h 超时 | `set_fault(CHARGE_TIMEOUT)` | FAULT |
| CC | `vbat ≥ Vref−0.5` 持续 1s | `outputs_on()` | CV |
| CC | `vout ≤ 10V` 且已过 1.5s | `set_fault(OUTPUT_START_TIMEOUT)` | FAULT |
| CV | `!enable` | `outputs_force_off()` | STANDBY |
| CV | 3h 超时 | `set_fault(CHARGE_TIMEOUT)` | FAULT |
| CV | `vbat ≥ Vref−0.5 ∧ 0 ≤ iout ≤ 1A` 持续 30s | `outputs_force_off()` + 绿灯 | FINISHED |
| FINISHED | `!enable` | — | STANDBY |
| FAULT | **❌ 自动恢复分支永不执行** | 每 tick `outputs_force_off()` | （只能人工 `CLEAR`） |
| default（INIT/TRICKLE/INVALID） | 任意 | `outputs_force_off()` | STANDBY |

---

## 4. 缺陷清单

| 编号 | 级别 | 问题 | 位置 | 后果 |
|---|---|---|---|---|
| **P0-A** | 🔴 | FAULT 自动恢复条件循环依赖（要求 `!latched` 才清 latch） | `llc_control.c:276-280` | 死代码，故障只能人工 `CLEAR`；给人"已有自动恢复"的错觉 |
| **P0-B** | 🔴 | `pfc_is_ready()` 三重门共用一个超时故障码 | `pfc_control.c:82-90` | 母线过压 / AC 超限 / 母线不足三种故障都报 `PRECHARGE_TIMEOUT`，无诊断性 |
| **P1-A** | 🟠 | CC 入口立即闭合输出继电器，未等输出建立 | `llc_control.c:163` | 电池反灌风险；并使 L320 输出建立超时检测失效 |
| **P1-B** | 🟠 | CC→CV 判据依赖 `s_voltage_reference_v > 10V`，状态机自身无防御 | `llc_control.c:316` | 靠 `serial_console` 的 START 守卫兜底，换 CAN 入口即暴露 |
| **P1-C** | 🟠 | FINISHED 判据的 `iout_a >= 0` 在运放未标定时不可靠 | `llc_control.c:339` | 恒假→永不结束；误真→未充满就结束 |
| **P1-D** | 🟠 | `main.c:154-157` standalone 跳过逻辑被注释掉 | `main.c:154-157` | 串口 `PFC ON` 手动测试时，任何故障会每 tick 强制 `pfc_disable()` 打断测试 |
| **P2-A** | 🟡 | FAULT 态每 1ms tick 执行 `outputs_force_off()`（4 GPIO + 2 PWM） | `llc_control.c:274` | 功能幂等但浪费，建议移入 enter 一次性执行 |
| **P2-B** | 🟡 | 三处注释与代码不符：CC/CV 超时（写 8s/3s，实为 8h/3h）、OCP（写 30A，实为 24A） | `llc_control.c:313,334,37` | v1 已指出，仍未改 |
| **P2-C** | 🟡 | A2 文件头注释称"CV 切换基准"，实现不切换 | `llc_control.c:7-9` | 误导维护者去补一个不存在的切换 |
| **P3-A** | ⚪ | `enter_state` 幂等守卫使重复 `enter_fault` 不执行关断 | `llc_control.c:137` | 已被 L274 每 tick 兜底覆盖，当前无害 |

---

## 5. 关键补丁

### 5.1 P0-A：FAULT 恢复条件

**问题代码**：
```c
if(!s_enable_requested && !protect_fault_latched() && !protect_fault_active_hw())
{
    g_fault = FAULT_NONE;
    enter_state(MAIN_STEP_STANDBY);
}
```

**修法一（最小改动，推荐）**：去掉 `!protect_fault_latched()`，改为显式清锁存。需在 `protect.h` 暴露一个清锁存接口：
```c
/* protect.c 新增 */
void protect_clear_latch(void)
{
    s_fault_latched = false;
    g_fault = FAULT_NONE;
}
```
```c
/* llc_control.c:276-280 替换为 */
if (!s_enable_requested && !protect_fault_active_hw()) {
    protect_clear_latch();            /* 先清软件锁存 */
    enter_state(MAIN_STEP_STANDBY);   /* 再回待机（enter 内会 force_off） */
}
```

**修法二（保留人工确认语义）**：若产品要求"故障必须人工确认才能恢复"，则直接**删掉这个分支**，并加注释说明，避免留下永不执行的死代码：
```c
if (g_charger_state == MAIN_STEP_FAULT) {
    /* 故障需人工确认：STOP 后发 CLEAR（见 serial_console） */
    return;
}
```
> 注意：若采用修法二，**必须**把 L274 的 `outputs_force_off()` 移入 `enter_state(FAULT)`（否则 return 后就没人关功率了——虽然 enter 里已有关断，但重复故障场景会漏）。

### 5.2 P0-B：让预充故障可诊断

在 `pfc_control.c` 增加细分故障上报，或在 `power_supervisor` 的 PRECHARGE 超时分支里判定根因：
```c
else if (elapsed_reached(s_state_started_ms, PFC_READY_TIMEOUT_MS)) {
    if (g_adc_multi.bus_vol_v > VBUS_OVP_V) {
        protect_set_fault(FAULT_BUS_OVP);              /* 母线过压 */
    } else if ((g_adc_multi.ac_vol_v < PFC_AC_INPUT_MIN_V) ||
               (g_adc_multi.ac_vol_v > PFC_AC_INPUT_MAX_V)) {
        protect_set_fault(FAULT_HARDWARE_PRO);         /* AC 越限（或新增专用码） */
    } else {
        protect_set_fault(FAULT_PRECHARGE_TIMEOUT);    /* 真·建压超时 */
    }
}
```

### 5.3 P1-A：CC 入口延后闭合输出继电器

```c
case MAIN_STEP_CC:
    apply_references();
    gpio_bit_reset(LLC_EN_PORT, LLC_EN_PIN);   /* 只开 LLC */
    /* 暂不 outputs_on()：等输出建立 */
    break;
```
并在 tick 的 CC 分支里加：
```c
else if (g_adc_multi.vout_v >= OUTPUT_RELAY_MIN_V &&
         !s_output_relay_closed) {
    outputs_on();                              /* 输出建立后再合继电器 */
    s_output_relay_closed = true;
}
```
配合 L320 的 1.5s 超时，逻辑就自洽了（超时前未建立 → 报 `FAULT_OUTPUT_START_TIMEOUT`）。

---

## 6. 与 PFC 子状态机的配合（v2 更新）

- 主从关系不变：本状态机通过 `pfc_enable()`（STANDBY→PRECHARGE）与 `pfc_is_ready()`（PRECHARGE→CC）驱动 PFC 子状态机，无竞态。
- **PFC 子状态机 `RUN` 态仍是 OVP 空壳**（`pfc_control.c:69-72` 只有注释"加入OVP"），母线过压只能靠 `pfc_is_ready()` 的 `bus <= 420V` 被动体现为"not ready"，**不会主动 set `FAULT_BUS_OVP`**。这是 P0-B 的根因。
- 🆕 **P1-D 新情况**：`main.c:154-157` 的 `//if (!serial_console_pfc_test_active())` 已被注释，**`power_supervisor_tick_1khz()` 现在无条件执行**。因此：
  - 串口 `PFC ON` 做 standalone 母线测试时，若触发任何故障 → 进 FAULT → 每 tick `pfc_disable()` → **手动闭合的继电器被强制断开**，测试中断；
  - 反过来说，这也意味着"故障必断 PFC"这条安全链现在是通的（v1 时期不通）。**安全上变好，可测试性变差**——建议 standalone 测试时先确认无故障锁存。

---

## 7. 初版联调前置条件（v2 更新）

1. **模拟电池 ≥ 10V**：`vbat_v >= BATTERY_PRESENT_V` 是 STANDBY→PRECHARGE 的唯一入口。只接电子负载（0V）会永远停在 STANDBY。
2. **母线稳态落在 [360V, 420V]**：下限由 `LLC_ENTRY_V` 定，上限由 `VBUS_OVP_V` 定。母线由 NCP1654 硬件反馈决定（MCU 不可调，见 main.h 评审 P0-2），需先确认硬件设定值落在窗内，否则必然 3s 超时。
3. **AC 采样落在 [180V, 264V]**：⚠️ 你实测的 **165V 低压工况会被判 AC 欠压**，`pfc_is_ready()` 返回 false → 预充超时。若要在 165V 联调，需临时下调 `PFC_AC_INPUT_MIN_V`。
4. **先 `SET` 再 `START`**：`SET` 范围 10~80V / 0~20A，`START` 会校验 `voltage_reference ≥ 10V`。
5. **NSI1312 偏置标定后再验证 FINISHED**：`iout_a` 未标定时，充满判据不可信（P1-C）。初版只验证到 CV 即可。
6. **5s 看门狗**：`serial_console.c` 使能后 5s 无命令自动 STOP，长时间联调需周期 `PING`。

---

## 8. 修复优先级

- **P0-A**：FAULT 恢复——按 §5.1 二选一（推荐修法一，保留自动恢复能力）
- **P0-B**：预充故障细分——§5.2
- **P1-A**：CC 入口延后合继电器——§5.3（涉及功率安全，建议联调前改）
- **P1-C**：标定 NSI1312 偏置后再启用 FINISHED 判据
- **P2-B**：删掉三处过时注释（一行改动，防误判）

---

*附：`power_supervisor_tick_1khz_can()`（CAN 版）仍整段 `#if 0` 未启用，不影响 bench 联调。*
