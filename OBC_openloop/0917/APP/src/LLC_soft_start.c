#include "main.h"
#include "LLC_soft_start.h"

#if LLC_SOFTSTART_ENABLE
/* =====================================================================
 *  LLC frequency soft-start context
 * ===================================================================*/
static llc_softstart_ctx_t s_llc_softstart;

static void llc_softstart_reset(void);
static void llc_softstart_begin(float target_hz);
static void llc_softstart_tick(void);

/* 余弦 S 曲线：0→1 */
static inline float ease_cos(float t)
{
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    return 0.5f * (1.0f - cosf(3.14f * t));
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

static void ss_apply(float freq_hz)
{
    float f = f_clampf(freq_hz, LLC_F_MIN_HZ, LLC_F_MAX_HZ);
    s_llc_softstart.last_hz = f;
    llc_pwm_set_freq((uint32_t)f);
	llc_pwm_set_duty(LLC_PWM_DUTY);
}

static void llc_softstart_reset(void)
{
	/* ========== 1. 清除内部状态标志 ========== */
		s_llc_softstart.active = false;
		s_llc_softstart.pause = false;
		s_llc_softstart.paused_elapsed_ms = 0U;
		s_llc_softstart.start_ms = 0U;
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;
	/* ========== 2. 设置初始/目标频率 ========== */
	 s_llc_softstart.start_hz = f_clampf(LLC_SOFTSTART_START_HZ, LLC_F_MIN_HZ, LLC_F_MAX_HZ);
	 s_llc_softstart.target_hz = f_clampf(LLC_SOFTSTART_TARGET_HZ, LLC_F_MIN_HZ, LLC_F_MAX_HZ);


	/* ========== 3. 计算安全占空窗口 ========== */
		ss_apply(s_llc_softstart.start_hz);

}

/* Start (or restart) a soft-start ramp to target_duty_0_1.
 * 对应 MATLAB 中 start_cmd != 0 的分支。
 */
static void llc_softstart_begin(float target_hz)
{
	 /* ========== 1. 参数初始化 ========== */
		s_llc_softstart.duration_ms = LLC_SOFTSTART_DURATION_MS;  //软启动的总持续时间（毫秒）
		/* 起始与目标频率安全钳位 */
		float start_hz = f_clampf(LLC_SOFTSTART_START_HZ, LLC_F_MIN_HZ, LLC_F_MAX_HZ);
		float final_hz = f_clampf(target_hz, LLC_F_MIN_HZ, LLC_F_MAX_HZ);
	
		/* ========== 2. 清状态标志 ========== */
		s_llc_softstart.pause      = false;  //标志位，指示是否暂停软启动,这边是不暂停软启动
		s_llc_softstart.paused_elapsed_ms = 0U;
	
		s_llc_softstart.start_hz = start_hz;
		s_llc_softstart.target_hz = final_hz;
		/* ========== 3. 判定是否跳过软启动 ========== */
		if ((s_llc_softstart.duration_ms == 0U) || (start_hz == final_hz)) {
        s_llc_softstart.active = false; //标志位，指示软启动是否正在进行,这边是未在进行
        ss_apply(final_hz);
    } else {
        s_llc_softstart.active = true; 
        s_llc_softstart.start_ms = g_ms;
        ss_apply(start_hz);
    }


}
/* 在软启动过程中更新最新的频率防止越界。*/
void llc_softstart_update_target(float new_target_hz)
{	s_llc_softstart.target_hz = f_clampf(new_target_hz, LLC_F_MIN_HZ, LLC_F_MAX_HZ);

}

void llc_softstart_init(void)
{
    if (!s_llc_softstart.initialized) {
        llc_softstart_reset();
        s_llc_softstart.initialized = true;
    }
}

void llc_softstart_start(float target_hz)
{
    if (!s_llc_softstart.initialized) {
        llc_softstart_init();
    }

    llc_softstart_begin(target_hz);
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
* 函数功能：
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2025年12月24
* 注    意：
*********************************************************************************************************/

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
    ss_apply(f_clampf(LLC_SOFTSTART_FAILSAFE_HZ, LLC_F_MIN_HZ, LLC_F_MAX_HZ)); //0.0
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

	 if (s_llc_softstart.pause) {
        ss_apply(s_llc_softstart.last_hz);
        return;
    }

	uint32_t elapsed = (uint32_t)(g_ms - s_llc_softstart.start_ms);

    if (elapsed >= s_llc_softstart.duration_ms) {
        s_llc_softstart.active = false;
        ss_apply(s_llc_softstart.target_hz);
        return;
    }
	float progress = (s_llc_softstart.duration_ms > 0U) ? ((float)elapsed / (float)s_llc_softstart.duration_ms):1.0f;

	#if defined(LLC_SOFTSTART_USE_COSINE_EASE) && (LLC_SOFTSTART_USE_COSINE_EASE)
    float k = ease_cos(progress);
	#else
    float k = ease_exp(progress, LLC_SOFTSTART_EXP_K);
	#endif
    float hz = s_llc_softstart.start_hz +(s_llc_softstart.target_hz - s_llc_softstart.start_hz) * k;
    ss_apply(hz);

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
        
}
static void llc_softstart_begin(float target_hz)
{
           
}
static void llc_softstart_tick(void)
{
}
#endif /* LLC_SOFTSTART_ENABLE */
