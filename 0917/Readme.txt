#PFC 与 LLC 状态机切换评估

## PFC 状态机

PFC 状态机由 `PFC_ST_IDLE`、`PFC_ST_CHARGING`、`PFC_ST_READY`、`PFC_ST_FAULT` 四个状态组成，核心逻辑位于 `pfc_state_enter` 与 `pfc_app_tick_1khz`。进入任一状态都会记录 `entry_ms`、`vbus_ok_since_ms`、`dropout_since_ms`，并在 `pfc_state_enter` 中完成硬件使能或关闭操作。`PFC_ST_IDLE` 与 `PFC_ST_FAULT` 都会禁止 PFC；`PFC_ST_CHARGING`、`PFC_ST_READY` 保持使能。【F:0917/APP/src/main.c?L404-L463】

在 `pfc_app_tick_1khz` 中，若检测到保护故障会立即跳转至 `PFC_ST_FAULT`。
- **IDLE → CHARGING**：需要收到 `enable_cmd`，且满足启动延时 `PFC_STARTUP_DELAY_MS`；否则保持空闲。【F:0917/APP/src/main.c?L469-L500】
- **CHARGING → READY**：在保持 `enable_cmd` 的前提下，只要 VBUS 达到 `PFC_VBUS_READY_V` 且持续 `PFC_READY_DELAY_MS` 即转入 READY；期间若电压回落会重置计时。若撤销 `enable_cmd` 或出现故障，将分别退回 IDLE 或进入 FAULT。【F:0917/APP/src/main.c?L500-L534】
- **READY → CHARGING**：READY 状态下持续监测 VBUS，当电压低于 `PFC_VBUS_READY_V - PFC_VBUS_READY_HYST_V` 并持续 `PFC_VBUS_DROPOUT_MS` 后，返回 CHARGING 重新补电；保持 `enable_cmd` 与无故障即可留在 READY。【F:0917/APP/src/main.c?L518-L545】
- **FAULT → CHARGING/IDLE**：若仍保持使能命令且故障消失，在经过 `PFC_RESTART_DELAY_MS` 后可重启进入 CHARGING；若撤销使能且故障已解除，则回到 IDLE。该设计确保故障后存在冷却时间并支持人工复位。【F:0917/APP/src/main.c?L545-L567】

总体来看，PFC 状态切换充分考虑了启动延时、电压检测去抖、掉压回退和故障自恢复路径，逻辑闭环完整。

## LLC 状态机

LLC 状态机包含 `ST_IDLE`、`ST_WAIT_VBUS`、`ST_LLC_RUN`、`ST_FAULT`。`llc_state_enter` 负责在状态切换时复位积分、控制 PWM 以及在等待阶段请求 PFC 启动。`ST_WAIT_VBUS` 请求 PFC 开启并保持 PWM 关闭，`ST_LLC_RUN` 根据开环或闭环模式设置运行频率并打开 PWM，`ST_IDLE`/`ST_FAULT` 则关闭 PWM 并强制关闭 PFC。【F:0917/APP/src/main.c?L624-L684】

`llc_app_tick_1khz` 控制状态迁移：
- **IDLE → WAIT_VBUS**：系统初始化后立即进入 VBUS 等待态，保证流程统一。【F:0917/APP/src/main.c?L688-L708】
- **WAIT_VBUS → LLC_RUN**：需要满足三个条件：无保护故障、PFC 状态非 FAULT 且 `pfc_app_ready()` 为真，同时检测输出电压 `vmeas` 达到 `LLC_ENTRY_V` 并持续 `LLC_START_DELAY_MS`（100ms）。未满足时会不断刷新 `entry_ms` 形成去抖。【F:0917/APP/src/main.c?L708-L724】
- **LLC_RUN → WAIT_VBUS**：若 PFC 未就绪或 VBUS 掉至 `LLC_ENTRY_V - PFC_VBUS_READY_HYST_V` 以下，会回退到等待态；检测到保护故障则进入 FAULT。【F:0917/APP/src/main.c?L724-L735】
- **任意状态 → FAULT**：在等待或运行过程中，一旦 `protect_fault_latched()`、`protect_fault_active_hw()` 或 PFC 进入 FAULT，则直接跳转至 `ST_FAULT` 并由 `llc_state_enter` 关闭输出。当前代码中 FAULT 状态无自动退出，需要外部逻辑处理。【F:0917/APP/src/main.c?L708-L736】

LLC 状态机与 PFC 状态高度耦合：WAIT_VBUS 会请求 PFC 启动，RUN 状态对 PFC readiness 及电压实时监控并在异常时回退，体现了主从级联控制策略。状态流向清晰，具备延时与滞回以抑制抖动。

## 协同流程总结

1. 初始化时 LLC 进入 WAIT_VBUS，并触发 PFC 开启；PFC 在达到目标电压并满足延时后进入 READY。
2. LLC 确认 VBUS 达到 `LLC_ENTRY_V` 且稳定 100 ms 后进入 RUN，打开 PWM。
3. 若运行中 VBUS 掉压或 PFC 重启，LLC 会退回 WAIT_VBUS；若出现保护故障，PFC/LLC 分别转入 FAULT 并关闭输出。

该状态机设计实现了先 PFC 建压、后 LLC 上电、再根据电压反馈维持运行的主流程，并提供了多重安全回退机制。后续若需要增加故障恢复，可在 `ST_FAULT` 分支增加条件判断或外部复位逻辑。

## LLC 开环模式实现功能

项目在 `LLC_USE_OPEN_LOOP` 宏开启时，引入一个基于分段配置的开环频率控制器，其核心功能包括：

* **分段频率曲线**：在 `main.c` 中定义了三段式扫频配置，依次完成从最小频率升至初始频率并保持 200 ms、再以 500 Hz/ms 的斜率拉升至最大频率保持 200 ms，最后回落到初始频率；每段都可设置保持时间与斜率，实现软启动和预热流程。【F:0917/APP/src/main.c?L184-L193】
* **状态化的开环控制器**：`llc_open_loop.h` 定义了段表、当前段索引、目标频率、保持计时与运行/保持标志，用于跟踪扫频进度与当前输出频率命令。【F:0917/HW/include/llc_open_loop.h?L36-L75】
* **生命周期管理接口**：`llc_open_loop_init` 将控制器重置到首段起点并挂接配置，`llc_open_loop_start`/`stop` 分别清理保持状态、拉回到首段起点、置运行标志，确保每次进入 RUN 状态都从预设序列开始或在退出时收尾归零。【F:0917/HW/src/llc_open_loop.c?L103-L166】
* **逐毫秒执行与自动段切换**：`llc_open_loop_tick` 在运行时按照设定斜率调节 `f_cmd`，到达段终点后进入保持计时并在保持完成或保持时间为 0 时自动切换到下一段；当最后一段完成后会清空保持标志并停止运行，保持最终频率在控制器中缓存。【F:0917/HW/src/llc_open_loop.c?L193-L270】
* **与 LLC 状态机联动**：进入 `ST_LLC_RUN` 时未完成的开环序列会被启动，`control_loop_tick_1khz` 周期性读取开环频率并在序列完成后锁存最终频率供后续运行使用，同时在其他状态下停止扫频，避免无谓的频率更新。【F:0917/APP/src/main.c?L621-L757】【F:0917/APP/src/main.c?L780-L805】

## 优化建议

### PFC 状态机

1. **提取通用的故障与延时判断**：`pfc_app_tick_1khz` 在多个分支内重复读取 `protect_fault_latched()`、`protect_fault_active_hw()` 以及重复维护 `vbus_ok_since_ms`、`dropout_since_ms` 计时器，可通过内联函数封装“故障是否存在”“计时器启动/复位”等通用操作，既减少重复判断，又便于在不同状态共享统一的去抖策略。【F:0917/APP/src/main.c?L477-L570】
2. **消除 switch 的隐式贯穿逻辑**：当前 `PFC_ST_CHARGING` 分支缺少显式 `break`，依赖 `switch` 贯穿执行 READY 分支逻辑，这会让 READY 状态的掉压计时在充电阶段也被触发，增加理解成本。建议改为在成功切换状态后立即 `break`，并在循环外通过 `while` 或 `do/while` 结构重评估状态，使状态跳转和状态处理解耦。【F:0917/APP/src/main.c?L498-L551】
3. **统一状态切换副作用**：`pfc_state_enter` 对不同状态下的继电器操作和计时器初始化存在多处注释掉的代码，建议将硬件动作与状态切换绑定在专门的 `pfc_state_apply_outputs` 函数中，并按需启用/禁用继电器，避免后续扩展时遗漏硬件同步。【F:0917/APP/src/main.c?L365-L448】

### LLC 状态机

1. **提前返回处理故障**：`llc_app_tick_1khz` 在 WAIT_VBUS 与 RUN 两个分支重复检查三种故障条件，可以在函数开头集中判断并直接跳入 FAULT，减少重复代码，保持状态分支只关注电压门限与延时逻辑。【F:0917/APP/src/main.c?L685-L723】
2. **避免依赖 case 贯穿执行**：为了在单个 tick 内完成 WAIT_VBUS → RUN 的转移，当前通过省略 `break` 实现，这与 PFC 状态机类似地增加了阅读难度。可以改写为 `while (llc_state_changed)` 循环，每当 `llc_state_enter` 修改状态后重新进入 switch，从而显式表述“可能连续切换多次”，避免隐含贯穿。【F:0917/APP/src/main.c?L685-L719】
3. **抽象 PFC 就绪判定**：RUN 分支不仅要检查 PFC 准备好，还要处理滞回。可考虑在 PFC 层提供 `pfc_bus_within_ready_window()` 之类的辅助函数，以便 LLC 只依赖 PFC 暴露的接口，而不是直接使用 `PFC_VBUS_READY_HYST_V` 常量，提高模块隔离度。【F:0917/APP/src/main.c?L715-L718】

### 公共改进方向

1. **集中管理毫秒计时器**：当前 PFC 与 LLC 通过 `entry_ms`、`vbus_ok_since_ms`、`dropout_since_ms` 等字段手工维护去抖延时，可考虑引入通用的“毫秒计时器”结构（如 `{bool active; uint32_t start_ms; uint32_t delay_ms;}`），通过统一的 `timer_start_if_ready()`、`timer_expired()` 接口减少重复的 `(uint32_t)(g_ms - start)` 比较语句。【F:0917/APP/src/main.c?L477-L570】【F:0917/APP/src/main.c?L685-L719】
2. **缓存采样和控制判定结果**：`control_loop_tick_1khz` 在一次循环内多次读取 `llc_app_state()`、`pfc_app_ready()` 等接口，可缓存结果或使用结构体承载“本周期状态”，既能减小函数调用开销，也能提升调试时的一致性。【F:0917/APP/src/main.c?L735-L759】
3. **结合模块测试逻辑**：模块测试代码与正式流程通过条件编译混在一起，后续可将测试上下文 `s_module_tests` 抽离至独立文件，并在状态机里提供钩子函数（如 `pfc_on_state_changed`、`llc_on_state_changed`）供测试/调试模块注册回调，以降低主流程的条件编译噪声。【F:0917/APP/src/main.c?L70-L225】

## 控制器模块一览

综上，项目当前涉及以下几类控制器/控制回路：

1. **PFC 状态控制器**：由 `pfc_app_ctx_t` 维护状态、计时和使能指令，通过 `pfc_state_enter` 与 `pfc_app_tick_1khz` 完成启动、补电、掉压回退和故障恢复等逻辑，是整流级的主控单元。【F:0917/APP/src/main.c?L83-L171】【F:0917/APP/src/main.c?L404-L567】
2. **LLC 闭环频率控制器**：`llc_t` 结构封装电压 PI 参量与频率限幅，`llc_step` 根据误差更新积分与目标频率，是 LLC 模块在闭环模式下维持母线电压的核心控制器。【F:0917/APP/include/main.h?L129-L139】【F:0917/APP/src/main.c?L589-L657】
3. **LLC 开环扫频控制器**：`llc_open_loop_ctrl_t` 记录扫频段、运行状态与保持计时，通过 `llc_open_loop_tick` 驱动多段频率曲线，用于上电阶段的开环调频流程。【F:0917/HW/include/llc_open_loop.h?L36-L75】【F:0917/HW/src/llc_open_loop.c?L103-L260】
4. **母线电压调节控制器**：`bus_vol_adj_ctrl_t` 使用 PI 算法调节 PB0 PWM 占空比，对应的 `bus_vol_adj_tick` 在 LLC 运行时细调直流母线电压，是辅助级的调节回路。【F:0917/HW/include/pwm.h?L7-L24】【F:0917/HW/src/pwm.c?L69-L118】

上述四个控制器分属 PFC 整流、LLC 谐振以及母线辅助调节三个层级，既有状态机控制也有连续的 PI 调节，构成当前固件的主要控制面。
