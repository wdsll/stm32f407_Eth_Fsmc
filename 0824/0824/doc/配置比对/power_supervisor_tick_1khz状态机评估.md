# power_supervisor_tick_1khz 状态机评估（初版软硬联调视角）

> 评估对象：`0824/HW/src/llc_control.c` 中的 `power_supervisor_tick_1khz()`（充电主状态机，每 1ms 由 SysTick 调用）
> 目标：第一版软硬联调能不能跑、状态切换是否合理（不要求功能完全）
> 日期：2026-08-19

---

## 0. 一句话结论

**能适应第一版软硬联调。** 状态机结构完整、无永久卡死分支、FAULT 可远程恢复、驱动链路（串口 SET/START/STOP/CLEAR）通畅。相比早期版本，A1/S3/S4/B2/B3 几处问题**已修复**；残留项对初版联调影响很小。唯一需要立刻改的是 **两处过期注释**（会让维护者误判超时值）。

---

## 1. 当前实际状态转移表（已按源码逐行确认）

| 当前态 | 转移条件 | 动作 | 目标态 |
|---|---|---|---|
| STANDBY | `enable && vbat>=10V` | `pfc_enable()` | PRECHARGE |
| STANDBY | 否则 | `outputs_off()` | 停留 STANDBY |
| PRECHARGE | `!enable` | `outputs_off()` | STANDBY |
| PRECHARGE | `pfc_is_ready()`（母线≥360V） | `apply_references()` + `LLC_EN=1` | CC |
| PRECHARGE | 3s 超时（`PFC_READY_TIMEOUT_MS`） | `protect_set_fault(FAULT_PRECHARGE_TIMEOUT)` | FAULT |
| CC | `!enable` | `outputs_off()` | STANDBY |
| CC | 8h 超时 | `FAULT_CHARGE_TIMEOUT` | FAULT |
| CC | 电压到达（去抖 1s，见 C4） | — | CV |
| CC | `vout>10V` | 合输出继电器 + 绿灯 | 停留 CC |
| CC | 1.5s 输出未建立 | `FAULT_OUTPUT_START_TIMEOUT` | FAULT |
| CV | `!enable` | `outputs_off()` | STANDBY |
| CV | 3h 超时 | `FAULT_CHARGE_TIMEOUT` | FAULT |
| CV | 电压达标 + 电流≤1A（去抖 30s） | `outputs_off()` | FINISHED |
| FINISHED | `!enable` | — | STANDBY |
| FAULT | `!enable && !latched && !hw_fault` | — | STANDBY |
| default（TRICKLE/INVALID） | 任意 | `outputs_off()` | STANDBY |

---

## 2. 相比早期版本，已修复的逐项确认 ✅

| 问题 | 早期结论 | 当前状态 |
|---|---|---|
| **A1** PRECHARGE 永久卡死 | 无超时分支 | ✅ 已修：`llc_control.c:275-277` 加 3s 超时进 FAULT，`PFC_READY_TIMEOUT_MS=3000U`(`main.h:90`) 有效 |
| **S3** STANDBY 同 tick 断-开抖动 | `outputs_off()` 每 tick 先全关又立刻 `pfc_enable` | ✅ 已修：`llc_control.c:252` 的 `outputs_off()` 注释掉，改到 `else` 分支(`:261`)，不再每 tick 断-合 PFC 继电器 |
| **S4** 故障码名实不符 | 输出超时错报 `FAULT_BUS_UVP` | ✅ 已修：`:305` 改报专用码 `FAULT_OUTPUT_START_TIMEOUT` |
| **B2** FAULT 不可恢复 | FAULT 在 CAN 处理后 return，远程清不掉 | ✅ 已修：FAULT 处理移到 tick **顶部**(`llc_control.c:237-247`)，且恢复条件完整 |
| **B3** CC/CV 超时误用 8s/3s | 注释称调试值 | ⚠️ 值已对：`CHARGE_CC_TIMEOUT_MS=8h`、`CHARGE_CV_TIMEOUT_MS=3h`(`main.h:109-110`)，**但注释没改，见 §4** |
| **2.4** FAULT 永久锁死 | CLEAR 只清硬件锁不踢状态机 | ✅ 已不成立：`protect_clear_fault_async()` 清软件锁 + `!protect_fault_latched() && !protect_fault_active_hw()` 在 tick 顶部回 STANDBY |

FAULT 退出依赖的 4 个函数均存在于 `protect.h`（`protect_fault_active_hw` / `protect_fault_latched` / `protect_clear_fault_async` / `protect_set_fault`），链路通畅；串口 `CLEAR` 调 `protect_clear_fault_async()` 即可清锁。

---

## 3. 仍存在的残留项（初版联调影响小）

| 编号 | 位置 | 描述 | 初版影响 |
|---|---|---|---|
| **C4** | `:293`、`:326` | CC→CV 判据用 `vbat_v` 而非 `vout_v`。**但逻辑自洽**：充电机关心的是电池端电压，继电器合上后 `vout≈vbat`；CV 完成判据也用 `vbat`，两处一致。属设计选择，非 bug | 无。若产品定义要求看输出端 `vout`，再统一 |
| **A2** | `:6-9` vs `:271` | 头注释写"CV 应切换基准"，但 `apply_references()` 只在 PRECHARGE→CC 调一次，CC/CV 共用设定点。实际靠模拟 IC 双环自动切换 CC/CV | 无（模拟 IC 负责切换），注释误导 |
| **B1** | `:113-118` | `outputs_off()` 同 tick 同步断 LLC_EN + 输出继电器 + PFC，无"先关 LLC→放电→断继电器"时序 | 初版低压小电流风险低；量产前需加缓冲 |
| **C1** | `:327` | `iout_a >= 0` 恒真冗余 | 无害 |
| **C5** | `:113-118` | `outputs_off()` 不复位 PWM 占空比。但 PRECHARGE→CC 的 `apply_references()`(`:271`) 与每次 `set_references()` 都会刷新，风险被覆盖 | 无 |

---

## 4. 🔴 必须改：两处过期注释（会误导维护者）

`llc_control.c` 中超时注释仍是旧值，与实际宏定义矛盾：

```c
// :290（CC 超时分支）
protect_set_fault(FAULT_CHARGE_TIMEOUT);   //B3：CC 超时=8s(调试值)，正常应 8h
// 实际：CHARGE_CC_TIMEOUT_MS = 8U*60U*60U*1000U = 8 小时  ✅

// :318（CV 超时分支）
protect_set_fault(FAULT_CHARGE_TIMEOUT); //B3：CV 超时=3s(调试值)，正常应 3h
// 实际：CHARGE_CV_TIMEOUT_MS = 3U*60U*60U*1000U = 3 小时  ✅
```

**建议**：删除 `//B3：...调试值...` 旧注，改为 `// 8h 超时保护（main.h: CHARGE_CC_TIMEOUT_MS）` 之类，避免被误认为还是调试值。

---

## 5. 初版联调前置条件（非状态机缺陷，是模拟前端条件）

1. **电池端 `vbat ≥ 10V`**：只接电子负载（零电压源）会永远停在 STANDBY。用「稳压电源 + 电子负载」当模拟电池。
2. **PFC 母线稳态 ≥ 360V**（`LLC_ENTRY_V`）：NCP1654 反馈网络须把母线调到 ≥365V，否则 PRECHARGE 3s 超时锁 FAULT。
3. **5s 看门狗**：`serial_console.c` 中使能后 5s 无命令自动 STOP，长时间联调需周期 PING。
4. **FINISHED 需电流 ≤1A 持续 30s**：满电流联调停在 CV 即可，不要求真充满。

---

## 6. 修复优先级（针对本状态机）

- **P0**：改 §4 两处过期注释（一行注释，防误判）
- **P1**：统一 C4 的 `vbat`/`vout` 判据口径（需产品定义确认，非必须）
- **P2**：B1 安全断电时序、A2 注释与实现对齐（量产前）

---

## 7. 与 PFC 子状态机（`pfc_tick_1khz`）的配合

- 本状态机通过 `pfc_enable()`（STANDBY→PRECHARGE）和 `pfc_is_ready()`（PRECHARGE→CC）驱动 PFC 子状态机。
- 本状态机 `outputs_off()` 调 `pfc_disable()` 断 PFC 继电器，**与 PFC 子状态机是主从关系**，无竞态。
- PFC 子状态机 `RUN` 态为 OVP 空壳（见 PFC 评审），但本状态机 CC/CV 态靠 `protect` 模块兜底过压，初版不阻断联调。

---

*附：本函数与 `power_supervisor_tick_1khz_can()`（整个 `#if 0` 在 `:353-474`）是同一套逻辑的 CAN 版，当前未启用，不影响 bench 联调。*
