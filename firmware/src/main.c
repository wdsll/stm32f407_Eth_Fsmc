#include <stdbool.h>
#include <stdint.h>
#include "charger_config.h"
#include "charger_control.h"

/* Minimal GD32F303 register definitions; reset clock (IRC8M) is intentionally used. */
#define REG32(a) (*(volatile uint32_t *)(a))
#define RCU_APB2EN REG32(0x40021018u)
#define RCU_APB1EN REG32(0x4002101Cu)
#define GPIOA_CTL1 REG32(0x40010804u)
#define GPIOB_CTL1 REG32(0x40010C04u)
#define GPIOC_CTL0 REG32(0x40011000u)
#define GPIOC_CTL1 REG32(0x40011004u)
#define GPIOA_OCTL REG32(0x4001080Cu)
#define GPIOB_OCTL REG32(0x40010C0Cu)
#define GPIOC_OCTL REG32(0x4001100Cu)
#define GPIOC_ISTAT REG32(0x40011008u)
#define SYSTICK_CTRL REG32(0xE000E010u)
#define SYSTICK_LOAD REG32(0xE000E014u)
#define SYSTICK_VAL  REG32(0xE000E018u)

static volatile uint32_t milliseconds;
static volatile uint8_t pwm_phase;
static volatile uint16_t cv_pwm;
static volatile uint16_t current_pwm;

static void gpio_write(volatile uint32_t *port, uint8_t pin, bool high)
{
    if (high) *port |= 1u << pin;
    else *port &= ~(1u << pin);
}

void SysTick_Handler(void)
{
    static uint8_t divider;
    pwm_phase = (uint8_t)((pwm_phase + 1u) % 20u);
    gpio_write(&GPIOA_OCTL, 8u, (uint16_t)pwm_phase * 50u < cv_pwm);
    gpio_write(&GPIOB_OCTL, 13u, (uint16_t)pwm_phase * 50u < current_pwm);
    if (++divider == 10u) { /* 10 kHz interrupt / 10 = 1 ms */
        divider = 0u;
        ++milliseconds;
    }
}

static void hardware_init(void)
{
    RCU_APB2EN |= (1u << 2) | (1u << 3) | (1u << 4); /* GPIO A/B/C */
    /* PA8, PB13/PB14/PB15, PC6/8/9/10/12: push-pull outputs/input as used below. */
    GPIOA_CTL1 = (GPIOA_CTL1 & ~0xFFu) | 0x33u;
    GPIOB_CTL1 = (GPIOB_CTL1 & ~0xFFF00000u) | 0x33300000u;
    GPIOC_CTL0 = (GPIOC_CTL0 & ~(0xFu << 24)) | (0x8u << 24); /* PC6 pull-down */
    GPIOC_OCTL &= ~(1u << 6);
    GPIOC_CTL1 = (GPIOC_CTL1 & ~((0xFu << 0) | (0xFu << 4) |
                                  (0xFu << 8) | (0xFu << 16))) |
                 (0x3u << 0) | (0x3u << 4) | (0x3u << 8) | (0x3u << 16);

    /* Safe state before starting the time base. Active levels follow the net names. */
    GPIOA_OCTL &= ~(1u << 9);                 /* OUT_RELAY */
    GPIOB_OCTL &= ~((1u << 14) | (1u << 15)); /* fault clear, LLC_EN */
    GPIOC_OCTL &= ~((1u << 10) | (1u << 12)); /* PFC relay, fan */
    SYSTICK_LOAD = 799u; /* IRC8M / 10 kHz - 1 */
    SYSTICK_VAL = 0u;
    SYSTICK_CTRL = 7u;
}

/* ADC conversion is deliberately a bring-up hook. It returns unsafe values until
 * board-specific calibration has been measured and installed. */
static charger_inputs_t sample_inputs(void)
{
    charger_inputs_t in = {0};
    in.hardware_fault = true;
    in.start_request = (GPIOC_ISTAT & (1u << 6)) != 0u;
    return in;
}

static void apply_outputs(const charger_outputs_t *out)
{
    cv_pwm = out->cv_pwm_permille;
    current_pwm = out->current_pwm_permille;
    gpio_write(&GPIOB_OCTL, 15u, out->llc_enable);
    gpio_write(&GPIOC_OCTL, 10u, out->pfc_relay);
    gpio_write(&GPIOA_OCTL, 9u, out->output_relay);
    gpio_write(&GPIOC_OCTL, 8u, out->llc_enable); /* red: power active */
    gpio_write(&GPIOC_OCTL, 9u, !out->llc_enable); /* green: standby */
}

int main(void)
{
    charger_control_t control;
    uint32_t last = 0u;
    hardware_init();
    charger_control_init(&control);
    for (;;) {
        uint32_t now = milliseconds;
        if (now != last) {
            charger_inputs_t inputs = sample_inputs();
            charger_control_step(&control, &inputs, now - last);
            apply_outputs(&control.outputs);
            last = now;
        }
    }
}
