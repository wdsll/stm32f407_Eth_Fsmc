#include "pfc_control.h"
#include <math.h>

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

/* ===== Hardware scaling ===== */
#ifndef PFC_VBUS_RTOP_OHM
#define PFC_VBUS_RTOP_OHM      (2000000.0f)
#endif
#ifndef PFC_VBUS_RBOT_OHM
#define PFC_VBUS_RBOT_OHM      (10000.0f)
#endif
#ifndef PFC_AC_RTOP_OHM
#define PFC_AC_RTOP_OHM        (20000.0f)
#endif
#ifndef PFC_AC_RBOT_OHM
#define PFC_AC_RBOT_OHM        (10000.0f)
#endif
#ifndef PFC_NTC_PULLUP_OHM
#define PFC_NTC_PULLUP_OHM     (10000.0f)
#endif
#ifndef PFC_NTC_R0_OHM
#define PFC_NTC_R0_OHM         (10000.0f)
#endif
#ifndef PFC_NTC_BETA
#define PFC_NTC_BETA           (3950.0f)
#endif
/*
AC 相关 4 个宏：定义 什么电压范围 才算正常市电、掉电多久才算真正掉、AC 过压时立刻停机。
预充相关 3 个宏：定义 母线要充到多少伏 才吸合继电器、充多久还上不去就宣告失败、继电器吸合后要等多久才开始拉 BUS_ADJ。
Ready 相关 2 个宏：母线电压达到多少并保持多久，才认为 PFC “ready”，可以放行 LLC。
母线保护 2 个宏：过压关机点 + 欠压关机点。
温度 2 个宏：预警温度（你还没用）和硬关机温度。
故障重启 1 个宏：故障后最短等待多久才允许再试一次。
*/

/* ===== Operating thresholds ===== */
#define PFC_AC_VALID_MIN_VRMS          (85.0f)   //认为“有市电”的最低有效电压（RMS）
#define PFC_AC_VALID_MAX_VRMS          (265.0f) //认为“正常输入”的最高市电电压（RMS），对应 230Vac 标准上限。
#define PFC_AC_LOSS_DEBOUNCE_MS        (200U)   //AC 检测的“防抖时间”。
#define PFC_PRECHARGE_TARGET_V         (230.0f) //直流母线预充完成的目标电压。
#define PFC_PRECHARGE_TIMEOUT_MS       (800U)  //PFC_PRECHARGE_TIMEOUT_MS = 800 ms
#define PFC_RELAY_SETTLE_MS            (80U)  //主继电器从“拉合”到“稳定导通”的等待时间。
//在 PFC_ST_RAMP_UP 中，当 vbus_v >= PFC_TARGET_MIN_V - 5V（比如 ≥355V）且持续一定时间，就认为 PFC 已经稳定，切到 RUN，允许 LLC 启动。
#define PFC_READY_HYSTERESIS_V         (5.0f) //PFC“已就绪”的电压滞回窗口,避免母线在目标附近轻微波动时，让“就绪/未就绪”频繁切换.
//防止刚拉升时短暂穿过门限就让 LLC 误判“母线 OK”，导致 LLC 一启动母线又掉。
#define PFC_READY_STABLE_MS            (80U) //母线电压要在“就绪范围内”持续的时间。
#define PFC_VBUS_OVP_V                 (430.0f)
#define PFC_VBUS_UVP_V                 (320.0f)
//温度保护
#define PFC_TEMP_WARN_C                (100.0f)
#define PFC_TEMP_FAULT_C               (120.0f)

//防止在瞬时过压、过温等故障情况下频繁重启（抖动），给硬件留一段“冷却 / 放电”时间。
#define PFC_FAULT_RESTART_MS           (2000U) //发生故障后，允许自动重启前需要等待的时间（2s）。
#define PFC_AC_OVERVOLTAGE_MARGIN_V    (5.0f)  //在上面的 max 基础上加一个“软保护 margin”。

/* ===== Bus voltage command shaping ===== */
#define PFC_TARGET_DEFAULT_V           (VBUS_TARGET_V)
#define PFC_TARGET_MIN_V               (360.0f)
#define PFC_TARGET_MAX_V               (410.0f)
#define PFC_TARGET_FROM_BAT_GAIN       (1.05f)
#define PFC_TARGET_MIN_FROM_BAT_V      (360.0f)
#define PFC_RAMP_UP_RATE_V_PER_MS      (2.0f)
#define PFC_RAMP_DOWN_RATE_V_PER_MS    (4.0f)

/* ===== BUS_VOL_ADJ PWM 待定 ===== */
#define PFC_ADJ_DUTY_NEUTRAL           (0.50f)
#define PFC_ADJ_DUTY_MIN               (0.05f)
#define PFC_ADJ_DUTY_MAX               (0.95f)
#define PFC_ADJ_KP                     (0.0025f)
#define PFC_ADJ_KI                     (0.0006f)
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef struct {
    float vac_v; //当前 AC 电压“瞬时值/等效值”（运放输出换算到一次侧后的电压，单位 V）
    float vac_rms; //AC 电压的 RMS（有效值，单位 V），带低通/去抖，用于判断 AC_OK
    float vbus_v; //PFC 直流母线电压（隔离采样 + 放大后反算到母线侧，单位 V）
    float vbat_v; //电池端电压 / DC 输出端电压（来自 VBT_SENSE，单位 V）
    float vout_v; //LLC 输出电压（来自 VOUT_SENSE，单位 V）
    float tpfc_c; //PFC 功率器件 NTC 温度（换算后的摄氏度）
	 /* ---- 原始 ADC 码值（方便调试/标定） ---- */
    uint16_t vac_raw;
    uint16_t vbus_raw;
    uint16_t tpfc_raw;
    uint16_t vbat_raw;
    uint16_t vout_raw;
} pfc_measure_t;
/**
 * @brief PFC 控制上下文（状态机 + 命令 + 量测数据）
 *
 * 所有 PFC 运行时需要记住的变量都收进来，方便在 pfc_tick_1khz() 中统一管理。
 */
typedef struct {
    pfc_state_t state;
    uint32_t entry_ms;

    uint32_t ac_ok_since_ms;
    uint32_t ready_since_ms;
    uint32_t precharge_begin_ms;
    uint32_t fault_since_ms;

    float vbus_target;     /* 上层目标 */
    float vbus_cmd;        /* 内部斜坡命令 */

    float bus_adj_integ;
    float bus_adj_duty;

    bool enable_cmd;
    bool fault_latched;
    pfc_measure_t meas; ///< 最近一次采样的 PFC 测量数据（电压、电流、温度等）
} pfc_ctx_t;

static pfc_ctx_t s_pfc;

/*********************************************************************************************************
*                                              小工具函数
*********************************************************************************************************/
static float adc_to_v_div(uint16_t raw, float rtop, float rbot)
{
    float v_adc = (raw * VREF_ADC) / 4095.0f;
    return v_adc * (rtop + rbot) / rbot;
}

static float adc_ntc_to_c(uint16_t raw)
{	
    float v_adc = (raw * VREF_ADC) / 4095.0f;
    if (v_adc <= 0.001f) {
        return 150.0f;
    }
    float r_ntc = (v_adc * PFC_NTC_PULLUP_OHM) / (VREF_ADC - v_adc);
    if (r_ntc <= 0.0f) {
        return 150.0f;
    }
    float inv_t = (1.0f / (273.15f + 25.0f)) + (1.0f / PFC_NTC_BETA) * logf(r_ntc / PFC_NTC_R0_OHM);
    float temp_k = 1.0f / inv_t;
    return temp_k - 273.15f;
}

static float lpf(float prev, float sample, float alpha)
{
    return prev + alpha * (sample - prev);
}

/*********************************************************************************************************
*                                              硬件输出（继电器 + BUS_ADJ）
*********************************************************************************************************/
/* 继电器脚：优先用 PFC_MAIN_RELAY_*，若工程里没定义可退回 PFC_EN_*（兼容你旧工程） */
#if defined(PFC_MAIN_RELAY_PORT) && defined(PFC_MAIN_RELAY_PIN) && defined(PFC_MAIN_RELAY_RCU)
#define PFC_RELAY_PORT   PFC_MAIN_RELAY_PORT
#define PFC_RELAY_PIN    PFC_MAIN_RELAY_PIN
#define PFC_RELAY_RCU    PFC_MAIN_RELAY_RCU
#elif defined(PFC_EN_PORT) && defined(PFC_EN_PIN) && defined(PFC_EN_RCU)
#define PFC_RELAY_PORT   PFC_EN_PORT
#define PFC_RELAY_PIN    PFC_EN_PIN
#define PFC_RELAY_RCU    PFC_EN_RCU
#endif

void pfc_hw_set_relay(bool closed)
{
#if defined(PFC_RELAY_PORT) && defined(PFC_RELAY_PIN) && defined(PFC_RELAY_RCU)
    static bool s_last = false;
    if (s_last == closed) {
        return;
    }
    s_last = closed;

    rcu_periph_clock_enable(PFC_RELAY_RCU);
    gpio_init(PFC_RELAY_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PFC_RELAY_PIN);

    if (closed) {
        gpio_bit_set(PFC_RELAY_PORT, PFC_RELAY_PIN);
    } else {
        gpio_bit_reset(PFC_RELAY_PORT, PFC_RELAY_PIN);
    }
#else
    (void)closed;
#endif
}

static void pfc_bus_adj_set(float duty)
{
    if (duty <= 0.0f) {
        duty = 0.0f;
    } else {
        duty = CLAMP(duty, PFC_ADJ_DUTY_MIN, PFC_ADJ_DUTY_MAX);
    }
    pb0_pwm_set_duty(duty);
    s_pfc.bus_adj_duty = duty;
}

/* 关闭所有输出：BUS_ADJ=0 + 继电器释放 + 积分清零 */
static void pfc_outputs_off(void)
{
    pfc_bus_adj_set(0.0f);
    s_pfc.bus_adj_integ = 0.0f;
    pfc_hw_set_relay(false);
}

/*********************************************************************************************************
*                                              目标电压与 BUS_ADJ 控制
*********************************************************************************************************/
static float pfc_target_from_battery(float manual_target, float vbat)
{
    float dyn_target = CLAMP(manual_target, PFC_TARGET_MIN_V, PFC_TARGET_MAX_V);
    float bat_target = CLAMP(vbat * PFC_TARGET_FROM_BAT_GAIN, PFC_TARGET_MIN_FROM_BAT_V, PFC_TARGET_MAX_V);
    if (bat_target > dyn_target) {
        dyn_target = bat_target;
    }
    return dyn_target;
}

static void pfc_bus_adj_tick(float vbus_meas)
{
    float error = s_pfc.vbus_cmd - vbus_meas;

    float integ = s_pfc.bus_adj_integ + (PFC_ADJ_KI * error);
    float duty_unclamped = PFC_ADJ_DUTY_NEUTRAL + (PFC_ADJ_KP * error) + integ;
    float duty = CLAMP(duty_unclamped, PFC_ADJ_DUTY_MIN, PFC_ADJ_DUTY_MAX);

    /* anti-windup：若打限幅，把积分回算到“刚好在限幅边界” */
    if (duty != duty_unclamped) {
        integ = duty - PFC_ADJ_DUTY_NEUTRAL - (PFC_ADJ_KP * error);
    }

    float integ_min = PFC_ADJ_DUTY_MIN - PFC_ADJ_DUTY_NEUTRAL;
    float integ_max = PFC_ADJ_DUTY_MAX - PFC_ADJ_DUTY_NEUTRAL;
    s_pfc.bus_adj_integ = CLAMP(integ, integ_min, integ_max);

    pfc_bus_adj_set(duty);
}

/*********************************************************************************************************
*                                              状态机辅助
*********************************************************************************************************/
static void pfc_state_enter(pfc_state_t next)
{
    if (s_pfc.state == next) {
        return;
    }
    s_pfc.state = next;
    s_pfc.entry_ms = g_ms;
    if (next == PFC_ST_FAULT) {
        s_pfc.fault_since_ms = g_ms;
        s_pfc.fault_latched = true;
    }
    if (next == PFC_ST_PRECHARGE) {
        s_pfc.precharge_begin_ms = g_ms;
        s_pfc.ready_since_ms = 0U;
        /* 被动预充阶段不输出 BUS_ADJ，命令电压无意义，置 0 */
        s_pfc.vbus_cmd = 0.0f;
        s_pfc.bus_adj_integ = 0.0f;
    }

    if (next == PFC_ST_RAMP_UP) {
        s_pfc.ready_since_ms = 0U;
        /* vbus_cmd 起步策略：从当前实测 vbus 起步，避免 PI 一上来打满 */
        s_pfc.vbus_cmd = s_pfc.meas.vbus_v;
        s_pfc.bus_adj_integ = 0.0f;
    }

    if (next == PFC_ST_RUN) {
        /* 进入 RUN 后继续用 ready_since_ms 作为运行中计时也无所谓，这里清零即可 */
        s_pfc.ready_since_ms = 0U;
    }
}

static void pfc_sample_inputs(void)
{
    s_pfc.meas.vac_raw = adc1_aux_read_channel(AC_VOL_SAMPLE, ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vbus_raw = adc1_aux_read_channel(BUS_VOL_SAMPLE, ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.tpfc_raw = adc1_aux_read_channel(T_SENSE_PFC_MOS, ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vbat_raw = adc1_aux_read_channel(VBT_SENSE_CH, ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vout_raw = adc1_aux_read_channel(VOUT_SENSE_CH, ADC_SAMPLETIME_71POINT5);

    float vac = adc_to_v_div(s_pfc.meas.vac_raw, PFC_AC_RTOP_OHM, PFC_AC_RBOT_OHM);
    float vbus = adc_to_v_div(s_pfc.meas.vbus_raw, PFC_VBUS_RTOP_OHM, PFC_VBUS_RBOT_OHM);
    float vbat = adc_to_v_div(s_pfc.meas.vbat_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
    float vout = adc_to_v_div(s_pfc.meas.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
    float tpfc = adc_ntc_to_c(s_pfc.meas.tpfc_raw);

    s_pfc.meas.vac_v   = vac;
    s_pfc.meas.vbus_v  = vbus;
    s_pfc.meas.vbat_v  = vbat;
    s_pfc.meas.vout_v  = vout;
    s_pfc.meas.tpfc_c  = tpfc;

    /* 这里 vac_rms 实际是低通后的“等效值”，你后续如有真 RMS 计算可替换 */
    s_pfc.meas.vac_rms = lpf(s_pfc.meas.vac_rms, vac, 0.05f);
}

static bool pfc_ac_ok(void)
{
    float vac = s_pfc.meas.vac_rms;
    return (vac >= PFC_AC_VALID_MIN_VRMS) && (vac <= (PFC_AC_VALID_MAX_VRMS + PFC_AC_OVERVOLTAGE_MARGIN_V));
}

static bool pfc_ac_overvoltage(void)
{
    return s_pfc.meas.vac_rms > (PFC_AC_VALID_MAX_VRMS + PFC_AC_OVERVOLTAGE_MARGIN_V);
}

static void pfc_handle_fault(const char *reason)
{
    (void)reason;
    /* 故障：必须立即释放继电器 + BUS_ADJ=0 */
    pfc_outputs_off();
    pfc_state_enter(PFC_ST_FAULT);
}

/*********************************************************************************************************
*                                              对外 API
*********************************************************************************************************/
void pfc_init(void)
{
    s_pfc = (pfc_ctx_t){0};
    s_pfc.vbus_target = CLAMP(PFC_TARGET_DEFAULT_V, PFC_TARGET_MIN_V, PFC_TARGET_MAX_V);
    s_pfc.vbus_cmd = 0.0f;
    s_pfc.state = PFC_ST_OFF;
    pb0_pwm_init(PB0_PWM_BASE_HZ);
    pb0_pwm_set_duty(0.0f);
    adc1_aux_init();
    pfc_outputs_off();
}

void pfc_enable(void)
{
    s_pfc.enable_cmd = true;
    if (s_pfc.state == PFC_ST_OFF) {
        pfc_state_enter(PFC_ST_WAIT_AC);
    }
}
void pfc_disable(void)
{
    s_pfc.enable_cmd = false;
}

void pfc_set_vbus_target(float vbus_v)
{
    s_pfc.vbus_target = CLAMP(vbus_v, PFC_TARGET_MIN_V, PFC_TARGET_MAX_V);
}

/*********************************************************************************************************
*                                              1kHz tick
*********************************************************************************************************/
void pfc_tick_1khz(void)
{
    pfc_sample_inputs();

    /* Hardware fault or latched protection */
    if (protect_fault_active_hw() || protect_fault_latched()) {
        pfc_handle_fault("HARD_PRO");
        return;
    }

    if (s_pfc.meas.tpfc_c >= PFC_TEMP_FAULT_C) {
        pfc_handle_fault("TEMP");
        return;
    }

    /* AC overvoltage is treated as a fault */
    if (pfc_ac_overvoltage()) {
        pfc_handle_fault("VAC_OV");
        return;
    }

    if (s_pfc.meas.vbus_v >= PFC_VBUS_OVP_V) {
        pfc_handle_fault("VBUS_OV");
        return;
    }

    if (s_pfc.state == PFC_ST_FAULT) {
        pfc_outputs_off();
        return;
    }

    /* 3) disable：优雅停机（先降 vbus_cmd，期间保持继电器吸合；降到 0 再释放继电器） */
    if (!s_pfc.enable_cmd) {
        bool keep_relay = (s_pfc.state == PFC_ST_WAIT_RELAY) ||
                          (s_pfc.state == PFC_ST_RAMP_UP)   ||
                          (s_pfc.state == PFC_ST_RUN);
        pfc_hw_set_relay(keep_relay);

        if (s_pfc.vbus_cmd > 0.0f) {
            s_pfc.vbus_cmd = CLAMP(s_pfc.vbus_cmd - PFC_RAMP_DOWN_RATE_V_PER_MS, 0.0f, PFC_TARGET_MAX_V);
            /* 只有在继电器已吸合的阶段才允许 BUS_ADJ 输出 */
            if (keep_relay) {
                pfc_bus_adj_tick(s_pfc.meas.vbus_v);
            } else {
                pfc_bus_adj_set(0.0f);
            }
        } else {
            pfc_outputs_off();
            pfc_state_enter(PFC_ST_OFF);
        }
        return;
    }

    /* 4) 正常状态机 */
    switch (s_pfc.state) {
    case PFC_ST_OFF:
        pfc_outputs_off();
        if (pfc_ac_ok()) {
            s_pfc.ac_ok_since_ms = g_ms;
            pfc_state_enter(PFC_ST_WAIT_AC);
        }
        break;
    case PFC_ST_WAIT_AC:
        pfc_outputs_off();
        if (!pfc_ac_ok()) {
            s_pfc.ac_ok_since_ms = 0U;
            break;
        }
        if (s_pfc.ac_ok_since_ms == 0U) {
            s_pfc.ac_ok_since_ms = g_ms;
        } else if (elapsed_reached(s_pfc.ac_ok_since_ms, PFC_AC_LOSS_DEBOUNCE_MS)) {
            pfc_state_enter(PFC_ST_PRECHARGE);
        }
        break;

    case PFC_ST_PRECHARGE:
        /* 被动预充：继电器必须断开，BUS_ADJ 必须为 0 */
        pfc_hw_set_relay(false);
        pfc_bus_adj_set(0.0f);

        if (!pfc_ac_ok()) {
            pfc_state_enter(PFC_ST_WAIT_AC);
            break;
        }

        if (elapsed_reached(s_pfc.precharge_begin_ms, PFC_PRECHARGE_TIMEOUT_MS) &&
            s_pfc.meas.vbus_v < PFC_PRECHARGE_TARGET_V) {
            pfc_handle_fault("PRECHARGE_TIMEOUT");
            break;
        }
        if (s_pfc.meas.vbus_v >= PFC_PRECHARGE_TARGET_V) {
            pfc_state_enter(PFC_ST_WAIT_RELAY);
        }
        break;
    case PFC_ST_WAIT_RELAY:
        /* 吸合继电器，等待触点稳定；此阶段仍禁止 BUS_ADJ */
        pfc_hw_set_relay(true);
        pfc_bus_adj_set(0.0f);

        if (!pfc_ac_ok()) {
            pfc_outputs_off();
            pfc_state_enter(PFC_ST_WAIT_AC);
            break;
        }

        if (elapsed_reached(s_pfc.entry_ms, PFC_RELAY_SETTLE_MS)) {
            pfc_state_enter(PFC_ST_RAMP_UP);
        }
        break;
    case PFC_ST_RAMP_UP: {
        /* 继电器保持吸合，允许 BUS_ADJ 拉升 */
        pfc_hw_set_relay(true);

        if (!pfc_ac_ok()) {
            pfc_outputs_off();
            pfc_state_enter(PFC_ST_WAIT_AC);
            break;
        }

        float target = pfc_target_from_battery(s_pfc.vbus_target, s_pfc.meas.vbat_v);
        if (s_pfc.vbus_cmd < target) {
            s_pfc.vbus_cmd = CLAMP(s_pfc.vbus_cmd + PFC_RAMP_UP_RATE_V_PER_MS, 0.0f, target);
        } else if (s_pfc.vbus_cmd > target) {
            s_pfc.vbus_cmd = CLAMP(s_pfc.vbus_cmd - PFC_RAMP_DOWN_RATE_V_PER_MS, target, PFC_TARGET_MAX_V);
        }

        pfc_bus_adj_tick(s_pfc.meas.vbus_v);

        /* Ready 判定：电压达到最小门限并保持一段时间才进入 RUN */
        if (s_pfc.meas.vbus_v >= (PFC_TARGET_MIN_V - PFC_READY_HYSTERESIS_V)) {
            if (s_pfc.ready_since_ms == 0U) {
                s_pfc.ready_since_ms = g_ms;
            } else if (elapsed_reached(s_pfc.ready_since_ms, PFC_READY_STABLE_MS)) {
                pfc_state_enter(PFC_ST_RUN);
            }
        } else {
            s_pfc.ready_since_ms = 0U;
        }
        break;
    }

    case PFC_ST_RUN: {
        pfc_hw_set_relay(true);

        /* AC 掉电去抖：掉电则回到 WAIT_AC 并释放继电器 */
        if (!pfc_ac_ok()) {
            if (s_pfc.ac_ok_since_ms == 0U) {
                s_pfc.ac_ok_since_ms = g_ms;
            } else if (elapsed_reached(s_pfc.ac_ok_since_ms, PFC_AC_LOSS_DEBOUNCE_MS)) {
                pfc_outputs_off();
                s_pfc.vbus_cmd = 0.0f;
                pfc_state_enter(PFC_ST_WAIT_AC);
                break;
            }
        } else {
            s_pfc.ac_ok_since_ms = 0U;
        }

        float target = pfc_target_from_battery(s_pfc.vbus_target, s_pfc.meas.vbat_v);
        float rate = (target > s_pfc.vbus_cmd) ? PFC_RAMP_UP_RATE_V_PER_MS : PFC_RAMP_DOWN_RATE_V_PER_MS;

        if (fabsf(target - s_pfc.vbus_cmd) < rate) {
            s_pfc.vbus_cmd = target;
        } else if (target > s_pfc.vbus_cmd) {
            s_pfc.vbus_cmd += rate;
        } else {
            s_pfc.vbus_cmd -= rate;
        }

        pfc_bus_adj_tick(s_pfc.meas.vbus_v);

        /* UVP：运行中母线掉太低，认为异常（可按需求改成回 WAIT_AC 或 FAULT） */
        if (s_pfc.meas.vbus_v < PFC_VBUS_UVP_V) {
            pfc_handle_fault("VBUS_UV");
        }
        break;
    }

    default:
        pfc_outputs_off();
        pfc_state_enter(PFC_ST_OFF);
        break;
    }
}

/*********************************************************************************************************
*                                              getters
*********************************************************************************************************/
float pfc_get_vbus(void) { return s_pfc.meas.vbus_v; }
float pfc_get_vac(void)  { return s_pfc.meas.vac_rms; }
float pfc_get_temp_pfc(void) { return s_pfc.meas.tpfc_c; }

pfc_state_t pfc_state(void) { return s_pfc.state; }

bool pfc_is_ready(void)
{
    return (s_pfc.state == PFC_ST_RUN) &&
           (s_pfc.meas.vbus_v >= (PFC_TARGET_MIN_V - PFC_READY_HYSTERESIS_V));
}

bool pfc_is_fault(void) { return s_pfc.state == PFC_ST_FAULT; }
bool pfc_is_fault_latched(void) { return s_pfc.fault_latched; }

/* 故障清除：增加冷却/放电等待（PFC_FAULT_RESTART_MS） */
void pfc_clear_fault(void)
{
    if (!s_pfc.fault_latched) {
        return;
    }

    if (protect_fault_active_hw() || protect_fault_latched()) {
        return;
    }

    if (!elapsed_reached(s_pfc.fault_since_ms, PFC_FAULT_RESTART_MS)) {
        return;
    }

    s_pfc.fault_latched = false;
    pfc_state_enter(PFC_ST_OFF);
}
