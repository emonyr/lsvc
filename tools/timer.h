#ifndef __TIMER_TOOL_H__
#define __TIMER_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "parallel.h"

typedef enum {
	TIMER_INIT,
	TIMER_RUNNING,
	TIMER_STOP,
}utils_timer_state_t;

typedef struct utils_timer {
	utils_timer_state_t v;
	parallel_spin_t lock;
	parallel_thread_t thread;
	uint32_t after;		// in seconds
	uint32_t interval;	// in seconds
	uint32_t repeat;	// 1=repeat
	void *(*timeout_handler)(void *);
	void *arg;
}utils_timer_t;

extern uint64_t utils_timer_now(void);
extern int utils_timer_sleep(int second);
extern int utils_timer_usleep(int usecond);


extern void *utils_timer_start(void *cb, void *arg, uint32_t after, 
										uint32_t interval, uint32_t repeat);
extern int utils_timer_stop(void *timer);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __TIMER_TOOL_H__ */