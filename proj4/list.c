#include "list.h"

#include <stdlib.h>

/* Initialization is not thread-safe */
void List_Init(list_t *list)
{
	list->head = NULL;

	spinlock_init(&list->lock);
}

void List_Insert(list_t *list, void *element, unsigned int key)
{
	list_node *current_node = malloc(sizeof(list_node));

	current_node->key = key;
	current_node->element = element;

	spinlock_acquire(&list->lock);

	current_node->next = list->head;
	list->head = current_node;

	spinlock_release(&list->lock);
}

void List_Delete(list_t *list, unsigned int key)
{
	list_node *current_node;
	list_node *old_node;

	spinlock_acquire(&list->lock);

	current_node = list->head;
	old_node = NULL;

	while(current_node && current_node->key != key)
	{
		old_node = current_node;
		current_node = current_node->next;
	}

	if(current_node)
	{
		/* head of list */
		if(!old_node)
		{
			list->head = current_node->next;
		}
		else
		{
			old_node->next = current_node->next;
		}
	}

	spinlock_release(&list->lock);
}

/* Destruction is not thread-safe */
void List_Destroy(list_t *list)
{
	list_node *old_node = list->head;
	list_node *current_node = list->head;

	while(current_node)
	{
		old_node = current_node;
		current_node = current_node->next;

		free(old_node);
	}

	list->head = NULL;

	spinlock_destroy(&list->lock);
}

void *List_Lookup(list_t *list, unsigned int key)
{
	list_node *current_node;

	spinlock_acquire(&list->lock);

	current_node = list->head;

	while(current_node && current_node->key != key)
	{
		current_node = current_node->next;
	}
	spinlock_release(&list->lock);

	return current_node?current_node->element:NULL;
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
