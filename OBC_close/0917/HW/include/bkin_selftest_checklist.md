# BKIN 刹车脚上电自检 — 工况验证清单

> 模块：bkin_selftest.c / bkin_selftest.h
> 硬件：GD32F30x TIMER0 BKIN (PB12), 低有效, IPU 上拉
> 信号链路：LMV393(OCP) → HARD_PRO → LTV-817S(光耦) → PB12(BKIN)
> 日期：2026-05-07

---

## 一、自检测试项总览

| 编号 | 测试项 | 验证内容 | PASS 标志 | FAIL 码 |
|------|--------|----------|-----------|---------|
| T1 | GPIO 空闲电平 | IPU 上拉 → PB12 应为高 | `BKIN_TEST_PASS` | `FAIL_GPIO_LOW (0x01)` |
| T2a | Break 标志置位 | PB12 LOW → TIMER_INT_FLAG_BRK 置位 | Break flag = 1 | `FAIL_BRK_FLAG (0x08)` |
| T2b | MOE 自动清零 | Break 触发后 POEN 应被硬件清零 | MOE = 0 | `FAIL_MOE_NOT_CLR (0x04)` |
| T2c | Break ISR 执行 | ISR 入口可达，protect_fault_latched() = 1 | fault latched | `FAIL_BRK_IRQ (0x02)` |
| T3 | 故障恢复 | IPU 恢复后 PB12 回高，光耦截止 | PB12 = HIGH | `FAIL_NO_RECOVER (0x10)` |

---

## 二、工况验证场景

### 场景 A：正常上电（无硬件故障）

- **前置条件**：PFC/LLC 未启动，光耦截止，PB12 浮空被 IPU 上拉为高
- **操作步骤**：直接调用 `bkin_selftest_run()`
- **预期结果**：
  - T1：PB12 读到高电平 → PASS
  - T2：PB12 切 OUT_PP LOW → Break 标志置位、MOE 清零、ISR 执行 → 全 PASS
  - T3：恢复 IPU 后 PB12 回高 → PASS
- **结果码**：`0x00 (BKIN_TEST_PASS)`
- **后续动作**：允许进入 PFC/LLC 启动流程

### 场景 B：BKIN 对地短路（硬件故障）

- **模拟方式**：PB12 引脚外接线到 GND，或光耦输出端 C-E 短路
- **预期结果**：
  - T1：PB12 读到低电平 → `FAIL_GPIO_LOW (0x01)`
  - T2 不执行（BKIN 已为低，无需再模拟）
  - T3：恢复 IPU 后 PB12 仍为低 → `FAIL_NO_RECOVER (0x10)`
- **结果码**：`0x01` 或 `0x11`
- **后续动作**：`while(1)` 阻断，不可启动
- **故障定位**：
  - 仅 `0x01` → 光耦短路或 PB12 对地短路
  - `0x11` → 同上且恢复不了，硬件严重故障

### 场景 C：Break 中断未使能 / ISR 异常

- **模拟方式**：
  1. 注释掉 `NVIC_EnableIRQ(TIMER0_BRK_IRQn)` 或
  2. 将 `TIMER0_BRK_IRQHandler` 改为空函数
- **预期结果**：
  - T1：PASS（PB12 正常高电平）
  - T2a：Break 标志可能置位（硬件行为，不依赖 ISR）→ PASS
  - T2b：MOE 被硬件清零 → PASS
  - T2c：ISR 未执行 → `FAIL_BRK_IRQ (0x02)`
- **结果码**：`0x02`
- **故障定位**：NVIC 配置错误或 ISR 函数名未正确链接

### 场景 D：BKIN 通道断路（TIMER 内部故障或复用配置错误）

- **模拟方式**：将 PB12 GPIO 配置为普通输入（非 BKIN 复用），如 `GPIO_MODE_IN_FLOATING`
- **预期结果**：
  - T1：PASS（浮空输入可能读到高或低，但 IPU 情况下通常为高）
  - T2a：Break 标志未置位 → `FAIL_BRK_FLAG (0x08)`
  - T2b：MOE 未清零 → `FAIL_MOE_NOT_CLR (0x04)`
  - T2c：ISR 未执行 → `FAIL_BRK_IRQ (0x02)`
- **结果码**：`0x0E` (BRK_IRQ | MOE_NOT_CLR | BRK_FLAG)
- **故障定位**：GPIO 复用配置错误或 TIMER0 BKIN 通道损坏

### 场景 E：上电时 BKIN 已为低（真实 OCP 故障存在）

- **前置条件**：LMV393 检测到过流 → HARD_PRO 持续高 → 光耦持续导通 → PB12 持续低
- **预期结果**：
  - T1：PB12 读到低电平 → `FAIL_GPIO_LOW (0x01)`
  - T2：MOE=0 无法开启（Step 2b 短暂使能 MOE 后 Break 立即触发）
  - T3：恢复 IPU 后 PB12 仍为低 → `FAIL_NO_RECOVER (0x10)`
- **结果码**：`0x11` (GPIO_LOW | NO_RECOVER)
- **安全意义**：**这是最危险的工况** — OCP 保护通道已触发，说明硬件故障已存在，绝对不可启动
- **后续动作**：`while(1)` 阻断，必须排查硬件故障

---

## 三、位域结果码速查表

| 结果码 | 二进制 | 含义 |
|--------|--------|------|
| 0x00 | 00000 | 全部通过 |
| 0x01 | 00001 | GPIO 空闲低电平 |
| 0x02 | 00010 | Break ISR 未触发 |
| 0x04 | 00100 | Break 后 MOE 未清零 |
| 0x06 | 00110 | ISR 未触发 + MOE 未清零 |
| 0x08 | 01000 | Break 标志未置位 |
| 0x0C | 01100 | Break 标志未置位 + MOE 未清零 |
| 0x0E | 01110 | ISR + MOE + Break 标志全异常 |
| 0x10 | 10000 | BKIN 恢复后仍为低 |
| 0x11 | 10001 | GPIO 低 + 恢复失败（真实故障） |

---

## 四、验证实操步骤

### 4.1 必备工具

- [x] 串口调试助手（查看 `[BKIN-SELFTEST]` 打印）
- [x] 万用表（测量 PB12 电压）
- [x] 杜邦线（模拟 PB12 对地短路）

### 4.2 验证顺序

1. **场景 A（正常上电）**：不加任何模拟，观察串口输出 `=== ALL PASSED ===`
2. **场景 B（对地短路）**：PB12 接 GND → 上电 → 观察 `FAIL: GPIO_LOW` + `while(1)`
3. **场景 C（ISR 异常）**：修改代码禁用 Break NVIC → 编译烧录 → 观察 `FAIL: Break ISR NOT executed`
4. **场景 D（BKIN 断路）**：修改 PB12 GPIO 为 IN_FLOATING → 编译烧录 → 观察多项 FAIL
5. **场景 E（真实故障）**：触发 LMV393 → 观察 `0x11`（此步需上 HV，暂不可测）

### 4.3 串口打印示例

**正常通过**：
```
[BKIN-SELFTEST] === Start (no HV) ===
[BKIN-SELFTEST] Step1: GPIO idle level check...
[BKIN-SELFTEST] PASS: PB12 idle HIGH
[BKIN-SELFTEST] Step2: Break interrupt + MOE test...
[BKIN-SELFTEST] PASS: Break flag set
[BKIN-SELFTEST] PASS: MOE cleared by Break
[BKIN-SELFTEST] PASS: Break ISR fired, fault latched
[BKIN-SELFTEST] Step3: Fault recovery test...
[BKIN-SELFTEST] PASS: BKIN recovered HIGH
[BKIN-SELFTEST] === ALL PASSED ===
```

**GPIO 短路**：
```
[BKIN-SELFTEST] === Start (no HV) ===
[BKIN-SELFTEST] Step1: GPIO idle level check...
[BKIN-SELFTEST] FAIL: PB12 is LOW at idle (opto short / BKIN-GND short)
[BKIN-SELFTEST] Step3: Fault recovery test...
[BKIN-SELFTEST] FAIL: BKIN still LOW after recovery delay
[BKIN-SELFTEST] === FAILED: 0x11 ===
```

---

## 五、安全检查要点

| # | 检查项 | 要求 | 验证方法 |
|---|--------|------|----------|
| 1 | 自检期间无 PWM 输出 | MOE=0 或 CCR=0 | 示波器观测 PWM 引脚 |
| 2 | GPIO 模式恢复 | OUT_PP 测完后立即恢复 IPU | 代码审查 Step 2e |
| 3 | MOE 安全退出 | 自检结束后 MOE=0 | 读取 `selftest_moe_state()` |
| 4 | 中断标志清除 | 退出前 `TIMER_INT_FLAG_BRK` 已清 | 代码审查 Step 4 |
| 5 | 故障锁存清除 | 退出前 `protect_clear_fault()` 已调 | 代码审查 Step 4 |
| 6 | 不影响正常启动 | 自检通过后 PFC/LLC 可正常启动 | 场景 A 验证后续启动流程 |
| 7 | 自检失败可阻断 | 自检失败 → `while(1)` | 场景 B/C/D 验证 |

---

## 六、已知限制与后续改进

1. **场景 E 需上 HV 才能完整验证** — 当前仅能通过 PB12 接 GND 模拟，无法验证 LMV393 → 光耦全链路
2. **无光耦在线检测** — 若光耦开路（C-E 断开），BKIN 永远为高，真实 OCP 无法触发保护 → T1/T2 PASS 但运行时保护失效
3. **MOE 短暂为 1** — Step 2b 中 `timer_primary_output_config(ENABLE)` 会短暂使能 MOE（CCR=0 无脉冲），但硬件上 POEN 从 0→1 有约 1 个 PWM 周期的过渡
4. **建议增加运行时周期性自检** — 当前仅上电时运行一次，运行中若 BKIN 通道异常（如焊点虚焊）无法发现
