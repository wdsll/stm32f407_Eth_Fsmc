# ATSAMD21G18A MPLAB X 最小 Demo

## 直接打开

1. 将整个压缩包解压至短英文路径，例如 D:\MCW_Demo\。
2. 启动 MPLAB X IDE，选择 File → Open Project。
3. 选择 firmware\ATSAMD21G18A_Demo.X 文件夹，点击 Open Project。
4. 右键工程 → Properties，确认 Device = ATSAMD21G18A。
5. 在 Compiler Toolchain 选择本机安装的 MPLAB XC32。此工程沿用官方示例的 XC32 v4.60 配置，建议先使用该版本。
6. 在 Packs 确认 Microchip SAMD21_DFP 3.6.144 与 ARM CMSIS 5.9.0。缺少时通过 Tools → Packs 安装；工程已携带源码所需的器件/CMSIS头文件，但 IDE 仍需要器件包识别器件。
7. 执行 Clean and Build Project（锤子图标）。首次打开由 IDE 生成 nbproject/Makefile-*.mk，包内没有绑定其他电脑的私有路径。
8. 下载调试时，在 Hardware Tool 选择实际探针。工程默认 Atmel-ICE / SWD，请按本机硬件修改。只编译不需要连接板子。

注意：.X 是工程文件夹后缀，不是单个 .x 文件。请保留 firmware/src 与 .X 的相对目录结构。
如果工具链显示为红色或找不到 v4.60，先在 Properties 重新选择已安装且支持该器件的 XC32；不同版本仍需在本机验证。

## 包含内容

- 完整 MPLAB X 工程描述：nbproject/project.xml、configurations.xml、顶层 Makefile。
- ATSAMD21G18A 专用设备头文件及 XC32 链接脚本（Flash 256 KB、SRAM 32 KB）。
- 官方 Harmony 3 的 XC32 启动、向量表、初始化、时钟、PORT、NVMCTRL、NVIC 等源码。
- 新增 main.c：轮询 SysTick，提供 1 ms 软件计数与 500 ms 心跳。
- 新增 board.h：可选 GPIO 闪烁参数。
- 使用内部 DFLL 开环约 48 MHz，不要求外接晶振；该时钟不适合直接当作高精度定时/USB时钟方案。
- 无 RTOS、无 UART 输出、无通信协议，仅用作后续导入源码的起点。

## 运行验证

默认 DEMO_GPIO_ENABLE = 0，不重新配置板级 GPIO。下载后可在调试器 Watch 窗口观察：

- g_uptime_ms：持续增加。
- g_heartbeat：约每 500 ms 增加一次。

要看 LED 闪烁，先核对实际 PCB 原理图，再修改 firmware/src/board.h：

```c
#define DEMO_GPIO_ENABLE 1
#define DEMO_LED_GROUP 0U       /* 0 = PORTA，1 = PORTB */
#define DEMO_LED_PIN 17U        /* 示例 PA17，并非假定你的 MCW LED 在此脚 */
#define DEMO_LED_ACTIVE_LOW 1
```

启用后每约 500 ms 翻转一次，完整亮灭周期约 1 s。请使用实际连接 LED 的有效引脚。
SysTick 使用 COUNTFLAG 轮询，不启用中断；主循环阻塞超过 1 ms 时会漏计，因此它只是基础 Demo 的时基。

## 后续导入 MCW 源码

1. 先让当前 Demo 在本机完成一次 Clean and Build。
2. 将 MCW 应用代码复制到 firmware/src/mcw/，在工程 Source Files 上右键 Add Existing Items / Add Existing Items from Folders 添加 .c。
3. 在 Project Properties → XC32 (Global Options) / xc32-gcc 的 Include directories 中增加相关头文件目录。
4. 不要同时保留两个 main()，也不要重复导入另一套 startup、向量表、SystemInit 或同名中断处理函数。
5. 原 MCW 若依赖 ASF/Atmel START，请连同其驱动、配置和所需库移植，并逐项核对 API；不能只复制业务 .c 就认为移植完成。
6. 当前 Demo 尚未包含或适配你的 MCW 源码。

## 来源与修改说明

基础：Microchip 官方 csp_apps_sam_d21_da1 的 port_led_on_off_polling。
仓库：https://github.com/Microchip-MPLAB-Harmony/csp_apps_sam_d21_da1
固定提交：9331e79cf2937d2b3166813c6d2886b2481162e3
基础目录：apps/port/port_led_on_off_polling/firmware

G18A 器件文件：Microchip.SAMD21_DFP.3.6.144.atpack
下载：https://packs.download.microchip.com/Microchip.SAMD21_DFP.3.6.144.atpack

此包是基于官方源码修改的模板，不是 Microchip 发布的原样 G18A 示例。
已切换器件、设备头文件及链接脚本，移除 Xplained Pro 的 PA15/PB30 板级配置，添加轮询心跳与可选 GPIO。
未保留原 J18A 的 MCC 配置，避免重新生成时覆盖 G18A 修改；本工程可直接编辑/构建源码，但不宣称支持现成的 MCC 图形化再生成。
Microchip 和 ARM 文件的原有许可头保留，Microchip 顶层许可见 Microchip_License.md。

## 验证范围

- 两个项目 XML 能正确解析；所有 itemPath 都指向包内实际文件。
- 已核对 Device、include 路径、链接脚本指向 G18A。
- 使用主机 GCC -fsyntax-only 检查 main、初始化、时钟、GPIO、NVMCTRL、NVIC、向量表等 9 个 C 文件，通过。
- 这不是 ARM 目标代码编译。未检查 XC32 专用启动文件/系统调用的目标编译，未执行 MPLAB X 打开、XC32 编译链接或实板测试。
- 最终构建和运行需要在你的 MPLAB X + XC32 环境验证，不能视作已编译验证的交付固件。
