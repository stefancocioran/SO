#ifndef __HASHMAP_H
#define __HASHMAP_H

#include "LinkedList.h"

struct pair {
	void *key;
	void *value;
};

struct HashMap {
	struct LinkedList *buckets;
	int size; // Total number of nodes found in all buckets
	int hmax; // Buckets count
	unsigned int (*hash_function)(void *a);
	int (*cmp_function)(void *a, void *b);
};

void init_hmap(struct HashMap *ht, int hmax,
	       unsigned int (*hash_function)(void *a),
	       int (*cmp_function)(void *a, void *b));

void put(struct HashMap *ht, void *key, int key_size_bytes, void *value,
	 int value_size_bytes);

void *get(struct HashMap *ht, void *key);

int has_key(struct HashMap *ht, void *key);

void remove_entry(struct HashMap *ht, void *key);

int get_hmap_size(struct HashMap *ht);

void free_hmap(struct HashMap *ht);

int cmp_strings(void *a, void *b);

unsigned int hash_function(void *a);

#endif
