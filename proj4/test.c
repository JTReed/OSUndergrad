#include "counter.h"
#include "list.h"
#include "hash.h"

#include <stdio.h>
#include <pthread.h>

typedef struct list_insert_arg
{
	list_t *list;
	void *element;
	unsigned int key;
}
list_insert_arg;

typedef struct list_lookup_arg
{
	list_t *list;
	unsigned int key;
}
list_lookup_arg;

void* pthread_list_insert(void* arg)
{
	list_insert_arg *arg_struct = arg;

	List_Insert(arg_struct->list, arg_struct->element, arg_struct->key);

	return NULL;
}

void* pthread_list_lookup(void* arg)
{
	list_lookup_arg *arg_struct = arg;

	printf(	"Expected %p got %p\n",
		(void *)(arg_struct->key),
		List_Lookup(arg_struct->list, arg_struct->key) );

	return NULL;
}

int main()
{
	pthread_t thread[3];
	counter_t counter;

	printf("Testing Counter...\n");

	printf("Begin single thread test\n");

	printf("Counter Init\n");
	Counter_Init(&counter, 0);

	printf("Counter_GetValue\n");
	int counter_val = Counter_GetValue(&counter);
	printf("expected 0 got %d\n", counter_val);

	printf("Counter_Increment\n");
	Counter_Increment(&counter);
	counter_val = counter_val = Counter_GetValue(&counter);
	printf("expected 1 got %d\n", counter_val);

	printf("Counter_Decrement\n");
	Counter_Decrement(&counter);
	counter_val = Counter_GetValue(&counter);
	printf("expected 0 got %d\n", counter_val);

	printf("Begin multithreaded test\n");

	printf("Counter Init\n");
	Counter_Init(&counter, 0);

	pthread_create(&thread[0], NULL, Counter_Increment, (void*)&counter);
	printf("Counter is at %d after 1 increment call\n", Counter_GetValue(&counter));
	pthread_create(&thread[1], NULL, Counter_Increment, (void*)&counter);
	printf("Counter is at %d after 2 increment calls\n", Counter_GetValue(&counter));
	pthread_create(&thread[2], NULL, Counter_Increment, (void*)&counter);
	printf("Counter is at %d after 3 increment calls\n", Counter_GetValue(&counter));

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);

	printf("Count after 3 threads = %d\n", Counter_GetValue(&counter));

	printf("Done Testing Counter\n\n");

	list_insert_arg insert_arg[3];
	list_lookup_arg lookup_arg[3];
	list_t list;

	printf("Testing list...\n");

	printf("Begin single-thread test\n");

	printf("List_Init()\n");
	List_Init(&list);

	printf("List_Insert()\n");
	List_Insert(&list, (void *)0x1, 1);
	List_Insert(&list, (void *)0x2, 2);
	List_Insert(&list, (void *)0x3, 3);

	printf("Expected %p got %p\n", (void *)0x1, List_Lookup(&list, 1));
	printf("Expected %p got %p\n", (void *)0x3, List_Lookup(&list, 3));
	printf("Expected %p got %p\n", (void *)0x2, List_Lookup(&list, 2));

	printf("List_Delete()\n");
	List_Delete(&list, 1);

	printf("Expected %p got %p\n", (void *)0x2, List_Lookup(&list, 2));
	printf("Expected %p got %p\n", (void *)0x3, List_Lookup(&list, 3));
	printf("Expected %p got %p\n", NULL, List_Lookup(&list, 1));

	printf("Single-thread test completed\n");

	printf("Begin multi-thread test\n");

	printf("List_Init()\n");
	List_Init(&list);

	printf("Parallel List_Insert()\n");

	insert_arg[0].list = &list;
	insert_arg[0].element = (void *)0x1;
	insert_arg[0].key = 1;

	pthread_create(&thread[0], NULL, pthread_list_insert, &insert_arg[0]);

	insert_arg[1].list = &list;
	insert_arg[1].element = (void *)0x2;
	insert_arg[1].key = 2;

	pthread_create(&thread[1], NULL, pthread_list_insert, &insert_arg[1]);

	insert_arg[2].list = &list;
	insert_arg[2].element = (void *)0x3;
	insert_arg[2].key = 3;

	pthread_create(&thread[2], NULL, pthread_list_insert, &insert_arg[2]);

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);

	printf("Parallel List_Insert() completed\n");

	printf("Parallel List_Lookup()\n");

	lookup_arg[0].list = &list;
	lookup_arg[0].key = 1;

	pthread_create(&thread[0], NULL, pthread_list_lookup, &lookup_arg[0]);

	lookup_arg[1].list = &list;
	lookup_arg[1].key = 2;

	pthread_create(&thread[1], NULL, pthread_list_lookup, &lookup_arg[1]);

	lookup_arg[2].list = &list;
	lookup_arg[2].key = 3;

	pthread_create(&thread[2], NULL, pthread_list_lookup, &lookup_arg[2]);

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);

	printf("Parallel List_Lookup() completed\n");

	printf("List_Delete()\n");
	List_Delete(&list, 1);

	printf("Expected %p got %p\n", (void *)0x2, List_Lookup(&list, 2));
	printf("Expected %p got %p\n", (void *)0x3, List_Lookup(&list, 3));
	printf("Expected %p got %p\n", NULL, List_Lookup(&list, 1));

	printf("Multi-thread test completed\n");

	printf("Done testing list\n\n");

	hash_t hash;

	printf("Testing hash...\n");
	printf("Begin single-thread test\n");

	printf("Hash_Init()\n");
	Hash_Init(&hash, 2);

	printf("Hash_Insert()\n");
	Hash_Insert(&hash, (void *)0x1, 1);
	Hash_Insert(&hash, (void *)0x2, 2);
	Hash_Insert(&hash, (void *)0x3, 3);

	printf("Hash_Lookup()\n");
	printf("Expected %p got %p\n", (void *)0x2, Hash_Lookup(&hash, 2));
	printf("Expected %p got %p\n", (void *)0x3, Hash_Lookup(&hash, 3));
	printf("Expected %p got %p\n", (void *)0x1, Hash_Lookup(&hash, 1));

	printf("Hash_Delete()\n");
	Hash_Delete(&hash, 1);

	printf("Hash_Lookup()\n");
	printf("Expected %p got %p\n", (void *)0x2, Hash_Lookup(&hash, 2));
	printf("Expected %p got %p\n", (void *)0x3, Hash_Lookup(&hash, 3));
	printf("Expected %p got %p\n", NULL, Hash_Lookup(&hash, 1));

	printf("Single-thread test completed\n");
	printf("Done testing hash...\n");

	return 0;
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
