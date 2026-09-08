# main.h 代码评审 · 逐行注释

> 评审对象：`App/include/main.h`（v0.1.0，2026-07-29，Rengar）
> 交叉核对：`adc_dma.c/.h`、`pwm_llc.c`、`main.c`、`protect.c`
> 评审日期：2026-09-02
> 结论：**发现 2 个 P0 + 3 个 P1，其中 P0-1 会直接破坏 PFC 预充判定**

---

## 0. 总体判断

这份头文件的**硬件资源描述质量比前一版明显提升**（引脚号、封装脚位、分压比、NTC 参数都有据可查），但存在两个致命问题：

1. **PA6 一个物理引脚被分配给两个互斥功能**（I_SENSE 电流采样 / BUS_VOL_ADJ 母线调节 PWM），靠"两个编译分支永不交集"侥幸不炸；
2. **ADC raw 值做减法时无符号下溢**，母线 0V 时会被算成 ~8149V，直接击穿 PFC 预充判定逻辑。

另外，**所有温度保护在数据源层面就是断的**（4 个 NTC 通道未进扫描序列）。

---

## 1. 逐段注释

### 1.1 系统时钟与 ADC 基准（L17-27）

```c
#define SYSTEM_CLOCK_HZ         (120000000U)   /* OK：与 InitRCU() 120MHz 一致 */
#define VREF_ADC_V              (3.30f)        /* OK：与板载 3V3 一致，但用实测值更准 */
#define ADC_FULL_SCALE_COUNTS   (4095U)        /* OK：12bit 右对齐 */
#define VREF_ADC                VREF_ADC_V     /* 兼容别名，OK */
#define ADC_RESOLUTION          ADC_FULL_SCALE_COUNTS  /* 别名，OK */
```
判读：无问题。注意 `VREF_ADC_V` 用标称 3.30V，实际 LDO 可能有 ±2% 偏差，会等比例传递到所有电压测量。台架标定时建议改为实测值。

### 1.2 电压采样分压网络（L29-54）

| 网络 | 分压比（计算值） | 注释值 | ADC 满量程 | 判读 |
|---|---|---|---|---|
| VBUS 1.32M + 8.6k | 8.6/1328.6 = 1/154.49 | 1/154.65 | 3.3×154.49 = **509.8V** | ✅ OVP 420V 有 90V 余量 |
| AC 1.32M + 5.1k | 5.1/1325.1 = 1/259.82 | 1/259.8 | 3.3×259.82 = 857V | ✅ 264VAC 峰值 373V 不溢出 |
| VOUT 240k + 4.7k | 4.7/244.7 = 1/52.06 | 1/52.06 | 3.3×52.06 = **171.8V** | ✅ 94V OVP 有余量 |
| VBT 同 VOUT | 1/52.06 | — | 171.8V | ✅ |
| 3V3 监测 5.1k+10k | 10/15.1 = 0.662 | — | 3.3/0.662 = 4.98V | ✅ 自检窗 2.8~3.6V 有效 |

```c
#define AC_RMS_WINDOW_SAMPLES   (5000U / 5U)   /* = 1000，可读性差 */
```
⚠️ **P2**：`5000/5` 含义完全靠注释解释（5000=5kHz 采样率，5=?）。建议改为：
```c
#define AC_SAMPLE_RATE_HZ       (5000U)
#define AC_RMS_WINDOW_MS        (200U)
#define AC_RMS_WINDOW_SAMPLES   (AC_SAMPLE_RATE_HZ * AC_RMS_WINDOW_MS / 1000U)
```

```c
#define AC_RMS_CALIBRATION      (1.0f)         /* 待标定 */
```
⚠️ **P2**：AC 前端是**全波整流**（见 `adc_dma.c:107-116` 注释），整流桥二极管压降（~0.3~0.7V）会让 RMS 系统性偏低，尤其低压输入时。1.0 意味着尚未补偿，低压 165V 点误差最明显。台架必须标定。

### 1.3 电流与温度采样（L56-72）

```c
#define ISHUNT_OHM              (0.00333f)   /* 3×0.01R 并联，OK */
#define IAMP_GAIN               (20.0f)      /* NSI1312 增益，待标定 */
#define NTC_PULLUP_OHM          (10000.0f)   /* OK */
#define NTC_BETA                (3950.0f)    /* OK，常见 10k B3950 */
#define TEMP_DERATING_START_C   (65.0f)      /* 降额起点 */
#define TEMP_DERATING_FULL_C    (85.0f)      /* 满降额 */
#define TEMP_SHUTDOWN_C         (95.0f)      /* 关机 */
```
参数本身合理。❌ **但见 P1-3：这些温度参数没有任何数据源**。

### 1.4 控制目标（L74-92）

```c
#define PFC_AC_INPUT_MIN_V       (180.0f)     /* 注释：First-bench acceptable AC RMS */
#define PFC_AC_INPUT_MAX_V       (264.0f)
#define VBUS_TARGET_V           (380.0f)     /* ❌ 全工程零引用 */
#define VBUS_MIN_START_V        (350.0f)     /* pfc_control.c 用 */
#define LLC_ENTRY_V             (360.0f)     /* 与上一行语义重叠 */
#define VBUS_OVP_V              (420.0f)     /* ⚠️ 定义了但从未被 set（P1） */
#define PFC_READY_TIMEOUT_MS    (3000U)
#define VOUT_OVP_V              (94.0f)      /* 72V 系统（20串满充84V），余量10V ✅ */
#define IOUT_OCP_A              (24.0f)      /* 额定20A，20%余量 ✅ */
```

- ❌ **`VBUS_TARGET_V (380.0f)` 全工程零引用**。这不是笔误——因为 BUS_VOL_ADJ 通道硬件冲突（见 P0-2），**母线电压 MCU 根本无法调节**，380V 只是纸面目标。当前母线实际值完全由 NCP1654 硬件反馈决定（这也解释了纯硬件测试时母线能到 ~400V）。
- ⚠️ `VBUS_MIN_START_V (350)` 与 `LLC_ENTRY_V (360)` 两个启动阈值语义重叠，建议统一命名（`PFC_RUN_ENTRY_V` / `LLC_START_V`）避免误用。
- ⚠️ `PFC_AC_INPUT_MIN_V = 180V`：你实测 165V 输入能跑 1300W，但固件会判定 AC 欠压拒绝启动。bench 阶段若要在 165V 验证，需临时下调。

### 1.5 充电阶段参数（L96-104）

```c
#define CHARGE_CV_ENTRY_MARGIN_V        (0.5f)
#define CHARGE_CV_ENTRY_DEBOUNCE_MS     (1000U)
#define CHARGE_FINISH_CURRENT_A         (1.0f)
#define CHARGE_FINISH_VOLTAGE_MARGIN_V  (0.5f)
#define CHARGE_FINISH_DEBOUNCE_MS       (30000U)
#define CHARGE_CC_TIMEOUT_MS            (8U * 60U * 60U * 1000U)   /* 8h ✅ */
#define CHARGE_CV_TIMEOUT_MS            (3U * 60U * 60U * 1000U)   /* 3h ✅ */
```
合理。截止判定 1.0A / 30s 去抖对 20A 系统是 5% 截止电流，符合常规。

### 1.6 模拟采样引脚定义（L106-157）

```c
#define AC_VOL_SAMPLE_PIN    GPIO_PIN_1   /* PA1  ✅ 已扫描 */
#define FAN_CS_SAMPLE_PIN    GPIO_PIN_2   /* PA2  ❌ 未扫描 */
#define BUS_VOL_SAMPLE_PIN   GPIO_PIN_3   /* PA3  ✅ 已扫描 */
#define T_SENSE_CASE_PIN     GPIO_PIN_4   /* PA4  ❌ 未扫描 */
#define VOUT_SENSE_PIN       GPIO_PIN_5   /* PA5  ✅ 已扫描 */
#define I_SENSE_PIN          GPIO_PIN_6   /* PA6  ✅ 已扫描 ⚠️ 与 BUS_VOL_ADJ 冲突 */
#define T_SENSE_PFC_MOS_PIN  GPIO_PIN_7   /* PA7  ❌ 未扫描 */
#define T_SENSE_TR_PIN       GPIO_PIN_4   /* PC4  ❌ 未扫描 */
#define VBT_SENSE_PIN        GPIO_PIN_5   /* PC5  ✅ 已扫描 */
#define T_SENSE_LLC_MOS_PIN  GPIO_PIN_1   /* PB1  ❌ 未扫描 */
#define ADC_CHANNEL_QTY      (5U)         /* 实际只扫 5 个 */
```

**定义了 10 个采样通道，实际只扫描 5 个**（`adc_dma.c:37-40`）。未扫描的 5 个：FAN_CS、T_SENSE_CASE、T_SENSE_PFC_MOS、T_SENSE_TR、T_SENSE_LLC_MOS。

`adc_dma.h:17,19,22,23,25` 对应字段**全部处于注释状态**——说明这是"有意暂时关闭"，但后果是 **4 路温度 + 风扇电流检测全部失效**（见 P1-3/P1-4）。

⚠️ 引脚号注释错误：
```c
/* U17.20 / PA6: BUS_VOL_ADJ, TIMER2_CH0 (default mapping). */
#define BUS_VOL_ADJ_PIN   GPIO_PIN_6
```
LQFP64 封装下 **PA4=U17.20、PA6=U17.22**。此处既把 PA6 标成 U17.20（错），又与 `T_SENSE_CASE /* U17.20 */` 撞号。应改为 `U17.22`。

### 1.7 PWM 基准输出（L159-195）

```c
#define CV_PWM_PIN      GPIO_PIN_8   /* PA8, TIMER0_CH0  ✅ 无冲突 */
#define CUR_PWM_PIN     GPIO_PIN_0   /* PA0, TIMER1_CH0  ✅ PA0 不进 ADC 扫描 */
#define BUS_VOL_ADJ_PIN GPIO_PIN_6   /* PA6, TIMER2_CH0  ❌ 与 I_SENSE 撞引脚 */
#define CC_CV_PWM_PIN_CONFLICT  (1U) /* ❌ 定义后零引用（死宏） */
#define PWM_ANALOG_CALIBRATION_MODE  (0U)   /* 当前生效：正常固件分支 */
#define PWM_BASE_HZ             (20000U)    /* 20kHz，RC 滤波做 DAC ✅ */
#define CV_PWM_DUTY_INIT        (0.05f)     /* 起步 5%，软启动友好 ✅ */
#define CC_PWM_DUTY_INIT        (0.02f)     /* 起步 2% ✅ */
```

```c
#if 1
#define LLC_SOFTSTART_DURATION_MS  (500U)
#define LLC_SOFTSTART_CV_TARGET    (0.50f)
#define LLC_SOFTSTART_CC_TARGET    (0.30f)
#endif
```
⚠️ **P2**：`#if 1` 是调试残留，应删除或改为 `#ifndef LLC_SOFTSTART_DISABLED`。

### 1.8 功率级与继电器控制（L197-245）

```c
#define LLC_EN_PIN        GPIO_PIN_15  /* PB15，低电平使能（main.c:203 初始置高=关）✅ */
#define PFC_RELAY_PIN     GPIO_PIN_10  /* PC10，初始复位=断开 ✅ 上电安全 */
#define OUT_RELAY_PIN     GPIO_PIN_9   /* PA9，初始复位 ✅ */
#define FAN_CTL_PIN       GPIO_PIN_12  /* PC12，初始关 ✅ */
#define HARD_FAULT_CLR_PIN GPIO_PIN_14 /* PB14 ✅ */
#define LLC_FAULT_CHECK_PIN GPIO_PIN_12 /* PB12，输入 */
#define LED_RED_PIN       GPIO_PIN_8   /* PC8，初始亮=待机 ✅ */
#define LED_GREEN_PIN     GPIO_PIN_9   /* PC9 ✅ */
```
所有功率 GPIO 初始态均为**关断/安全**，符合要求。红绿 LED 同在 GPIOC，无端口 bug。

### 1.9 通信与调试接口（L247-283）

```c
#define CAN0_RX_PIN  GPIO_PIN_11  /* PA11 ✅ GD32F303 CAN0 默认脚 */
#define CAN0_TX_PIN  GPIO_PIN_12  /* PA12 ✅ */
#define CAN_BAUDRATE (500000U)    /* 500k ✅ */
#define DEBUG_USART  USART2       /* PB10/PB11 ✅ USART2 默认脚 */
#define SWD_DIO_PIN  GPIO_PIN_13  /* PA13 ✅ */
#define SWD_CLK_PIN  GPIO_PIN_14  /* PA14 ✅ */
```
均正确，无重映射需求。

### 1.10 中断优先级（L285-306）

```c
#define IRQ_PRIO_FAULT_PREEMPT      (0U)   /* 最高 */
#define IRQ_PRIO_DMA_PREEMPT        (1U)
#define IRQ_PRIO_SYSTICK_PREEMPT    (2U)
#define IRQ_PRIO_MEASURE_PREEMPT    (2U)  sub=1
#define IRQ_PRIO_CAN_PREEMPT        (3U)

static inline uint8_t irq_priority_encode(uint8_t preempt, uint8_t sub)
{
    return (uint8_t)(((uint32_t)preempt << 2U) | ((uint32_t)sub & 0x03U));
}
```
- ✅ `<< 2` 与 `main.c:233` 的 `NVIC_PRIGROUP_PRE2_SUB2` 一致。
- ⚠️ **`IRQ_PRIO_FAULT_PREEMPT (0)` 定义了但没有任何中断使用它**。当前故障处理是 `protect_tick_1khz()` 轮询（最坏 ~1ms 响应），没有 EXTI/BKIN 硬件刹车。这个宏给人"已有最高优先级硬件保护"的错觉。
- ⚠️ `static inline` 函数在头文件：每个包含它的 .c 生成一份，代码体积可接受，但同一函数在多编译单元内联是合法的，无隐患。

### 1.11 类型定义（L308-345）

```c
typedef enum { MAIN_STEP_INIT, ... MAIN_STEP_INVALID } charger_state_t;
typedef enum { FAULT_NONE, ... FAULT_INVALID } fault_type_t;
```
✅ 完整。注意 `FAULT_BUS_OVP`、`FAULT_BUS_UVP`、`FAULT_CHARGE_TIMEOUT` 等枚举成员**从未被 `set`**（见 P1-2）。

### 1.12 自检与工具宏（L347-370）

```c
#define ADC_STARTUP_V3V3_MIN_V      (2.8f)   /* ✅ 合理 */
#define ADC_STARTUP_V3V3_MAX_V      (3.6f)
#define ADC_STARTUP_SAMPLE_COUNT    (8U)

static inline uint32_t elapsed_since(uint32_t start_ms) { return g_ms - start_ms; }        /* ✅ 32位回绕安全 */
static inline bool elapsed_reached(uint32_t start_ms, uint32_t duration_ms) { ... }        /* ✅ */
static inline float f_clampf(float value, float low, float high) { ... }                   /* ✅ */
```
✅ 时间比较全部基于无符号减法，32 位回绕安全，写得对。

---

## 2. 缺陷清单

| 编号 | 级别 | 问题 | 位置 | 后果 |
|---|---|---|---|---|
| **P0-1** | ✅ 已修复 | uint16_t 减法下溢，母线 0V 被算成 ~8149V | `adc_dma.c:105-106` | 预充判定立即通过 → 跳过预充；启用 OVP 后误报（2026-09-02 已打补丁） |
| **P0-2** | 🔴 致命 | PA6 同时分配 I_SENSE(ADC) 与 BUS_VOL_ADJ(TIMER2_CH0) | `main.h:21/173` | 互斥功能共脚，一旦同时启用即毁电流采样 |
| **P1-1** | 🟠 高 | 温度保护数据源缺失（4 路 NTC 未扫描） | `adc_dma.c:37-40` | 过温降额/关机完全失效 |
| **P1-2** | 🟠 高 | `VBUS_OVP_V(420)` 定义了但 `FAULT_BUS_OVP` 从未 set | `protect.c` | 母线过压无软件保护 |
| **P1-3** | 🟠 高 | FAN_CS 未扫描，风扇故障检测失效 | `adc_dma.c:38` | 风扇堵转/断线不可知 |
| **P2-1** | 🟡 中 | `VBUS_TARGET_V(380)` 零引用 | `main.h:78` | 母线不可调，纸面目标 |
| **P2-2** | 🟡 中 | `AC_RMS_CALIBRATION=1.0`，整流桥压降未补偿 | `main.h:52` | RMS 系统性偏低，低压点最明显 |
| **P2-3** | 🟡 中 | `-73` 硬编码魔数，AC/BUS 共用且无注释 | `adc_dma.c:93-94` | 来源不明，无法维护 |
| **P2-4** | 🟡 中 | `CC_CV_PWM_PIN_CONFLICT` 定义后零引用 | `main.h:178` | 死宏，不驱动任何行为 |
| **P2-5** | 🟡 中 | `AC_RTOP/AC_RBOT` 与 `adc_dma.c` 的 `AC_DIV_RATIO` 重复定义 | `main.h` / `adc_dma.c:4` | 改一处不生效 |
| **P2-6** | 🟡 中 | `#if 1` 调试残留 | `main.h:187` | 代码卫生 |
| **P3-1** | ⚪ 低 | `PFC_AC_INPUT_MIN_V=180V` 会拒绝 165V 输入 | `main.h:75` | 与实测 165V 可跑矛盾 |
| **P3-2** | ⚪ 低 | `IRQ_PRIO_FAULT_PREEMPT` 无对应中断 | `main.h:287` | 误导性定义 |
| **P3-3** | ⚪ 低 | `#include <math.h>` 放在 main.h | `main.h:15` | 编译依赖扩散 |

---

## 3. P0 补丁

### 3.1 P0-1：修复无符号下溢 ✅ 已落实（2026-09-02）

**问题代码**（原 `HW/src/adc_dma.c:93-94`）：
```c
g_adc_multi.ac_vol_raw  = s_adc0_dma_buf[0] - 73;   /* raw < 73 时下溢为 65463+ */
g_adc_multi.bus_vol_raw = s_adc0_dma_buf[1] - 73;
```

**验证**：母线 = 0V 时 raw≈0 → `0 - 73` = **65463** →
`adc_raw_to_voltage(65463, 1.32M, 8.6k) = 65463/4095 × 3.3 × 154.49 ≈ 8149 V`

后果链：
1. `pfc_tick()` 的 `if (bus_vol_v >= VBUS_MIN_START_V)` → 8149 ≥ 350 → **立即进 RUN，预充等待被完全跳过**；
2. 若按建议补上 `VBUS_OVP` 判据 → 上电瞬间即误报过压；
3. AC 通道过零点同样下溢 → 65463² ≈ 4.29e9，单个样本污染整窗 RMS。

**补丁（已写入 `HW/src/adc_dma.c`）**：
```c
/* adc_dma.c:7-10 —— 失调常量命名化 + 说明下溢危害 */
#define ADC_OFFSET_COUNTS       (73U)

/* adc_dma.c:20-24 —— 饱和减法 */
static uint16_t adc_sub_offset_sat(uint16_t raw, uint16_t offset)
{
    return (raw > offset) ? (uint16_t)(raw - offset) : 0U;
}

/* adc_dma.c:105-106 —— 替换原裸减法 */
g_adc_multi.ac_vol_raw  = adc_sub_offset_sat(s_adc0_dma_buf[0], ADC_OFFSET_COUNTS);
g_adc_multi.bus_vol_raw = adc_sub_offset_sat(s_adc0_dma_buf[1], ADC_OFFSET_COUNTS);
```

**修复后行为**：母线 0V → raw≈0 → 钳到 0 → `bus_vol_v = 0V` ✅；`pfc_tick` 的 `bus_vol_v >= 350` 恢复为真实判据，预充等待不再被跳过。

> ⚠️ 尚未编译验证。改动仅为常量替换 + 新增 static 函数，风险极低。

### 3.2 P0-2：解开 PA6 引脚冲突

**现状**：靠两个编译分支互斥侥幸不炸——
- 正常分支（`PWM_ANALOG_CALIBRATION_MODE=0`，当前生效）：`main.c:263-264` 只调 cv/cc init，**不调 `bus_vol_adj_pwm_init`** → PA6 保持 AIN → 电流采样正常；
- CAL 分支：`main.c:249` 调了 `bus_vol_adj_pwm_init` → PA6 变 AF_PP，但该分支**不初始化 ADC** → 不冲突。

**风险**：任何人（包括我之前给的 B1 建议）在正常分支补上 `bus_vol_adj_pwm_init()`，就会立刻把 PA6 从 ADC 输入改成 PWM 输出，**电流采样瞬间失效且无任何编译/运行提示**。

**推荐解法**：把 BUS_VOL_ADJ 重映射到 **PC6**（GD32F303 TIMER2_CH0 完全重映射）。需确认原理图 PC6 是否空闲/已引出。

```c
/* main.h */
#define BUS_VOL_ADJ_PORT    GPIOC
#define BUS_VOL_ADJ_PIN     GPIO_PIN_6      /* PC6, TIMER2_CH0 full remap */
#define BUS_VOL_ADJ_RCU     RCU_GPIOC
```
并在 `pwm_llc.c:bus_vol_adj_pwm_init()` 中启用重映射：
```c
rcu_periph_clock_enable(RCU_AF);
gpio_pin_remap_config(GPIO_TIMER2_FULL_REMAP, ENABLE);
```

**若 PC6 不可用**（原理图未引出），则接受现状并**删除 BUS_VOL_ADJ 相关定义 + 死宏 + VBUS_TARGET_V**，明确记录"母线电压 MCU 不可调"，避免后人误补。

---

## 4. 与实测的交叉验证

| 实测现象 | 本文件对应解释 |
|---|---|
| 纯硬件使能母线能到 ~400V | ✅ 印证 `VBUS_TARGET_V` 零引用——母线由 NCP1654 硬件反馈决定，与 MCU 无关 |
| 之前判断的 "B1：正常分支漏 `bus_vol_adj_pwm_init`" | 🔄 **性质修正**：不是"漏了"，而是 PA6 冲突下的**有意规避**。修法不是补调用，而是先解引脚冲突 |
| 串口 STATUS 母线读数异常/预充瞬间通过 | 🔴 P0-1 下溢的预期表现，需实测确认 |
| 165V 能跑 1300W | ⚠️ `PFC_AC_INPUT_MIN_V=180V` 会让固件拒绝该工况，bench 时需临时下调 |
