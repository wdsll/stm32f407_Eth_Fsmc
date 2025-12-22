#include "pfc_control.h"
#include <math.h>

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

/* ===== Hardware scaling ===== */
#define PFC_VBUS_ADC_GAIN_V_PER_VIN   (144.3464f)   // VBUS = ADC_V * 144.3464
#define PFC_AC_ADC_GAIN_V_PER_VIN     (233.38f)     // VAC  = ADC_V * 233.38

/* 上电自检：VBUS ≈ 1.414 * VAC */
#define PFC_VBUS_VAC_RATIO            (1.414f)
#define PFC_VBUS_VAC_RATIO_TOLERANCE  (0.15f)       // ±15%
#define PFC_VBUS_VAC_RATIO_STABLE_MS  (50U)

/* 调节稳定性判定（使能后） */
#define PFC_STABLE_ERROR_V            (5.0f)        // |Vbus-target| <= 5V
#define PFC_STABLE_TIME_MS            (200U)        // 持续 200ms 才算稳定
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

#define PFC_PRECHARGE_TARGET_RATIO     (1.1414f) //预充目标：1.1414 * Vac
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
/* ===== Bus command shaping ===== */
#ifndef VBUS_TARGET_V
#define VBUS_TARGET_V                  (400.0f)
#endif
/* ===== Bus voltage command shaping ===== */
#define PFC_TARGET_DEFAULT_V           (VBUS_TARGET_V)
#define PFC_TARGET_MIN_V               (360.0f)
#define PFC_TARGET_MAX_V               (410.0f)
#define PFC_TARGET_FROM_BAT_GAIN       (1.05f)
#define PFC_TARGET_MIN_FROM_BAT_V      (360.0f)
#define PFC_RAMP_UP_RATE_V_PER_MS      (2.0f)
#define PFC_RAMP_DOWN_RATE_V_PER_MS    (4.0f)

/* ===== 方案A：vbus_cmd -> 前馈 duty（来自你的仿真 plant 逆映射）===== */
#define PFC_FF_VBUS_BASE_V             (400.0f)   // 仿真里的 VBUS_BASE
#define PFC_FF_K_DUTY2VBUS_V           (300.0f)   // 仿真里的 K_DUTY2VBUS（V / duty）
#define PFC_FF_DUTY_NEUTRAL            (0.50f)
#define PFC_FF_DUTY_MIN                (0.05f)
#define PFC_FF_DUTY_MAX                (0.95f)

/* 当 vbus_cmd 低于该阈值，直接把 PWM 关掉（避免一直卡在 duty_min 导致母线降不下来） */
#define PFC_FF_DISABLE_CMD_V           (260.0f)
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


    bool enable_cmd;
    bool fault_latched;
	
    uint32_t vac_ratio_since_ms;
    uint32_t regulation_stable_since_ms;
    bool bus_matches_ac;
    bool regulation_stable;
    pfc_measure_t meas; ///< 最近一次采样的 PFC 测量数据（电压、电流、温度等）
} pfc_ctx_t;

static pfc_ctx_t s_pfc;

/*********************************************************************************************************
*                                              小工具函数
*********************************************************************************************************/
static float adc_to_v_scaled(uint16_t raw, float gain)
{
    float v_adc = (raw * VREF_ADC) / 4095.0f;
    return v_adc * gain;
}

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


static void pfc_update_regulation_stability(float target, float vbus)
{
    bool within = fabsf(target - vbus) <= PFC_STABLE_ERROR_V;

    if (within) {
        if (s_pfc.regulation_stable_since_ms == 0U) {
            s_pfc.regulation_stable_since_ms = g_ms;
        }
        s_pfc.regulation_stable = elapsed_reached(s_pfc.regulation_stable_since_ms, PFC_STABLE_TIME_MS);
    } else {
        s_pfc.regulation_stable_since_ms = 0U;
        s_pfc.regulation_stable = false;
    }
}
/*********************************************************************************************************
*                                 硬件输出：合并“继电器+PFC使能”为一个脚
*********************************************************************************************************/
/* 继电器脚：优先用 PFC_MAIN_RELAY_*，若工程里没定义可退回 PFC_EN_*（兼容你旧工程） */
#if defined(PFC_MAIN_RELAY_PORT) && defined(PFC_MAIN_RELAY_PIN) && defined(PFC_MAIN_RELAY_RCU)
#define PFC_MAIN_OUT_PORT   PFC_MAIN_RELAY_PORT
#define PFC_MAIN_OUT_PIN    PFC_MAIN_RELAY_PIN
#define PFC_MAIN_OUT_RCU    PFC_MAIN_RELAY_RCU
#elif defined(PFC_EN_PORT) && defined(PFC_EN_PIN) && defined(PFC_EN_RCU)
#define PFC_RELAY_PORT   PFC_EN_PORT
#define PFC_RELAY_PIN    PFC_EN_PIN
#define PFC_RELAY_RCU    PFC_EN_RCU
#endif

void pfc_hw_set_main(bool on)
{
		static bool s_gpio_initialized = false;
	  static bool s_last = false;
		bool first = !s_gpio_initialized;

		if (first)
		{
			  rcu_periph_clock_enable(PFC_MAIN_OUT_RCU);
				gpio_init(PFC_MAIN_OUT_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PFC_MAIN_OUT_PIN);
				s_gpio_initialized = true;
		}
    /* 首次调用一定要落一次电平；非首次且状态未变才直接返回 */
    if (!first && (s_last == on)) {
        return;
    }
    s_last = on;
    if (on) 
			gpio_bit_set(PFC_MAIN_OUT_PORT, PFC_MAIN_OUT_PIN);
    else    
			gpio_bit_reset(PFC_MAIN_OUT_PORT, PFC_MAIN_OUT_PIN);
}
/* 你的 BUS_ADJ PWM（PB0） */
static void pfc_pwm_set(float duty)
{
    if (duty <= 0.0f) {
        duty = 0.0f;
    } else {
        duty = CLAMP(duty, PFC_FF_DUTY_MIN, PFC_FF_DUTY_MAX);
    }
    pb0_pwm_set_duty(duty);
}

/* 关输出：PWM=0 + MAIN=0 */
static void pfc_outputs_off(void)
{
    pfc_pwm_set(0.0f);

    pfc_hw_set_main(false);
}

/*********************************************************************************************************
*                          方案A：vbus_cmd 斜坡 -> duty 前馈（无闭环 PI）
*********************************************************************************************************/
static float pfc_duty_ff_from_vbus_cmd(float vbus_cmd)
{
    /* 低命令直接关 PWM，给母线放电/下降空间 */
    if (vbus_cmd <= 0.0f || vbus_cmd <= PFC_FF_DISABLE_CMD_V) {
        return 0.0f;
    }

    /* duty = duty_neutral + (vbus_cmd - VBUS_BASE)/K */
    float duty = PFC_FF_DUTY_NEUTRAL + (vbus_cmd - PFC_FF_VBUS_BASE_V) / PFC_FF_K_DUTY2VBUS_V;
    return CLAMP(duty, PFC_FF_DUTY_MIN, PFC_FF_DUTY_MAX);
}

static float pfc_target_from_battery(float manual_target, float vbat)
{

    float dyn_target = CLAMP(manual_target, PFC_TARGET_MIN_V, PFC_TARGET_MAX_V);
    float bat_target = CLAMP(vbat * PFC_TARGET_FROM_BAT_GAIN, PFC_TARGET_MIN_FROM_BAT_V, PFC_TARGET_MAX_V);
    /* anti-windup：若打限幅，把积分回算到“刚好在限幅边界” */
    if (bat_target > dyn_target) dyn_target = bat_target;
    return dyn_target;
}

/*********************************************************************************************************
*                                              状态机辅助
*********************************************************************************************************/
static void pfc_state_enter(pfc_state_t next)
{
    if (s_pfc.state == next) return;

    s_pfc.state = next;
    s_pfc.entry_ms = g_ms;
    s_pfc.regulation_stable_since_ms = 0U;
    s_pfc.regulation_stable = false;
    if (next == PFC_ST_FAULT) {
        s_pfc.fault_since_ms = g_ms;
        s_pfc.fault_latched = true;
    }
    if (next == PFC_ST_WAIT_AC) {
        s_pfc.precharge_begin_ms = 0U;
        s_pfc.ready_since_ms = 0U;
        s_pfc.vbus_cmd = 0.0f;
    }

    if (next == PFC_ST_RAMP_UP) {
        s_pfc.ready_since_ms = 0U;
        /* 起步从当前 VBUS，避免一上来 duty 打满 */
        s_pfc.vbus_cmd = s_pfc.meas.vbus_v;
    }

    if (next == PFC_ST_RUN) {
        /* 进入 RUN 后继续用 ready_since_ms 作为运行中计时也无所谓，这里清零即可 */
        s_pfc.ready_since_ms = 0U;
    }
}

static void pfc_sample_inputs(void)
{
    s_pfc.meas.vac_raw  = adc1_aux_read_channel(AC_VOL_SAMPLE,   ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vbus_raw = adc1_aux_read_channel(BUS_VOL_SAMPLE,  ADC_SAMPLETIME_71POINT5);
    //s_pfc.meas.tpfc_raw = adc1_aux_read_channel(T_SENSE_PFC_MOS,  ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vbat_raw = adc1_aux_read_channel(VBT_SENSE_CH,     ADC_SAMPLETIME_71POINT5);
    s_pfc.meas.vout_raw = adc1_aux_read_channel(VOUT_SENSE_CH,    ADC_SAMPLETIME_71POINT5);

    float vac  = adc_to_v_scaled(s_pfc.meas.vac_raw,  PFC_AC_ADC_GAIN_V_PER_VIN);
    float vbus = adc_to_v_scaled(s_pfc.meas.vbus_raw, PFC_VBUS_ADC_GAIN_V_PER_VIN);
    float vbat = adc_to_v_div(s_pfc.meas.vbat_raw, VBT_RTOP_OHM, VBT_RBOT_OHM);
    float vout = adc_to_v_div(s_pfc.meas.vout_raw, VOUT_RTOP_OHM, VOUT_RBOT_OHM);
   //float tpfc = adc_ntc_to_c(s_pfc.meas.tpfc_raw);

    s_pfc.meas.vac_v   = vac;
    s_pfc.meas.vbus_v  = vbus;
    s_pfc.meas.vbat_v  = vbat;
    s_pfc.meas.vout_v  = vout;
    //s_pfc.meas.tpfc_c  = tpfc;

    /* 你目前 vac_rms 用低通等效，后续可换真 RMS */
    s_pfc.meas.vac_rms = lpf(s_pfc.meas.vac_rms, vac, 0.05f);
	    /* Δ 判断 PFC 母线≈1.414 × Vac（支持上电自检） */
    /* 上电自检：VBUS ≈ 1.414 × VAC（带容差+稳定计时） */
    bool ratio_ok = false;
    if (vac > 1.0f) {
        float expected = vac * PFC_VBUS_VAC_RATIO;
        float tol = expected * PFC_VBUS_VAC_RATIO_TOLERANCE;
        ratio_ok = (vbus >= (expected - tol)) && (vbus <= (expected + tol));
    }

    if (ratio_ok) {
        if (s_pfc.vac_ratio_since_ms == 0U) {
            s_pfc.vac_ratio_since_ms = g_ms;
        }
        if (elapsed_reached(s_pfc.vac_ratio_since_ms, PFC_VBUS_VAC_RATIO_STABLE_MS)) {
            s_pfc.bus_matches_ac = true;
        }
    } else {
        s_pfc.vac_ratio_since_ms = 0U;
        s_pfc.bus_matches_ac = false;
    }
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

    /* 保护/故障 */
    if (protect_fault_active_hw() || protect_fault_latched()) {
        pfc_handle_fault("HARD_PRO");
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

    /* disable：优雅停机（保持 MAIN=1，vbus_cmd 斜坡降到 0；再 MAIN=0） */
    if (!s_pfc.enable_cmd) {
        bool keep_main = (s_pfc.state == PFC_ST_WAIT_RELAY) ||
                         (s_pfc.state == PFC_ST_RAMP_UP)   ||
                         (s_pfc.state == PFC_ST_RUN);
        pfc_hw_set_main(keep_main);

        if (s_pfc.vbus_cmd > 0.0f) {
            s_pfc.vbus_cmd = CLAMP(s_pfc.vbus_cmd - PFC_RAMP_DOWN_RATE_V_PER_MS, 0.0f, PFC_TARGET_MAX_V);
            /* 只有在继电器已吸合的阶段才允许 BUS_ADJ 输出 */
            if (keep_main) {
                float duty = pfc_duty_ff_from_vbus_cmd(s_pfc.vbus_cmd);
                pfc_pwm_set(duty);
            } else {
                pfc_pwm_set(0.0f);
            }
        } else {
            pfc_outputs_off();
            pfc_state_enter(PFC_ST_OFF);
        }
        return;
    }

    /* 正常状态机 */
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
        /* 1) AC 范围必须 OK */
        if (!pfc_ac_ok()) {
            s_pfc.ac_ok_since_ms = 0U;
            s_pfc.precharge_begin_ms = 0U;
            break;
        }
        /* 2) 上电自检：VBUS ≈ 1.414×VAC */
        if (!s_pfc.bus_matches_ac) {
            s_pfc.ac_ok_since_ms = 0U;
            s_pfc.precharge_begin_ms = 0U;
            break;
        }
        if (s_pfc.ac_ok_since_ms == 0U) {
            s_pfc.ac_ok_since_ms = g_ms;
        } else if (elapsed_reached(s_pfc.ac_ok_since_ms, PFC_AC_LOSS_DEBOUNCE_MS)) {
            if (s_pfc.precharge_begin_ms == 0U) {
                s_pfc.precharge_begin_ms = g_ms;
            }
            float precharge_target_v = s_pfc.meas.vac_rms * PFC_PRECHARGE_TARGET_RATIO;
            if (elapsed_reached(s_pfc.precharge_begin_ms, PFC_PRECHARGE_TIMEOUT_MS) &&
                s_pfc.meas.vbus_v < precharge_target_v) {
                pfc_handle_fault("PRECHARGE_TIMEOUT");
                break;
            }
            if (s_pfc.meas.vbus_v >= precharge_target_v) {
                pfc_state_enter(PFC_ST_WAIT_RELAY);
            }
        }
        break;
    case PFC_ST_WAIT_RELAY:
        /* MAIN 拉起（继电器+PFC使能同脚），PWM仍为0，等触点稳定 */
        pfc_hw_set_main(true);
        pfc_pwm_set(0.0f);

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
        pfc_hw_set_main(true);

        if (!pfc_ac_ok()) {
            pfc_outputs_off();
            pfc_state_enter(PFC_ST_WAIT_AC);
            break;
        }

        float target = pfc_target_from_battery(s_pfc.vbus_target, s_pfc.meas.vbat_v);
        /* vbus_cmd 斜坡 */
        if (s_pfc.vbus_cmd < target) {
            s_pfc.vbus_cmd = CLAMP(s_pfc.vbus_cmd + PFC_RAMP_UP_RATE_V_PER_MS, 0.0f, target);
        } else if (s_pfc.vbus_cmd > target) {
            s_pfc.vbus_cmd = CLAMP(s_pfc.vbus_cmd - PFC_RAMP_DOWN_RATE_V_PER_MS, target, PFC_TARGET_MAX_V);
        }

        /* 方案A：开环前馈 duty */
        float duty = pfc_duty_ff_from_vbus_cmd(s_pfc.vbus_cmd);
        pfc_pwm_set(duty);
        /* 3) 使能后：采样判断是否稳定（可用于放行 LLC） */
        pfc_update_regulation_stability(target, s_pfc.meas.vbus_v);

        /* Ready 判定：达到最小门限并保持 */
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
        pfc_hw_set_main(true);

        /* AC 掉电去抖：掉电则回 WAIT_AC 并关输出 */
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
        /* vbus_cmd 跟随目标（斜坡） */
        float rate = (target > s_pfc.vbus_cmd) ? PFC_RAMP_UP_RATE_V_PER_MS : PFC_RAMP_DOWN_RATE_V_PER_MS;

        if (fabsf(target - s_pfc.vbus_cmd) < rate) {
            s_pfc.vbus_cmd = target;
        } else if (target > s_pfc.vbus_cmd) {
            s_pfc.vbus_cmd += rate;
        } else {
            s_pfc.vbus_cmd -= rate;
        }

        /* 开环前馈 duty */
        float duty = pfc_duty_ff_from_vbus_cmd(s_pfc.vbus_cmd);
        pfc_pwm_set(duty);

        /* 调节稳定性（用于你上层逻辑/放行 LLC） */
        pfc_update_regulation_stability(target, s_pfc.meas.vbus_v);
        /* UVP：运行中母线掉太低，认为异常（可按需求改成回 WAIT_AC 或 FAULT） */
        /* UVP：运行中母线掉太低 */
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
    /* 你也可以把 regulation_stable 作为“稳定”的更严格条件 */
    return (s_pfc.state == PFC_ST_RUN) &&
           (s_pfc.meas.vbus_v >= (PFC_TARGET_MIN_V - PFC_READY_HYSTERESIS_V)) &&
           (s_pfc.regulation_stable);
}

bool pfc_is_fault(void) { return s_pfc.state == PFC_ST_FAULT; }
bool pfc_is_fault_latched(void) { return s_pfc.fault_latched; }

/* 故障清除：增加冷却/放电等待（PFC_FAULT_RESTART_MS） */
void pfc_clear_fault(void)
{
    if (!s_pfc.fault_latched) return;

    if (protect_fault_active_hw() || protect_fault_latched()) 
			return;

    if (!elapsed_reached(s_pfc.fault_since_ms, PFC_FAULT_RESTART_MS)) 
			return;

    s_pfc.fault_latched = false;
    pfc_state_enter(PFC_ST_OFF);
}
