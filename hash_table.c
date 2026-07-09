/**
 * Implementation of the hash_table.h prototypes. Module for a hash table with basic functions
 * such as getting and insertion. Handles collisions by chaining. 
 *
 * @author Garret Wilson
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hash_table.h"


/*
 * Rolling hash function to turn a key into an index value
 *
 * @param key: the string to hash
 * @return index for the key
 */
unsigned int hash(char* key) {
  unsigned int h = 0;

  while (*key) {
    h = PRIME * h + (*key);
    key++;
  }

  int index = h % SIZE;

  return index;
}


/*
 * Adds an entry to the hash table, using chaining if an index is currently occupied.
 * If the key already exists in the hash table, replace it.
 *
 * @param ht: hash table to add to
 * @param key: key for node
 * @param val: value for node
 * @return 0 if successsful new add, 1 if replacement
 */
int hash_table_add(HashTable* ht, char* key, double val) {
  unsigned int index = hash(key);

  // non-occupied index
  if (ht->buckets[index] == NULL) {
    ht->buckets[index] = malloc(sizeof(Node));

    ht->buckets[index]->key = strdup(key);
    ht->buckets[index]->value = val;
    ht->buckets[index]->next = NULL;
    ht->size++;

    return 0;
  }

  // occupied index
  Node* curr_node = ht->buckets[index];
  while (curr_node) {
    if (strcmp(curr_node->key, key) == 0) {
      curr_node->value = val;
      return 1;
    }
    curr_node = curr_node->next;
  }

  // create new node
  Node* new_node = malloc(sizeof(Node));
  new_node->key = strdup(key);
  new_node->value = val;

  // append to front of list
  new_node->next = ht->buckets[index];
  ht->buckets[index] = new_node;

  ht->size++;

  return 0;
}


/*
 * Get the value associated with a key from the hash table.
 *
 * @param key: key whos value to find
 * @return node value if entry exists, -1 if entry does not exist
 */
double hash_table_get(HashTable* ht, char* key) {
  unsigned int index = hash(key);
  Node* curr_node = ht->buckets[index];

  // if empty bucket, return -1.0
  if (curr_node == NULL) { return -1.0; }

  // walk down list
  while (curr_node) {
    // if found, return value
    if (strcmp(curr_node->key, key) == 0) {
      return curr_node->value;
    }

    curr_node = curr_node->next;
  }

  // wasn't found in list mapped to hash value
  return -1.0;
}


/*
 * Set the value of an entry in the hash table.
 *
 * @param ht: hash table to add to
 * @param key: key for node
 * @param val: value for node
 * @return 0 if successsful set, -1 if key does not exist
 */
int hash_table_set(HashTable* ht, char* key, double new_val) {
  // if entry doesnt exist, get out
  if (hash_table_get(ht, key) == -1) { return -1; }

  unsigned int index = hash(key);

  Node* curr_node = ht->buckets[index];
  while(curr_node) {
    if (strcmp(curr_node->key, key) == 0) {
      curr_node->value = new_val;
      return 0;
    }

    curr_node = curr_node->next;
  }

  return -1;
}


/*
 * Visualize the hash table with an easy print to console
 *
 * @param ht: hash table to be printed
 */
void hash_table_print(HashTable* ht, FILE* file) {
  // go through each bucket
  for (int i=0; i < SIZE; i++) {
    Node* curr_node = ht->buckets[i];

    if (curr_node == NULL) { continue; }

    // print a buckets list
    while (curr_node) {
      fprintf(file, "key: %s, value: %lf", curr_node->key, curr_node->value);
      fprintf(file, "%s", " | ");
      curr_node = curr_node->next;
    }

    fprintf(file, "%s", "\n");
  }

  fflush(file);
}


