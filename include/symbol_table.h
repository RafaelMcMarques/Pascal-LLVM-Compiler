#ifndef SYMBOL_TABLE_H 
#define SYMBOL_TABLE_H 

#include <stdlib.h>
#include "ast.h"

// describes a function parameter
typedef struct parameter_t {
  varType_e type;
  int byReference;
} parameter_t;

// describes a symbol
typedef struct symbol_t {
  char *srcName;
  int isGlobal;
  idType_e symbType;
  varType_e returnType;

  // only if variable
  int isPtr;

  // only if function
  int paramCount;
  parameter_t* params[100];
} symbol_t;

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



void initGlobalSymTab(); 

void initLocalSymTab();

void destroyGlobalSymTab();

void destroyLocalSymTab();

void printSymbol(symbol_t *symb);

void insertSymbolGlobalScope(symbol_t *symb);

int isScopeLocal();

void insertSymbolCurrScope(symbol_t *symb);

symbol_t *getSymbol(char *id);

symbol_t *getGlobalSymbol(char *id);

symbol_t *createFunctionSymbol(subprogramHeaderNode_t *headerNode);

#endif