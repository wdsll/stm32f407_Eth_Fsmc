功率因数校正（PFC）和 LLC 状态机(v02)
1.  从《PFC ctl》《Main》《Control》三张图看
	PFC 控制芯片：U2 NCP1654BD65R2G（图纸里标注 “PFC控制芯片”），引脚：
		VM / CS / Brown-Out / Vcontrol / Feedback / VCC / Driver 等。
	栅极驱动：U28 NSG4420i，标注 DRIVER_PFC，连接到 PFC MOS 栅极。
		MCU 通过排线 J7/J9 和 PFC 板交互的关键信号（出现在 Main + Control 图里）：
		RELAY_PFC_EN_1 – PFC 继电器 / 预充驱动信号（通过 Q7“Precharge_Driver”）
		BUS_VOL_ADJ_1 – 通过 FOD8342 线性光耦和一只 817 光耦进 NCP1654 的控制端，用来微调母线电压设定，同时兼做硬件关断通道。
		BUS_VOL_SAMPLE_2 – PFC 后的 400V 母线采样，经分压到 MCU 侧。
		AC_VOL_SAMPLE – 输入 AC 电压采样，经分压滤波到 MCU。
		T_SENSE_PFC_MOS_1 – PFC MOS 管 NTC 温度采样。
	还有系统共用的：
		VOUT_SENSE / VBT_SENSE / I_SENSE – 电池电压、电池电流、LLC 输出电压，用来做功率/状态判定。
		HARD_PRO / HARD_PRO_READ – 来自光耦 / 比较器的硬件保护信号（图里在 BUS_VOL_ADJ 光耦那一块）。
		PFC 控制芯片本身已经完成 电流整形 + 电压环，所以 MCU 这边更像是“上位机 + 状态机 + 附加保护”。
2. MCU—PFC 软件需要覆盖的功能块
	2.1 PFC 启停 / 继电器 & 预充控制
	涉及信号：RELAY_PFC_EN，BUS_VOL_ADJ，HARD_PRO，BUS_VOL_SAMPLE，AC_VOL_SAMPLE
	软件要实现一套明确的上电/下电流程：
	(1)上电前条件判断:采 AC_VOL_SAMPLE，判定是否有有效市电、是否在允许范围（比如 85–265Vac）。
					  检查系统是否无故障（HARD_PRO 低、温度正常等）。 
	(2)预充阶段：通过 RELAY_PFC_EN 驱动 Q7，让直流母线通过预充电阻慢慢充到一定电压（例如 200–250V）。
				 用 BUS_VOL_SAMPLE 实时监测母线电压上升情况：预充超时但母线上不去 → 报“预充失败”，禁止继续上电。上升过快/过高 → 报异常，立即断开预充。
	(3)闭合主继电器 + 允许 PFC 工作:预充完成后，继续用 RELAY_PFC_EN 闭合主 PFC 继电器（图纸里驱动部分同时负责预充和主继电器）
									将 BUS_VOL_ADJ 从 0 逐渐拉到对应目标母线电压的值，给 NCP1654 一个“母线电压设定”，等效开启 PFC。
	(4)停机 & 故障关断:正常停机：先将 BUS_VOL_ADJ 缓慢拉低到 0，使 PFC 停止工作，再断开 RELAY_PFC_EN。
					   硬件保护触发（HARD_PRO 置位或 MCU 判定严重过压/温度）：
					   立刻拉低 BUS_VOL_ADJ，断开 RELAY_PFC_EN，同时上报全局故障，禁止自动重启或设置重启策略。
	这一块可以在 pfc_control.c 里实现一个简单状态机：PFC_OFF → WAIT_AC → PRECHARGE → WAIT_RELAY → RAMP_UP → RUN → FAULT
	2.2 母线电压目标管理（BUS_VOL_ADJ 闭环/开环控制）
	涉及信号：BUS_VOL_ADJ，BUS_VOL_SAMPLE，VBT_SENSE，VOUT_SENSE
	硬件允许 MCU 通过 BUS_VOL_ADJ 远程调节 NCP1654 的电压环设定，所以软件至少要实现：
	(1)母线目标电压计算:标准恒压模式：固定目标，例如 Vbus_target = 400V。VBUS=PA3ACD*144.3464
	(2)BUS_VOL_ADJ 输出算法:
		如果硬件是 MCU PWM + RC 滤波 → BUS_VOL_ADJ：在 pfc_control 里提供一个 pfc_bus_adj_set(float value_0_1)，对应 PWM duty。
	建立 Vbus_target ? BUS_VOL_ADJ 的标定（简单线性：查表或斜率 + 偏置）VBUS=PA3ACD*144.3464
	(3)母线电压监测与软闭环:
		每 1ms 采一次 BUS_VOL_SAMPLE，换算成实际 Vbus(VBUS=BUS_VOL_SAMPLE*144.3464)。
		待定 允许做一个很慢的 PI（带宽几十 Hz 就够）：目标：校正硬件误差/温漂，让母线长期跟随目标电压而不过冲。
		开环版本开始先用“开环 + 限幅”，后面有需要再加 PI。
	(4)与 LLC 的协同:LLC 启动前：先要求 PFC 把 Vbus 拉到“LLC 允许工作最小值”（比如 360V），再放行 LLC。
					 充电过程中：LLC 想改变工作点时，可以通过一个接口 pfc_set_vbus_target() 请求 PFC 改母线。
	2.3 采样与估算：输入电压、母线、电流、温度
	信号：AC_VOL_SAMPLE，BUS_VOL_SAMPLE，T_SENSE_PFC_MOS，VBT_SENSE，I_SENSE，VOUT_SENSE,软件需要一个“慢速采样任务”（比如 1kHz / 500Hz），统一完成：
	(1)AC电压检测：对 AC_VOL_SAMPLE 做整流/绝对值 + 滑动平均，估算 Vac_RMS。VAC=AC_VOL_SAMPLE*233.38
				   判定是否有 AC；低压/高压限（欠压不启动，过压停机）。
	(2)PFC 母线电压:BUS_VOL_SAMPLE → 换算成 400V 母线实际值；VBUS=BUS_VOL_SAMPLE*144.3464
					状态机中预充完成条件；PFC 运行中的过压/欠压保护；BUS_VOL_ADJ 反馈闭环。
	(3)PFC mos温度：T_SENSE_PFC_MOS 是 NTC 分压，需要查表/多项式转成温度（°C），设置两级门限：告警温度和关断温度
	(4)系统输入输出：VBT_SENSE、VOUT_SENSE、I_SENSE 等，虽然是整机共用，但 PFC 这边会用来：
					 估算当前输入功率 / 输出功率；
					 决定是否需要限制 PFC 输出（例如电池已接近满电）。
	2.4 保护与故障管理
	信号：HARD_PRO，BUS_VOL_SAMPLE，AC_VOL_SAMPLE，T_SENSE_PFC_MOS 等
	在软件上需要定义一套 PFC 相关的DTC策略：
	(1)硬件保护输入（Hard_Pro）:由光耦 / 比较器组合成的硬件保护，一旦触发说明外部已经采取了措施（如切断驱动）。
								MCU 需要：
									以 EXTI 或轮询方式监测 HARD_PRO；这个版本用EXTI
									触发时立刻进入 PFC_FAULT 状态，关掉 RELAY_PFC_EN 和 BUS_VOL_ADJ；
									将故障原因上报到整机故障管理模块，通常需要人工干预/掉电才允许复位。
	(2)软件保护母线过压：Vbus > Vbus_ovp 时，先拉低 BUS_VOL_ADJ，若仍不降则关掉 PFC。
						 母线欠压：在 RUN 状态下长期 Vbus < Vbus_uvp，判定 PFC 无力或 AC 掉电。
						 AC 超限：Vac 低于上电阈值，可以平滑停机；Vac 过高则立即关断。
						 温度过高：PFC MOS 温度超过阈值时限功率或停机。
	(3)故障记忆与恢复策略:区分 “可自动恢复” vs “需要掉电/人工确认” 的故障类型。
						  给 LLC 控制模块暴露接口：pfc_is_fault_latched() / pfc_clear_fault()。
	
	2.5 与 LLC / 主状态机的接口
	结合你现在的 llc_control.c / pfc_control.c 结构，PFC 软件至少要提供：
		pfc_init() / pfc_enable() / pfc_disable()
		pfc_set_vbus_target(float vbus) – 内部转成 BUS_VOL_ADJ。
		pfc_tick_1kHz() – 跑上面的状态机 + 采样 + 保护。
		pfc_get_vbus() / pfc_get_vac() / pfc_get_temp_pfc() 等只读接口。
		pfc_is_ready() – 母线电压 OK、无故障，可供 LLC 进入软启。
		pfc_is_fault() – PFC 故障，LLC 必须停。
	这样整机顶层状态机里可以写出这样的顺序：等 PFC pfc_is_ready() == true，才调用 llc_softstart_begin()，运行中只要 pfc_is_fault() 为真，就立即关掉 LLC + PFC。
3. 先看一下 LLC 相关硬件接口（从 Main + Control 两张图里来的）
	驱动输出：LLC_DRVH+ / LLC_DRVL+：从控制板通过隔离（NSI6602 之类）送到主板 LLC 半桥驱动（高/低臂栅极）。
			  LLC_EN：使能 LLC 驱动 / 上电控制。
	采样 & 监控：
			  VOUT_SENSE_1：LLC 输出电压采样。
			  VBT_SENSE_1：电池电压采样（整机共用，但 LLC 电压环会参考）。
			  I_SENSE_1：输出电流采样，用于限流 / CC 模式。
			  T_SENSE_LLCMOS_1：LLC 主 MOS NTC 温度。
			  共用的 HARD_PRO_READ / HARD_PRO：硬件保护比较器的输出（PFC + LLC 共用）。
			  外部条件：母线电压 BUS_VOL_SAMPLE_2（其实在 PFC 章节里，但 LLC 上电要看它）。PFC 状态 / 故障标志（软件接口，非硬件脚）。
4. LLC 软件需要实现的功能模块
	4.1 PWM 驱动层：产生 LLC 半桥 PWM 对应：pwm_llc.c + 部分 Timer.c
	(1)高级定时器配置，
		使用TIMER0高级定时器：
			两路互补输出 → 通过隔离后成为 LLC_DRVH+ / LLC_DRVL+。
			配置死区时间 dt_ns，并保证 软启模块只在安全占空窗内活动（你 MATLAB 里那个 safe window）。
			使能 BKIN 从硬件比较器 / HARD_PRO 进来，一旦触发立刻关 PWM。
	(2)频率/占空控制API：
		llc_pwm_set_freq_khz(float f_khz);
		llc_pwm_set_duty(float duty_0_1);（一般运行时固定接近 50%，只在软启阶段微调）；
	自动根据 f_khz 计算周期 per_ns，供软启模块算安全占空窗。
	(3)输出使能管理:llc_pwm_outputs_enable(bool on);
					硬件上拉 LLC_EN，并打开/关闭 PWM 输出通道。与整机状态机配合：只有在 PFC 母线就绪 + 无故障 时才允许使能。
	4.2 LLC 软启动（Soft-start） 对应：llc_soft_start.c / 你刚从 MATLAB 翻译的 llc_softstart_step
	主要目标：从 0 功率平滑地升到目标占空/频率，避免冲击与跨过死区。
	(1)占空软启（你已经有的）
		输入：target_duty、start_cmd、fault、pause_req、per_ns、dt_ns。
		功能：根据 per_ns & dt_ns 计算安全占空窗 [d_min, d_max]；
			  从 LLC_SOFTSTART_START_DUTY 以 S 曲线（cos 或 exp）缓慢爬到目标占空；
			  故障时回退到 FAILSAFE_DUTY 并退出；
			  暂停/恢复时保持当前 duty 不跳变；
			  软启结束后置 active_out = 0，把控制权交给正常环路。
	(2)Vref 软启（可选，但很常用）			
		将 电压环的目标电压 Vref 从 0 慢慢升到设定值（比如 44V），避免一上电就“满负荷调节”。
		可以做成一个简单的斜坡或 S 曲线，在 llc_control.c 里实现即可。
	(3)和状态机的配合
		LLC状态机只有一个状态是对应软启动的ST_LLC_SOFTSTART：
			进入时：llc_softstart_begin(target_duty)；
			运行时：周期性 llc_softstart_tick() + 用 active_out 判断是否完成；
			软启完成后切到 ST_LLC_RUN 并启用电压环。  
	4.3	LLC 开环调试 / 功率扫描（Bring-up & Bode）	对应：llc_open_loop.c	
		硬件刚焊完或做环路建模时，你需要：
		(1)固定频率/占空运行:llc_open_loop_set_freq(float f_khz);llc_open_loop_enable(bool on);
	在不开环环路的情况下，仅靠 f_cmd 控制 LLC 输出，便于示波器测波形、量变压器、确认谐振点。
		(2)扫频/扫占空脚本接口:
			软件层：支持从 PC/串口或内部脚本控制频率列表，逐点停留一段时间，并记录 VOUT / IOUT / VBUS。
			你现在 MATLAB 那个 Bode + CSV 也依赖这块：MCU 负责扫频，仿真或外部脚本负责采数据。
	4.4.闭环控制：CV / CC / CP 模式 对应：llc_control.c
		(1)电压环（主环）
			采样 VOUT_SENSE_1，换算出输出电压。高速采样上一版本的PWM触发
			与目标 Vref 做差，经过数字 PI（带积分限幅与 anti-windup）输出一个 频率指令 f_cmd：
				误差大 → 降频接近谐振 → 增大增益；
				误差小 / 过压 → 升频远离谐振 → 降低增益。
			带频率夹紧：f_min（靠近谐振）和 f_max（轻载/保护）
		(2)电流环（限流 / CC 模式）（如果你要做 CC）
			采 I_SENSE_1，设定限值 I_limit。高速采样上一版本的PWM触发
			两种常见方式：
				软限流：当 I > I_limit 时，限制 f_cmd 不再下降（相当于限制最大增益）。
				硬 CC：加入一个电流环 PI，与电压环竞争，选择“最限制”的那一边输出 f_cmd。
		(3)模式切换,双环竞争
			CV → CC：充电电流打到 I_limit，切换为恒流模式（电压允许缓慢上升）。
			CC → CV：接近目标电压时自动回到电压控制。
			软件上通常用一个简单的条件判断 + 滞回，放在 llc_control_tick() 里。
		(4)控制周期
			你现在电压环大约用 1 kHz（1 ms）比较合理，相比 80~150 kHz 的开关频率已经很慢：
			每次周期：读 ADC → 做滤波 → 更新 PI → 更新 f_cmd → 写到 pwm_llc。
			需要保证计算时间 << 1ms，避免占用太多 CPU。
	4.5. 采样与观察值计算 对应：adc_dma.c + llc_control.c 中的处理
		(1)ADC 通道分配：VOUT_SENSE_1（输出电压），VBT_SENSE_1（电池电压），I_SENSE_1（输出电流），T_SENSE_LLCMOS_1（LLC MOS 温度）
		(2)滤波和标定：每个量都要有：ADC → 电压/电流/温度 物理量的换算，外加简单低通滤波。
					   温度用查表或多项式；电压/电流使用比例系数即可。
		(3)观测接口：给上位机/调试串口提供 llc_get_vout(), llc_get_iout(), llc_get_temp_llc() 等，方便调试
	4.6 DTC llc侧：大部分保护其实是“全机共用”，但触发时 LLC 一定要立即停：
		(1)硬件快速保护:通过 BKIN / 外部比较器实现，例如：一次侧过流；次级严重过压；
						软件响应：在中断任务中检测 HARD_PRO_READ：一旦触发，记录故障原因，设置 LLC_FAULT 状态，禁止再次启动；通知 PFC 和整机上层。
		(2)软件保护
			VOUT 过压：> Vout_OVP 时立即升频或停机；
			I_SENSE 过流：> I_OC（大于 CC 值很多）时判定短路，停机；
			T_SENSE_LLCMOS 过温：> T_SHUTDOWN 时停机，> T_WARN 时限功率；
			与 PFC 类似：区分可恢复/不可恢复，挂在统一故障管理模块。
	4.7 与 PFC / 整机状态机的协同
		虽然 PFC 已经有一套状态机，但 LLC 这边要配合它工作：
		(1)启动顺序:只有在pfc_is_ready() == true（母线电压到位，PFC 无故障）,并且自身无故障的时候才会允许进入 软启并且使能LLC_EN
		(2)停机顺序：正常停机：LLC 先软停（升频、减占空 / 降 Vref），然后关闭 PWM，再通知 PFC 可以降母线；
					 出现 PFC 故障：pfc_is_fault() 为真时，立即停 LLC（保护优先级最高）。
		(3)功率 / 模式协调
			根据电池电压/电流、BMS 指令选择 LLC 的 CV/CC 参数；
			在极低输入电压 / PFC 降额时，限制 LLC 的最大输出功率。



			
