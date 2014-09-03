#ifndef LIST_H
#define LIST_H

#include "spin.h"

/* list */
typedef struct list_node
{
	unsigned int key;
	void *element;
	struct list_node *next;
}
list_node;

typedef struct list_t
{
	spinlock_t lock;
	list_node *head;
}
list_t;

void List_Init(list_t *list);
void List_Insert(list_t *list, void *element, unsigned int key);
void List_Delete(list_t *list, unsigned int key);
void List_Destroy(list_t *list);
void *List_Lookup(list_t *list, unsigned int key);
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
