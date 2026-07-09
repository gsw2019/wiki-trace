/**
 * Prototypes, macros, and structs for building a hash table
 *
 * @author Garret Wilson
 */


#ifndef HASH_TABLE_H
#define HASH_TABLE_H


#include <stdio.h>


#define SIZE  1031
#define PRIME 37


typedef struct Node {
  char* key;
  double value;
  struct Node* next;
} Node;


typedef struct {
  Node* buckets[SIZE];
  int size;
} HashTable;


// hash function
unsigned int hash(char* key);

// operationss of hash table
int hash_table_add(HashTable* ht, char* key, double val);
double hash_table_get(HashTable* ht, char* key);
int hash_table_set(HashTable* ht, char* key, double val);

// visualize hash table
void hash_table_print(HashTable* ht, FILE* file);

#endif

