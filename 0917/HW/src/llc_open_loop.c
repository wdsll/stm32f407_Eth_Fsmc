/*********************************************************************************************************
* 模块名称：llc_open_loop.c
* 摘    要：开环模式
* 当前版本：1.0.0
* 作    者：
* 完成日期：2025年09月25日 
* 内    容：实现LLC开环控制逻辑
* 注    意：确保输入参数有效性，避免空指针访问                                                                  
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "llc_open_loop.h"
#include <math.h>
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/


/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/



/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/
//定义一个宏，用于重置控制器的保持状态 该宏的目的是将控制结构体中的“保持状态”标志位和计时器清零，表示当前不再处于保持状态
//holding ：标记是否处于保持状态 
//hold_elapsed_ms ：记录保持状态的持续时间（毫秒）。
#define RESET_HOLD_STATE(ctrl) do { \
	(ctrl)->holding = false; \
	(ctrl)->hold_elapsed_ms = 0U; \
} while (0)
 
/*********************************************************************************************************
* 函数名称：llc_open_loop_advance
* 函数功能：
* 输入参数：llc_open_loop_ctrl_t *ctrl：指向 LLC 开环控制器的结构体指针，包含分段信息、当前索引、运行状态等。
* 输出参数：
* 返 回 值：bool ：返回 true 表示成功推进到下一个分段；返回 false 表示无法推进（如分段数为 0 或已到达最后一个分段）。
* 创建日期：2025年09月30日
* 注    意：
*********************************************************************************************************/
//检查输入参数有效性，若控制器指针为空或分段数为0，返回 false 
static bool llc_open_loop_advance(llc_open_loop_ctrl_t *ctrl) 
{
	if(!ctrl || ctrl->segment_count == 0)
	{
		return false;
	}
	//if (ctrl->segment_count == 0 || ctrl->current_index + 1U >= ctrl->segment_count)
	if (ctrl->current_index + 1U >= ctrl->segment_count)  //如果当前分段是最后一个分段
	{	
		ctrl->running = false; //停止运行
		ctrl->holding = false;
		ctrl->current_index = ctrl->segment_count - 1U;//设置当前索引为最后一个分段
		ctrl->hold_elapsed_ms = 0U;
		//ctrl->f_cmd = ctrl->segments[ctrl->current_index].stop_hz;
		const llc_open_loop_segment_t *current_segment = &ctrl->segments[ctrl->current_index]; 
		ctrl->f_cmd = current_segment->stop_hz; //设置目标频率为最后一个分段的停止频率（ stop_hz ）。
		return false;
	}
	ctrl->current_index++; //增加当前索引
	ctrl->holding = false;
	ctrl->hold_elapsed_ms = 0U;
	//RESET_HOLD_STATE(ctrl);
	//ctrl->f_cmd = ctrl->segments[ctrl->current_index].start_hz;
	const llc_open_loop_segment_t *current_segment = &ctrl->segments[ctrl->current_index]; 
	ctrl->f_cmd = current_segment->start_hz; //设置目标频率为下一个分段的起始频率（ start_hz ）。
	return true;
}
/*********************************************************************************************************
* 函数名称：llc_open_loop_init
* 函数功能：该函数用于初始化一个开环控制器的状态。开环控制器通常用于不需要反馈信号的系统，通过预设的分段数据来控制输出频率的变化。
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2025年09月30日
* 注    意：
*********************************************************************************************************/
void llc_open_loop_init(llc_open_loop_ctrl_t *ctrl,const llc_open_loop_segment_t *segments,size_t segment_count)
{
	if (!ctrl || !segments) {
			return;
	}
	//设置分段数据和分段数,重置当前索引、目标频率、保持状态和运行状态。
	//RESET_HOLD_STATE(ctrl); //指向开环控制器结构的指针，用于存储和更新控制器的状态。
	ctrl->segments = segments; //指向分段数据数组的指针，每个分段包含起始频率（ start_hz ）等信息。
	ctrl->segment_count = segment_count; //分段数据的数量。
	ctrl->current_index = 0U;
	//计算初始目标频率 f_cmd ：如果分段数据有效（ segments 和 segment_count 均非零），则使用第一个分段的起始频率；否则设为 0.0f 。
	ctrl->f_cmd = (segments && segment_count) ? segments[0].start_hz : 0.0f;
	ctrl->hold_elapsed_ms = 0U;  //记录保持状态的持续时间（毫秒）；
	ctrl->running = false; //表示控制器尚未启动。
	ctrl->holding = false; //标记是不处于保持状态 
}
/*********************************************************************************************************
* 函数名称：llc_open_loop_start
* 函数功能：该函数用于初始化控制器的状态，并启动开环控制的运行
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2025年09月30日
* 注    意：通过重置状态和设置初始值，确保控制器从一个已知的、一致的状态开始运行
*********************************************************************************************************/
void llc_open_loop_start(llc_open_loop_ctrl_t *ctrl)
{
	if(!ctrl || ctrl->segment_count == 0U) {
			return;
	}
	//RESET_HOLD_STATE(ctrl);
	ctrl->current_index = 0U;  //表示从第一个段（segment）开始执行。
	ctrl->f_cmd = ctrl->segments[0].start_hz;
	ctrl->hold_elapsed_ms = 0U;
	ctrl->running = true;  //表示控制器已启动并运行。
	ctrl->holding = false; //还没进入持续状态
}
/*********************************************************************************************************
* 函数名称：llc_open_loop_stop
* 函数功能：用于停止 LLC（谐振变换器）开环控制的函数。它的主要职责是重置控制器的状态，确保在停止操作时，控制器恢复到初始或安全状态。
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2025年09月30日
* 注    意：通常在需要中断 LLC 开环控制时调用此函数，例如在系统关机、故障处理或手动停止操作时。
*********************************************************************************************************/
void llc_open_loop_stop(llc_open_loop_ctrl_t *ctrl)
{
	if(!ctrl) {
			return;
	}
	//RESET_HOLD_STATE(ctrl);
	ctrl->running = false;	//表示控制器已停止运行。
	ctrl->holding = false;
	ctrl->hold_elapsed_ms = 0U;
	if (ctrl->segment_count&&ctrl->segments)  //如果控制器有分段控制，则将目标频率设置为第一个分段的起始频率。
	{
			ctrl->current_index = 0U;
			ctrl->f_cmd = ctrl->segments[0].start_hz;
	} else {
		//如果没有分段控制，则将 current_index 重置为 0 ，并将 f_cmd 设置为 0.0f （即停止输出频率）
			ctrl->current_index = 0U;
			ctrl->f_cmd = 0.0f;
	}
}

/*********************************************************************************************************
* 函数名称：llc_open_loop_running
* 函数功能：用于检查 llc_open_loop_ctrl_t 类型的控制器是否正在运行
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2025年09月30日
* 注    意：该函数的主要目的是提供一种安全的方式来检查控制器的运行状态。通过检查指针是否为空，避免了直接访问空指针可能导致的程序崩溃
*********************************************************************************************************/
bool llc_open_loop_running(const llc_open_loop_ctrl_t *ctrl)
{
    return ctrl ? ctrl->running : false;
}

//static inline float f_absf(float x){ return x < 0 ? -x : x; }
/*********************************************************************************************************
* 函数名称：llc_open_loop_tick
* 函数功能：它的核心功能是根据预设的分段参数（如频率、斜率、保持时间等）动态调整输出频率（ f_cmd ），并在必要时进入保持状态或切换到下一个分段。
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2025年09月30日
* 注    意：
*********************************************************************************************************/
void llc_open_loop_tick(llc_open_loop_ctrl_t *ctrl) 
{
	//检查控制器是否有效、正在运行且有分段 如果任一条件不满足，函数直接返回。
	if (!ctrl || !ctrl->running || ctrl->segment_count == 0U) {
			return;
	}
	//条件是控制器仍在运行且还有分段需要处理
	while (ctrl->running && ctrl->segment_count > 0) {
		//获取当前分段（ seg ）的信息
		const llc_open_loop_segment_t *seg = &ctrl->segments[ctrl->current_index];
		
		if(ctrl->holding)
		{
			//如果保持时间为 0 或已超过保持时间，调用 llc_open_loop_advance 切换到下一个分段。
			if(seg->hold_time_ms == 0||ctrl->hold_elapsed_ms >= seg->hold_time_ms)
			{
				if(!llc_open_loop_advance(ctrl))  //表示未能成功推进到下一个分段
				{
					return;
				}
				continue;
			}
			//否则，增加保持时间计数器（ hold_elapsed_ms ）并返回。
			ctrl->hold_elapsed_ms++;
			return;
		}
		//计算目标频率与当前频率的差值（ diff ）和步进值（ step ）
		float diff = seg->stop_hz - ctrl->f_cmd;
    float step = fabsf(seg->slew_hz_per_ms);
		//如果步进值无效（<=0），直接跳到目标频率并进入保持状态。
		if (step <= 0.0f) {
			ctrl->f_cmd = seg->stop_hz; //设置目标频率为分段的停止频率
			ctrl->holding = true; //保持状态标志
			ctrl->hold_elapsed_ms = 0U; //重置保持时间计数器
			if (seg->hold_time_ms == 0U)  //保持时间（ms）
			{
					if (!llc_open_loop_advance(ctrl))
					{
							return;
					}
					continue;
				}
				return;
			}
		//如果差值 diff 为正：
		if (diff > 0.0f) {
			//如果差值小于等于步进值，直接设置频率为目标值并进入保持状态。
				if (fabsf(diff - step) < 1e-6f || diff <= step) {
						ctrl->f_cmd = seg->stop_hz; //设置目标频率为分段的停止频率
						ctrl->holding = true; //保持状态标志
						ctrl->hold_elapsed_ms = 0U;
						if (seg->hold_time_ms == 0U)  //保持时间（ms）
							{
								if (!llc_open_loop_advance(ctrl)) {
										return;
								}
								continue;
						}
				} else {
					//否则，按步进值增加当前频率
						ctrl->f_cmd += step;
				}
				return;
		}
		else if(diff < 0.0f)  //当频率差值小于0时，根据差值的绝对值与步长的关系进行不同的处理
		{
			float abs_diff = fabsf(diff);
			if (abs_diff <= step) 
			{
				ctrl->f_cmd = seg->stop_hz;
				ctrl->holding = true;
				ctrl->hold_elapsed_ms = 0U;
				if (seg->hold_time_ms == 0U) 
				{
					if (!llc_open_loop_advance(ctrl)) 
					{
						return;
					}
					continue;
				}
			}
			else 
			{
				ctrl->f_cmd -= step;
			}
			return;	
		}
		//默认进入保持状态，并根据是否需要保持时间决定是否前进。
		ctrl->holding = true; //保持状态标志
		ctrl->hold_elapsed_ms = 0U; //重置保持时间计数器
		if(seg->hold_time_ms == 0) //根据保持时间决定是否切换到下一个分段。
		{
			if(!llc_open_loop_advance(ctrl)) //	如果切换失败，则返回 
			{
				return;
			}
			continue;
		}
		return;
	}
}