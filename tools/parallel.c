#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "parallel.h"


void parallel_spin_init(parallel_spin_t *lock)
{
	lock->n = 0;
}

void parallel_spin_lock(parallel_spin_t *lock)
{
	do {
		while (lock->n);
	} while (__sync_lock_test_and_set(&lock->n, 1));

	__sync_synchronize();
}

void parallel_spin_unlock(parallel_spin_t *lock)
{
	(void)__sync_lock_release(&lock->n);
}

int parallel_thread_create(parallel_thread_t *p)
{
	return pthread_create(&p->id, NULL, p->routine, p->arg);
}

int parallel_thread_join(parallel_thread_t *p)
{
	return pthread_join(&p->id, NULL);
}