/* =============================================================================
 * ATSAMD21G18A startup for Keil MDK (ARM Compiler 6 / armclang)
 *
 * Replaces Harmony 3's startup_xc32.c, which is XC32-specific
 * (libpic32c.h, __pic32c_data_initialization, __libc_init_array).
 *
 * Reset flow:
 *   1. Hardware loads SP from vector[0] and PC from vector[1].
 *      - vector[0] = &_stack = &__initial_sp = 0x20008000 (top of 32KB SRAM),
 *        wired via the project-wide define  _stack=__initial_sp
 *        (interrupts.c does: .pvStack = &_stack).
 *      - vector[1] = Reset_Handler (this file).
 *   2. __main() (MicroLIB scatter-load): copies .data from flash, zero-fills
 *      .bss, sets SP from __initial_sp, then calls main().
 *   3. main() -> SYS_Initialize() exactly as in the MPLAB project.
 *
 * The vector table itself lives in firmware/src/config/samd21g18a/interrupts.c
 * (section ".vectors"), placed first in flash by ATSAMD21G18A.sct.
 * =============================================================================
 */
#include <stdint.h>
#include "device.h"          /* -> samd21g18a.h -> core_cm0plus.h (SCB)      */
#include "device_vectors.h"  /* H3DeviceVectors                              */

extern int  main(void);
extern void __main(void);    /* ARM C library entry: scatter-load + main()   */

/* Vector table defined in firmware/src/config/samd21g18a/interrupts.c */
extern const H3DeviceVectors exception_table;

/*
 * Initial stack pointer, consumed by two parties:
 *   - interrupts.c vector table entry[0]  (via the define _stack=__initial_sp)
 *   - MicroLIB __main/__rt_entry          (re-programs SP to this address)
 * It is an anchor symbol placed AT the top of SRAM (0x20008000) by the
 * scatter file (SP_ANCHOR region); only its address is ever taken.
 * Note: armclang may silently drop __attribute__((at())) under some option
 * combinations, so the absolute placement is enforced in ATSAMD21G18A.sct.
 */
uint32_t __initial_sp __attribute__((section(".bss.__initial_sp"), used));

__attribute__((noreturn))
void Reset_Handler(void)
{
    /* Vector table sits at the very start of flash; set VTOR explicitly */
    SCB->VTOR = (uint32_t)&exception_table;

    /* C library init: copy .data, zero .bss, set SP, then call main() */
    __main();

    while (1)
    {
        /* main() must never return */
    }
}
