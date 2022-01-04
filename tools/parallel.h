#ifndef __PARALLEL_TOOL_H__
#define __PARALLEL_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	volatile int n;
}parallel_spin_t;

#define PARALLEL_SPINLOCK_INITIALIZER {0}

extern void parallel_spin_init(parallel_spin_t *lock);
extern void parallel_spin_lock(parallel_spin_t *lock);
extern void parallel_spin_unlock(parallel_spin_t *lock);

typedef struct {
	unsigned long int id;
	void *(*routine)(void *);
	void *arg;
}parallel_thread_t;

extern int parallel_thread_create(parallel_thread_t *p);
extern int parallel_thread_join(parallel_thread_t *p);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __PARALLEL_TOOL_H__ */