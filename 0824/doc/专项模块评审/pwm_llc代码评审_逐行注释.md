# pwm_llc.c 驱动层代码评审（逐行注释）

> 评审日期：2026-08-25
> 文件：`HW/src/pwm_llc.c`（209 行）+ `HW/include/pwm_llc.h`
> 上下文：本模块**不驱动功率级**，只产生 3 路 PWM → 经板载 RC 滤波变成 DC 参考电压，送给模拟 IC（NCP4390/ NCP1654）做 CC/CV/母线电压设定点。PWM 频率固定 20 kHz（`PWM_BASE_HZ`）。
> 重要前提：**当前 `PWM_ANALOG_CALIBRATION_MODE = 0U`（main.h:185），即编译的是正常充电固件，不再是校准固件。**

---

## 一、关键宏与引脚对照（评审基准）

| 引用 | 定义 | 值 |
|---|---|---|
| `CV_PWM` | PA8 / TIMER0 CH0（APB2） | 母线电压参考（CV setpoint） |
| `CC_PWM` | PA0 / TIMER1 CH0（APB1），别名 `CUR_PWM_*` | 电流参考（CC setpoint） |
| `BUS_VOL_ADJ` | PA6 / TIMER2 CH0（APB1） | 母线电压微调参考 |
| `PWM_BASE_HZ` | `main.h:187` | 20000（20 kHz） |
| `PWM_DUTY_SAFE` | `main.h:188` | 0.0f |
| `CV_PWM_DUTY_MIN/MAX/INIT` | `main.h:189-191` | 0.05 / 0.95 / 0.05 |
| `CC_PWM_DUTY_MIN/MAX/INIT` | `main.h:192-194` | 0.02 / 0.95 / 0.02 |
| `f_clampf` | `main.h:348` | `static inline` 钳位函数 |
| `PWM_ANALOG_CALIBRATION_MODE` | `main.h:185` | **0U（当前正常固件）** |

三路引脚 PA8/PA0/PA6 **互不相同**，无复用冲突（`CC_CV_PWM_PIN_CONFLICT=1U` 只是文档性标志位，非实际冲突）。

---

## 二、逐行注释

### 文件头与包含（1-26）

| 行 | 代码 | 注释 |
|---|---|---|
| 1-8 | 文件头注释 | 注明模块名/作者/「需勾选 MicroLIB」。MicroLIB 备注正确且重要。 |
| 13 | `#include "pwm_llc.h"` | 仅依赖自身头文件，头再 include `main.h`。 |
| 14 | `#include <string.h>` | 供 `memset` 清零 OC 结构体用。 |
| 22 | `static float s_cv_duty = CV_PWM_DUTY_INIT;` | CV 占空比运行态副本，初始 0.05。 |
| 23 | `static float s_cc_duty = CC_PWM_DUTY_INIT;` | CC 占空比运行态副本，初始 0.02。 |

### 时钟辅助函数（28-33）

| 行 | 代码 | 注释 |
|---|---|---|
| 28 | `/* TIMER1 APB1: ... */` | 注释因乱码（中文字符损坏）不可读，但逻辑如下。 |
| 29-33 | `timer1_clk_hz()` | 取 APB1 时钟；GD32F30x 规则：当 APB1 预分频 ≠1 时，挂在 APB1 的定时器时钟 = 2×APB1，否则 = APB1。代码用 `(RCU_CFG0 & RCU_CFG0_APB1PSC)` 判预分频字段非零 → 返回 `apb1*2`，否则 `apb1`。**逻辑正确**，但建议显式比较 `(field != 0U)<<1` 更清晰（见 P3-①）。 |

### CV_PWM 初始化（35-83）

| 行 | 代码 | 注释 |
|---|---|---|
| 36 | `void cv_pwm_init(uint32_t freq_hz, float duty)` | TIMER0 CH0（PA8），高级定时器。 |
| 38-40 | `rcu_periph_clock_enable(...)` | 开 GPIOA、TIMER0、AF 复用时钟。 |
| 42 | `gpio_init(..., GPIO_MODE_AF_PP, 50MHz, PIN)` | 复用推挽输出，正确。 |
| 43 | `timer_deinit(CV_PWM_TIMER)` | 复位定时器，避免残留配置。 |
| 46 | `tclk = rcu_clock_freq_get(CK_APB2)` | **⚠️ 见 P3-②**：TIMER0 在 APB2，未做「APB2 预分频≠1 时 ×2」处理。常规配置 APB2=120MHz（PSC=1），此时 timer 时钟=APB2，正确；但若将来改 APB2 PSC≠1，TIMER0 频率会算错（与 TIMER1/2 的 `timer1_clk_hz` 处理不对称）。 |
| 47-48 | `period = tclk/freq_hz; if(period>0) period-=1;` | 周期寄存器 = 时钟/频率 - 1，标准算法。 |
| 50-56 | `timer_parameter_struct t; ... timer_init` | 预分频 0、向上计数、周期、时钟分频 1，配置主定时器。 |
| 59-67 | `timer_oc_parameter_struct oc; ... output_config` | OC 参数清零后使能 CH0 输出、极性高、空闲低。`outputnstate=DISABLE`（互补通道不用），`ocnpolarity` 虽设但不生效，无害。 |
| 68-72 | `#if CAL ... #else ...` | **CAL 模式** clamp 到 [0,1]；**正常模式** clamp 到 [CV_PWM_DUTY_MIN=0.05, MAX=0.95]。当前 CAL=0U → 走 else。 |
| 74-75 | `timer_channel_output_pulse_value_config(..., (period+1)*s_cv_duty)` | 用 `(period+1)` 还原真实周期数，占空比换算正确。 |
| 76 | `timer_channel_output_mode_config(..., PWM0)` | PWM0 模式，极性高 → 占空比=比较值/周期。 |
| 77 | `timer_channel_output_shadow_config(..., SHADOW_ENABLE)` | 比较值走影子寄存器，**占空比更新无毛刺**。✅ |
| 80 | `timer_primary_output_config(..., ENABLE)` | 高级定时器 TIMER0 必须使能主输出，否则 CH0 无波形。✅ 正确且必要。 |
| 81 | `timer_auto_reload_shadow_enable` | 周期也走影子，更新同步。✅ |
| 82 | `timer_enable` | 启动定时器。 |

### CV_PWM 设定/读取（85-98）

| 行 | 代码 | 注释 |
|---|---|---|
| 85-96 | `cv_pwm_set_duty` | 更新占空比。**关键点**：CAL 模式 clamp [0,1]；正常模式 clamp **[PWM_DUTY_SAFE=0.0, 0.95]**（91 行）。注意正常模式下限是 0.0 而非 init 用的 `CV_PWM_DUTY_MIN=0.05`（见 P2-① 不一致）。 |
| 87 | `//s_cv_duty = f_clampf(duty, CV_PWM_DUTY_MIN, CV_PWM_DUTY_MAX);` | **死注释**（被 #if 取代但未删），留着误导维护者。P3-③。 |
| 93-95 | `period = TIMER_CAR(...); pulse = (period+1)*duty` | 实时读取当前周期寄存器再换算，正确，且依赖影子更新无瞬态错误。 |
| 98 | `cv_pwm_get_duty` | 返回运行态副本，非读硬件，足够。 |

### CC_PWM 初始化（100-139）

| 行 | 代码 | 注释 |
|---|---|---|
| 100-108 | `cc_pwm_init` | TIMER1 CH0（PA0），通用定时器。同样的时钟/IO/复位流程。 |
| 110 | `tclk = timer1_clk_hz();` | **正确**走 APB1 ×2 逻辑（见 28-33）。 |
| 122-126 | OC 配置 | 只设 `outputstate`/`ocpolarity`，比 CV 少了 `outputnstate/ocnpolarity/idlestate`——因为 TIMER1 通用定时器无这些字段，正确。 |
| 128-132 | `#if CAL ... #else clamp [CC_MIN=0.02, 0.95]` | 同 CV 范式。 |
| 133-138 | 设比较值/模式/影子/自动重装/使能 | 一致。注意 CC **没有** `timer_primary_output_config`——通用定时器不需要，正确（CV 的 80 行是 TIMER0 专属）。 |

### CC_PWM 设定/读取（141-153）

| 行 | 代码 | 注释 |
|---|---|---|
| 141-151 | `cc_pwm_set_duty` | 同 CV 逻辑；正常模式 clamp **[PWM_DUTY_SAFE=0.0, 0.95]**（146 行），与 init 下限 0.02 不一致（P2-①）。 |
| 153 | `cc_pwm_get_duty` | 返回副本。 |

### BUS_VOL_ADJ（155-209）

| 行 | 代码 | 注释 |
|---|---|---|
| 156 | `static float s_bus_vol_adj_duty = PWM_DUTY_SAFE;` | 初始 0.0。 |
| 158-195 | `bus_vol_adj_pwm_init` | TIMER2 CH0（PA6，APB1）。流程与 CC 基本一致；**额外在末尾 194 行调用 `bus_vol_adj_pwm_set_duty(duty)` 统一设初值**，比 CV/CC 更整洁。 |
| 165-170 | 时钟/IO/复位 | 开 GPIOA、TIMER2、AF。 |
| 172 | `tclk = timer1_clk_hz();` | TIMER2 同属 APB1，✅ 复用 APB1 ×2 逻辑正确。 |
| 173-175 | `period = tclk/freq; if(period==0) period=1; period-=1;` | 与 CV/CC 的 `if(period>0) period-=1` 写法不同但结果等价；冗余判断无害。P3-④ 风格不统一。 |
| 184-191 | OC 配置/模式/影子 | 一致。TIMER2 通用定时器，无需 primary_output_config。 |
| 192-193 | 自动重装影子/使能 | 一致。 |
| 197-204 | `bus_vol_adj_pwm_set_duty` | **clamp 恒为 [0.0, 1.0]**，不受 CAL 宏门控（该通道无 MIN/MAX 宏，可接受）。 |
| 206-208 | `bus_vol_adj_pwm_get_duty` | 返回副本。 |

---

## 三、问题清单（按优先级）

### 🔴 B1（必改，真实缺失）— 正常固件未初始化 `bus_vol_adj`
- **证据**：`main.c:261-262` 正常分支只 `cv_pwm_init` + `cc_pwm_init`，**漏了 `bus_vol_adj_pwm_init`**（CAL 分支 247 行有）。当前 `PWM_ANALOG_CALIBRATION_MODE=0U`，即烧的是正常固件。
- **后果**：PA6/TIMER2 从未被初始化 → 引脚悬空/无波形 → NCP1654 母线电压参考引脚输入不确定，PFC 母线基准失稳，可能导致母线电压失控或 OVP。
- **修复**：在 `main.c:262` 之后补一行
  ```c
  bus_vol_adj_pwm_init(PWM_BASE_HZ, PWM_DUTY_SAFE);
  ```
  （建议初值 0 或经标定后的安全中点，需结合 NCP1654 母线参考极性确认。）

### 🟡 P2（建议初版内修）
- **P2-① 下限不一致**：`cv/cc_pwm_init` 用 `CV/CC_PWM_DUTY_MIN`（0.05/0.02），但 `cv/cc_pwm_set_duty` 正常模式下限是 `PWM_DUTY_SAFE=0.0`。一旦运行期 set_duty 被调到 0，参考电压=0，模拟 IC 可能进入非预期状态。建议 set_duty 正常模式下限也用 `CV/CC_PWM_DUTY_MIN`，与 init 对齐。

### 🟢 P3（整洁 / 健壮性）
- **P3-①** `timer1_clk_hz()` 注释乱码，且 ×2 判断建议写成显式 `(((RCU_CFG0 & field)>>shift) != 0U)` 形式，避免依赖「非掩码即非零」的隐式语义。
- **P3-②** TIMER0（CV）直接用 `CK_APB2` 未做 ×2 处理，与 TIMER1/2 不对称。常规配置无碍，但应加注释说明「假定 APB2 PSC=1」，或统一用同样的时钟获取函数以抗配置变更。
- **P3-③** `cv_pwm_set_duty` 87 行死注释应删除。
- **P3-④** period 计算三种写法（`if(period>0) period-=1` / `if(period==0)period=1;period-=1`）统一为一种。

---

## 四、正面评价（保留项）

- ✅ 三路 PWM 频率 20 kHz，对 RC 滤波成 DC 参考是合理选择（足够高于 RC 拐点，纹波小）。
- ✅ 全部采用 `TIMER_OC_SHADOW_ENABLE` + `timer_auto_reload_shadow_enable`，占空比/周期更新无毛刺。
- ✅ 高级定时器 TIMER0 正确调用 `timer_primary_output_config(ENABLE)`；通用定时器 TIMER1/2 正确省略。
- ✅ 三路引脚 PA8/PA0/PA6 互不冲突。
- ✅ 占空比全程 `f_clampf` 兜底，无越界风险；CAL 模式开放 [0,1] 便于打点。
- ✅ `bus_vol_adj` 在 init 末尾统一 set_duty，比 CV/CC 更规范（可作为 CV/CC 改造样板）。

---

## 五、结论

**整体质量合格**：时钟配置、影子寄存器、引脚分配、钳位保护都正确，作为「PWM→模拟参考」的驱动层没有功能逻辑错误。

**唯一必须改的是 B1**——正常固件漏初始化 `bus_vol_adj`（PA6/TIMER2），这会让 PFC 母线参考失稳。之前因 `PWM_ANALOG_CALIBRATION_MODE=1U`（校准固件）掩盖了，现在切回正常固件（`=0U`）就暴露了。建议上功率前先补这一行。

> 下一步：补 `bus_vol_adj_pwm_init` 调用（B1）→ 顺手对齐 set_duty 下限（P2-①）→ 清死注释/统一 period 写法（P3）。要不要我直接把 B1 那一行加进 `main.c`？
