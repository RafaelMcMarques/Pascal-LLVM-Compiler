#include "frontend.h"

#include <stdio.h>
#include <stdlib.h>

void printASTRoot(astNode_t *root) {
  printf("Printing ast root\n");
  programNode_t *program = root->payload.program;
  printf("Program name: %s\n", program->programName);

  printf("Global vars: \n");
  listNode_t *varsDecList = program->varDeclarationList->payload.list;
  for (int i = 0; i < varsDecList->childrenCount; i++) {
    varsDefinitionNode_t *varsDec =
        varsDecList->children[i]->payload.varsDefinition;

    printf("vars of type %d: ", varsDec->varsType);
    listNode_t *idsList = varsDec->varsList->payload.list;
    for (int j = 0; j < idsList->childrenCount; j++) {
      printf("%s ", idsList->children[j]->payload.id->id);
    }
  }
  printf("\n");
}

astNode_t *createProgramNode(char *programName,
                             astNode_t *varDeclarationsList) {
  astNode_t *node = malloc(sizeof(astNode_t));
  programNode_t *programNode = malloc(sizeof(programNode_t));

  programNode->programName = programName;
  programNode->varDeclarationList = varDeclarationsList;

  node->nodeType = PROGRAM_NODE;
  node->payload.program = programNode;

  printf("AST Done\n");
  printASTRoot(node);

  return node;
}

astNode_t *createIdNode(char *id) {
  astNode_t *node = malloc(sizeof(astNode_t));
  idNode_t *idNode = malloc(sizeof(idNode_t));

  idNode->id = id;

  node->nodeType = ID_NODE;
  node->payload.id = idNode;

  return node;
}

astNode_t *createListNode(listNodeType_e listNodeType) {
  astNode_t *node = malloc(sizeof(astNode_t));
  listNode_t *listNode = malloc(sizeof(listNode_t));

  listNode->childrenCount = 0;
  listNode->listNodeType = listNodeType;

  node->nodeType = LIST_NODE;
  node->payload.list = listNode;

  return node;
}

void addChildToListNode(astNode_t *listNode, astNode_t *child) {
  printf("Adding child to list\n");
  if (listNode->nodeType != LIST_NODE) {
    printf("Wrong node type passed to addChildToListNode\n");
    exit(1);
  }

  listNode_t *list = listNode->payload.list;

  // TODO: NENHUMA seguranca aqui
  list->children[list->childrenCount] = child;
  list->childrenCount++;
}

astNode_t *createVarsDefinitionNode(varType_e varsType, astNode_t *varsList) {
  astNode_t *node = malloc(sizeof(astNode_t));
  varsDefinitionNode_t *varsDefNode = malloc(sizeof(varsDefinitionNode_t));

  varsDefNode->varsType = varsType;
  varsDefNode->varsList = varsList;

  node->nodeType = VARS_DEFINITION_NODE;
  node->payload.varsDefinition = varsDefNode;

  return node;
}