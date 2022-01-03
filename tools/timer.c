#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "thread.h"
#include "kvlist.h"

static thread_spin_t fastlock = THREAD_SPINLOCK_INITIALIZER;
static struct kvlist *timerlist = NULL;

typedef void (timeout_handler)(void* arg, timer_t timer_id);
#ifndef container_of
#define container_of(ptr, type, member) ({ \
        const __typeof__(((type *)0)->member ) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type,member) );})
#endif 

typedef struct utils_timer {
	timer_t timerid;
	int drop;
	struct sigevent evp;
	struct itimerspec it;
	timeout_handler *cb;
	int repeat;
	void *arg;
}utils_timer_t;


void timer_handler_f(union sigval v)
{
	utils_timer_t *t;
	char timer_str[32] = {0};
	
	thread_spin_lock(&fastlock);	
	if(v.sival_ptr){		
		t = (utils_timer_t *)v.sival_ptr; 
		sprintf(timer_str, "%x", (unsigned int)t->timerid);
		if(kvlist_get(timerlist, timer_str)){
			if(t->cb){                
                //log_debug("timer_handler %x\n", t->timerid);
				t->cb(t->arg, t->timerid);
			}
			if(!t->repeat){
				timer_delete(t->timerid);
				free(t);
				kvlist_delete(timerlist, timer_str);
			}
		}
		else {
			/* utils_timer_stop called */
			timer_delete(t->timerid);
			free(t);
		}
	}	
	thread_spin_unlock(&fastlock);

}

int utils_timer_stop(utils_timer_t *timer)
{
	if(!timer){
		return -1;
	}

	char timer_str[32] = {0};
	sprintf(timer_str, "%x", (unsigned int)timer->timerid);
	
	/* delete timer from timerlist */
	thread_spin_lock(&fastlock);
	kvlist_delete(timerlist, timer_str);	
	// log_debug("Stop timer 0x%x done\n", timer);	
	thread_spin_unlock(&fastlock);
	
	return 0;
}

void *utils_timer_start(void *cb, void *arg, 
				unsigned int after, unsigned int interval, unsigned int repeat)
{	
	int ret;
	utils_timer_t *t = NULL;
	
	/* FIXME: there is a race condition here if start two timer from two threads at once */
	if(timerlist == NULL){
		timerlist = (struct kvlist *)malloc(sizeof(struct kvlist));
		if(timerlist){
			kvlist_init(timerlist, kvlist_strlen);
		}		
	}
	
	t = (utils_timer_t *)malloc(sizeof(utils_timer_t));
	if(!t){
		log_err("Malloc memory error(%s)\n", strerror(errno));
		return NULL;
	}
	memset(t, 0, sizeof(utils_timer_t));
	t->arg = arg;
	t->repeat = repeat;
	t->cb = cb;
	t->evp.sigev_value.sival_ptr = t;
	t->evp.sigev_notify = SIGEV_THREAD;
	t->evp.sigev_notify_function  = timer_handler_f;
	ret = timer_create(CLOCK_MONOTONIC, &t->evp, &t->timerid);
	if(ret < 0){
		log_err("Fail to create timer\n");
		return NULL;
	}
	t->it.it_interval.tv_sec = interval;
	t->it.it_interval.tv_nsec = 0;
	t->it.it_value.tv_sec = after;
	t->it.it_value.tv_nsec = 0;
	
	ret = timer_settime(t->timerid, 0, &t->it, NULL);
	if(ret < 0){
		log_err("Fail to set timer\n");
		return NULL;
	}

	/* save timer ID */
	char timer_str[32] = {0};
	sprintf(timer_str, "%x", (unsigned int)t->timerid);
	
	thread_spin_lock(&fastlock);
	kvlist_set(timerlist, timer_str, timer_str);	
	thread_spin_unlock(&fastlock);
	
	// log_debug("Start timer:0x%x\n", t->timerid);
	
    return (void *)t;
}

int utils_timer_check(utils_timer_t *timer){
    if(!timer){        
        log_err("Invaild timer\n");
        return -1;
    }

    struct itimerspec tmp_it;
	char timer_str[32] = {0};
    sprintf(timer_str, "%x", (unsigned int)timer->timerid);
    if(kvlist_get(timerlist, timer_str)){
        int ret = timer_gettime(timer , &tmp_it);
        if(ret != 0){
            log_err("Failed to get timer value\n");
            return -1;
        }else{
            return tmp_it.it_value.tv_sec;
        }
    }else{
        log_err("Timer is already over\n");
    }

    return -1;
}



