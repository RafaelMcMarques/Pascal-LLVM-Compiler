#include "../include/symbol_table.h"

#include <stdio.h>
#include <string.h>

hashTable_t *GlobalSymTable = NULL, *LocalSymTable = NULL;

#define INITIAL_SIZE 101

/* --- Função de hash (djb2) --- */
static unsigned long hash_function(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash;
}

/* --- Criação da tabela --- */
hashTable_t *createHT(size_t size) {
  hashTable_t *ht = malloc(sizeof(hashTable_t));
  if (!ht)
    return NULL;
  ht->size = size > 0 ? size : INITIAL_SIZE;
  ht->buckets = calloc(ht->size, sizeof(hashNode_t *));
  if (!ht->buckets) {
    free(ht);
    return NULL;
  }
  return ht;
}

/* --- Inserção --- */
int insertHT(hashTable_t *ht, const char *key, void *value) {
  if (!ht || !key)
    return 0;
  unsigned long hash = hash_function(key) % ht->size;
  hashNode_t *node = ht->buckets[hash];

  /* Verifica se chave já existe */
  while (node) {
    if (strcmp(node->key, key) == 0) {
      return 0;
    }
    node = node->next;
  }

  /* Cria novo nó */
  node = malloc(sizeof(hashNode_t));
  if (!node)
    return 0;
  node->key = strdup(key);
  node->value = value;
  node->next = ht->buckets[hash];
  ht->buckets[hash] = node;
  return 1;
}

/* --- Busca --- */
void *getFromHT(hashTable_t *ht, const char *key) {
  if (!ht || !key)
    return NULL;
  unsigned long hash = hash_function(key) % ht->size;
  hashNode_t *node = ht->buckets[hash];

  while (node) {
    if (strcmp(node->key, key) == 0)
      return node->value;
    node = node->next;
  }
  return NULL;
}

/* --- Remoção --- */
int removeFromHT(hashTable_t *ht, const char *key) {
  if (!ht || !key)
    return 0;
  unsigned long hash = hash_function(key) % ht->size;
  hashNode_t *node = ht->buckets[hash], *prev = NULL;

  while (node) {
    if (strcmp(node->key, key) == 0) {
      if (prev)
        prev->next = node->next;
      else
        ht->buckets[hash] = node->next;
      free(node->key);
      free(node);
      return 1;
    }
    prev = node;
    node = node->next;
  }
  return 0;
}

/* --- Destruição completa --- */
void destroyHT(hashTable_t *ht) {
  if (!ht)
    return;
  for (size_t i = 0; i < ht->size; i++) {
    hashNode_t *node = ht->buckets[i];
    while (node) {
      hashNode_t *tmp = node;
      node = node->next;
      free(tmp->key);
      free(tmp);
    }
  }
  free(ht->buckets);
  free(ht);
  ht = NULL;
}

void initGlobalSymTab() { GlobalSymTable = createHT(0); }

void initLocalSymTab() { LocalSymTable = createHT(0); }

void destroyGlobalSymTab() {
  destroyHT(GlobalSymTable);
  GlobalSymTable = NULL;
}

void destroyLocalSymTab() {
  destroyHT(LocalSymTable);
  LocalSymTable = NULL;
}

void printSymbol(symbol_t *symb) {

  printf("SYMBOL %s:\n", symb->srcName);
  char *vof = symb->symbType == VARIABLE_ID ? "var" : "func";
  printf("  symbType: %s\n", vof);
  printf("  returnType: %d\n", symb->returnType);

  if (symb->symbType != FUNCTION_ID) {
    printf("  isPtr: %d\n", symb->isPtr);
  } else {
    printf("  paramCount: %d\n", symb->paramCount);
    printf("  Parameters:");
    for (int i = 0; i < symb->paramCount && i < 100; i++) {
      parameter_t *p = symb->params[i];
      printf("Type:%d byRef=%d, ", p->type, p->byReference);
    }
    printf("\n");
  }
}

void insertSymbolGlobalScope(symbol_t *symb) {
  if (!insertHT(GlobalSymTable, symb->srcName, symb)) {
    printf("ERROR: redefinition of %s\n", symb->srcName);
    exit(1);
  }
}

int isScopeLocal() { return LocalSymTable != NULL; }

hashTable_t *getCurrentSymTab() {
  return isScopeLocal() ? LocalSymTable : GlobalSymTable;
}

// inserts a symbol in current scope (use only for variables)
void insertSymbolCurrScope(symbol_t *symb) {
  hashTable_t *currSymTab = getCurrentSymTab();

  if (!insertHT(currSymTab, symb->srcName, (void *)symb)) {
    printf("ERROR: redefinition of %s\n", symb->srcName);
    exit(1);
  }
}

symbol_t *getSymbol(char *id) {
  symbol_t *symb = NULL;

  if (isScopeLocal()) {
    symb = (symbol_t *)getFromHT(LocalSymTable, id);
  }

  if (!symb) {
    symb = (symbol_t *)getFromHT(GlobalSymTable, id);
  }

  if (!symb) {
    printf("ERROR: Undefined reference to %s\n", id);
    exit(1);
    return NULL; // wont reach
  }

  return symb;
}

symbol_t *getGlobalSymbol(char *id) {
  symbol_t *symb = NULL;

  symb = (symbol_t *)getFromHT(GlobalSymTable, id);

  if (!symb) {
    printf("ERROR: Undefined reference to %s\n", id);
    exit(1);
    return NULL; // wont reach
  }

  return symb;
}

// returns symbol for a function (name, type, params...)
symbol_t *createFunctionSymbol(subprogramHeaderNode_t *headerNode) {
  symbol_t *symb = malloc(sizeof(symbol_t));
  symb->srcName = headerNode->id->payload.id->id;
  symb->returnType = headerNode->returnType;
  symb->symbType = FUNCTION_ID;
  symb->isPtr = 0;
  symb->isGlobal = 1;

  if (!headerNode->params) {
    symb->paramCount = 0;
    return symb;
  }

  // register params
  int paramCount = 0;
  listNode_t *paramsList = headerNode->params->payload.list;
  for (int i = 0; i < paramsList->childrenCount; i++) {
    paramDefNode_t *paramDefNode =
        paramsList->children[i]->payload.paramDefNode;
    listNode_t *idsList = paramDefNode->idsList->payload.list;

    varType_e type = paramDefNode->varType;
    int byRef = paramDefNode->byReference;
    int quant = idsList->childrenCount;

    for (int j = 0; j < quant; j++) {
      parameter_t *param = malloc(sizeof(parameter_t));
      param->byReference = byRef;
      param->type = type;
      symb->params[paramCount] = param;
      paramCount++;
    }
  }

  symb->paramCount = paramCount;
  return symb;
}
