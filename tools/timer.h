#ifndef __TIMER_TOOL_H__
#define __TIMER_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void *utils_timer_start(void *cb, void *arg, uint32_t after, 
										uint32_t interval, uint32_t repeat);
extern int utils_timer_stop(void *timer);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __TIMER_TOOL_H__ */