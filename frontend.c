#include "frontend.h"
#include "AST_node_types.h"

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

astNode_t *createProgramNode(char *programName, astNode_t *varDeclarationsList,
                             astNode_t *subprograms, astNode_t *codeBlock) {
  astNode_t *node = malloc(sizeof(astNode_t));
  programNode_t *programNode = malloc(sizeof(programNode_t));

  programNode->programName = programName;
  programNode->varDeclarationList = varDeclarationsList;
  programNode->subprogramDeclarationList = subprograms;
  programNode->mainCodeBlock = codeBlock;

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

astNode_t *createIdNode2(char *id, idType_e type) {
  astNode_t *node = malloc(sizeof(astNode_t));
  idNode_t *idNode = malloc(sizeof(idNode_t));

  idNode->id = id;
  idNode->idType = type;

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

astNode_t *createParamDefNode(varType_e varType, int byReference,
                              astNode_t *idsList) {
  astNode_t *node = malloc(sizeof(astNode_t));
  paramDefNode_t *paramDefNode = malloc(sizeof(paramDefNode_t));

  paramDefNode->byReference = byReference;
  paramDefNode->varType = varType;
  paramDefNode->idsList = idsList;

  node->nodeType = PARAMETER_DEF_NODE;
  node->payload.paramDefNode = paramDefNode;

  return node;
}

astNode_t *createSubprogramHeaderNode(astNode_t *id, varType_e returnType,
                                      astNode_t *params) {

  astNode_t *node = malloc(sizeof(astNode_t));
  subprogramHeaderNode_t *subprogramHeaderNode =
      malloc(sizeof(subprogramHeaderNode_t));

  subprogramHeaderNode->id = id;
  subprogramHeaderNode->returnType = returnType;
  subprogramHeaderNode->params = params;

  node->nodeType = SUBPROGRAM_HEADER_NODE;
  node->payload.subprogramHeader = subprogramHeaderNode;

  return node;
}

astNode_t *createSubprogramDefNode(astNode_t *header, astNode_t *declarations,
                                   astNode_t *codeBlock) {
  astNode_t *node = malloc(sizeof(astNode_t));
  subprogramDefNode_t *subprogramDefNode = malloc(sizeof(subprogramDefNode_t));

  subprogramDefNode->declarations = declarations;
  subprogramDefNode->codeBlock = codeBlock;
  subprogramDefNode->header = header;

  node->nodeType = SUBPROGRAM_DEF_NODE;
  node->payload.subprogramDef = subprogramDefNode;

  return node;
}

astNode_t *createAssignmentNode(astNode_t *variable, astNode_t *exp) {
  astNode_t *node = malloc(sizeof(astNode_t));
  assignmentNode_t *assignmentNode = malloc(sizeof(assignmentNode_t));

  assignmentNode->variable = variable;
  assignmentNode->exp = exp;

  node->nodeType = ASSIGNMENT_NODE;
  node->payload.assignment = assignmentNode;

  return node;
}

astNode_t *createFunctionCallNode(astNode_t *id, int hasArguments,
                                  astNode_t *arguments) {

  astNode_t *node = malloc(sizeof(astNode_t));
  functionCallNode_t *functionCallNode = malloc(sizeof(functionCallNode_t));

  functionCallNode->id = id;
  functionCallNode->hasArguments = hasArguments;
  functionCallNode->arguments = arguments;

  node->nodeType = FUNCTION_CALL_NODE;
  node->payload.functionCall = functionCallNode;

  return node;
}

astNode_t *wrap_node(astNode_t *wrapped, listNodeType_e type) {
  // already wrapped
  if (wrapped->nodeType == LIST_NODE) {
    return wrapped;
  }

  astNode_t *listNode = malloc(sizeof(astNode_t));
  listNode = createListNode(type);

  addChildToListNode(listNode, wrapped);
  return listNode;
}

astNode_t *createIfNode(astNode_t *condition, astNode_t *thenBlock,
                        astNode_t *elseBlock) {

  astNode_t *node = malloc(sizeof(astNode_t));
  ifNode_t *ifNode = malloc(sizeof(ifNode_t));

  ifNode->condition = condition;
  ifNode->thenBlock = wrap_node(thenBlock, STATEMENT_LIST);
  ifNode->elseBlock = wrap_node(elseBlock, STATEMENT_LIST);

  node->nodeType = IF_NODE;
  node->payload.ifNode = ifNode;

  return node;
}

astNode_t *createWhileNode(astNode_t *condition, astNode_t *codeBlock) {

  astNode_t *node = malloc(sizeof(astNode_t));
  whileNode_t *whileNode = malloc(sizeof(whileNode_t));

  whileNode->condition = condition;
  whileNode->codeBlock = wrap_node(codeBlock, STATEMENT_LIST);

  node->nodeType = WHILE_NODE;
  node->payload.whileNode = whileNode;

  return node;
}