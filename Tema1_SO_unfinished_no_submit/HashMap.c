#include "HashMap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Compare keys function
 */
int cmp_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/*
 * Hashing function strings
 */
unsigned int hash_function(void *a)
{
	/*
	 * http://www.cse.yorku.ca/~oz/hash.html
	 */
	unsigned char *puchar_a = (unsigned char *)a;
	unsigned long hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c; /* hash * 33 + c */

	return hash;
}

/*
 * Initialize hashmap
 */
void init_hmap(struct HashMap *hmap, int hmax,
	       unsigned int (*hash_function)(void *),
	       int (*cmp_function)(void *, void *))
{
	hmap->size = 0;
	hmap->hmax = hmax;
	hmap->hash_function = hash_function;
	hmap->cmp_function = cmp_function;
	hmap->buckets =
	    (struct LinkedList *)malloc(hmax * sizeof(struct LinkedList));
	for (int i = 0; i < hmax; ++i) {
		hmap->buckets[i].size = 0;
		hmap->buckets[i].head = NULL;
	}
}

/*
 * Add new entry into hashmap
 */
void put(struct HashMap *hmap, void *key, int key_size_bytes, void *value,
	 int value_size_bytes)
{
	int index = hmap->hash_function(key) % hmap->hmax;

	if (hmap->buckets[index].size == 0) {
		struct pair *data = (struct pair *)malloc(sizeof(struct pair));
		void *newkey = (void *)malloc(key_size_bytes);
		void *newvalue = (void *)malloc(value_size_bytes);

		memcpy(newkey, key, key_size_bytes);
		memcpy(newvalue, value, value_size_bytes);
		data->key = newkey;
		data->value = newvalue;
		add_node(&hmap->buckets[index], 0, data);
		hmap->size++;
		return;
	}
	struct Node *entry = hmap->buckets[index].head;

	while (entry != NULL) {
		struct pair *data = (struct pair *)entry->data;

		if (hmap->cmp_function(key, data->key) == 0) {
			data->value = value;
			return;
		}
		entry = entry->next;
	}
	struct pair *data = (struct pair *)malloc(sizeof(struct pair));
	void *newkey = (void *)malloc(key_size_bytes);

	memcpy(newkey, key, key_size_bytes);
	data->key = newkey;
	data->value = value;
	add_node(&hmap->buckets[index], hmap->buckets[index].size, data);
	hmap->size++;
}

/*
 * Returns value corresponding to the given key
 */
void *get(struct HashMap *hmap, void *key)
{
	int index = hmap->hash_function(key) % hmap->hmax;
	struct Node *entry = hmap->buckets[index].head;

	while (entry != NULL) {
		struct pair *data = (struct pair *)entry->data;

		if (hmap->cmp_function(key, data->key) == 0)
			return data->value;
		entry = entry->next;
	}

	return NULL;
}

/*
 * Return 1 if key is associated to a value or 0 otherwise
 */
int has_key(struct HashMap *hmap, void *key)
{
	int index = hmap->hash_function(key) % hmap->hmax;
	struct Node *curr = hmap->buckets[index].head;

	while (curr != NULL) {
		struct pair *data = (struct pair *)curr->data;

		if (hmap->cmp_function(key, data->key) == 0)
			return 1;
		curr = curr->next;
	}

	return 0;
}

/*
 * Remove a hasmap entry
 */
void remove_entry(struct HashMap *hmap, void *key)
{
	int index = hmap->hash_function(key) % hmap->hmax, pos = 0;
	struct Node *entry = hmap->buckets[index].head;

	while (entry != NULL) {
		struct pair *data = (struct pair *)entry->data;

		if (hmap->cmp_function(key, data->key) == 0)
			break;
		pos++;
		entry = entry->next;
	}
	if (entry != NULL) {
		struct pair *data = (struct pair *)entry->data;

		remove_node(&hmap->buckets[index], pos);
		hmap->size--;
		free(data->key);
		free(data->value);
		free(data);
		free(entry);
	}
}

/*
 * Free the hashmap's allocated memmory
 */
void free_hmap(struct HashMap *hmap)
{
	for (int i = 0; i < hmap->hmax; ++i) {
		struct Node *curr = hmap->buckets[i].head;

		while (curr != NULL) {
			struct pair *data = (struct pair *)curr->data;

			remove_entry(hmap, data->key);
			curr = hmap->buckets[i].head;
		}
	}
	free(hmap->buckets);
	free(hmap);
}

/*
 * Return hashmap's size
 */
int get_hmap_size(struct HashMap *hmap)
{
	if (hmap == NULL)
		return -1;

	return hmap->size;
}
