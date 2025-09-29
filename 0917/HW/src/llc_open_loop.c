/*********************************************************************************************************
* 模块名称：llc_open_loop.c
* 摘    要：开环模式
* 当前版本：1.0.0
* 作    者：
* 完成日期：2025年09月25日 
* 内    容：
* 注    意：                                                                  
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
//定义一个宏，用于重置控制器的保持状态
//holding ：标记是否处于保持状态 
//hold_elapsed_ms ：记录保持状态的持续时间（毫秒）。
#define RESET_HOLD_STATE(ctrl) do { \
	(ctrl)->holding = false; \
	(ctrl)->hold_elapsed_ms = 0U; \
} while (0)
 

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
		RESET_HOLD_STATE(ctrl); //重置保存状态
		ctrl->running = false; //停止运行
		//ctrl->holding = false;
		ctrl->current_index = ctrl->segment_count - 1U;//设置当前索引为最后一个分段
		//ctrl->hold_elapsed_ms = 0U;
		//ctrl->f_cmd = ctrl->segments[ctrl->current_index].stop_hz;
		const llc_open_loop_segment_t *current_segment = &ctrl->segments[ctrl->current_index]; 
		ctrl->f_cmd = current_segment->stop_hz; //设置目标频率为最后一个分段的停止频率（ stop_hz ）。
		return false;
	}
	ctrl->current_index++; //增加当前索引
	//ctrl->holding = false;
	//ctrl->hold_elapsed_ms = 0U;
	RESET_HOLD_STATE(ctrl);
	//ctrl->f_cmd = ctrl->segments[ctrl->current_index].start_hz;
	const llc_open_loop_segment_t *current_segment = &ctrl->segments[ctrl->current_index]; 
	ctrl->f_cmd = current_segment->start_hz; //设置目标频率为下一个分段的起始频率（ start_hz ）。
	return true;
}
/*********************************************************************************************************
* 函数名称：llc_open_loop_init
* 函数功能：检查控制器指针是否为空,如果为空则返回，否则将控制器的分段数组指针和分段数存储在控制器中，并将当前索引设置为0，目标频率设置为第一个分段的起始频率，并将运行标志设置为false。
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2025年09月25日
* 注    意：
*********************************************************************************************************/
void llc_open_loop_init(llc_open_loop_ctrl_t *ctrl,const llc_open_loop_segment_t *segments,size_t segment_count)
{
	if (!ctrl) {
			return;
	}
	//设置分段数据和分段数,重置当前索引、目标频率、保持状态和运行状态。
	RESET_HOLD_STATE(ctrl);
	ctrl->segments = segments;
	ctrl->segment_count = segment_count;
	ctrl->current_index = 0U;
	ctrl->f_cmd = (segments && segment_count) ? segments[0].start_hz : 0.0f;
	//ctrl->hold_elapsed_ms = 0U;
	ctrl->running = false;
	//ctrl->holding = false;
}

void llc_open_loop_start(llc_open_loop_ctrl_t *ctrl)
{
	if(!ctrl || ctrl->segment_count == 0U) {
			return;
	}
	RESET_HOLD_STATE(ctrl);
	ctrl->current_index = 0U;
	ctrl->f_cmd = ctrl->segments[0].start_hz;
	//ctrl->hold_elapsed_ms = 0U;
	ctrl->running = true;
	//ctrl->holding = false;
}

void llc_open_loop_stop(llc_open_loop_ctrl_t *ctrl)
{
	if(!ctrl) {
			return;
	}
	RESET_HOLD_STATE(ctrl);
	ctrl->running = false;
	//ctrl->holding = false;
	//ctrl->hold_elapsed_ms = 0U;
	if (ctrl->segment_count) {
			ctrl->current_index = 0U;
			ctrl->f_cmd = ctrl->segments[0].start_hz;
	} else {
			ctrl->current_index = 0U;
			ctrl->f_cmd = 0.0f;
	}
}


bool llc_open_loop_running(const llc_open_loop_ctrl_t *ctrl)
{
    return ctrl ? ctrl->running : false;
}


void llc_open_loop_tick(llc_open_loop_ctrl_t *ctrl) 
{
	//检查控制器是否有效、正在运行且有分段
	if (!ctrl || !ctrl->running || ctrl->segment_count == 0U) {
			return;
	}
	//循环处理当前分段
	while (ctrl->running && ctrl->segment_count) {
		const llc_open_loop_segment_t *seg = &ctrl->segments[ctrl->current_index];
		
		if(ctrl->holding)
		{
			if(seg->hold_time_ms == 0||ctrl->hold_elapsed_ms >= seg->hold_time_ms)
			{
				if(!llc_open_loop_advance(ctrl))
				{
					return;
				}
				continue;
			}
			ctrl->hold_elapsed_ms++;
			return;
		}
		//计算目标频率与当前频率的差值（ diff ）和步进值（ step ）
		float diff = seg->stop_hz - ctrl->f_cmd;
    float step = seg->slew_hz_per_ms;
		//如果步进值无效（<=0），直接跳到目标频率并进入保持状态。
		if (step <= 0.0f) {
			ctrl->f_cmd = seg->stop_hz;
			ctrl->holding = true;
			ctrl->hold_elapsed_ms = 0U;
			if (seg->hold_time_ms == 0U) {
					if (!llc_open_loop_advance(ctrl)) {
							return;
					}
					continue;
				}
				return;
			}

		if (diff > 0.0f) {
				if (diff <= step) {
						ctrl->f_cmd = seg->stop_hz;
						ctrl->holding = true;
						ctrl->hold_elapsed_ms = 0U;
						if (seg->hold_time_ms == 0U) {
								if (!llc_open_loop_advance(ctrl)) {
										return;
								}
								continue;
						}
				} else {
						ctrl->f_cmd += step;
				}
				return;
		}
		//默认进入保持状态，并根据是否需要保持时间决定是否前进。
		ctrl->holding = true;
		ctrl->hold_elapsed_ms = 0U;
		if(seg->hold_time_ms == 0)
		{
			if(!llc_open_loop_advance(ctrl))
			{
				return;
			}
			continue;
		}
		return;
	}
}