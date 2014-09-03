#ifndef SPIN_H
#define SPIN_H

#include <pthread.h>

typedef struct spinlock_t
{
	#ifdef POSIX_MUTEX
	pthread_mutex_t mutex;
	#else
	unsigned int flag;
	#endif
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);
void spinlock_destroy(spinlock_t *lock);
#endif

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
