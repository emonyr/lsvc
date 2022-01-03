#ifndef __THREAD_TOOL_H__
#define __THREAD_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	volatile int n;
}thread_spin_t;

#define THREAD_SPINLOCK_INITIALIZER {0}

extern void thread_spin_init(thread_spin_t *lock);
extern void thread_spin_lock(thread_spin_t *lock);
extern void thread_spin_unlock(thread_spin_t *lock);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __THREAD_TOOL_H__ */