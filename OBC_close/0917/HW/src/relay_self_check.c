/**
 * relay_self_check.c - 继电器 GPIO 软控验证模块
 *
 * 当前版本定位：
 * 1. 只验证 MCU 软件能否控制两个继电器控制脚输出高/低电平
 * 2. 不判断继电器触点是否真的闭合
 * 3. 不判断继电器是否粘连
 * 4. 不再使用 GPIO 浮空输入回读，避免误判
 *
 * 测试对象：
 *   - PFC 主继电器控制脚: PC10 / RELAY_PFC_EN，默认高电平 ON
 *   - 输出继电器控制脚:  PB14 / OUT_RELAY，默认高电平 ON
 *
 * 推荐测试方式：
 *   1. 首次上板不接 AC，只上低压辅助电源
 *   2. 示波器/万用表测 PC10、PB14、RELAY_PFC_EN_1、OUT_RELAY 后级驱动点
 *   3. 看 OFF -> ON -> OFF 电平是否按日志变化
 */

#include "relay_self_check.h"
#include "main.h"
#include "gd32f30x.h"
#include "debug_printf.h"

/* ============================================================
 *  main.h 可能未定义的宏，这里做兼容补全
 * ============================================================ */

#ifndef OUT_RELAY
#define OUT_RELAY                   GPIOB
#endif

#ifndef OUT_RELAY_PIN
#define OUT_RELAY_PIN               GPIO_PIN_14
#endif

#ifndef OUT_RELAY_RCU
#define OUT_RELAY_RCU               RCU_GPIOB
#endif

#ifndef PFC_MAIN_RELAY_PORT
#define PFC_MAIN_RELAY_PORT         GPIOC
#endif

#ifndef PFC_MAIN_RELAY_PIN
#define PFC_MAIN_RELAY_PIN          GPIO_PIN_10
#endif

#ifndef PFC_MAIN_RELAY_RCU
#define PFC_MAIN_RELAY_RCU          RCU_GPIOC
#endif

/*
 * ACTIVE_LEVEL:
 * 1 = GPIO 输出高电平表示继电器 ON
 * 0 = GPIO 输出低电平表示继电器 ON
 */
#ifndef CHARGE_OUT_RELAY_ACTIVE_LEVEL
#define CHARGE_OUT_RELAY_ACTIVE_LEVEL   1
#endif

#ifndef PFC_MAIN_RELAY_ACTIVE_LEVEL
#define PFC_MAIN_RELAY_ACTIVE_LEVEL     1
#endif

/* ============================================================
 *  软控测试配置
 * ============================================================ */

/*
 * 是否启用 PFC 主继电器 GPIO 软控测试
 * 如果接了 AC，建议先改成 0，只测输出继电器控制脚。
 */
#ifndef RELAY_DRV_TEST_ENABLE_PFC_MAIN
#define RELAY_DRV_TEST_ENABLE_PFC_MAIN  0
#endif

/*
 * 是否启用输出继电器 GPIO 软控测试
 */
#ifndef RELAY_DRV_TEST_ENABLE_OUT
#define RELAY_DRV_TEST_ENABLE_OUT       1
#endif

/*
 * 每个继电器执行几轮 OFF -> ON -> OFF
 */
#ifndef RELAY_DRV_TEST_REPEAT
#define RELAY_DRV_TEST_REPEAT           1U
#endif

/*
 * OFF 保持时间
 */
#ifndef RELAY_DRV_TEST_OFF_MS
#define RELAY_DRV_TEST_OFF_MS           300U
#endif

/*
 * ON 保持时间
 */
#ifndef RELAY_DRV_TEST_ON_MS
#define RELAY_DRV_TEST_ON_MS            500U
#endif

/*
 * 两个继电器测试之间的间隔
 */
#ifndef RELAY_DRV_TEST_GAP_MS
#define RELAY_DRV_TEST_GAP_MS           300U
#endif

/* ============================================================
 *  外部函数
 * ============================================================ */

/*
 * delay_ms() 在 main.c 里已有实现：
 * void delay_ms(uint32_t duration_ms)
 */
extern void delay_ms(uint32_t duration_ms);

/* ============================================================
 *  内部工具函数
 * ============================================================ */

static void relay_gpio_write(uint32_t port,
                             uint32_t pin,
                             uint8_t level)
{
    if (level != 0U) {
        gpio_bit_set(port, pin);
    } else {
        gpio_bit_reset(port, pin);
    }
}


static uint8_t relay_on_level(uint8_t active_high)
{
    return (active_high != 0U) ? 1U : 0U;
}


static uint8_t relay_off_level(uint8_t active_high)
{
    return (active_high != 0U) ? 0U : 1U;
}


static void relay_gpio_init_off(uint32_t port,
                                uint32_t pin,
                                rcu_periph_enum rcu_periph,
                                uint8_t active_high)
{
    rcu_periph_clock_enable(rcu_periph);

    gpio_init(port, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, pin);

    relay_gpio_write(port, pin, relay_off_level(active_high));
}


static void relay_gpio_force_off_all(void)
{
    relay_gpio_write(PFC_MAIN_RELAY_PORT,
                     PFC_MAIN_RELAY_PIN,
                     relay_off_level(PFC_MAIN_RELAY_ACTIVE_LEVEL));

    relay_gpio_write(OUT_RELAY,
                     OUT_RELAY_PIN,
                     relay_off_level(CHARGE_OUT_RELAY_ACTIVE_LEVEL));
}


/**
 * @brief  单个继电器 GPIO 软控测试
 * @note   只输出 OFF -> ON -> OFF，不做触点状态判断
 */
static void relay_gpio_drive_test(uint32_t port,
                                  uint32_t pin,
                                  rcu_periph_enum rcu_periph,
                                  const char *name,
                                  uint8_t active_high)
{
    uint8_t off = relay_off_level(active_high);
    uint8_t on  = relay_on_level(active_high);

    rcu_periph_clock_enable(rcu_periph);
    gpio_init(port, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, pin);

    debug_printf("[RELAY-DRV] %s test start, active_%s\r\n",
                 name,
                 (active_high != 0U) ? "HIGH" : "LOW");

    for (uint32_t i = 0U; i < RELAY_DRV_TEST_REPEAT; i++) {

        /* 1. OFF */
        relay_gpio_write(port, pin, off);
        debug_printf("[RELAY-DRV] %s round %lu: OFF, gpio_level=%d\r\n",
                     name,
                     (unsigned long)(i + 1U),
                     (int)off);
        delay_ms(RELAY_DRV_TEST_OFF_MS);

        /* 2. ON */
        relay_gpio_write(port, pin, on);
        debug_printf("[RELAY-DRV] %s round %lu: ON, gpio_level=%d\r\n",
                     name,
                     (unsigned long)(i + 1U),
                     (int)on);
        delay_ms(RELAY_DRV_TEST_ON_MS);

        /* 3. OFF */
        relay_gpio_write(port, pin, off);
        debug_printf("[RELAY-DRV] %s round %lu: OFF, gpio_level=%d\r\n",
                     name,
                     (unsigned long)(i + 1U),
                     (int)off);
        delay_ms(RELAY_DRV_TEST_OFF_MS);
    }

    debug_printf("[RELAY-DRV] %s test done\r\n", name);
}


/* ============================================================
 *  公共 API
 * ============================================================ */

void relay_sc_init(void)
{
    /*
     * 初始化两个继电器 GPIO 为推挽输出，默认全部断开。
     * 注意：这里只初始化控制脚，不判断继电器触点状态。
     */

    relay_gpio_init_off(PFC_MAIN_RELAY_PORT,
                        PFC_MAIN_RELAY_PIN,
                        PFC_MAIN_RELAY_RCU,
                        PFC_MAIN_RELAY_ACTIVE_LEVEL);

    relay_gpio_init_off(OUT_RELAY,
                        OUT_RELAY_PIN,
                        OUT_RELAY_RCU,
                        CHARGE_OUT_RELAY_ACTIVE_LEVEL);

    debug_printf("[RELAY-DRV] GPIO init done: PFC_MAIN=OFF, OUT_RELAY=OFF\r\n");
}


relay_check_result_t relay_sc_run(void)
{
    /*
     * 当前函数只执行 GPIO 软控验证。
     * 返回 PASS 代表测试序列执行完毕，不代表继电器触点真实闭合/释放。
     */

    debug_printf("[RELAY-DRV] ===== GPIO DRIVE TEST START =====\r\n");

    /* 测试前先确保全部 OFF */
    relay_gpio_force_off_all();
    delay_ms(RELAY_DRV_TEST_GAP_MS);

#if RELAY_DRV_TEST_ENABLE_PFC_MAIN
    relay_gpio_drive_test(PFC_MAIN_RELAY_PORT,
                          PFC_MAIN_RELAY_PIN,
                          PFC_MAIN_RELAY_RCU,
                          "PFC_MAIN_PC10",
                          PFC_MAIN_RELAY_ACTIVE_LEVEL);
#else
    debug_printf("[RELAY-DRV] PFC_MAIN_PC10 test skipped\r\n");
#endif

    delay_ms(RELAY_DRV_TEST_GAP_MS);

#if RELAY_DRV_TEST_ENABLE_OUT
    relay_gpio_drive_test(OUT_RELAY,
                          OUT_RELAY_PIN,
                          OUT_RELAY_RCU,
                          "OUT_RELAY_PB14",
                          CHARGE_OUT_RELAY_ACTIVE_LEVEL);
#else
    debug_printf("[RELAY-DRV] OUT_RELAY_PB14 test skipped\r\n");
#endif

    delay_ms(RELAY_DRV_TEST_GAP_MS);

    /* 测试完成后强制全部 OFF */
    relay_gpio_force_off_all();

    debug_printf("[RELAY-DRV] ===== GPIO DRIVE TEST DONE, ALL OFF =====\r\n");

    return RELAY_CHECK_PASS;
}


const char* relay_sc_result_str(relay_check_result_t result)
{
    switch (result) {
    case RELAY_CHECK_PASS:
        return "PASS:GPIO_DRIVE_TEST_DONE";

    case RELAY_CHECK_PFC_FAIL:
        return "FAIL:PFC_MAIN_RELAY";

    case RELAY_CHECK_OUT_FAIL:
        return "FAIL:OUT_RELAY";

    default:
        return "UNKNOWN";
    }
}