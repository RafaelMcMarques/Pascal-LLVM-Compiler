#include "ast.h"
#include "AST_node_types.h"

#include <stdarg.h>
#include <stdio.h>

int PrintCodeBlock = 1;

int depth = 0;
FILE *file = NULL;

void printRec(astNode_t *node);

void myPrint(const char *fmt, ...) {
  // printf("Called myprint\n");
  for (int i = 0; i < depth; i++) {
    if (file)
      fprintf(file, "\t\t");
    else
      printf("\t");
  }

  va_list args;
  va_start(args, fmt);
  if (file) {
    vfprintf(file, fmt, args);
    fflush(file);
  } else {
    vprintf(fmt, args);
  }
  va_end(args);
}

void printListNode(astNode_t *node) {

  listNode_t *listNode = node->payload.list;

  switch (listNode->listNodeType) {
  case STATEMENT_LIST:
    myPrint("STATEMENT LIST");
    break;
  case IDS_LIST:
    myPrint("IDS LIST");
    break;
  case SUBPROGRAM_LIST:
    myPrint("SUBPROGRAM LIST");
    break;
  case VARS_DEFINITIONS_LIST:
    myPrint("VAR DEFS LIST");
    break;
  case PARAM_DEF_LIST:
    myPrint("PARAM DEFS LIST");
    break;
  default:
    myPrint("Unknown list type node");
    break;
  }
  myPrint("(%d)\n", listNode->childrenCount);
  depth++;
  for (int i = 0; i < listNode->childrenCount; i++) {
    if (!listNode->children[i]) {
      myPrint("EMPTY CHILD NODE\n");
      continue;
    }
    printRec(listNode->children[i]);
  }
  depth--;
}

void printProgramNode(astNode_t *node) {
  programNode_t *programNode = node->payload.program;

  myPrint("PROGRAM\n");
  myPrint("name: %s\n", programNode->programName);

  depth++;
  printRec(programNode->varDeclarationList);
  printRec(programNode->subprogramDeclarationList);
  printRec(programNode->mainCodeBlock);
  depth--;
}

void printVarsDefNode(astNode_t *node) {
  varsDefinitionNode_t *varsDefNode = node->payload.varsDefinition;

  myPrint("VARS DEFINITION\n");
  myPrint("vars type: %d\n", varsDefNode->varsType);

  depth++;
  printRec(varsDefNode->varsList);
  depth--;
}

// folha
void printIdNode(astNode_t *node) {
  idNode_t *idNode = node->payload.id;

  myPrint("ID\n");

  depth++;
  myPrint(idNode->idType == VARIABLE_ID ? "type: variable\n"
                                        : "type: function\n");
  myPrint("id: %s\n", idNode->id);
  depth--;
}

void printParameterDefNode(astNode_t *node) {
  paramDefNode_t *paramDefNode = node->payload.paramDefNode;

  myPrint("PARAM DEF\n");

  depth++;
  myPrint("varType: %d\n", paramDefNode->varType);
  myPrint("byReference: %d\n", paramDefNode->byReference);

  printRec(paramDefNode->idsList);
  depth--;
}

void printSubprogramHeaderNode(astNode_t *node) {
  subprogramHeaderNode_t *headerNode = node->payload.subprogramHeader;

  myPrint("SUBPROGRAM HEADER\n");

  depth++;
  myPrint("return type: %d\n", headerNode->returnType);

  printRec(headerNode->id);
  printRec(headerNode->params);
  depth--;
}

void printSubprogramDefNode(astNode_t *node) {
  subprogramDefNode_t *subpNode = node->payload.subprogramDef;

  myPrint("SUBPROGRAM DECLARATION\n");

  depth++;
  printRec(subpNode->header);
  printRec(subpNode->declarations);
  printRec(subpNode->codeBlock);
  depth--;
}

void printAssignmentNode(astNode_t *node) {
  assignmentNode_t *asNode = node->payload.assignment;

  myPrint("ASSIGNMENT\n");

  depth++;
  printRec(asNode->variable);
  printRec(asNode->exp);
  depth--;
}

void printFunctionCallNode(astNode_t *node) {
  functionCallNode_t *fNode = node->payload.functionCall;

  myPrint("FUNCTION CALL\n");

  depth++;
  printRec(fNode->id);

  if (fNode->hasArguments) {
    printRec(fNode->arguments);
  } else {
    myPrint("arguments: no\n");
  }
  depth--;
}

void printCodeBlockNode(astNode_t *node) {
  myPrint("CODE BLOCK\n");

  if (!PrintCodeBlock) {
    return;
  }

  codeBlockNode_t *cbNode = node->payload.codeBlock;

  depth++;
  printRec(cbNode->satatementList);
  depth--;
}

void printIfNode(astNode_t *node) {
  ifNode_t *ifNode = node->payload.ifNode;

  myPrint("IF STATEMENT\n");

  depth++;
  printRec(ifNode->condition);
  printRec(ifNode->thenBlock);
  printRec(ifNode->elseBlock);
  depth--;
}

void printWhileNode(astNode_t *node) {
  whileNode_t *whileNode = node->payload.whileNode;

  myPrint("WHILE STATEMENT\n");

  depth++;
  printRec(whileNode->condition);
  printRec(whileNode->codeBlock);
  depth--;
}

void printRec(astNode_t *node) {
  if (!node) {
    myPrint("NULL NODE\n");
    return;
  }

  switch (node->nodeType) {

  case LIST_NODE:
    printListNode(node);
    break;
  case PROGRAM_NODE:
    printProgramNode(node);
    break;
  case VARS_DEFINITION_NODE:
    printVarsDefNode(node);
    break;
  case ID_NODE:
    printIdNode(node);
    break;
  case PARAMETER_DEF_NODE:
    printParameterDefNode(node);
    break;
  case SUBPROGRAM_HEADER_NODE:
    printSubprogramHeaderNode(node);
    break;
  case SUBPROGRAM_DEF_NODE:
    printSubprogramDefNode(node);
    break;
  case ASSIGNMENT_NODE:
    printAssignmentNode(node);
    break;
  case FUNCTION_CALL_NODE:
    printFunctionCallNode(node);
    break;
  case CODE_BLOCK_NODE:
    printCodeBlockNode(node);
    break;
  case IF_NODE:
    printIfNode(node);
    break;
  case WHILE_NODE:
    printWhileNode(node);
    break;
  default:
    myPrint("UNKNOWN NODE\n");
    break;
  }
}

void printAST(astNode_t *root, char *file_name) {
  if (file_name) {
    file = fopen(file_name, "a"); // open file for writing
    depth = 0;
    if (file == NULL) {
      printf("Error opening file.\n");
      exit(1);
    }
  }

  depth = 0;
  printRec(root);
  if (file)
    fclose(file);
}