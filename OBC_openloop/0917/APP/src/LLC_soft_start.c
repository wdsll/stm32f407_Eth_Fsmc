#include "main.h"
#include "LLC_soft_start.h"

#if LLC_SOFTSTART_ENABLE
/* =====================================================================
 * LLC PWM soft-start context
 * ===================================================================*/
static llc_softstart_ctx_t s_llc_softstart;

static void llc_softstart_reset(void);
static void llc_softstart_begin(float target_duty);
static void llc_softstart_tick(void);

/* 根据当前周期/死区，计算“有效占空安全窗” */
/* Re-compute the "safe" duty window according to PWM period & deadtime.
 * 对应 MATLAB 里的 update_safe_window(per_ns, dt_ns, extra_margin)
 */
static void ss_update_safe_window(void)
{
	  uint32_t per_ns = llc_pwm_get_period_ns();  //获取 PWM 周期时间（单位为纳秒）
    uint32_t dt_ns  = llc_pwm_get_deadtime_ns(); //获取 PWM 死区时间（单位为纳秒）
	  float guard = 0.0f;
    if (per_ns > 0U && dt_ns <= (UINT32_MAX / 2)) {
        /* 互补两沿都插死区：保守取 2*deadtime */
        guard = (2.0f * (float)dt_ns) / (float)per_ns;  // 0~1
    }
		else
		{
			guard = 0.0f;
		}

		guard += LLC_SOFTSTART_EXTRA_MARGIN; //  其中guard为死区时间保护带加上额外经验余量。

    /* 下限不超过 0.49，上限不低于 0.51，避免靠近 50% 附近偶发直通/无效脉宽 */
    s_llc_softstart.duty_min_safe = f_clampf(guard, 0.0f, 0.49f);
    s_llc_softstart.duty_max_safe = f_clampf(1.0f - guard,  0.51f, 0.99f);
}

/* 余弦 S 曲线：0→1 */
static inline float ease_cos(float t)
{
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    return 0.5f * (1.0f - cosf(3.1415926535f * t));
}

static void ss_apply(float duty)
{
    float d = f_clampf(duty, s_llc_softstart.duty_min_safe, s_llc_softstart.duty_max_safe);
		s_llc_softstart.last_duty = d;
    llc_pwm_set_duty(d);
}


/* 指数曲线：0→1 */
static inline float ease_exp(float t, float k)
{
    if (t <= 0.f) 
			return 0.f;
    if (t >= 1.f) 
			return 1.f;
    float denom = 1.0f - expf(-k);
    if (denom < 1e-6f) 
			return t;
    return (1.0f - expf(-k * t)) / denom;
}

/* =====================================================================
 * Public API
 * ===================================================================*/
/* Reset context to a safe, idle state.
 * 对应 MATLAB 第一次进入时 persistent 的初始化。
 */
static void llc_softstart_reset(void)
{
	/* ========== 1. 清除内部状态标志 ========== */
		s_llc_softstart.active = false;
		s_llc_softstart.pause = false;
		s_llc_softstart.paused_elapsed_ms = 0U;
		s_llc_softstart.start_ms = 0U;
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;
	/* ========== 2. 设置初始/目标占空比 ========== */
		s_llc_softstart.start_duty = f_clampf(LLC_SOFTSTART_START_DUTY, 0.0f, 0.99f);  // 0.1

		s_llc_softstart.target_duty =  f_clampf(LLC_SOFTSTART_TARGET_DUTY, 0.0f, 0.99f);  //0.5
	
	/* 根据当前周期/死区，计算“有效占空安全窗” */
	/* ========== 3. 计算安全占空窗口 ========== */
		ss_update_safe_window(); //更新了 s_llc_softstart.duty_min_safe和s_llc_softstart.duty_max_safe
	
	/* 确保初始占空在安全范围内 */
	    if (s_llc_softstart.start_duty < s_llc_softstart.duty_min_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_min_safe;
    else if (s_llc_softstart.start_duty > s_llc_softstart.duty_max_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_max_safe;
	
	/* ========== 4. 应用初始占空比 ========== */
		ss_apply(s_llc_softstart.start_duty); //0.1
		s_llc_softstart.last_duty = s_llc_softstart.start_duty;
	/* ========== 5. 调试日志（可选） ========== */
#if defined(DEBUG_PRINTF_LLCSOFTSTART)
    debug_printf("[LLC-OpenLoop-SS] reset: start=%.3f target=%.3f dur=%lu ms safe[%.3f, %.3f]\n",
                 s_llc_softstart.start_duty,
                 s_llc_softstart.target_duty,
                 (unsigned long)s_llc_softstart.duration_ms,
                 s_llc_softstart.duty_min_safe,
                 s_llc_softstart.duty_max_safe);
#endif
}

/* Start (or restart) a soft-start ramp to target_duty_0_1.
 * 对应 MATLAB 中 start_cmd != 0 的分支。
 */
static void llc_softstart_begin(float target_duty)
{
	 /* ========== 1. 参数初始化 ========== */
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;  //软启动的总持续时间（毫秒）
		/* 起始与目标占空比安全钳位 */
		float start_duty = f_clampf(LLC_SOFTSTART_START_DUTY, 0.0f, 0.99f);
		float final_duty = f_clampf(target_duty, 0.0f, 0.99f);
	
		/* ========== 2. 清状态标志 ========== */
		s_llc_softstart.pause      = false;  //标志位，指示是否暂停软启动,这边是不暂停软启动
		s_llc_softstart.paused_elapsed_ms = 0U;
	
		s_llc_softstart.start_duty = start_duty;
		s_llc_softstart.target_duty = final_duty;
	/* ========== 3. 计算安全占空窗 ========== */
		ss_update_safe_window();
	/* 强制占空比落入安全范围 */
	  if (s_llc_softstart.start_duty < s_llc_softstart.duty_min_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_min_safe;
    else if (s_llc_softstart.start_duty > s_llc_softstart.duty_max_safe)
        s_llc_softstart.start_duty = s_llc_softstart.duty_max_safe;

    if (s_llc_softstart.target_duty < s_llc_softstart.duty_min_safe)
        s_llc_softstart.target_duty = s_llc_softstart.duty_min_safe;
    else if (s_llc_softstart.target_duty > s_llc_softstart.duty_max_safe)
        s_llc_softstart.target_duty = s_llc_softstart.duty_max_safe;
	
	/* ========== 4. 判定是否跳过软启动 ========== */
		//如果目标占空比 target_duty 小于或等于初始占空比 start_duty ，或者软启动时间为 0，则直接跳过软启动：
		if(final_duty <= start_duty || s_llc_softstart.duration_ms == 0U)
		{
			s_llc_softstart.active = false; //标志位，指示软启动是否正在进行,这边是未在进行
			s_llc_softstart.start_duty = final_duty;
			s_llc_softstart.target_duty = final_duty;
			ss_apply(final_duty);
			s_llc_softstart.last_duty    = final_duty;
#if defined(DEBUG_PRINTF_LLCSOFTSTART)
        debug_printf("[LLC-SS] skipped: fixed duty=%.3f (no ramp)\n", final_duty);
#endif
		}
		else
		{
			/* 需要 ramp：从 start_duty 缓升到 target_duty */
			s_llc_softstart.active = true; //软启动正在进行
			s_llc_softstart.start_ms = g_ms; //记录当前时间戳 start_ms 
			ss_apply(start_duty);
			s_llc_softstart.last_duty = start_duty;
#if defined(DEBUG_PRINTF_LLCSOFTSTART) && (DEBUG_PRINTF_LLCSOFTSTART)
    debug_printf("[LLC-SS] begin: start=%.3f, target=%.3f, dur=%lu ms, safe[%.3f, %.3f], mode=%s\n",
                 s_llc_softstart.start_duty,
                 s_llc_softstart.target_duty,
                 (unsigned long)s_llc_softstart.duration_ms,
                 s_llc_softstart.duty_min_safe,
                 s_llc_softstart.duty_max_safe,
#if defined(LLC_SOFTSTART_USE_COSINE_EASE) && (LLC_SOFTSTART_USE_COSINE_EASE)
                 "cosine");
#else
                 "exp");
#endif
#endif
		}
}
/* 在软启动过程中更新最终占空比目标。*/
void llc_softstart_update_target(float new_target_0_1)
{
    s_llc_softstart.target_duty = f_clampf(new_target_0_1, 0.0f, 0.99f);
}

void llc_softstart_init(void)
{
    if (!s_llc_softstart.initialized) {
        llc_softstart_reset();
        s_llc_softstart.initialized = true;
        s_llc_softstart.initialized = true;
    }
}

void llc_softstart_start(float target_duty_0_1)
{
    if (!s_llc_softstart.initialized) {
        llc_softstart_init();
    }

    llc_softstart_begin(target_duty_0_1);
}


void llc_softstart_on_fault(void)
{
    if (!s_llc_softstart.initialized) {
        return;
    }

    llc_softstart_abort();
}
/*********************************************************************************************************
* 函数名称：llc_softstart_set_pause
* 函数功能：这段代码的主要功能是控制软启动（soft start）的暂停和恢复逻辑。
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年10月20
* 注    意：代码通过设置 pause 标志来控制软启动过程的暂停和恢复。
			暂停逻辑：当 pause 为 true 时，记录当前软启动已经运行的时间（ elapsed ），并将其保存到 paused_elapsed_ms 中，同时标记软启动为暂停状态。
			恢复逻辑：当 pause 为 false 时，根据之前保存的暂停时间（ paused_elapsed_ms ）重新计算软启动的起始时间（ start_ms ），并恢复软启动过程。
*********************************************************************************************************/
/* Pause / resume request.
 * 对应 MATLAB 中 pause_req != 0 时保持 last_duty，不推进时间。
 */
void llc_softstart_set_pause(bool pause)
{
    if(pause)
		{
			if (!s_llc_softstart.pause && s_llc_softstart.active)
			{
				//如果软启动未暂停且处于激活状态，计算从开始到当前的时间差
				uint32_t elapsed = (uint32_t)(g_ms - s_llc_softstart.start_ms);
				
				if (elapsed > s_llc_softstart.duration_ms)
				{
					elapsed = s_llc_softstart.duration_ms; //确保 elapsed 不超过总持续时间 duration_ms 。
				}
				s_llc_softstart.paused_elapsed_ms = elapsed; //保存 elapsed 到 paused_elapsed_ms ，并标记为暂停状态
			}
			s_llc_softstart.pause = true;
		}
		else
		{
			 if (s_llc_softstart.pause && s_llc_softstart.active)
			 {
				 //如果软启动处于暂停状态且激活，读取之前保存的 paused_elapsed_ms
				 uint32_t elapsed = s_llc_softstart.paused_elapsed_ms;  //paused_elapsed_ms ：记录暂停时已经运行的时间
					if (elapsed > s_llc_softstart.duration_ms)
					{
						//确保 elapsed 不超过总持续时间 duration_ms 。 当前软启动已经运行的时间（ elapsed ）
							elapsed = s_llc_softstart.duration_ms;
					}
					if (g_ms >= elapsed)
					{
						//根据当前时间 g_ms 和 elapsed 重新计算 start_ms ，确保时间逻辑正确。
							s_llc_softstart.start_ms = g_ms - elapsed;
					}
					else
					{
							s_llc_softstart.start_ms = 0U;
					}
			 }
			 s_llc_softstart.pause = false;
		}
}
/* 异常 / 保护触发时的终止：对应 MATLAB 中 fault != 0 的分支。 */
void llc_softstart_abort(void)
{
    s_llc_softstart.active = false;
    s_llc_softstart.pause = false;
		s_llc_softstart.paused_elapsed_ms = 0U;
    ss_update_safe_window(); // 以防期间改过频率/死区 卡的是周期和死区时间吧
    ss_apply(f_clampf(LLC_SOFTSTART_FAILSAFE_DUTY, 0.0f, 0.99f)); //0.0
}
/* 
*周期性调用（例如 1ms 调一次），驱动 S 曲线软启动。
 * 对应 MATLAB 主体里推进 elapsed_ms / 计算 progress 的部分。
 */
static void llc_softstart_tick(void)
{
    if (!s_llc_softstart.active) 
		{
			return;
		}
		
    /* 用你项目已有的故障判据 */
    if (protect_fault_latched() || protect_fault_active_hw()) {
        llc_softstart_abort();
        return;
    }
		 /* 更新安全窗口（频率变化时死区可能变化） */
		ss_update_safe_window();
		/* 若暂停，则保持当前占空比不变 */
		if (s_llc_softstart.pause) 
		{
			ss_apply(s_llc_softstart.last_duty);
			return;
		}
		 /* 计算已运行时间 */
		uint32_t elapsed = (uint32_t)(g_ms - s_llc_softstart.start_ms);
		
		 /*     → 锁定目标占空 */
		if(elapsed >= s_llc_softstart.duration_ms)  // 软启动持续时间（毫秒）
		{
			s_llc_softstart.active = false;
			ss_apply(s_llc_softstart.target_duty);
			s_llc_softstart.last_duty = s_llc_softstart.target_duty;
#if defined(DEBUG_PRINTF_LLCSOFTSTART) 
        debug_printf("[LLC-SS] done: duty=%.3f after %lu ms\n",
                     s_llc_softstart.target_duty, (unsigned long)elapsed);
#endif
			return;
		}
		 /* 计算进度 (0.0~1.0) */
		float progress = (s_llc_softstart.duration_ms > 0U) ? ((float)elapsed / (float)s_llc_softstart.duration_ms) : 1.0f;

		
		 /* 将线性换成 S 曲线（如需线性，把 ease 改成 progress） */
#if defined(LLC_SOFTSTART_USE_COSINE_EASE) && (LLC_SOFTSTART_USE_COSINE_EASE)
    float k = ease_cos(progress);
#else
    float k = ease_exp(progress, LLC_SOFTSTART_EXP_K);
#endif
		
		//使用线性插值公式计算当前占空比
		float duty = s_llc_softstart.start_duty +
								 (s_llc_softstart.target_duty - s_llc_softstart.start_duty) * k;
		duty = f_clampf(duty, s_llc_softstart.duty_min_safe, s_llc_softstart.duty_max_safe);
		ss_apply(duty);
		s_llc_softstart.last_duty = duty;
}

void llc_softstart_tick_1khz(void)
{
    if (!s_llc_softstart.initialized) {
        llc_softstart_init();
    }

    llc_softstart_tick();
}
#else

static void llc_softstart_reset(void)
{
                //llc_pwm_set_duty(f_clampf(LLC_PWM_DUTY, 0.0f, 0.99f));
}
static void llc_softstart_begin(float target_duty)
{
                //llc_pwm_set_duty(f_clampf(target_duty, 0.0f, 0.99f));
}
static void llc_softstart_tick(void)
{
                /* Soft-start disabled. Nothing to do. */
}
#endif /* LLC_SOFTSTART_ENABLE */
