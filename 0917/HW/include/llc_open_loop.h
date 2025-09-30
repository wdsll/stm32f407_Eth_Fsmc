/*********************************************************************************************************
* 模块名称：llc_open_loop.h
* 摘    要：开环模式
* 当前版本：1.0.0
* 作    者：Rengar
* 完成日期：2025年09月25日 
* 内    容：定义LLC开环控制的分段配置和控制逻辑
						提供初始化、启动、停止和运行控制接口
* 注    意：使用时需确保分段配置正确，避免频率跳变
						调用tick函数需周期性执行（建议1ms周期）
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
//开环控制段配置结构体 
//用于定义LLC开环控制的分段参数
typedef struct
{
	float start_hz; ///< 起始频率（Hz）
	float stop_hz;  ///< 终止频率（Hz）
	float slew_hz_per_ms; ///< 频率变化斜率（Hz/ms）
	uint32_t hold_time_ms; ///< 保持时间（ms）
}llc_open_loop_segment_t;

//LLC开环控制器结构体 
//用于管理LLC开环控制的状态和参数
typedef struct {
    const llc_open_loop_segment_t *segments; //< 分段配置数组
    size_t segment_count;   ///< 分段数量
    size_t current_index;	///< 当前分段索引
    float f_cmd;		///< 当前目标频率（Hz）
    uint32_t hold_elapsed_ms;  ///< 当前分段时间累计（ms）
    bool running; ///< 运行标志
    bool holding;///< 保持状态标志
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
