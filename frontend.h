#ifndef FRONTEND_H
#define FRONTEND_H

#include "AST_node_types.h"

typedef enum lexemeType_e {
  NUMBER_LITERAL,
  OPERATION,
  IDENTIFIER,
} lexemeType_e;

astNode_t *createIdNode(char *id);

astNode_t *createIdNode2(char *id, idType_e type);

astNode_t *createListNode(listNodeType_e listNodeType);

void addChildToListNode(astNode_t *listNode, astNode_t *child);

astNode_t *createProgramNode(char *programName, astNode_t *varDeclarationsList, astNode_t *subprogramList, astNode_t *codeBlock);

astNode_t *createVarsDefinitionNode(varType_e varsType, astNode_t *varsList);

astNode_t *createParamDefNode(varType_e varType, int byReference,
                              astNode_t *idsList);


astNode_t *createSubprogramHeaderNode(astNode_t *id, varType_e returnType,
                                      astNode_t *params);

astNode_t *createSubprogramDefNode(astNode_t *header, astNode_t *declarations,
                                   astNode_t *codeBlock);


astNode_t *createAssignmentNode(astNode_t *variable, astNode_t *exp);


astNode_t *createFunctionCallNode(astNode_t *id, int hasArguments,
                                  astNode_t *arguments);


astNode_t *createIfNode(astNode_t *condition, astNode_t *thenBlock,
                        astNode_t *elseBlock);

astNode_t *createWhileNode(astNode_t *condition, astNode_t *codeBlock);

#endif