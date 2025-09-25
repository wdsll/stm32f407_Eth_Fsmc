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

static bool llc_open_loop_advance(llc_open_loop_ctrl_t *ctrl)
{
	if(!ctrl || ctrl->segment_count == 0)
	{
		return false;
	}
	if (ctrl->current_index + 1U >= ctrl->segment_count) 
	{	
		ctrl->running = false;
		ctrl->holding = false;
		ctrl->current_index = ctrl->segment_count - 1U;
		ctrl->hold_elapsed_ms = 0U;
		ctrl->f_cmd = ctrl->segments[ctrl->current_index].stop_hz;
		return false;
	}
	ctrl->current_index++;
	ctrl->holding = false;
	ctrl->hold_elapsed_ms = 0U;
	ctrl->f_cmd = ctrl->segments[ctrl->current_index].start_hz;
	return true;
}
void llc_open_loop_init(llc_open_loop_ctrl_t *ctrl,const llc_open_loop_segment_t *segments,size_t segment_count)
{
	if (!ctrl) {
			return;
	}

	ctrl->segments = segments;
	ctrl->segment_count = segment_count;
	ctrl->current_index = 0U;
	ctrl->f_cmd = (segments && segment_count) ? segments[0].start_hz : 0.0f;
	ctrl->hold_elapsed_ms = 0U;
	ctrl->running = false;
	ctrl->holding = false;
}

void llc_open_loop_start(llc_open_loop_ctrl_t *ctrl)
{
	if(!ctrl || ctrl->segment_count == 0U) {
			return;
	}
	
	ctrl->current_index = 0U;
	ctrl->f_cmd = ctrl->segments[0].start_hz;
	ctrl->hold_elapsed_ms = 0U;
	ctrl->running = true;
	ctrl->holding = false;
}

void llc_open_loop_stop(llc_open_loop_ctrl_t *ctrl)
{
	if(!ctrl) {
			return;
	}
	ctrl->running = false;
	ctrl->holding = false;
	ctrl->hold_elapsed_ms = 0U;
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
	if (!ctrl || !ctrl->running || ctrl->segment_count == 0U) {
			return;
	}
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
		float diff = seg->stop_hz - ctrl->f_cmd;
    float step = seg->slew_hz_per_ms;
		
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