.syntax unified
.cpu cortex-m4
.thumb
.global _start
.extern main
.extern SysTick_Handler

.section .isr_vector,"a",%progbits
.word _stack_top
.word _start
.rept 13
.word default_handler
.endr
.word SysTick_Handler

.section .text._start,"ax",%progbits
.thumb_func
_start:
  ldr r0, =_sidata
  ldr r1, =_sdata
  ldr r2, =_edata
1: cmp r1, r2
  bcs 2f
  ldr r3, [r0], #4
  str r3, [r1], #4
  b 1b
2: ldr r1, =_sbss
  ldr r2, =_ebss
  movs r3, #0
3: cmp r1, r2
  bcs 4f
  str r3, [r1], #4
  b 3b
4: bl main
  b default_handler

.thumb_func
default_handler:
  b default_handler
