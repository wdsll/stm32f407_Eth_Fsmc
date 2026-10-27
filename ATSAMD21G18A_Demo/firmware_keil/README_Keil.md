# Keil MDK 移植版 — ATSAMD21G18A_Demo

这是 `firmware/`（MPLAB X + XC32 工程）的 **Keil MDK 并行版本**。
MPLAB 工程一行源码都没改，两边共享同一套 `firmware/src` 源码，可并行维护。

## 前置条件

1. Keil MDK 5.25+（已验证：MDK Plus 5.25 + ARM Compiler 6.9）
2. 器件包 **Microchip::SAMD21_DFP**（本机已安装 3.6.144 到
   `%LOCALAPPDATA%\Arm\Packs\Microchip\SAMD21_DFP\3.6.144`，
   与 MPLAB 工程所用 DFP 版本一致；缺了可在 Pack Installer 里在线装更新版）

## 使用

1. 双击 `ATSAMD21G18A_Demo.uvprojx` 打开。
2. 直接 F7 编译；HEX 自动输出到 `Objects\ATSAMD21G18A_Demo.hex`。
3. 下载/调试：Options for Target → Debug → 选 **CMSIS-DAP Debugger**
   （EDBG、Atmel-ICE 都走这个驱动），Port = SWD。
   MPLAB 那边用的 Atmel-ICE / EDBG 在 Keil 里继续用同一根线。
4. Watch 窗口观察 `g_uart_done`、`g_uptime_ms` 等变量。

## 与 MPLAB 工程的差异（全部集中在 firmware_keil/ 内）

| 项 | MPLAB/XC32 | Keil/AC6 |
|---|---|---|
| 启动文件 | `startup_xc32.c`（依赖 libpic32c.h，**排除**） | `startup_samd21_keil.c`（新增） |
| 链接脚本 | `ATSAMD21G18A.ld`（GNU ld，**排除**） | `ATSAMD21G18A.sct`（scatter，新增） |
| libc 存根 | `libc_syscalls.c`（newlib `_exit`，**排除**） | ARM C 库自带，不需要 |
| stdio | `xc32_monitor.c`（**排除**） | 未用 printf；以后要用再配 MicroLIB retarget |
| 芯片宏 | XC32 由 DFP 自动注入（工程里宏为空） | 头文件守卫是普通 include-guard，**无需任何宏** |
| 优化 | -O1 | -O1（Options → C/C++ → Optimization -O1） |
| 堆/栈 | XC32 默认（heap 512B） | 栈顶 = 0x20008000（SRAM 顶），未用 heap |

## 关键机制（排查问题时看这里）

- **栈顶符号桥接**：`interrupts.c` 的向量表第 0 项是 `.pvStack = &_stack`，
  `_stack` 原是 XC32 链接脚本符号。本工程用全局宏
  **`_stack=__initial_sp`**（Options → C/C++ → Define）把它接到
  `startup_samd21_keil.c` 里的 `__initial_sp`；该锚点由 scatter 文件的
  SP_ANCHOR 区（0x20008000，SRAM 顶）强制定位——注意 armclang 会静默丢弃
  `__attribute__((at()))`，所以绝对定位必须写在 scatter 里，别改回 at()。
- **向量表必须落在 0x00000000**：scatter 文件里 `*(.vectors, +First)` 保证
  `exception_table` 位于 Flash 起始；改 scatter 时别删这行。
- **启动流程**：硬件取向量[0]=SP、向量[1]=Reset_Handler → 设 VTOR →
  `__main()`（MicroLIB：搬 .data、清 .bss、重设 SP）→ `main()` →
  `SYS_Initialize()`，与 XC32 版行为一致。
- **C 库**：用 MicroLIB（Options → Target → Use MicroLIB 已勾选）。
  Demo 没用到 printf/heap。以后若要 printf：保留 MicroLIB，自己写一个
  `fputc()` 重定向到 `SERCOM0_USART_Write` 即可，不要把 `xc32_monitor.c`
  加回来。

## 已验证内容（2026-10-27）

- UV4 命令行 `-b` 全量编译通过（armclang 6.9 + armlink），无 error；
- map 确认 `exception_table` 位于 0x00000000、`__initial_sp` 位于 0x20008000；
- 生成了 `Objects\ATSAMD21G18A_Demo.axf/.hex`。
- **未做实板下载运行**——首次上板请先按上面第 3 步配好 CMSIS-DAP。

## 以后往工程里加代码

1. 业务 `.c` 放 `firmware/src/` 下（建议建 `firmware/src/app/`），在 Keil 的
   Application 组右键 Add Existing Files；头文件目录若新增，补进 IncludePath。
2. Harmony 配置变更（MCC/MPLAB 重新生成 `plib_*.c`）后，Keil 侧不用动，
   源码是共享的；但**别**把 MPLAB 生成的 startup/向量表文件再加进 Keil 工程。
