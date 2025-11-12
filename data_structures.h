#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stdlib.h>
typedef struct HashNode {
  char *key;
  void *value;
  struct HashNode *next;
} HashNode;

typedef struct {
  HashNode **buckets;
  size_t size;
} HashTable;

typedef struct StackNode {
  void *data;
  struct StackNode *next;
} StackNode;

typedef struct {
  StackNode *top;
} Stack;


HashTable *ht_create(size_t size);

int ht_insert(HashTable *ht, const char *key, void *value);

void *ht_get(HashTable *ht, const char *key);

int ht_remove(HashTable *ht, const char *key);

void ht_destroy(HashTable *ht);

Stack *createStack();

void push(Stack *s, void *data);

void *pop(Stack *s);

void *peek(Stack *s);


int isEmpty(Stack *s);

void freeStack(Stack *s);

#endif