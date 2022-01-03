#include <stdio.h>
#include <stdlib.h>

#include "thread.h"


void thread_spin_init(thread_spin_t *lock)
{
	lock->n = 0;
}

void thread_spin_lock(thread_spin_t *lock)
{
	do {
		while (lock->n);
	} while (__sync_lock_test_and_set(&lock->n, 1));

	__sync_synchronize();
}

void thread_spin_unlock(thread_spin_t *lock)
{
	(void)__sync_lock_release(&lock->n);
}
