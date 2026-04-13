#ifndef CHARGE_CTRL_H
#define CHARGE_CTRL_H

#include "main.h"

typedef enum
{
    CHG_ST_IDLE = 0,
    CHG_ST_PRECHARGE,
    CHG_ST_RELAY_ON,
    CHG_ST_CC,
    CHG_ST_CV,
    CHG_ST_STOPPING,   /* LLC软停中，等待ST_IDLE后再断继电器 */
    CHG_ST_DONE,
    CHG_ST_FAULT,
} charge_state_t;

/* charge 层停止/故障原因枚举，与 state 正交 */
typedef enum
{
    CHG_STOP_NONE = 0,       /* 正常运行中，无停止 */
    CHG_STOP_DONE,            /* 充电完成正常终止 */
    CHG_STOP_LLC_FAULT,       /* LLC 底层故障（OVP/OCP/OTP） */
    CHG_STOP_LLC_DROPOUT,     /* LLC 运行中异常掉出 RUN */
    CHG_STOP_RELAY_FAIL,      /* 继电器上电自检失败 */
    CHG_STOP_PRECHARGE_TIMEOUT,/* 预充超时（压差始终未匹配） */
    CHG_STOP_STOPPING_TIMEOUT, /* STOPPING 超时（软停未在规定时间内完成） */
} charge_stop_reason_t;

typedef struct
{
    float cv_target_v;            /* 恒压目标 */
    float cc_target_a;            /* 恒流目标 */
    float precharge_current_a;    /* 预充阶段限流 */
    float precharge_margin_v;     /* 开路拉升到 VBT + margin */
    float v_match_window_v;       /* VOUT 与 VBT 的匹配窗口 */
    float vbat_present_min_v;     /* 模拟电池存在阈值 */
    float vbat_absent_max_v;      /* 掉电/拔掉阈值 */
    float cv_enter_margin_v;      /* 进入CV的电压裕量 */
    float term_current_a;         /* 截止电流 */
    uint32_t cv_enter_hold_ms;    /* CC→CV：电压进入窗口后保持时间 */
    uint32_t detect_debounce_ms;  /* 电池检测去抖 */
    uint32_t precharge_hold_ms;   /* 压差匹配保持时间 */
    uint32_t precharge_timeout_ms;/* 预充超时（entry_ms 计时），0=不使能 */
    uint32_t stopping_timeout_ms; /* STOPPING 兜底超时（LLC 软停应很快，5-10s），0=不使能 */
    uint32_t relay_settle_ms;     /* 继电器闭合稳定时间 */
    uint32_t term_hold_ms;        /* 截止保持时间 */
} charge_cfg_t;

typedef struct
{
    charge_state_t state;
    float vbat_v;
    float vout_v;
    float iout_a;
    charge_stop_reason_t stop_reason;   /* 上次停止/故障原因 */
} charge_status_t;

void charge_ctrl_init(void);
void charge_ctrl_tick_1khz(void);

void charge_ctrl_set_cfg(const charge_cfg_t *cfg);
void charge_ctrl_set_target(float cv_v, float cc_a);

charge_state_t charge_ctrl_state(void);
charge_stop_reason_t charge_ctrl_stop_reason(void);
void charge_ctrl_get_status(charge_status_t *st);



#endif