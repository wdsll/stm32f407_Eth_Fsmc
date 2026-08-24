#ifndef CONDITION_HELD_H
#define CONDITION_HELD_H

#include "main.h"
typedef struct{
	bool active;
	uint32_t started_ms;
}condition_qualification_t;

static inline void condition_qualification_reset(condition_qualification_t *qualification)
{
	qualification->active = false;
}

static inline bool condition_held(condition_qualification_t *qualification,bool condition,uint32_t duration_ms)
{
	if(!condition)
	{
		condition_qualification_reset(qualification);
		return false;
	}
	if(!qualification -> active)
	{
		qualification->active = true;
		qualification->started_ms = g_ms;
		return false;
	}
	return elapsed_reached(qualification->started_ms,duration_ms);
}

#endif
