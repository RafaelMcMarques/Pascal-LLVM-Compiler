#include "ast.h"

#include <stdarg.h>
#include <stdio.h>

int depth = 0;
FILE *file;

void printRec(astNode_t *node);

void myPrint(const char *fmt, ...) {
  // printf("Called myprint\n");
  for (int i = 0; i < depth; i++) {
    fprintf(file, " ");
    // printf(" ");
  }

  va_list args;
  va_start(args, fmt);
  int result = vfprintf(file, fmt, args);
  // printf(fmt, args);
  va_end(args);
  fflush(file);
}

void printListNode(astNode_t *node) {

  listNode_t *listNode = node->payload.list;

  switch (listNode->listNodeType) {
  case CODE_BLOCK:
    myPrint("ENUNCIADO LIST");
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

  myPrint(idNode->idType == VARIABLE_ID ? "type: variable\n"
                                        : "type: function\n");
  myPrint("id: %s\n", idNode->id);
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
  default:
    myPrint("UNKNOWN NODE\n");
    break;
  }
}

void printAST(astNode_t *root) {
  file = fopen("output.txt", "w"); // open file for writing
  depth = 0;
  if (file == NULL) {
    printf("Error opening file.\n");
    exit(1);
  }

  depth = 0;
  printRec(root);
  fclose(file);
}