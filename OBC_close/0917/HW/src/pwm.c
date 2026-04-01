#include "pwm.h"
#include "main.h"

/*********************************************************************************************************
*                                              内部变量声明
*********************************************************************************************************/
/*
 * BUS_VOL_ADJ:
 * MCU 通过光耦去影响 PFC 控制侧的调节口。
 *
 * 当前版本策略：
 * 1. 不再输出 PWM
 * 2. PB0 仅作为普通 GPIO 使用
 * 3. 只支持两种状态：
 *    - 强制高电平
 *    - 强制低电平
 *
 * 适用场景：
 * - 先验证 PB0 拉高/拉低对母线目标的影响
 * - 轻载/异常工况下做简单强制策略
 * - 后续若需要恢复 PWM，再单独扩展
 */
static bus_vol_adj_ctrl_t s_bus_adj;

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define BUS_VBUS_TARGET_MIN_V         (368.0f)
#define BUS_VBUS_TARGET_MAX_V         (390.0f)

#define BUS_VOUT_FALLBACK_BOOST_V     (8.0f)
#define BUS_VOUT_LOW_MARGIN_V         (0.6f)

/* 目标电压在该区间时，PB0 强制高电平 */
#define BUS_VOUT_FORCE_HIGH_MIN_V     (44.0f)
#define BUS_VOUT_FORCE_HIGH_MAX_V     (54.0f)

/*********************************************************************************************************
*                                              内部类型定义
*********************************************************************************************************/
typedef enum
{
    BUS_PB0_LEVEL_LOW = 0,
    BUS_PB0_LEVEL_HIGH
} bus_pb0_level_t;

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/
static bool s_pb0_gpio_initialized = false;
static bus_pb0_level_t s_pb0_level = BUS_PB0_LEVEL_HIGH;

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
static inline float clampf(float x, float a, float b)
{
    return (x < a) ? a : ((x > b) ? b : x);
}

static void pb0_gpio_init_once(void)
{
    if (!s_pb0_gpio_initialized) {
        rcu_periph_clock_enable(RCU_GPIOB);
        gpio_init(PB0_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PB0_PIN);
        s_pb0_gpio_initialized = true;
    }
}

static void pb0_force_high(void)
{
    pb0_gpio_init_once();

    if (s_pb0_level != BUS_PB0_LEVEL_HIGH) {
        gpio_bit_set(PB0_PORT, PB0_PIN);
        s_pb0_level = BUS_PB0_LEVEL_HIGH;
    } else {
        /* 首次上电后也确保输出正确 */
        gpio_bit_set(PB0_PORT, PB0_PIN);
    }
}

static void pb0_force_low(void)
{
    pb0_gpio_init_once();

    if (s_pb0_level != BUS_PB0_LEVEL_LOW) {
        gpio_bit_reset(PB0_PORT, PB0_PIN);
        s_pb0_level = BUS_PB0_LEVEL_LOW;
    } else {
        /* 首次上电后也确保输出正确 */
        gpio_bit_reset(PB0_PORT, PB0_PIN);
    }
}

/*********************************************************************************************************
*                                              兼容旧接口
*********************************************************************************************************/
/*
 * 兼容旧工程接口：
 * 当前版本不再真正初始化 PWM，而是直接把 PB0 切为 GPIO。
 */
void pb0_pwm_init(uint32_t pwm_hz)
{
    (void)pwm_hz;
    pb0_force_high();
}

/*
 * 兼容旧工程接口：
 * 当前版本不再根据 duty 输出 PWM。
 * 约定：
 *   d > 0.5f  -> 强制高
 *   d <= 0.5f -> 强制低
 *
 * 这样可以尽量兼容旧调用逻辑，又不会保留“假 PWM”语义。
 */
void pb0_pwm_set_duty(float d)
{

   pb0_force_high();
 
}

/*********************************************************************************************************
*                                              公共接口实现
*********************************************************************************************************/
void bus_vol_adj_reset(void)
{
    s_bus_adj.integ = 0.0f;
    s_bus_adj.duty_cmd = 1.0f;   /* 当前版本：1.0 表示强制高 */
    pb0_force_high();
}

void bus_vol_adj_init(void)
{
		pb0_force_high();
}

void bus_vol_adj_set_target_vbus(float target_vbus)
{
    s_bus_adj.target_v = clampf(target_vbus, BUS_VBUS_TARGET_MIN_V, BUS_VBUS_TARGET_MAX_V);
}

float bus_vol_adj_target_from_vout(float vout_ref)
{
    float v = vout_ref;
    float base_target;

    if (v < 37.0f) {
        v = 37.0f;
    }

    if (v <= 41.0f) {
        base_target = 41.0f * 9.0f;
    } else if (v >= 44.0f) {
        base_target = 390.0f;
    } else {
        float t = (v - 41.0f) / 3.0f;
        base_target = 369.0f + t * (390.0f - 369.0f);
    }

    return clampf(base_target, BUS_VBUS_TARGET_MIN_V, BUS_VBUS_TARGET_MAX_V);
}

/*
 * 当前版本 bus_vol_adj_tick:
 * 不再做 PWM 占空比调节，只保留最简单行为：
 * enabled == true  -> 强制高
 * enabled == false -> 强制低
 */
void bus_vol_adj_tick(float vbus, bool enabled)
{
    (void)vbus;

    if (enabled) {
        s_bus_adj.duty_cmd = 1.0f;
        pb0_force_high();
    } else {
        s_bus_adj.duty_cmd = 0.0f;
        pb0_force_low();
    }
}

/*
 * 当前建议策略：
 * 1. vout_ref 在 44V~54V：强制高
 * 2. 其它区间：强制低
 *
 * 如果后面你想改成：
 * - 37V附近强制高
 * - 44V~54V强制高
 * - 其余低
 * 也很容易改
 */
void bus_vol_adj_follow_vout(float vout_ref, float vout_meas, float vbus_meas, bool enabled)
{
    (void)vout_ref;
    (void)vout_meas;
    (void)vbus_meas;
    (void)enabled;
}