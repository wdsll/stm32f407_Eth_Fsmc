/*********************************************************************************************************
* 模块名称：llc_open_loop.h
* 摘    要：开环模式
* 当前版本：1.0.0
* 作    者：Rengar
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
#ifndef _LLC_OPEN_LOOP_H_
#define _LLC_OPEN_LOOP_H_

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/*********************************************************************************************************
*                                              枚举结构体
*********************************************************************************************************/
typedef struct
{
	float start_hz;
	float stop_hz;
	float slew_hz_per_ms;
	uint32_t hold_time_ms;
}llc_open_loop_segment_t;

typedef struct {
    const llc_open_loop_segment_t *segments;
    size_t segment_count;
    size_t current_index;
    float f_cmd;
    uint32_t hold_elapsed_ms;
    bool running;
    bool holding;
} llc_open_loop_ctrl_t;

/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/

void llc_open_loop_init(llc_open_loop_ctrl_t *ctrl,const llc_open_loop_segment_t *segments,size_t segment_count);

void llc_open_loop_start(llc_open_loop_ctrl_t *ctrl);

void llc_open_loop_stop(llc_open_loop_ctrl_t *ctrl);

void llc_open_loop_tick(llc_open_loop_ctrl_t *ctrl);

bool llc_open_loop_running(const llc_open_loop_ctrl_t *ctrl);

static inline float llc_open_loop_get_freq(const llc_open_loop_ctrl_t *ctrl)
{
	 return ctrl ? ctrl->f_cmd : 0.0f;
}


#ifdef __cplusplus
}
#endif
#endif
