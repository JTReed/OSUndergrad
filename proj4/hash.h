#ifndef HASH_H
#define HASH_H

#include "list.h"

/* No lock is needed since the list functions are already thread-safe. */
typedef struct hash_t
{
	void *element;
	int buckets;
	list_t* bucket_list;
}
hash_t;

void Hash_Init(hash_t *hash, int buckets);
void Hash_Insert(hash_t *hash, void *element, unsigned int key);
void Hash_Delete(hash_t *hash, unsigned int key);
void Hash_Destroy(hash_t *hash);
void *Hash_Lookup(hash_t *hash, unsigned int key);
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
