#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

struct Node {
	void *data;
	struct Node *next;
};

struct LinkedList {
	struct Node *head;
	int size;
};

void init_list(struct LinkedList *list);

void add_node(struct LinkedList *list, int pos, void *data);

struct Node *remove_node(struct LinkedList *list, int pos);

int get_size(struct LinkedList *list);

void *get_node(struct LinkedList *list, int index);

void free_list(struct LinkedList **list);

#endif /* __LINKEDLIST_H__ */
