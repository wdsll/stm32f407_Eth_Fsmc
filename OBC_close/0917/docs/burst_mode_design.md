# LLC Burst Mode 软件详细设计文档

## 文档信息
| 项目 | 内容 |
|------|------|
| 版本 | v1.0 |
| 日期 | 2026-03-23 |
| 作者 | Firmware Team |
| 状态 | 已实施 |

---

## 目录
1. [概述](#1-概述)
2. [设计目标](#2-设计目标)
3. [状态机设计](#3-状态机设计)
4. [关键参数配置](#4-关键参数配置)
5. [算法实现](#5-算法实现)
6. [时序图](#6-时序图)
7. [关键代码解析](#7-关键代码解析)
8. [调试与监控](#8-调试与监控)

---

## 1. 概述

### 1.1 什么是Burst Mode
Burst Mode（打嗝模式/突发模式）是LLC变换器在极轻载工况下的一种高效工作模式。当输出电流低于设定阈值时，控制器以间歇方式工作：
- **ON阶段**：固定高频（如180kHz）工作，不运行PI环，快速给输出电容充电
- **OFF阶段**：完全关闭PWM，依靠输出电容维持电压，实现零开关损耗

### 1.2 为什么需要Burst Mode
| 问题 | 解决方案 |
|------|----------|
| 轻载时开关损耗占比高，效率低 | 减少开关次数，降低损耗 |
| 固定频率轻载难以稳压 | 通过ON/OFF占空比调节等效输出能量 |
| 传统PWM模式轻载发热严重 | 间歇工作降低平均功耗 |

### 1.3 适用工况
- 输出电流 < 1.5A（第一档极轻载）
- 输出电压需要维持在46V±1V
- 母线电压正常（>360V）

---

## 2. 设计目标

### 2.1 核心设计原则

```
┌─────────────────────────────────────────────────────────────┐
│                    Burst Mode 设计原则                       │
├─────────────────────────────────────────────────────────────┤
│  1. 极轻载进入    → 电流<1.5A持续1秒才进入（防抖动）          │
│  2. 进入慢退出快  → 进入延迟1s，退出延迟50ms                  │
│  3. 高频准备关断  → 先升到200kHz，再关PWM（软关断）           │
│  4. ON期间不跑PI  → 固定180kHz开环工作                       │
│  5. 退出无冲击    → 记录进入前频率，退出时恢复                │
│  6. 首次软进入    → 首次进入先升频到最高，再进入OFF           │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 性能指标
| 指标 | 目标值 | 说明 |
|------|--------|------|
| 进入阈值 | 1.5A | 低于此值持续1s进入Burst |
| 退出阈值 | 2.5A | 高于此值持续50ms退出Burst |
| 滞环宽度 | 1.0A | 防止频繁进出 |
| 目标电压 | 46V | Burst模式稳压目标 |
| 电压滞环 | ±1V | ON/OFF切换窗口 |
| ON频率 | 180kHz | 固定高频，不跑PI |
| 准备频率 | 200kHz | 关断前升到最高频 |
| ON窗口 | 2~20ms | 最短2ms，超时20ms强制关 |
| OFF窗口 | 4~50ms | 最短4ms，超时50ms强制开 |

---

## 3. 状态机设计

### 3.1 主状态与Burst子状态关系

```
                    ┌─────────────────────────────────────────────┐
                    │           LLC 主状态机                        │
                    │                                             │
     ┌───────────┐  │  ┌──────────┐    ┌─────────────┐            │
     │  ST_IDLE  │◄─┼──┤ ST_LLC_RUN├───►│ST_BURST_MODE│            │
     └─────┬─────┘  │  └──────────┘    └──────┬──────┘            │
           │        │                         │                    │
           │        │                    (Burst子状态机)           │
           │        │                         │                    │
           └────────┴─────────────────────────┘                    │
                                              (电流>2.5A时退出)    │
                    └─────────────────────────────────────────────┘
```

### 3.2 Burst Mode 子状态机（核心）

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                          Burst Mode 子状态机                                  │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────────┐                                                        │
│  │  ENTRY_PREPARE   │ ◄── 首次进入Burst（软进入）                            │
│  │  (首次进入升频)   │     目标：200kHz，持续3ms                               │
│  └────────┬─────────┘                                                        │
│           │ 升频完成                                                           │
│           ▼                                                                  │
│  ┌──────────────────┐      电压 < 45V           ┌──────────────────┐        │
│  │      OFF         │ ────────────────────────► │       ON         │        │
│  │   (PWM关闭)      │  OFF超时50ms强制           │   (PWM开启)      │        │
│  │  持续时间：4~50ms │ ◄────────────────────────  │  固定180kHz      │        │
│  │  电压下降到45V   │      电压 > 47V            │  持续时间：2~20ms │        │
│  └──────────────────┘  (ON_PREPARE阶段)          └──────────────────┘        │
│           ▲                                              │                   │
│           │ 高频准备完成                                  │ ON超时20ms强制   │
│           │ 关PWM                                         ▼                  │
│  ┌──────────────────┐                            ┌──────────────────┐        │
│  │   ON_PREPARE     │ ◄────────────────────────── │   (ON阶段内)     │        │
│  │  (高频准备关断)   │     电压>47V且ON>2ms        │  电压 > 47V触发   │        │
│  │ 目标：200kHz      │                            │                  │        │
│  │ 持续：1ms后关PWM  │                            │                  │        │
│  └──────────────────┘                            └──────────────────┘        │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 3.3 状态转换条件表

| 当前状态 | 下一状态 | 转换条件 | 动作 |
|----------|----------|----------|------|
| ENTRY_PREPARE | OFF | 时间>3ms | 关PWM，频率=180kHz |
| OFF | ON | Vout<45V 且 OFF>4ms | 开PWM，频率=180kHz |
| OFF | ON | OFF超时50ms | 强制开启 |
| ON | ON_PREPARE | Vout>47V 且 ON>2ms | 升频到200kHz |
| ON | ON_PREPARE | ON超时20ms | 强制准备关断 |
| ON_PREPARE | OFF | 时间>1ms | 关PWM |
| (任意) | ST_LLC_RUN | Iout>2.5A 持续50ms | 退出Burst，恢复PI |

---

## 4. 关键参数配置

### 4.1 参数定义（main.h）

```c
/* ============================================================
 * LLC_BURST_MODE 轻载突发模式 - 配置参数
 * ============================================================ */

#define LLC_BURST_MODE_ENABLE         1         // 1: 使能Burst模式

/* ---- 进入/退出阈值（第一档：极轻载）---- */
#define LLC_BURST_IOUT_ENTER_A        (1.5f)    // 进入阈值 <1.5A
#define LLC_BURST_IOUT_EXIT_A         (2.5f)    // 退出阈值 >2.5A (滞环1A)
#define LLC_BURST_ENTER_DELAY_MS      (1000U)   // 进入延迟1秒(慢进入)
#define LLC_BURST_EXIT_DELAY_MS       (50U)     // 退出延迟50ms(快退出)

/* ---- ON/OFF 时间窗口 ---- */
#define LLC_BURST_ON_MIN_MS           (2U)      // ON最短2ms
#define LLC_BURST_ON_MAX_MS           (20U)     // ON最长20ms，超时强制关
#define LLC_BURST_OFF_MIN_MS          (4U)      // OFF最短4ms
#define LLC_BURST_OFF_MAX_MS          (50U)     // OFF最长50ms，超时强制开

/* ---- 电压控制参数 ---- */
#define LLC_BURST_VOUT_HYST_V         (1.0f)    // 电压滞环1V
#define LLC_BURST_VOUT_TARGET_V       (46.0f)   // 目标电压46V
#define LLC_BURST_VOUT_LOW_TH_V       (45.0f)   // 低于此值开启 (46-1)
#define LLC_BURST_VOUT_HIGH_TH_V      (47.0f)   // 高于此值关闭 (46+1)

/* ---- 频率参数 ---- */
#define LLC_BURST_F_HZ                (180000.0f) // Burst用高频180kHz
#define LLC_BURST_F_PREPARE_MS        (1U)        // 高频准备1ms后关PWM
#define LLC_BURST_ENTRY_RAMP_MS       (3U)        // 首次进入升频3ms
```

### 4.2 参数选择依据

#### 4.2.1 阈值选择（1.5A/2.5A）
```
负载条件分析：
├── 空载/极轻载 (~0-1A)  → 必须进入Burst
├── 轻载第一档 (1-1.5A)  → 进入Burst
├── 过渡区 (1.5-2.5A)    → 保持当前状态(滞环)
├── 轻载第二档 (2.5-5A)  → 退出Burst，运行PI
└── 正常负载 (>5A)       → 正常运行

滞环设计目的：防止负载在临界值附近波动时频繁进出Burst模式
```

#### 4.2.2 时间窗口设计
| 参数 | 值 | 设计理由 |
|------|-----|----------|
| ON_MIN=2ms | 最小开启时间 | 确保变压器充分励磁，避免磁偏 |
| ON_MAX=20ms | 最大开启时间 | 限制单周期能量，防止过压 |
| OFF_MIN=4ms | 最小关断时间 | 确保电容放电，避免连续导通 |
| OFF_MAX=50ms | 最大关断时间 | 限制电压跌落，防止欠压 |

---

## 5. 算法实现

### 5.1 核心数据结构

```c
/* Burst Mode 运行时变量 */
typedef struct {
    /* ... 其他变量 ... */
    
    /* Burst Mode 变量 */
    burst_state_t burst_state;         /* Burst 子状态 */
    uint32_t burst_begin_ms;           /* Burst 阶段开始时间 */
    float vout_burst_target;           /* Burst 模式目标电压 */
    uint32_t burst_enter_delay_ms;     /* Burst进入延迟计时 */
    uint32_t burst_exit_delay_ms;      /* Burst退出延迟计时 */
    float f_pre_burst_hz;              /* 进入Burst前的频率(用于bumpless) */
    uint8_t burst_first_entry;         /* 首次进入Burst标志 */
} llc_runtime_ctx_t;

/* Burst子状态枚举 */
typedef enum
{
    BURST_STATE_ENTRY_PREPARE  = 0,    /* 首次进入：升频准备 */
    BURST_STATE_OFF,                   /* OFF：PWM关闭 */
    BURST_STATE_ON,                    /* ON：PWM开启，固定180kHz */
    BURST_STATE_ON_PREPARE,            /* 准备关闭：升频到200kHz */
} burst_state_t;
```

### 5.2 进入Burst条件判断（ST_LLC_RUN中）

```c
case ST_LLC_RUN:
{
    /* Burst进入检测 - 慢进入，需持续轻载超过延迟时间 */
    if(s_llc_rt.meas.iout_a < LLC_BURST_IOUT_ENTER_A)
    {
        if(s_llc_rt.burst_enter_delay_ms == 0)
        {
            s_llc_rt.burst_enter_delay_ms = g_ms; /* 开始计时 */
        }
        else if(elapsed_reached(s_llc_rt.burst_enter_delay_ms, 
                                LLC_BURST_ENTER_DELAY_MS))
        {
            /* 持续轻载超过1秒，进入Burst */
            s_llc_rt.burst_enter_delay_ms = 0;
            llc_state_enter(ST_BURST_MODE);
            break;
        }
    }
    else
    {
        s_llc_rt.burst_enter_delay_ms = 0; /* 电流回升，重置计时 */
    }
    break;
}
```

### 5.3 Burst状态机处理（ST_BURST_MODE中）

```c
case ST_BURST_MODE:
{
    float vout = s_llc_rt.meas.vout_v;
    float vtarget = s_llc_rt.vout_burst_target;
    
    /* ===== 状态1：首次进入软升频 ===== */
    if(s_llc_rt.burst_state == BURST_STATE_ENTRY_PREPARE)
    {
        if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_ENTRY_RAMP_MS))
        {
            s_llc_rt.burst_state = BURST_STATE_OFF;
            s_llc_rt.burst_begin_ms = g_ms;
            llc_pwm_outputs_enable(0);
            llc_set_freq(LLC_BURST_F_HZ, true);  /* 关键切换：强制更新 */
        }
    }
    /* ===== 状态2：OFF - 等待电压下降 ===== */
    else if(s_llc_rt.burst_state == BURST_STATE_OFF)
    {
        if(vout < (vtarget - LLC_BURST_VOUT_HYST_V))
        {
            if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_OFF_MIN_MS))
            {
                s_llc_rt.burst_state = BURST_STATE_ON;
                s_llc_rt.burst_begin_ms = g_ms;
                llc_pwm_outputs_enable(1);
                llc_set_freq(LLC_BURST_F_HZ, true);  /* 关键切换：强制更新 */
            }
        }
        else if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_OFF_MAX_MS))
        {
            /* 强制开启，防止电压过低 */
            s_llc_rt.burst_state = BURST_STATE_ON;
            s_llc_rt.burst_begin_ms = g_ms;
            llc_pwm_outputs_enable(1);
            llc_set_freq(LLC_BURST_F_HZ, true);  /* 关键切换：强制更新 */
        }
    }
    /* ===== 状态3：ON_PREPARE - 高频准备关断 ===== */
    else if(s_llc_rt.burst_state == BURST_STATE_ON_PREPARE)
    {
        if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_F_PREPARE_MS))
        {
            s_llc_rt.burst_state = BURST_STATE_OFF;
            s_llc_rt.burst_begin_ms = g_ms;
            llc_pwm_outputs_enable(0);
            llc_set_freq(LLC_BURST_F_HZ, true);  /* 关键切换：强制更新 */
        }
    }
    /* ===== 状态4：ON - 固定高频工作 ===== */
    else /* BURST_STATE_ON */
    {
        if(vout > (vtarget + LLC_BURST_VOUT_HYST_V))
        {
            if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_ON_MIN_MS))
            {
                /* 进入高频准备阶段，而不是立即关PWM */
                s_llc_rt.burst_state = BURST_STATE_ON_PREPARE;
                s_llc_rt.burst_begin_ms = g_ms;
                llc_set_freq(LLC_F_MAX_HZ, true);  /* 关键切换：强制更新 */
            }
        }
        else if(elapsed_reached(s_llc_rt.burst_begin_ms, LLC_BURST_ON_MAX_MS))
        {
            /* 强制高频准备后关闭 */
            s_llc_rt.burst_state = BURST_STATE_ON_PREPARE;
            s_llc_rt.burst_begin_ms = g_ms;
            llc_set_freq(LLC_F_MAX_HZ, true);  /* 关键切换：强制更新 */
        }
    }
    
    /* ===== 退出Burst检测 - 快退出 ===== */
    if(s_llc_rt.meas.iout_a > LLC_BURST_IOUT_EXIT_A)
    {
        if(s_llc_rt.burst_exit_delay_ms == 0)
        {
            s_llc_rt.burst_exit_delay_ms = g_ms;
        }
        else if(elapsed_reached(s_llc_rt.burst_exit_delay_ms, 
                                LLC_BURST_EXIT_DELAY_MS))
        {
            /* 电流持续超过阈值，快速退出Burst */
            s_llc_rt.burst_exit_delay_ms = 0;
            llc_pwm_outputs_enable(1);
            
            /* 退出时bumpless：用进入Burst前的频率做初始化 */
            llc_ctrl_bumpless_init(LLC_VOUT_TARGET_V, vout, 
                                   s_llc_rt.f_pre_burst_hz);
            s_llc.f_cmd = s_llc_rt.f_pre_burst_hz;
            llc_set_freq(s_llc.f_cmd, true);  /* 关键切换：强制更新 */
            
            llc_state_enter(ST_LLC_RUN);
        }
    }
    else
    {
        s_llc_rt.burst_exit_delay_ms = 0;
    }
    break;
}
```

### 5.4 状态初始化（llc_state_enter）

```c
case ST_BURST_MODE:
    llc_driver_en_set(true);
    s_llc_rt.vout_burst_target = LLC_VOUT_TARGET_V;
    
    /* 记录进入Burst前的频率，用于退出时bumpless */
    s_llc_rt.f_pre_burst_hz = f_clampf(s_llc.f_cmd, s_llc.f_min, s_llc.f_max);
    
    if(s_llc_rt.burst_first_entry)
    {
        /* 首次进入Burst：先升频到最高频准备，再进入OFF */
        s_llc_rt.burst_state = BURST_STATE_ENTRY_PREPARE;
        s_llc_rt.burst_first_entry = 0;
        llc_pwm_outputs_enable(1);
        llc_set_freq(LLC_F_MAX_HZ, true);  /* 关键切换：强制更新 */
    }
    else
    {
        /* 非首次进入：直接进入OFF状态 */
        s_llc_rt.burst_state = BURST_STATE_OFF;
        llc_pwm_outputs_enable(0);
        llc_set_freq(LLC_BURST_F_HZ, true);  /* 关键切换：强制更新 */
    }
    s_llc_rt.burst_begin_ms = g_ms;
    break;
```

---

## 6. 时序图

### 6.1 首次进入Burst软进入过程

```
时间(ms)    0         3         10        20        30        40
            │         │         │         │         │         │
电流(A)     ▼         ▼         ▼         ▼         ▼         ▼
  3.0 ─────────────────────────────────────────────────────────  退出阈值
            │         │         │         │         │         │
  2.5 ──────┼─────────┼─────────┼─────────┼─────────┼─────────  退出阈值
            │         │         │         │         │         │
  1.5 ──────┼─────────┼─────────┼─────────┼─────────┼─────────  进入阈值
            │▓▓▓▓▓▓▓▓▓│         │         │         │         │
  0.5 ──────▓─────────▓────────────────────────────────────────  实际电流
            │         │         │         │         │         │
状态        │ ENTRY   │  OFF    │   ON    │  OFF    │   ON    │
            │ PREPARE │         │         │         │         │
            │         │         │         │         │         │
PWM         │█████████│         │█████████│         │█████████│
            │         │         │         │         │         │
频率(kHz)   │         │         │         │         │         │
  200 ──────┤░░░░░░░░░┤         │         │         │         │
            │░░░░░░░░░│         │         │         │         │
  180 ──────┤░░░░░░░░░┤─────────┤░░░░░░░░░┤─────────┤░░░░░░░░░│
            │         │         │         │         │         │
            ▼         ▼         ▼         ▼         ▼         ▼

图例：███ = PWM开启    ░░░ = 频率设置
```

### 6.2 正常Burst工作周期

```
时间(ms)    0    2    4    6    8    10   12   14   16
            │    │    │    │    │    │    │    │    │
电压(V)     ▼    ▼    ▼    ▼    ▼    ▼    ▼    ▼    ▼
  47 ───────────────────────────────────────────────────  关闭阈值
            │         ┌────┐              ┌────┐
  46 ───────┤─────────┤    ├──────────────┤    ├───────  目标电压
            │         │    │              │    │
  45 ───────┘         └────┘              └────┘         开启阈值
            │    │    │    │    │    │    │    │    │
状态        │OFF │    │    │ ON │PREP│OFF │    │ ON │
            │    │    │    │    │ARE │    │    │    │
            │    │    │    │    │    │    │    │    │
PWM         │    │    │    │████│████│    │    │████│
            │    │    │    │    │    │    │    │    │
频率        │    │    │    │180 │200 │    │    │180 │
(kHz)       │    │    │    │    │    │    │    │    │
            ▼    ▼    ▼    ▼    ▼    ▼    ▼    ▼    ▼
```

### 6.3 退出Burst过程（bumpless）

```
时间(ms)    0         50        100       150
            │         │         │         │
电流(A)     ▼         ▼         ▼         ▼
  3.0 ──────┼─────────┼─────────┼─────────┼─────────────  退出阈值
            │         │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  实际电流>2.5A
  2.5 ──────┼─────────┼▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
            │         │         │         │
  1.5 ──────┼─────────┼─────────┼─────────┼─────────────  进入阈值
            │         │         │         │
状态        │  Burst  │  Burst  │ ST_LLC  │
            │   ON    │   ON    │   RUN   │
            │         │(50ms后) │         │
            │         │         │         │
频率(kHz)   │         │         │         │
  180 ──────┤░░░░░░░░░┤─────────┤         │
            │         │         │         │
  120 ──────┤         │         │         │
            │         │         │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  恢复PI环
  100 ──────┤         │         │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  频率渐进
            │         │         │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
            │         │         │(从100kHz │
            │         │         │ 开始恢复)│
            ▼         ▼         ▼         ▼

关键：退出时用f_pre_burst_hz（如100kHz）做bumpless初始化，
      而不是用Burst的180kHz，避免频率跳变
```

---

## 7. 关键代码解析

### 7.1 定时器更新策略（关键安全改进）

```c
/**
 * 频率设置函数（已增强安全策略）
 * 参数：force_update - 是否强制立即更新（关键状态切换时需要）
 * 策略：
 *   - 正常运行PI调节：force_update=false（保持自然更新节奏）
 *   - Burst开关包/强制启动/恢复/停止等关键切换：force_update=true
 */
void llc_pwm_set_freq(uint32_t f_hz, bool force_update)
{
    /* 更新 ARR（CAR），写入影子寄存器 */
    TIMER_CAR(TIMER0) = s_period;
    
    /* 根据占空比计算CCR并更新 */
    uint16_t pwm_ccr = duty_to_ccr(s_cfg.duty);
    timer_channel_output_pulse_value_config(TIMER0, LLC_PWM_CH, pwm_ccr);
    
    /* 根据场景决定是否强制立即更新 */
    if (force_update) {
        timer_event_software_generate(TIMER0, TIMER_EVENT_SRC_UPG);
    }
    /* force_update=false时：不触发UEV，等待自然更新边界（当前周期结束） */
}
```

**更新策略说明**：
```
┌─────────────────────────────────────────────────────────────┐
│                  定时器更新策略分类                           │
├─────────────────────────────────────────────────────────────┤
│  需要强制更新（force_update=true）的场景：                 │
│  ├─ Burst模式状态切换（ON↔OFF）                            │
│  ├─ 强制启动/停止（Force start/stop）                       │
│  ├─ 故障恢复/系统重启                                        │
│  └─ 软启动/软停止关键节点                                    │
│                                                              │
│  保持自然更新（force_update=false）的场景：                 │
│  ├─ 正常运行中的PI闭环微调                                   │
│  ├─ 负载变化引起的频率渐变                                   │
│  └─ 稳态运行的频率小范围调整                                 │
└─────────────────────────────────────────────────────────────┘
```

**安全改进意义**：
- **防止硬件损坏**：避免ARR/CCR不同步导致变压器饱和、MOSFET失效
- **提高系统稳定性**：关键切换时确保配置立即生效
- **优化性能**：正常运行时减少不必要的UEV开销

### 7.2 无冲击切换（bumpless）原理

```c
static void llc_ctrl_bumpless_init(float vref, float vmeas, float f_now)
{
    float kp = s_llc.kp;
    float f_min = s_llc.f_min;
    float f_max = s_llc.f_max;
    float f_nom = s_llc.f_nom;
    
    if (f_nom < f_min || f_nom > f_max) {
        f_nom = f_clampf(f_now, f_min, f_max);
    }
    
    float e = vref - vmeas;
    
    /* 核心：计算积分器初始值，使得PI输出 = 当前频率 */
    /* u = kp*e + integ = f_nom - f_now  */
    /* integ = (f_nom - f_now) - kp*e   */
    float integ = (f_nom - f_now) - kp * e;
    float i_lim = f_max - f_min;
    
    /* 限幅 */
    if (integ > i_lim) integ = i_lim;
    else if (integ < -i_lim) integ = -i_lim;
    
    s_llc.integ = integ;
    s_llc.f_cmd = f_clampf(f_now, f_min, f_max);
}
```

**原理说明**：
- 正常PI公式：`f_cmd = f_nom - (kp*e + integ)`
- 切换时希望`f_cmd = f_now`（当前实际频率）
- 反推积分器：`integ = (f_nom - f_now) - kp*e`
- 这样切换瞬间输出不会跳变

### 7.2 函数签名更新

```c
/* 函数签名已更新，增加force_update参数 */
void llc_pwm_set_freq(uint32_t f_hz, bool force_update);
static void llc_set_freq(float hz, bool force_update);
```

**调用场景分类**：
```c
/* 场景1：Burst模式状态切换（需要强制更新） */
llc_set_freq(LLC_BURST_F_HZ, true);  // Burst ON
llc_set_freq(LLC_F_MAX_HZ, true);   // Burst准备关断

/* 场景2：正常PI调节（自然更新边界） */
llc_set_freq(new_freq, false);       // 正常运行微调

/* 场景3：退出Burst恢复（强制更新） */
llc_set_freq(s_llc_rt.f_pre_burst_hz, true);  // 退出时恢复
```

### 7.3 进入Burst时的频率记录

```c
case ST_BURST_MODE:
    /* 记录进入Burst前的频率，用于退出时bumpless */
    s_llc_rt.f_pre_burst_hz = f_clampf(s_llc.f_cmd, s_llc.f_min, s_llc.f_max);
```

**为什么需要**：
- 假设正常运行时频率为100kHz（对应某个负载点）
- 进入Burst后ON阶段固定180kHz
- 如果退出时直接用180kHz做bumpless，频率会跳变80kHz
- 记录100kHz，退出时恢复，实现平滑过渡

### 7.3 首次软进入机制

```c
if(s_llc_rt.burst_first_entry)
{
    s_llc_rt.burst_state = BURST_STATE_ENTRY_PREPARE;
    s_llc_rt.burst_first_entry = 0;
    llc_pwm_outputs_enable(1);
    llc_set_freq(LLC_F_MAX_HZ);  /* 200kHz */
}
```

**为什么需要**：
- 从正常运行（如100kHz）突然关PWM进入Burst OFF，可能产生电压跌落或磁饱和
- 先升到200kHz（最高频）运行3ms：
  - 高频→小增益→输出能量小
  - 给控制环路和硬件一个过渡
  - 然后再关PWM进入OFF，更平滑

---

## 8. 调试与监控

### 8.1 关键观测点

| 变量名 | 说明 | 正常范围 |
|--------|------|----------|
| `s_llc_rt.burst_state` | Burst子状态 | 0~3 |
| `s_llc_rt.meas.iout_a` | 输出电流 | 0~3A |
| `s_llc_rt.meas.vout_v` | 输出电压 | 45~47V |
| `s_llc.f_cmd` | 频率命令 | 75~200kHz |
| `s_llc_rt.f_pre_burst_hz` | 进入前频率 | 75~150kHz |

### 8.2 状态码定义

```c
typedef enum
{
    BURST_STATE_ENTRY_PREPARE  = 0,    /* 首次进入升频 */
    BURST_STATE_OFF            = 1,    /* OFF：PWM关 */
    BURST_STATE_ON             = 2,    /* ON：PWM开，180kHz */
    BURST_STATE_ON_PREPARE     = 3,    /* 准备关：200kHz */
} burst_state_t;
```

### 8.3 调试日志建议

```c
/* 在llc_app_tick_1khz中添加 */
#if LLC_BURST_DEBUG
static uint32_t last_burst_log_ms = 0;
if(g_ms - last_burst_log_ms > 100)  /* 每100ms打印一次 */
{
    last_burst_log_ms = g_ms;
    debug_printf("[BURST] state=%d iout=%.2f vout=%.2f f=%.0f\n",
                 s_llc_rt.burst_state,
                 s_llc_rt.meas.iout_a,
                 s_llc_rt.meas.vout_v,
                 s_llc.f_cmd);
}
#endif
```

### 8.4 常见问题排查

| 现象 | 可能原因 | 排查方法 |
|------|----------|----------|
| 不进入Burst | 电流阈值设置过高 | 检查`LLC_BURST_IOUT_ENTER_A` |
| 频繁进出Burst | 滞环太小或滤波不足 | 检查电流采样滤波 |
| 电压波动大 | ON/OFF时间窗口不合适 | 调整`LLC_BURST_ON_MAX_MS` |
| 退出时频率跳变 | `f_pre_burst_hz`未正确记录 | 检查状态切换时序 |
| 首次进入有冲击 | 软进入未生效 | 检查`burst_first_entry`标志 |

---

## 9. 附录

### 9.1 缩写表

| 缩写 | 全称 | 说明 |
|------|------|------|
| Burst | Burst Mode | 突发模式/打嗝模式 |
| ON | ON State | PWM开启阶段 |
| OFF | OFF State | PWM关闭阶段 |
| bumpless | Bumpless Transfer | 无冲击切换 |
| PI | Proportional-Integral | 比例积分控制器 |

### 9.2 相关文件

| 文件 | 说明 |
|------|------|
| `HW/src/llc_control.c` | 主实现文件 |
| `HW/include/llc_control.h` | 头文件，状态定义 |
| `APP/include/main.h` | 参数配置文件 |

### 9.3 版本历史

| 版本 | 日期 | 修改内容 |
|------|------|----------|
| v1.0 | 2026-03-23 | 初始版本，完成首次进入、bumpless退出、频率恢复 |

---

**文档结束**
