# pwm_llc.c 驱动层代码评审（逐行注释）v2

> 评审日期：2026-09-03 v2（初版 2026-08-25）
> 修订记录：
> - 2026-08-25 v1 初版
> - 2026-09-03 v2（本版）：① **撤回 v1 "三路引脚互不相同、无复用冲突"的结论**——PA6 双重定义（I_SENSE/ADC_CH6 vs BUS_VOL_ADJ/TIMER2_CH0）是真实冲突，main.h:174-175 用户已加 `CC_CV_PWM_PIN_CONFLICT=1U` 标记；② **撤回 B1**——正常固件不调用 `bus_vol_adj_pwm_init` 是有意规避冲突的正确设计，v1"必须补一行"的建议照做会毁掉 I_SENSE 电流采样；③ 其余 P2/P3 结论按当前源码（209 行）复核维持。
> 文件：`HW/src/pwm_llc.c`（209 行）+ `HW/include/pwm_llc.h`
> 上下文：本模块**不驱动功率级**，只产生 3 路 PWM → 经板载 RC 滤波变成 DC 参考电压，送给模拟 IC（NCP1654/LLC 专用 IC）做 CC/CV 设定点。PWM 频率固定 20 kHz（`PWM_BASE_HZ`，main.h:182）。
> 重要前提：**当前 `PWM_ANALOG_CALIBRATION_MODE = 0U`（main.h:180），编译的是正常充电固件。**

---

## 一、关键宏与引脚对照（v2 修订）

| 引用 | 定义 | 值 |
|---|---|---|
| `CV_PWM` | PA8 / TIMER0 CH0（APB2） | LLC 输出电压参考（CV setpoint） |
| `CC_PWM` | PA0 / TIMER1 CH0（APB1），别名 `CUR_PWM_*` | 电流参考（CC setpoint） |
| `BUS_VOL_ADJ` | **PA6** / TIMER2 CH0（APB1） | 母线电压参考——**⚠ 与 I_SENSE 同脚，见第二节** |
| `I_SENSE` | **PA6** / ADC0 CH6（main.h:134-136） | 输出电流采样——正常固件的 PA6 归属 |
| `PWM_BASE_HZ` | main.h:182 | 20000 |
| `PWM_DUTY_SAFE` | main.h:183 | 0.0f |
| `CV_PWM_DUTY_MIN/MAX/INIT` | main.h:184-186 | 0.05 / 0.95 / 0.05 |
| `CC_PWM_DUTY_MIN/MAX/INIT` | main.h:187-189 | 0.02 / 0.95 / 0.02 |
| `PWM_DUTY_SLEW` | main.h:191 | 0.002 —— **全工程零引用（死宏）**，与 main.h 评审结论一致 |
| `PWM_ANALOG_CALIBRATION_MODE` | main.h:180 | **0U（当前正常固件）** |

**v1 "三路引脚 PA8/PA0/PA6 互不相同，`CC_CV_PWM_PIN_CONFLICT=1U` 只是文档性标志"——结论撤回**：该宏（main.h:175）是**真实冲突的显式标记**，PA6 同时被定义为 `I_SENSE_PIN`（:135）与 `BUS_VOL_ADJ_PIN`（:170）。

---

## 二、PA6 引脚冲突专节（v2 核心）

### 冲突事实链
1. `main.h:134-136`：`I_SENSE_PORT/PIN=GPIOA/GPIO_PIN_6`，`I_SENSE_CH=ADC_CHANNEL_6`（U17.22）；
2. `main.h:168-175`：`BUS_VOL_ADJ_PORT/PIN=GPIOA/GPIO_PIN_6`，`BUS_VOL_ADJ_TIMER=TIMER2/CH0`（注释里写 "U17.20 / PA6: BUS_VOL_ADJ"——U17.20 同时也是 main.h:128 `T_SENSE_CASE` 的引脚号，原理图标号本身有出入，需以原理图终版为准）；
3. `adc_dma.c:35-36`：`adc_multi_init_dma` 把 PA6 配成 `GPIO_MODE_AIN`（电流采样前提）；
4. `pwm_llc.c:168-169`：`bus_vol_adj_pwm_init` 把 PA6 配成 `GPIO_MODE_AF_PP`（TIMER2 PWM 输出前提）；
5. **两者后配置者获胜**：谁后初始化，PA6 就归谁，另一个功能静默失效。

### 当前固件的处置（正确）
- **正常固件**（`PWM_ANALOG_CALIBRATION_MODE=0`）：main.c `#else` 分支只调 `cv_pwm_init`+`cc_pwm_init`，**不调 `bus_vol_adj_pwm_init`** → PA6 归 ADC（I_SENSE）✓；
- **CAL 固件**（=1）：main.c CAL 分支调三路 PWM init、**不调 `adc_multi_init_dma`**（CAL 固件无 ADC 采样）→ PA6 归 TIMER2 ✓；
- 两个固件各自独占 PA6，互斥成立——**这是编译分支隔离，不是巧合，v1 把它当缺陷是误判**。

### 结论与建议
- **母线电压参考的真实来源**：正常固件下 NCP1654 的母线目标由其外围电阻网络设定，MCU 不参与（对照 Main wf3.pdf 原理图最终确认）。若确认母线参考不需要 MCU 调节，建议后续把 `bus_vol_adj_pwm_*` 三个函数与 main.h 的 BUS_VOL_ADJ 宏整体标记"仅 CAL 固件使用"，避免后人误以为正常固件也该初始化它；
- **长期解法（硬件改版）**：BUS_VOL_ADJ 换到空闲 AF 引脚（或放弃该功能），消除 PA6 双重定义；改版前任何"在正常固件补 bus_vol_adj_pwm_init"的想法都**禁止执行**；
- **防护建议（软件侧，低成本）**：在 `bus_vol_adj_pwm_init` 入口加 `#if !PWM_ANALOG_CALIBRATION_MODE #error "bus_vol_adj_pwm_init conflicts with I_SENSE(PA6) in normal firmware" #endif`，把互斥从"靠纪律"变成"靠编译器"。

---

## 三、逐行注释（209 行，v2 复核）

### 时钟辅助（28-34）
| 行 | 代码 | 注释 |
|---|---|---|
| 28-34 | `timer1_clk_hz()` | APB1 预分频≠1 时定时器时钟=2×APB1。**逻辑正确** ✓（:30-31 中文注释已修复 v1 记录的乱码）。仍建议显式比较位域（P3-①）。 |

### CV_PWM（36-98）
| 行 | 代码 | 注释 |
|---|---|---|
| 39-44 | 开时钟/AF_PP/deinit | ✓ |
| 46-48 | `tclk=CK_APB2; period=tclk/freq-1` | TIMER0 在 APB2，未做 ×2 处理——当前 APB2 PSC=1 正确；与 TIMER1/2 处理不对称（P3-②，加"假定 APB2 PSC=1"注释即可） |
| 50-67 | timer/OC 配置 | `memset` 清零后配置，PWM0/极性高/空闲低 ✓ |
| 68-72 | init 占空比钳位 | CAL:[0,1] / 正常:[0.05,0.95] ✓ |
| 74-77 | pulse 值/影子寄存器 | `(period+1)*duty` 换算正确；OC shadow ✓ |
| 80-82 | `timer_primary_output_config(ENABLE)` + AR shadow + enable | TIMER0 高级定时器必需主输出使能 ✓ |
| 85-96 | `cv_pwm_set_duty` | 正常模式钳 **[PWM_DUTY_SAFE=0.0, 0.95]**（:91）。⚠ **P2-① 维持**：set_duty 下限 0.0 与 init 的 `CV_PWM_DUTY_MIN=0.05` 不一致——运行期把参考打到 0V，模拟 IC 行为未定义（建议下限对齐 0.05）。`:87` 死注释维持（P3-③） |
| 98 | `cv_pwm_get_duty` | 返回副本 ✓ |

### CC_PWM（100-153）
| 行 | 代码 | 注释 |
|---|---|---|
| 101-138 | init | TIMER1 通用定时器，无 primary_output 需求，正确省略 ✓；时钟走 `timer1_clk_hz()` ✓ |
| 141-151 | `cc_pwm_set_duty` | 同 P2-①（:146 下限 0.0 vs init 0.02） |
| 153 | `cc_pwm_get_duty` | ✓ |

### BUS_VOL_ADJ（155-209）
| 行 | 代码 | 注释 |
|---|---|---|
| 155-156 | 区块注释/静态初值 | **v2 前提更新**：本区块仅在 CAL 固件中合法启用（PA6 冲突，见第二节） |
| 158-195 | `bus_vol_adj_pwm_init` | 流程与 CC 一致；`tclk=timer1_clk_hz()`（TIMER2 同 APB1）✓；末尾统一 set_duty ✓。⚠ **调用约束**：仅 CAL 分支可调（main.c:249），正常固件调用即毁 I_SENSE |
| 197-204 | `bus_vol_adj_pwm_set_duty` | 钳 [0,1] 不受 CAL 宏门控（无 MIN/MAX 宏，可接受） |
| 206-209 | get_duty | ✓ |

---

## 四、问题清单（v2）

### ~~🔴 B1~~ → ✅ 撤回（性质改判）
v1 "正常固件漏初始化 bus_vol_adj，必须补一行"——**撤回**。事实：PA6 与 I_SENSE 冲突，正常固件不初始化是**有意规避**；补上那行会立刻毁掉电流采样（详见第二节）。v1 修复代码**作废**。

### 🟠 P2
- **P2-① 下限不一致**（维持）：`cv/cc_pwm_set_duty` 正常模式下限 `PWM_DUTY_SAFE=0.0` ≠ init 的 `CV/CC_PWM_DUTY_MIN`（0.05/0.02）。运行期 set_duty(0) 会把参考电压打到 0V。建议正常模式 set_duty 下限对齐 `CV/CC_PWM_DUTY_MIN`。（注：`outputs_force_off` 用 `PWM_DUTY_SAFE` 是**关断语义**，属合法用途——若收紧下限，需给关断路径留显式 0 通道，例如 `pwm_force_safe()` 接口直接写 0，绕过钳位。）

### 🟡 P3（整洁/健壮性，维持）
- **P3-①** `timer1_clk_hz()` ×2 判断建议显式位域比较；
- **P3-②** TIMER0 用 `CK_APB2` 未做 ×2，加"假定 APB2 PSC=1"注释或统一时钟获取；
- **P3-③** `cv_pwm_set_duty:87` 死注释删除；
- **P3-④** period 计算两种写法统一；
- **P3-⑤（新增）** `PWM_DUTY_SLEW`（main.h:191）全工程零引用——软启动斜率限幅从未实现（与充电逻辑评审 P4 同源），要么实现要么删宏；
- **P3-⑥（新增）** 建议给 `bus_vol_adj_pwm_init` 加 `#error` 护栏（第二节防护建议）。

---

## 五、正面评价（v2 保留）

- ✅ 三路 PWM 20kHz 对 RC 滤波成 DC 参考合理；
- ✅ 全部 `TIMER_OC_SHADOW_ENABLE` + `timer_auto_reload_shadow_enable`，占空比/周期更新无毛刺；
- ✅ TIMER0 正确使能主输出；TIMER1/2 正确省略；
- ✅ 占空比全程 `f_clampf` 兜底；CAL 模式开放 [0,1] 便于打点；
- ✅ `bus_vol_adj` init 末尾统一 set_duty，比 CV/CC 规范（可作 CV/CC 改造样板）；
- ✅（v2 新增）CAL/正常固件对 PA6 的编译分支隔离设计，在硬件引脚受限下是当前最合理的软件处置。

---

## 六、结论（v2）

驱动层本身质量合格：时钟、影子寄存器、钳位保护都正确。**v1 的 B1 经 PA6 冲突核实后撤回**——正常固件"缺失"的 `bus_vol_adj_pwm_init` 调用是刻意为之，本模块与 ADC 模块在 PA6 上的互斥由编译分支保证。

行动项收敛为：
1. （可选，低成本高价值）`bus_vol_adj_pwm_init` 加 `#error` 护栏（P3-⑥）；
2. P2-① 下限对齐（注意给关断语义留 0 通道）；
3. P3 各项随版本节奏清理；
4. 硬件改版时把 PA6 双重定义列入议程（BUS_VOL_ADJ 换脚或删除）。
