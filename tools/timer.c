#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "mem.h"
#include "timer.h"

unsigned long int utils_timer_now(void)
{
	return time(NULL);
}

int utils_timer_sleep(int second)
{
	struct timeval tv;
	tv.tv_sec = second;
	tv.tv_usec = 0;

	return select(FD_SETSIZE,NULL,NULL,NULL,&tv);
}

int utils_timer_usleep(int usecond)
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = usecond;

	return select(FD_SETSIZE,NULL,NULL,NULL,&tv);
}

int _utils_timer_state_get(utils_timer_t *t)
{
	utils_timer_state_t state;
	parallel_spin_lock(&t->lock);
	state = t->v;
	parallel_spin_unlock(&t->lock);

	return state;
}

int _utils_timer_state_set(utils_timer_t *t, utils_timer_state_t state)
{
	parallel_spin_lock(&t->lock);
	t->v = state;
	parallel_spin_unlock(&t->lock);

	return state;
}

void _utils_timer_delete(utils_timer_t *t)
{
	mem_free(t);
}

void *_utils_timer_run(void *arg)
{
	utils_timer_t *t = arg;

	utils_timer_sleep(t->after);

	while (_utils_timer_state_get(t) != TIMER_STOP) {
		t->timeout_handler(t->arg);
		if (!t->repeat)
			break;
		utils_timer_sleep(t->interval);
	}

	return NULL;
}

void *utils_timer_start(void *cb, void *arg, 
				unsigned int after, unsigned int interval, unsigned int repeat)
{	
	utils_timer_t *t = mem_alloc(sizeof(utils_timer_t));
	
	if (!t) {
		log_err("Failed to allocate timer memory\n");
		goto failed;
	}
	
	t->v = TIMER_INIT;
	t->lock.n = 0;
	t->after = after;
	t->interval = interval;
	t->repeat = repeat;
	t->timeout_handler = cb;
	t->arg = arg;

	t->thread.routine = _utils_timer_run;
	t->thread.arg = t;
	parallel_thread_create(&t->thread);
	
	// log_debug("Timer started: %p\n", t);
	
    return t;
failed:
	mem_free(t);
	return t;
}

int utils_timer_stop(void *_t)
{
	utils_timer_t *t = _t;
	_utils_timer_state_set(t, TIMER_STOP);
	parallel_thread_kill(&t->thread);
	_utils_timer_delete(t);
	return 0;
}



