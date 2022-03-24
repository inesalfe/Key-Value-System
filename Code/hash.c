// https://www.journaldev.com/35238/hash-table-in-c-plus-plus

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

#define CAPACITY 50000 // Size of the Hash Table

unsigned long HashFunction(char * str) {
	unsigned long i = 0;
	// Sum the value each char in the string
	for (int j = 0; str[j]; j++) {
		i += str[j];
	}
	// Return a value within the capacity of the table
	return i % CAPACITY;
}

static LinkedList * AllocateList () {

	LinkedList * list = (LinkedList *) malloc (sizeof(LinkedList));
	return list;
}

static LinkedList * InsertInLinkedList(LinkedList * list, Ht_item * item) {

	// If list is NULL, the list is empty and we alocate the head
	if (!list) {
		LinkedList * head = AllocateList();
		head->item = item;
		head->next = NULL;
		list = head;
		return list;
	}

	// The the second element if NULL, we set the next of the head to that node
	else if (list->next == NULL) {
		LinkedList * node = AllocateList();
		node->item = item;
		node->next = NULL;
		list->next = node;
		return list;
	}

	// Advance until the last filled element
	LinkedList * temp = list;
	while (temp->next) {
		temp = temp->next;
	}

	// The next of the last filled ement is the item
	LinkedList * node = AllocateList();
	node->item = item;
	node->next = NULL;
	temp->next = node;

	return list;
}

static void FreeLinkedList(LinkedList * list) {

	LinkedList * temp = list;
	while (list) {
		temp = list;
		list = list->next;
		free(temp->item->key);
		free(temp->item->value);
		free(temp->item);
		free(temp);
	}
}

static LinkedList ** CreateOverflowBuckets(HashTable * table) {

	LinkedList ** buckets = (LinkedList **) calloc (table->size, sizeof(LinkedList *));
	for (int i = 0; i < table->size; i++)
		buckets[i] = NULL;
	return buckets;
}

static void FreeOverflowBuckets(HashTable * table) {

	LinkedList ** buckets = table->overflow_buckets;
	for (int i = 0; i < table->size; i++)
		FreeLinkedList(buckets[i]);
	free(buckets);
}

Ht_item * CreateItem(char * key, char * value) {

	Ht_item * item = (Ht_item *) malloc (sizeof(Ht_item));
	// Alocate memory for the kay and value
	item->key = (char *) malloc (strlen(key) + 1);
	item->value = (char *) malloc (strlen(value) + 1);

	strcpy(item->key, key);
	strcpy(item->value, value);

	return item;
}

HashTable * CreateTable(int size) {

	HashTable * table = (HashTable *) calloc (1, sizeof(HashTable));
	table->size = size;
	// Count starts at zero
	table->count = 0;
	table->items = (Ht_item **) calloc (table->size, sizeof(Ht_item *));
	for (int i = 0; i < table->size; i++)
		table->items[i] = NULL;
	table->overflow_buckets = CreateOverflowBuckets(table);

	return table;
}

void FreeItem(Ht_item * item) {

	free(item->key);
	free(item->value);
	free(item);
}

void FreeTable(HashTable * table) {

	for (int i = 0; i < table->size; i++) {
		Ht_item * item = table->items[i];
		if (item != NULL)
			FreeItem(item);
	}

	FreeOverflowBuckets(table);
	free(table->items);
	free(table);
}

void HandleCollision(HashTable * table, unsigned long index, Ht_item * item) {

	LinkedList * head = table->overflow_buckets[index];

	if (head == NULL) {
		// We need to create the list
		head = AllocateList();
		head->item = item;
		table->overflow_buckets[index] = head;
		return;
	}
	else {
		// Insert to the list
		table->overflow_buckets[index] = InsertInLinkedList(head, item);
		return;
	}
}

void InsertToTable(HashTable * table, char * key, char * value) {

	// Create the item
	Ht_item * item = CreateItem(key, value);

	// Compute the index
	unsigned long index = HashFunction(key);

	Ht_item * current_item = table->items[index];

	// Key does not exist in the table
	if (current_item == NULL) {

		// Insert directly
		table->items[index] = item;
		table->count++;
	}

	else {
		// If the key already exists, update the value
		if (strcmp(current_item->key, key) == 0) {
			// Free item and insert because the previous value was alocated for a specific string length
			FreeItem(current_item);
			table->items[index] = item;
			return;
		}

		else {
			// Collision
			table->count++;
			HandleCollision(table, index, item);
			return;
		}
	}
}

char * SearchInTable(HashTable * table, char * key) {

	int index = HashFunction(key);
	Ht_item * item = table->items[index];
	LinkedList * head = table->overflow_buckets[index];

	while (item != NULL) {
		// Item is in the table
		if (strcmp(item->key, key) == 0)
			return item->value;
		// Item is in the overflow buckets
		if (head == NULL)
			return NULL;
		// Advance to the next element in the Linked List
		item = head->item;
		head = head->next;
	}
	return NULL;
}

void DeleteFromTable(HashTable * table, char * key) {

	// Deletes an item from the table
	int index = HashFunction(key);
	Ht_item * item = table->items[index];
	LinkedList * head = table->overflow_buckets[index];

	if (item == NULL) {
		// Does not exist in the table
		return;
	}
	else {
		if (head == NULL && strcmp(item->key, key) == 0) {
			// The item is in the table and the Linked List is empty
			table->items[index] = NULL;
			FreeItem(item);
			table->count--;
			return;
		}
		else if (head != NULL) {
			// Item is in the table bue the list is not null
			if (strcmp(item->key, key) == 0) {
				// Remove this item and set the head of the list as the new item
				FreeItem(item);
				LinkedList * node = head;
				// Head is the second element
				head = head->next;
				node->next = NULL;
				// The item in the table is the of the head of the list
				table->items[index] = CreateItem(node->item->key, node->item->value);
				FreeLinkedList(node);
				table->overflow_buckets[index] = head;
				table->count--;
				return;
			}

			LinkedList * curr = head;
			LinkedList * prev = NULL;

			// Advance in the list until we find the key
			while (curr) {
				if (strcmp(curr->item->key, key) == 0) {
					// First element of the chain
					if (prev == NULL) {
						LinkedList * node = head;
						// Head is the second element
						head = head->next;
						node->next = NULL;
						FreeLinkedList(node);
						table->overflow_buckets[index] = head;
						table->count--;
						return;
					}
					else {
						// This is somewhere in the chain
						prev->next = curr->next;
						free(curr->item->key);
						free(curr->item->value);
						free(curr->item);
						free(curr);
						table->count--;
						PrintTable(table);
						return;
					}
				}
				prev = curr;
				curr = curr->next;
			}

		}
	}
}

void PrintTable(HashTable * table) {
	printf("\n-------------------\n");
	for (int i = 0; i < table->size; i++) {
		if (table->items[i]) {
			printf("Index:%d, Key:%s, Value:%s", i, table->items[i]->key, table->items[i]->value);
			if (table->overflow_buckets[i]) {
				printf(" => Overflow Bucket => ");
				LinkedList * head = table->overflow_buckets[i];
				while (head) {
					printf("Key:%s, Value:%s ", head->item->key, head->item->value);
					head = head->next;
				}
			}
			printf("\n");
		}
	}
	printf("-------------------\n");
}
