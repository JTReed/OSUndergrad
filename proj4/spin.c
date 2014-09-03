#include "spin.h"

#include <stdlib.h>

static inline uint xchg(volatile unsigned int *addr, unsigned int newval)
{
	uint result;

	/* xchg: exchange the two source operands
	 *
	 * two outputs: *addr and result
	 *
	 * "+m", m means the operand is directly from memory, + means read and write
	 *
	 * "=a": suggest gcc to put the operand value into eax register; `=' means
	 * write only
	 *
	 * one input: newval, "1" means it uses the same constraint as the earlier
	 * 1th, i.e., it will be put into eax and then be overwritten
	 *
	 * "cc" means the condition register might be altered
	 */
	asm volatile("lock; xchgl %0, %1" : "+m" (*addr), "=a" (result) : "1" (newval) : "cc");

	return result;
}

void spinlock_init(spinlock_t *lock)
{
	#ifdef POSIX_MUTEX
	pthread_mutex_init(&lock->mutex, NULL);
	#else
	lock->flag = 0;
	#endif
}

void spinlock_acquire(spinlock_t *lock)
{
	#ifdef POSIX_MUTEX
	pthread_mutex_lock(&lock->mutex);
	#else
	while(xchg(&lock->flag, 1) == 1);
	#endif
}

void spinlock_release(spinlock_t *lock)
{
	#ifdef POSIX_MUTEX
	pthread_mutex_unlock(&lock->mutex);
	#else
	lock->flag = 0;
	#endif
}

void spinlock_destroy(spinlock_t *lock)
{
	#ifdef POSIX_MUTEX
	pthread_mutex_destroy(&lock->mutex);
	#endif
}

/*
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
