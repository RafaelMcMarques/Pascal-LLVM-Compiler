#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stdlib.h>
#include "AST_node_types.h"
typedef struct hashNode_t {
  char *key;
  void *value;
  struct hashNode_t *next;
} hashNode_t;

typedef struct {
  hashNode_t **buckets;
  size_t size;
} hashTable_t;

typedef struct stackNode_t {
  void *data;
  struct stackNode_t *next;
} stackNode_t;

typedef struct stack_t{
  stackNode_t *top;
} stack_t;

typedef struct parameter_t {
  varType_e type;
  int byReference;
} parameter_t;

typedef struct symbol_t {
  char *srcName; // name in pascal
  int isGlobal;
  idType_e symbType;
  varType_e returnType;

  // only if var
  int isPtr;

  // only if function
  int paramCount;
  parameter_t* params[100];
} symbol_t;


hashTable_t *createHT(size_t size);

int insertHT(hashTable_t *ht, const char *key, void *value);

void *getFromHT(hashTable_t *ht, const char *key);

int removeFromHT(hashTable_t *ht, const char *key);

void destroyHT(hashTable_t *ht);

stack_t* createStack();

void stackPush(stack_t *s, void *data);

void *stackPop(stack_t *s);

void *stackPeek(stack_t *s);

int stackIsEmpty(stack_t *s);

void freeStack(stack_t *s);

#endif