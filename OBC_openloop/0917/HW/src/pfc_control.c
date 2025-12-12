#include "pfc_control.h"


typedef struct
{
	pfc_state_t state;
	uint32_t entry_ms;
	uint32_t vbus_ok_since_ms; 
	uint32_t dropout_since_ms; 
	uint32_t startup_cmd_ms;
	bool enable_cmd;
} pfc_app_ctx_t;

static float s_pfc_bus_v = 0.0f;
static bool s_pfc_hw_enabled = false;
static bool s_pfc_hw_relay = false;
static pfc_app_ctx_t s_pfc_app;

static void pfc_hw_init(void);
static void pfc_hw_set_enable(bool en);

#if LLC_BYPASS_PFC_CONTROL
static void pfc_state_enter(pfc_state_t next)
{
	(void)next;
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = g_ms;
	s_pfc_app.enable_cmd = true;
}
#else
static void pfc_state_enter(pfc_state_t next)
{
		static uint8_t s_pfc_clear_sent = 0;
		if (next == s_pfc_app.state) {
						return;
		}
		s_pfc_app.state = next;
		s_pfc_app.entry_ms = g_ms;
		s_pfc_app.dropout_since_ms = 0U;
		s_pfc_app.vbus_ok_since_ms = 0U;

		switch (next) {
		case PFC_ST_IDLE:
						pfc_hw_set_enable(false);
						s_pfc_hw_enabled = false;
						break;
		case PFC_ST_READY:
						pfc_hw_set_enable(true);
						s_pfc_app.vbus_ok_since_ms = g_ms;
						break;
		case PFC_ST_FAULT:
		default:
						pfc_hw_set_enable(false);
						break;
		}

		if (next != PFC_ST_FAULT) {
						s_pfc_clear_sent = 0;
		}
}
#endif

static void pfc_hw_init()
{
#if defined(PFC_EN_PORT) && defined(PFC_EN_PIN) && defined(PFC_EN_RCU)
		rcu_periph_clock_enable(PFC_EN_RCU);
		gpio_init(PFC_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PFC_EN_PIN);
		gpio_bit_reset(PFC_EN_PORT, PFC_EN_PIN);
#endif

#if defined(PFC_MAIN_RELAY_PORT) && defined(PFC_MAIN_RELAY_PIN) && defined(PFC_MAIN_RELAY_RCU)
		rcu_periph_clock_enable(PFC_MAIN_RELAY_RCU);
		gpio_init(PFC_MAIN_RELAY_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, PFC_MAIN_RELAY_PIN);
		gpio_bit_reset(PFC_MAIN_RELAY_PORT, PFC_MAIN_RELAY_PIN);
#endif

		s_pfc_hw_enabled = false;
		s_pfc_hw_relay = false;
}

static void pfc_hw_set_enable(bool en)
{
		if (s_pfc_hw_enabled == en) {
						return;
		}
		s_pfc_hw_enabled = en;
#if defined(PFC_EN_PORT) && defined(PFC_EN_PIN)
		if (en) {
						gpio_bit_set(PFC_EN_PORT, PFC_EN_PIN);
		} else {
						gpio_bit_reset(PFC_EN_PORT, PFC_EN_PIN);
		}
#else
        (void)en;
#endif
}

void pfc_hw_set_relay(bool closed)
{
		if (s_pfc_hw_relay == closed) {
						return;
		}
		s_pfc_hw_relay = closed;
#if defined(PFC_MAIN_RELAY_PORT) && defined(PFC_MAIN_RELAY_PIN)
		if (closed) {
						gpio_bit_set(PFC_MAIN_RELAY_PORT, PFC_MAIN_RELAY_PIN);
		} else {
						gpio_bit_reset(PFC_MAIN_RELAY_PORT, PFC_MAIN_RELAY_PIN);
		}
#else
		(void)closed;
#endif
}
/* PFC READY 且已在 READY 态保持一小段时间，供 LLC 启动前做最后确认 */
static inline bool pfc_ready_for_llc(void)
{
    return (s_pfc_app.state == PFC_ST_READY) &&
           elapsed_reached(s_pfc_app.entry_ms, PFC_READY_STABLE_BEFORE_LLC_MS);
}

void pfc_app_init()
{
	pfc_hw_init();
	s_pfc_bus_v = 0.0f;
	
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = g_ms;
	s_pfc_app.enable_cmd = true;
	
#else
	s_pfc_app.state = PFC_ST_IDLE;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = 0U;
	s_pfc_app.dropout_since_ms = 0U;
	s_pfc_app.startup_cmd_ms = 0U;
	s_pfc_app.enable_cmd = false;
#endif
	pfc_hw_set_enable(false);
}

//该函数的目的是在启动某种请求时，确保系统状态正确，并记录请求的起始时间（如果系统当前不处于空闲状态）。
void pfc_app_request_start(void)
{
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_app.enable_cmd = true;
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.startup_cmd_ms = g_ms;
	return;
#else
  if(!s_pfc_app.enable_cmd)
	{
		s_pfc_app.enable_cmd = true;
	}
	if(s_pfc_app.state == PFC_ST_IDLE)
	{
		s_pfc_app.startup_cmd_ms = 0U;
		s_pfc_app.entry_ms = g_ms;
	}
	else
	{
		s_pfc_app.entry_ms = g_ms;
	}
#endif
}

void pfc_app_force_off()
{
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_app.enable_cmd = true;
	s_pfc_app.state = PFC_ST_READY;
	s_pfc_app.entry_ms = g_ms;
	s_pfc_app.vbus_ok_since_ms = g_ms;
	s_pfc_app.startup_cmd_ms = g_ms;
	return;
#else
	s_pfc_app.enable_cmd = false;
	if(s_pfc_app.state != PFC_ST_IDLE)
	{
		 pfc_state_enter(PFC_ST_IDLE);
	}
#endif
}

void pfc_app_tick_1khz(float vbus_v)
{
#if LLC_BYPASS_PFC_CONTROL
	s_pfc_bus_v = vbus_v;
	s_pfc_app.enable_cmd = true;
	if(s_pfc_app.state != PFC_ST_READY)
	{
		s_pfc_app.state = PFC_ST_READY;
		s_pfc_app.entry_ms = g_ms;
		s_pfc_app.vbus_ok_since_ms = g_ms;
		s_pfc_app.startup_cmd_ms = g_ms;
	}
	return;
#else
	s_pfc_bus_v = vbus_v;
	static uint8_t s_pfc_clear_sent = 0;  // 防抖，只发一次
	// 检查是否存在故障（硬件故障或锁存故障）
	bool fault_active = protect_fault_latched()||protect_fault_active_hw();
	//排除故障状态
	if(fault_active && s_pfc_app.state != PFC_ST_FAULT)
	{
		pfc_state_enter(PFC_ST_FAULT);
		return;
	}
		/* 任何时刻辅源棕断稳定 → 直接退回 IDLE 并关断硬件 */
	if (aux_power_brownout_stable(AUX_DROP_DEBOUNCE_MS)){
			if (s_pfc_app.state != PFC_ST_IDLE) 
				pfc_state_enter(PFC_ST_IDLE);
			return;
	}

	switch(s_pfc_app.state)
	{
		case PFC_ST_IDLE:
			 /* 必须先保证辅源已经恢复且去抖通过 */
			if (!aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS)){
				s_pfc_app.startup_cmd_ms = 0U;
				// 如果硬件已启用，则禁用硬件
				if(s_pfc_hw_enabled)
				{
					pfc_hw_set_enable(false);
					s_pfc_hw_enabled = false; // 更新状态
				}
				s_pfc_app.vbus_ok_since_ms = 0U;
				break;
			}
			/* 未使能 → 保持硬件关闭并清计时 */
			if(!s_pfc_app.enable_cmd)
			{
				if(s_pfc_hw_enabled)
				{
					pfc_hw_set_enable(false);
					s_pfc_hw_enabled = false; // 更新状态
				}
				// 重置启动命令计时器和电压稳定计时器
				s_pfc_app.startup_cmd_ms = 0U;
				s_pfc_app.vbus_ok_since_ms = 0U;
				break;
			}
			// 如果启动命令计时器未初始化，则初始化
			if(s_pfc_app.startup_cmd_ms == 0U)
			{
				s_pfc_app.startup_cmd_ms = g_ms;
			}
			//PFC_STARTUP_DELAY_MS ：启动延时，确保 PFC 硬件在启用前等待足够时间。
    /* 达到启动延时后，允许上电驱动 */
    if (elapsed_reached(s_pfc_app.startup_cmd_ms, PFC_STARTUP_DELAY_MS) && !s_pfc_hw_enabled) {
        pfc_hw_set_enable(true);
        s_pfc_hw_enabled = true;
    }
		    /* 电压达到 READY 门限后开始计稳定时间；仅在“明显回落”时才清零 */
    if (vbus_v >= PFC_VBUS_READY_V) {
        if (s_pfc_app.vbus_ok_since_ms == 0U) {
            s_pfc_app.vbus_ok_since_ms = g_ms;
        } else if (elapsed_reached(s_pfc_app.vbus_ok_since_ms, PFC_READY_DELAY_MS)) {
            pfc_state_enter(PFC_ST_READY);
        }
    } else if (vbus_v < (PFC_VBUS_READY_V - PFC_VBUS_OK_RESET_MARGIN_V)) {
        /* 只有明显回落才清零，避免轻微抖动导致频繁清零 */
        s_pfc_app.vbus_ok_since_ms = 0U;
    }
			break;
		case PFC_ST_READY:
			// 检查是否收到禁用命令
			if(!s_pfc_app.enable_cmd)
			{
				pfc_state_enter(PFC_ST_IDLE);
				break;
			}
    /* 使用更低的阈值 + 更长的掉电延时，减少抖动引起的退出 */
    if (vbus_v >= PFC_VBUS_DROPOUT_THRESHOLD_V) {
        s_pfc_app.dropout_since_ms = 0U;
    } else {
        if (s_pfc_app.dropout_since_ms == 0U) {
            s_pfc_app.dropout_since_ms = g_ms;
        } else if (elapsed_reached(s_pfc_app.dropout_since_ms, PFC_VBUS_DROPOUT_MS_NEW)) {
            pfc_state_enter(PFC_ST_IDLE);
        }
    }
			break;
		case PFC_ST_FAULT:
			
		default:
			if(!fault_active)
			{
			 /* 用户撤销使能也允许退出 FAULT */ /* 条件 1：用户撤销 enable 且 BKIN 已经释放 → 直接回 IDLE */
				if (!s_pfc_app.enable_cmd && !protect_fault_active_hw()) {
						pfc_state_enter(PFC_ST_IDLE);
						s_pfc_clear_sent = 0;
				}
				
				/* 条件 2：允许自动重试：BKIN已高 + 辅源稳定 + 达到重试延时 */
				if(!protect_fault_active_hw()&&aux_power_ok_stable_since(AUX_OK_DEBOUNCE_MS)&&(uint32_t)(g_ms - s_pfc_app.entry_ms)>= PFC_RESTART_DELAY_MS)
				{
					 if (protect_fault_latched()) 
					 {
						 protect_clear_fault();
					 }
					 if (!s_pfc_clear_sent) {
							//protect_hw_clear_pulse(10);
							s_pfc_clear_sent = 1;
           }
					pfc_state_enter(PFC_ST_IDLE);
					 s_pfc_clear_sent = 0;  // 复位节流
					 break;
				}

			}
			break;
		}
#endif
		
		
}

pfc_state_t pfc_app_state(void)
{
  return s_pfc_app.state;
}

bool pfc_app_ready(void)
{
#if LLC_BYPASS_PFC_CONTROL
        return true;
#else
	return s_pfc_app.state == PFC_ST_READY;
#endif
}

float pfc_bus_voltage(void)
{
	return s_pfc_bus_v;
}
