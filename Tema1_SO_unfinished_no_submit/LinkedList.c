#include "LinkedList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Initialize list
 */
void init_list(struct LinkedList *list)
{
	list->head = NULL;
	list->size = 0;
}

/*
 * Add node to list at a given position
 */
void add_node(struct LinkedList *list, int pos, void *data)
{
	struct Node *prev, *curr, *new_node;

	if (list == NULL)
		return;

	curr = list->head;
	prev = NULL;

	if (pos < 0)
		return;
	else if (pos > list->size)
		pos = list->size;

	while (pos > 0) {
		prev = curr;
		curr = curr->next;
		pos--;
	}

	new_node = (struct Node *)malloc(sizeof(struct Node));
	new_node->data = data;
	new_node->next = curr;

	if (prev == NULL)
		list->head = new_node;
	else
		prev->next = new_node;

	list->size++;
}

/*
 * Remove node at the given position. Positions are indexed from zero.
 If pos > nodes_count - 1, the node is removed from the end of the list.
 Function returns a pointer to the removed node.
 */
struct Node *remove_node(struct LinkedList *list, int pos)
{
	struct Node *prev, *curr;

	if (list == NULL)
		return NULL;

	if (list->head == NULL)
		return NULL;

	curr = list->head;
	prev = NULL;

	if (pos > list->size - 1)
		pos = list->size - 1;
	else if (pos < 0)
		return NULL;

	while (pos > 0) {
		prev = curr;
		curr = curr->next;
		pos--;
	}

	if (prev == NULL)
		list->head = curr->next;
	else
		prev->next = curr->next;

	list->size--;

	return curr;
}

/*
 * Returns the size of the given list
 */
int get_size(struct LinkedList *list)
{
	if (list == NULL)
		return -1;

	return list->size;
}

/*
 * Returns the node at the given index
 */
void *get_node(struct LinkedList *list, int idx)
{
	struct Node *curr = list->head;
	int count = 0;

	while (curr != NULL) {
		if (count == idx)
			return curr->data;

		curr = curr->next;
		count++;
	}

	return NULL;
}

/*
 * Free the memmory used by all nodes found in the list and the list structure
 */
void free_list(struct LinkedList **pp_list)
{
	struct Node *currNode;

	if (pp_list == NULL || *pp_list == NULL)
		return;

	while (get_size(*pp_list) > 0) {
		currNode = remove_node(*pp_list, 0);
		free(currNode);
	}

	free(*pp_list);
	*pp_list = NULL;
}
