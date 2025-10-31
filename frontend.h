#ifndef FRONTEND_H
#define FRONTEND_H

#include "AST_node_types.h"

typedef enum lexemeType_e {
  NUMBER_LITERAL,
  OPERATION,
  IDENTIFIER,
} lexemeType_e;

astNode_t *createIdNode(char *id);

astNode_t *createListNode(listNodeType_e listNodeType);

void addChildToListNode(astNode_t *listNode, astNode_t *child);

astNode_t *createProgramNode(char *programName, astNode_t *varDeclarationsList);

astNode_t *createVarsDefinitionNode(varType_e varsType, astNode_t *varsList);

#endif