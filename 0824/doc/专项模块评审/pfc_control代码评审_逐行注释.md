# pfc_control.c 专项评审（逐行注释）v1

> 评审日期：2026-09-03
> 文件：`HW/src/pfc_control.c`（100 行）+ `HW/include/pfc_control.h`（21 行）
> 上下文：1.5kW OBC（72V/20A），PFC 由 **NCP1654 模拟控制器**全权闭环，MCU **不参与功率调节**——本模块只做三件事：PFC 继电器时序、母线建立监控（`bus_vol_v`）、给上层 `llc_control` 提供 `pfc_is_ready()` 启动门。此前 PFC 分析散在《充电逻辑_保护_串口调试_评审》《main.c代码评审》中，本版首次独立成篇。
> 关联阈值（main.h）：`VBUS_MIN_START_V=350`（进 RUN）、`LLC_ENTRY_V=360`（is_ready 下限）、`VBUS_OVP_V=420`（仅作 is_ready 上限）、`PFC_AC_INPUT_MIN/MAX=180/264`、`PFC_READY_TIMEOUT_MS=3000`。

---

## 〇、一句话结论

状态机骨架正确、`pfc_enable` 幂等防重入、RELAY_ON 3s 超时是 v1 评审后的有效补强。**最大缺口是 `PFC_STATE_RUN` 完全空壳——母线过压/欠压监控双双缺失**（`FAULT_BUS_OVP` 全工程从未触发，且旧版 RUN 态的 UVP 也在重构中消失了），运行期母线异常没有固件防线。其次是 `pfc_is_ready()` 三重门的**故障语义混淆**：母线过压、AC 越限都会表现为"3s 超时"，联调时误导定位。

---

## 一、pfc_control.h 逐行注释（21 行）

```c
  1  /* pfc_control.h - PFC 控制 (NCP1654 外置, MCU 监控+继电器) */   // 定位准确：MCU 是监控者不是控制器
  2  #ifndef _PFC_CONTROL_H_
  3  #define _PFC_CONTROL_H_                                            // include guard ✓
  4  #include "main.h"                                                  // 间接拿阈值宏/引脚宏/adc 类型 ✓

  6  typedef enum {
  7      PFC_STATE_OFF = 0,          // 继电器断开, NCP1654 无供电
  8      PFC_STATE_RELAY_ON,         // 继电器已闭合, 等母线建立(软启 ~50ms+母线爬升)
  9      PFC_STATE_RUN,              // NCP1654 已工作, MCU 监控母线   ⚠ 注释里的"监控"当前是空壳(见二.3)
 10      PFC_STATE_FAULT
 11  } pfc_state_t;                  // 四态机, 由 1kHz pfc_tick 驱动
 13  void pfc_init(void);            // 状态机复位+继电器断开
 14  void pfc_enable(void);          // 请求闭合继电器(仅 OFF 态响应)
 15  void pfc_disable(void);         // 无条件断开+回 OFF (fail-safe 入口)
 16  void pfc_tick_1khz(void);       // 1kHz 状态推进(main.c:151)
 17  pfc_state_t pfc_get_state(void);// 供串口 STATUS/诊断 ✓ (serial_console.c:154 已接入状态帧)
 18  bool pfc_is_ready(void);        // 三重启动门, llc_control PRECHARGE→CC 用
```

---

## 二、pfc_control.c 逐行注释（100 行）

> 标记：`✅` 亮点 / `⚠` 问题 / `🟢` 确认正确 / `🆕` 本次新发现。

```c
 13  #include "pfc_control.h"
 14  #include "adc_dma.h"        // g_adc_multi.bus_vol_v / ac_vol_v
 15  #include "protect.h"        // protect_set_fault() — RELAY_ON 超时上报用

 23  static pfc_state_t s_pfc_state = PFC_STATE_OFF;  // 静态初值 OFF，上电即安全态 🟢
 24  static uint32_t s_pfc_state_ms = 0U;             // 状态进入时间戳
        // 🟢 无 volatile 且无需：本变量只在 1kHz 任务上下文读改（pfc_tick/serial_console
        //   都在 super-loop 内），无 ISR 访问，不存在并发
        // ⚠ P5：s_pfc_state_ms 只在 pfc_enable()(:41) 更新，RELAY_ON→RUN 转换时**不重置**
        //   ——当前 RUN 无超时逻辑无影响；将来给 RUN 加超时会从 RELAY_ON 起点起算，埋雷

 29  void pfc_init(void)
 31      s_pfc_state = PFC_STATE_OFF;                 // 状态机复位
 32      gpio_bit_reset(PFC_RELAY_PORT, PFC_RELAY_PIN);  /* 初始断开 */
        // 🟢 与 main.c:198 hw_gpio_init 的复位重复，无害且语义更完整：
        //   pfc_init 在 preflight 之后才调(main.c:296)，此处保证"状态机回 OFF 时继电器必断"
        // 🟢 时序正确：hw_gpio_init(240) → 自检(282-293) → pfc_init(296) → supervisor_init(297)

 35  void pfc_enable(void)
 37      if (s_pfc_state == PFC_STATE_OFF) {          // ✅ 仅 OFF 态响应：幂等防重入，
            //   PRECHARGE 异常退出→STANDBY→再 PRECHARGE 的重复调用安全
            //   ⚠ P4：**不检查 AC 范围**——AC 掉线/越限时继电器照样闭合，
            //     白白消耗继电器寿命，3s 后才报"超时"（真实原因是无 AC）
 39          gpio_bit_set(PFC_RELAY_PORT, PFC_RELAY_PIN);   /* 闭合 PFC 继电器, NCP1654 开始工作 */
 40          s_pfc_state = PFC_STATE_RELAY_ON;
 41          s_pfc_state_ms = g_ms;                   // 计时起点（RELAY_ON 3s 超时用）
        // ⚠ P5：此处计时起点兼作将来 RUN 态的起点，RUN 进入时应重置
 43  }

 45  void pfc_disable(void)
 47      gpio_bit_reset(PFC_RELAY_PORT, PFC_RELAY_PIN);   // 无条件断继电器
 48      s_pfc_state = PFC_STATE_OFF;                 // 无条件回 OFF
        // ✅ fail-safe 设计：任何状态任何调用者都能关（outputs_force_off :127 每次故障/停机都调）
        // 🆕 P7：这个"无条件"让 PFC_STATE_FAULT 变成 **1-tick 态**——FAULT 分支调 pfc_disable()
        //   后 s_pfc_state 立即变 OFF，下一 tick 就在 OFF 态了。PFC 子机"自愈"，
        //   真正的锁存在上层（protect 的 latched + g_charger_state=FAULT）。
        //   语义成立但很隐晦，建议 :74-76 加注释说明，否则维护者会误以为 FAULT 卡死

 51  void pfc_tick_1khz(void)
 53      switch (s_pfc_state) {
 54      case PFC_STATE_OFF:
 55          break;                                   // 空：等 pfc_enable()（由 enter PRECHARGE 触发）🟢

 57      case PFC_STATE_RELAY_ON:
 58          /* 等待母线电压建立 (NCP1654 软启动 ~50ms) */
 59          if (g_adc_multi.bus_vol_v >= VBUS_MIN_START_V) {   // ≥350V → RUN
 60              s_pfc_state = PFC_STATE_RUN;
            //   ⚠ P5：进入 RUN 未重置 s_pfc_state_ms（计时器仍指向 pfc_enable 时刻）
            //   ⚠ P6：350(进RUN) 与 360(is_ready) 之间有 10V 间隙——"RUN 但不 ready"时
            //     状态帧显示 RUN、上层却不进 CC，只能靠上层 3s 超时兜底，无独立诊断
            // 🟢 350 门限合理：NCP1654 软启完成+母线爬升到位才宣布 RUN
 62          else if(elapsed_reached(s_pfc_state_ms, PFC_READY_TIMEOUT_MS))   // 3s 超时
 64              protect_set_fault(FAULT_PRECHARGE_TIMEOUT);  // ✅ v1 评审后新增，防 RELAY_ON 永久等待
            //   🆕 N6：AC 无输入/母线建不起/继电器坏 三种原因共用一个"超时"码，联调难定位
 65              s_pfc_state = PFC_STATE_FAULT;
            // 🆕 P7：置 FAULT 只是"形式上的"——下一 tick :75 的 pfc_disable() 立刻把它拉回 OFF。
            //   真正停功率的是 protect_set_fault 内部的 outputs_force_off→pfc_disable（同步完成）
 67          break;

 69      case PFC_STATE_RUN:
 70          /* 监控母线电压: NCP1654 外置控制, MCU 只做保护监控 加入OVP的保护*/
 72          break;
        // 🔴 P2（最大缺口）：**整个 RUN 态是空壳**：
        //   ① 无 OVP——VBUS_OVP_V(420) 全工程仅被 pfc_is_ready:87 当 is_ready 上限用，
        //      FAULT_BUS_OVP 从未触发；母线冲高只剩 NCP1654 自身模拟 OVP 一道防线
        //   ② 无 UVP——08-14 旧版这里曾有 "bus<304V 持续 200ms → FAULT_BUS_UVP"，
        //      重构时随空壳一起消失。运行中母线塌掉（PFC 失效/AC 掉电）MCU 无感知：
        //      LLC 输出跌落→若在启动 1.5s 内有 OUTPUT_START_TIMEOUT 兜底，
        //      充电中途则只剩 CC 8h 超时这种"小时级"兜底
        //   修复见第四节补丁 1（直接移植 0917 的 tick 顶部 OVP 方案）

 74      case PFC_STATE_FAULT:
 75          pfc_disable();                           // 见 :45-48 分析——执行后 s_pfc_state 立即变 OFF
 76          break;                                   //   PFC_FAULT 实际只停留 1 tick（P7）
 78  }

 80  pfc_state_t pfc_get_state(void) { return s_pfc_state; }   // 🟢 串口状态帧已消费(serial_console.c:154)

 82  bool pfc_is_ready(void)
 84      const bool ac_ok = (g_adc_multi.ac_vol_v >= PFC_AC_INPUT_MIN_V) &&    // ≥180V
 85                         (g_adc_multi.ac_vol_v <= PFC_AC_INPUT_MAX_V);      // ≤264V
        // ⚠ P4：180V 下限会拒绝 PFC 板实测可用的 165V 输入（165V/1300W 受交流源 8.4A
        //   限流）。业务口径二选一：整机不支持 165V→保持并注明；要支持→下调 ~150V
 86      const bool vbus_ok = (g_adc_multi.bus_vol_v >= LLC_ENTRY_V) &&        // ≥360V
 87                           (g_adc_multi.bus_vol_v <= VBUS_OVP_V);           // ≤420V
        // ⚠ P2/N6：420V 上限把"母线过压"伪装成"不 ready"——过压时 is_ready=false →
        //   PRECHARGE 卡 3s → 报"超时"而非"过压"，且过压期间无任何即时保护动作
 89      return (s_pfc_state == PFC_STATE_RUN) && ac_ok && vbus_ok;   // 三重门 🟢 结构清晰
        // 🟢 语义正确：三层各自独立可诊断（前提是上层把三态区分上报，见补丁 2）
 90  }
 91-100  （空行）
```

---

## 三、问题清单

| 编号 | 级别 | 位置 | 问题 | 影响 |
|---|---|---|---|---|
| **P2** | 🔴 严重 | `:69-72` | RUN 态空壳：母线 **OVP/UVP 双缺**（OVP 从未作为故障触发；旧版 UVP 在重构中消失） | 运行期母线异常无固件防线；过压只剩 NCP1654 模拟 OVP，欠压靠小时级超时兜底 |
| **N6** | 🟠 主要 | `:62-66` | RELAY_ON 3s 超时故障码无诊断性：无 AC/继电器坏/母线建不起共用 `FAULT_PRECHARGE_TIMEOUT`；且与 llc_control PRECHARGE 的 3s 超时**双层冗余**（pfc_tick 先跑，llc 层超时实际不可达） | 联调定位慢；冗余代码应择一保留 |
| **P4** | 🟠 主要 | `:37-42` + `:84` | `pfc_enable` 不查 AC 范围：AC 掉线/越限时照常闭合继电器白耗寿命；`is_ready` 的 180V 下限拒绝实测可用的 165V | 继电器寿命 + 165V 业务口径未定 |
| **P5** | 🟡 次要 | `:24/:41/:60` | `s_pfc_state_ms` 仅 enable 时更新，RELAY_ON→RUN 不重置 | RUN 加超时时计时起点错误的埋雷 |
| **P6** | 🟡 次要 | `:59/:86` | 350/360 双门限 10V 间隙："RUN 但不 ready"无独立诊断，只能靠上层超时兜底 | 状态帧 pfc_state=RUN 却不进 CC 时难判因 |
| **P7** | 🟡 次要 | `:45-48/:74-76` | `pfc_disable` 无条件置 OFF 使 PFC_FAULT 成为 1-tick 态（自愈语义正确但隐晦，无注释） | 可维护性；新维护者易误判 |
| — | 🟢 | 全文件 | `pfc_enable` 幂等防重入、init 双保险、无 volatile 需求、get_state 已进状态帧 | 结构正确 |

---

## 四、补丁建议

### 补丁 1（修 P2，从 0917 移植，必做）
RUN 态顶部加母线监控（注意：`protect_set_fault` 内部同步走 `outputs_force_off→pfc_disable`，返回后 `s_pfc_state` 已是 OFF，无需手动转 FAULT 态）：
```c
case PFC_STATE_RUN:
    /* NCP1654 外置控制, MCU 监控母线: 过压即时故障, 欠压防塌陷 */
    if (g_adc_multi.bus_vol_v >= VBUS_OVP_V) {          /* 420V */
        protect_set_fault(FAULT_BUS_OVP);
        break;
    }
    if (g_adc_multi.bus_vol_v < VBUS_MIN_START_V) {     /* 350V, RUN 中母线塌了 */
        protect_set_fault(FAULT_BUS_UVP);               /* 视需求可加 200ms 去抖 */
        break;
    }
    break;
```
0917 版本是在 tick 顶部做 400V 全状态覆盖（不限于 RUN 态），更彻底；0824 先按本补丁在 RUN 态落地即可覆盖主要风险。

### 补丁 2（修 N6，与 llc_control 评审 v3 补丁 B 配套）
pfc 层 RELAY_ON 超时细分（或直接删 pfc 层超时、只留 llc 层一处并在超时分支细分）：
```c
else if (elapsed_reached(s_pfc_state_ms, PFC_READY_TIMEOUT_MS)) {
    if (g_adc_multi.bus_vol_v > VBUS_OVP_V)
        protect_set_fault(FAULT_BUS_OVP);               /* 母线冲过头, is_ready 被上限挡住 */
    else
        protect_set_fault(FAULT_PRECHARGE_TIMEOUT);     /* 真建不起(AC/继电器/NCP1654) */
    s_pfc_state = PFC_STATE_FAULT;
}
```

### 补丁 3（修 P4 的继电器寿命部分）
`pfc_enable` 加 AC 门控（AC RMS 200ms 首窗口不受影响——PRECHARGE 需人工 SET+START，远晚于上电 200ms）：
```c
void pfc_enable(void)
{
    if (s_pfc_state != PFC_STATE_OFF) return;
    if (g_adc_multi.ac_vol_v < PFC_AC_INPUT_MIN_V ||
        g_adc_multi.ac_vol_v > PFC_AC_INPUT_MAX_V) {
        protect_set_fault(FAULT_PRECHARGE_TIMEOUT);     /* 建议新增 FAULT_AC_RANGE 更准 */
        return;
    }
    gpio_bit_set(PFC_RELAY_PORT, PFC_RELAY_PIN);
    ...
}
```
165V 业务口径（P4 另一半）需要你拍板：`PFC_AC_INPUT_MIN_V` 下调到 150V，或保持 180V 并在文档注明"165V 仅 PFC 单板调试口径"。

### 顺手清理（P5/P7）
- `:60` 进 RUN 时补 `s_pfc_state_ms = g_ms;`（消除 P5 埋雷）；
- `:74-76` 加注释：`/* pfc_disable() 内部置 OFF, 本态实际只维持 1 tick; 真锁存在 protect latched */`。

---

## 五、与 0917 对照（PFC 主线差异）

| 维度 | 0824 (本模块) | 0917 |
|---|---|---|
| 母线 OVP | ❌ RUN 空壳，420V 仅作 is_ready 上限 | ✅ tick 顶部 400V 全状态覆盖 |
| 母线 UVP | ❌ 旧版 304V/200ms 判据在重构中消失 | ✅ 有 |
| 故障时序 | ⚠ set_fault 后依赖 outputs_force_off 间接回 OFF | ✅ 每个故障路径显式先 `pfc_outputs_off()` |
| 预充超时 | RELAY_ON 3s（pfc 层）+ PRECHARGE 3s（llc 层）双层 | 单层 |
| 状态机归属 | pfc 子机 + llc supervisor 分离 | main.c 内 pfc 状态机自洽 |

---

## 六、上板/联调检查清单

- [ ] 补丁 1 落地后验证：母线人为抬到 >420V（或调低阈值模拟），确认立即 FAULT_BUS_OVP、三路断、红灯、状态帧 fault 码正确。
- [ ] 验证 RELAY_ON 超时：不接 AC（或 AC 源输出关断）→ START，3s 后应报 `FAULT_PRECHARGE_TIMEOUT`（补丁 2 后应为更准确的故障码）。
- [ ] 165V 业务口径确认：`PFC_AC_INPUT_MIN_V` 180V 是否下调。
- [ ] 继电器时序示波器确认：闭合瞬间→母线爬升→350V 进 RUN 的实际耗时（评估 3s 超时裕量是否合理）。
- [ ] 状态帧 `pfc_state` 字段在 STANDBY/PRECHARGE/CC 各态的值符合预期（0/1/2）。
- [ ] 确认 PFC 继电器闭合前 AC RMS 已有效（上电 200ms 首窗口与人工 START 的时间裕量）。

---

> 总结：本模块代码量小、骨架正确，`pfc_enable` 幂等与超时补强都是对的。**必须做的是补丁 1（RUN 态 OVP/UVP）**——这是 0824 保护体系与 0917 的最大差距；补丁 2/3 与 165V 口径确认随后。P7 的 1-tick FAULT 语义建议至少补注释，避免日后误判"PFC 卡死在 FAULT"。
