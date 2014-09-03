#include "counter.h"

#include <stdio.h>
#include <stdlib.h>

/* Initialization is not thread-safe */
void Counter_Init(counter_t *c, int value)
{
	c->count = value;

	spinlock_init(&c->lock);
}

int Counter_GetValue(counter_t *c)
{
	int return_val;

	spinlock_acquire(&c->lock);

	return_val = c->count;

	spinlock_release(&c->lock);

	return return_val;
}

void Counter_Increment(counter_t *c)
{
	spinlock_acquire(&c->lock);

	c->count = c->count+1;

	spinlock_release(&c->lock);
}

void Counter_Decrement(counter_t *c)
{
	spinlock_acquire(&c->lock);

	c->count = c->count-1;

	spinlock_release(&c->lock);
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
