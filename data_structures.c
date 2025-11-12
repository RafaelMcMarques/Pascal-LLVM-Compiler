#include "data_structures.h"

#include <stdio.h>
#include <string.h>

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
HashTable *ht_create(size_t size) {
  HashTable *ht = malloc(sizeof(HashTable));
  if (!ht)
    return NULL;
  ht->size = size > 0 ? size : INITIAL_SIZE;
  ht->buckets = calloc(ht->size, sizeof(HashNode *));
  if (!ht->buckets) {
    free(ht);
    return NULL;
  }
  return ht;
}

/* --- Inserção --- */
int ht_insert(HashTable *ht, const char *key, void *value) {
  if (!ht || !key)
    return 0;
  unsigned long hash = hash_function(key) % ht->size;
  HashNode *node = ht->buckets[hash];

  /* Verifica se chave já existe */
  while (node) {
    if (strcmp(node->key, key) == 0) {
      printf("Simb já na hash!\n");
      return 0;
    }
    node = node->next;
  }

  /* Cria novo nó */
  node = malloc(sizeof(HashNode));
  if (!node)
    return 0;
  node->key = strdup(key);
  node->value = value;
  node->next = ht->buckets[hash];
  ht->buckets[hash] = node;
  return 1;
}

/* --- Busca --- */
void *ht_get(HashTable *ht, const char *key) {
  if (!ht || !key)
    return NULL;
  unsigned long hash = hash_function(key) % ht->size;
  HashNode *node = ht->buckets[hash];

  while (node) {
    if (strcmp(node->key, key) == 0)
      return node->value;
    node = node->next;
  }
  return NULL;
}

/* --- Remoção --- */
int ht_remove(HashTable *ht, const char *key) {
  if (!ht || !key)
    return 0;
  unsigned long hash = hash_function(key) % ht->size;
  HashNode *node = ht->buckets[hash], *prev = NULL;

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
void ht_destroy(HashTable *ht) {
  if (!ht)
    return;
  for (size_t i = 0; i < ht->size; i++) {
    HashNode *node = ht->buckets[i];
    while (node) {
      HashNode *tmp = node;
      node = node->next;
      free(tmp->key);
      free(tmp);
    }
  }
  free(ht->buckets);
  free(ht);
}

Stack *createStack() {
  Stack *s = malloc(sizeof(Stack));
  if (!s) {
    perror("Erro ao criar pilha");
    exit(EXIT_FAILURE);
  }
  s->top = NULL;
  return s;
}

void push(Stack *s, void *data) {
  StackNode *n = malloc(sizeof(StackNode));
  if (!n) {
    perror("Erro ao empilhar");
    exit(EXIT_FAILURE);
  }
  n->data = data;
  n->next = s->top;
  s->top = n;
}

void *pop(Stack *s) {
  if (!s->top)
    return NULL;
  StackNode *n = s->top;
  void *data = n->data;
  s->top = n->next;
  free(n);
  return data;
}

void *peek(Stack *s) {
  if (!s->top)
    return NULL;
  return s->top->data;
}

int isEmpty(Stack *s) { return s->top == NULL; }

void freeStack(Stack *s) {
  while (!isEmpty(s))
    pop(s);
  free(s);
}
