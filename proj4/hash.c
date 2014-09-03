#include "hash.h"

#include <stdlib.h>

/* Initialization is not thread-safe */
void Hash_Init(hash_t *hash, int buckets)
{
	hash->buckets = buckets;
	hash->bucket_list = malloc(sizeof(list_t)*buckets);

	while(--buckets >= 0)
	{
		List_Init(&hash->bucket_list[buckets]);
	}
}

/* The list functions are already thread-safe, so there's no need to use locks
 * here.
 */
void Hash_Insert(hash_t *hash, void *element, unsigned int key)
{
	List_Insert(&hash->bucket_list[key%hash->buckets], element, key);
}

void Hash_Delete(hash_t *hash, unsigned int key)
{
	List_Delete(&hash->bucket_list[key%hash->buckets], key);
}

/* Destruction is not thread-safe */
void Hash_Destroy(hash_t *hash)
{
	while(hash->buckets > 0)
	{
		List_Destroy(&hash->bucket_list[--hash->buckets]);
	}

	/* hash->buckets should be zero at this point */
	free(hash->bucket_list);
}

void *Hash_Lookup(hash_t *hash, unsigned int key)
{
	return List_Lookup(&hash->bucket_list[key%hash->buckets], key);
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
