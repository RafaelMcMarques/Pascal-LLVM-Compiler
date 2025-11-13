#include "backend.h"

#include "AST_node_types.h"
#include "data_structures.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int PrintCodeBlock = 1;

static int depth = 0;
static FILE *file = NULL;

hashTable_t *GlobalSymTable = NULL, *LocalSymTable = NULL;

void resolveToLLVM(astNode_t *node);

int isScopeLocal() { return LocalSymTable != NULL; }

char *getVarTypeLLVM(varType_e varType) {
  char *type;
  switch (varType) {
  case (INT_TYPE):
    type = "i32";
    break;
  case (REAL_TYPE):
    type = "float";
    break;
  case (NO_TYPE):
    type = "void";
    break;
  default:
    type = "errorType";
    break;
  }
  return type;
}

hashTable_t *getCurrentSymTab() {
  if (isScopeLocal())
    return (hashTable_t *)LocalSymTable;

  return GlobalSymTable;
}

static void myPrint(const char *fmt, ...) {
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

static void myPrintSL(const char *fmt, ...) {
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

void resolveListNode(astNode_t *node) {

  listNode_t *listNode = node->payload.list;

  switch (listNode->listNodeType) {
  case STATEMENT_LIST:
    myPrint("STATEMENT LIST (%d)\n", listNode->childrenCount);
    if (!PrintCodeBlock)
      return;
    break;
  case IDS_LIST:
    myPrint("IDS LIST (%d)\n", listNode->childrenCount);
    break;
  case SUBPROGRAM_LIST:
    myPrint("SUBPROGRAM LIST (%d)\n", listNode->childrenCount);
    break;
  case VARS_DEFINITIONS_LIST:
    myPrint("VAR DEFS LIST (%d)\n", listNode->childrenCount);
    break;
  case PARAM_DEF_LIST:
    myPrint("PARAM DEFS LIST (%d)\n", listNode->childrenCount);
    break;
  case EXPRESSION_LIST:
    myPrint("EXPRESSION LIST (%d)\n", listNode->childrenCount);
    break;
  default:
    myPrint("Unknown list type node\n");
    break;
  }

  depth++;
  for (int i = 0; i < listNode->childrenCount; i++) {
    if (!listNode->children[i]) {
      myPrint("EMPTY CHILD NODE\n");
      continue;
    }
    resolveToLLVM(listNode->children[i]);
  }
  depth--;
}

void resolveProgramNode(astNode_t *node) {
  programNode_t *programNode = node->payload.program;

  myPrint("PROGRAM\n");
  myPrint("name: %s\n", programNode->programName);

  depth++;
  resolveToLLVM(programNode->varDeclarationList);
  resolveToLLVM(programNode->subprogramDeclarationList);
  resolveToLLVM(programNode->mainCodeBlock);
  depth--;
}

void insertSymbolCurrScope(symbol_t *symb) {
  hashTable_t *currSymTab = getCurrentSymTab();

  if (!insertHT(currSymTab, symb->srcName, (void *)symb)) {
    printf("ERROR: possible redefinition of %s\n", symb->srcName);
    exit(1);
  }
}

void declareVariable(symbol_t *symb) {
  char *name = symb->srcName;
  char *type = getVarTypeLLVM(symb->returnType);

  if (isScopeLocal()) {
    myPrint("%%%s = alloca %s;\n", name, type);
  } else {
    myPrint("@%s = global %s;\n", name, type);
  }
}

// leaf
void resolveVarsDefNode(astNode_t *node) {
  varsDefinitionNode_t *varsDefNode = node->payload.varsDefinition;

  myPrint("VARS DEFINITION\n");

  depth++;
  listNode_t *varsList = varsDefNode->varsList->payload.list;
  for (int i = 0; i < varsList->childrenCount; i++) {
    idNode_t *idNode = varsList->children[i]->payload.id;
    symbol_t *symb = malloc(sizeof(symbol_t));

    symb->srcName = strdup(idNode->id);
    symb->returnType = varsDefNode->varsType;
    symb->symbType = VARIABLE_ID;
    symb->isPtr = 1;

    insertSymbolCurrScope(symb);
    declareVariable(symb);
  }
  depth--;
}

// folha
void resolveIdNode(astNode_t *node) {
  idNode_t *idNode = node->payload.id;

  myPrint("ID\n");

  depth++;
  switch (idNode->idType) {
  case VARIABLE_ID:
    myPrint("type: variable\n");
    break;
  case FUNCTION_ID:
    myPrint("type: function\n");
    break;
  case UNKNOWN:
    myPrint("type: unkown\n");
    break;
  }
  myPrint("id: %s\n", idNode->id);
  depth--;
}

void resolveParameterDefNode(astNode_t *node) {
  paramDefNode_t *paramDefNode = node->payload.paramDefNode;

  varType_e type = paramDefNode->varType;
  char *typeStr = getVarTypeLLVM(type);
  int byRef = paramDefNode->byReference;

  listNode_t *idList = paramDefNode->idsList->payload.list;

  depth++;
  char *separator = "";
  for (int i = 0; i < idList->childrenCount; i++) {
    idNode_t *idNode = idList->children[i]->payload.id;
    symbol_t *symb = malloc(sizeof(symbol_t));

    // add to symbTab
    symb->srcName = idNode->id;
    symb->symbType = VARIABLE_ID;
    symb->returnType = type;
    symb->isPtr = byRef;
    insertSymbolCurrScope(symb);

    if (byRef) {
      myPrintSL("%s%s* %%%s", separator, typeStr, symb->srcName);
    } else {
      myPrintSL("%s%s %%%s", separator, typeStr, symb->srcName);
    }
    separator = ", ";
  }
  depth--;
}

void resolveHeaderParams(listNode_t *paramsList) {
  char *separator = "";
  myPrintSL("(");
  for (int i = 0; i < paramsList->childrenCount; i++) {
    myPrintSL("%s", separator);
    resolveParameterDefNode(paramsList->children[i]);
    separator = ", ";
  }
  myPrintSL(")\n");
}

void resolveSubprogramHeaderNode(astNode_t *node) {
  subprogramHeaderNode_t *headerNode = node->payload.subprogramHeader;

  myPrint("SUBPROGRAM HEADER\n");
  depth++;

  // insert int symtable
  symbol_t *symb = malloc(sizeof(symbol_t));
  symb->srcName = headerNode->id->payload.id->id;
  symb->returnType = headerNode->returnType;
  if (!insertHT(GlobalSymTable, symb->srcName, symb)) {
    printf("ERROR: possible redeclaration of %s\n", symb->srcName);
    exit(1);
  }

  // declare
  char *type = getVarTypeLLVM(headerNode->returnType);
  myPrint("define %s @%s", type, symb->srcName);

  // params
  resolveHeaderParams(headerNode->params->payload.list);

  depth--;
}

void resolveSubprogramDefNode(astNode_t *node) {
  subprogramDefNode_t *subpNode = node->payload.subprogramDef;

  myPrint("SUBPROGRAM DECLARATION\n");

  LocalSymTable = createHT(0);

  depth++;
  resolveToLLVM(subpNode->header);
  resolveToLLVM(subpNode->declarations);
  resolveToLLVM(subpNode->codeBlock);
  depth--;

  destroyHT(LocalSymTable);
}

void resolveAssignmentNode(astNode_t *node) {
  assignmentNode_t *asNode = node->payload.assignment;

  myPrint("ASSIGNMENT\n");

  depth++;
  resolveToLLVM(asNode->variable);
  resolveToLLVM(asNode->exp);
  depth--;
}

void resolveFunctionCallNode(astNode_t *node) {
  functionCallNode_t *fNode = node->payload.functionCall;

  myPrint("FUNCTION CALL\n");

  depth++;
  resolveToLLVM(fNode->id);

  if (fNode->hasArguments) {
    resolveToLLVM(fNode->arguments);
  } else {
    myPrint("arguments: no\n");
  }
  depth--;
}

void resolveIfNode(astNode_t *node) {
  ifNode_t *ifNode = node->payload.ifNode;

  myPrint("IF STATEMENT\n");

  depth++;
  resolveToLLVM(ifNode->condition);
  resolveToLLVM(ifNode->thenBlock);
  resolveToLLVM(ifNode->elseBlock);
  depth--;
}

void resolveWhileNode(astNode_t *node) {
  whileNode_t *whileNode = node->payload.whileNode;

  myPrint("WHILE STATEMENT\n");

  depth++;
  resolveToLLVM(whileNode->condition);
  resolveToLLVM(whileNode->codeBlock);
  depth--;
}

void resolveExpressionNode(astNode_t *node) {
  expressionNode_t *expressionNode = node->payload.expression;

  myPrint("EXPRESSION\n");

  depth++;
  resolveToLLVM(expressionNode->lOperand);
  myPrint("op: %s\n", expressionNode->op);
  resolveToLLVM(expressionNode->rOperand);
  depth--;
}

void resolveLiteralNode(astNode_t *node) {
  literalNode_t *literalNode = node->payload.literal;

  myPrint("LITERAL\n");

  depth++;
  myPrint("literal: %s\n", literalNode->literal);
  depth--;
}

void resolveToLLVM(astNode_t *node) {
  if (!node) {
    myPrint("NULL NODE\n");
    return;
  }

  switch (node->nodeType) {

  case LIST_NODE:
    resolveListNode(node);
    break;
  case PROGRAM_NODE:
    resolveProgramNode(node);
    break;
  case VARS_DEFINITION_NODE:
    resolveVarsDefNode(node);
    break;
  case ID_NODE:
    resolveIdNode(node);
    break;
  case PARAMETER_DEF_NODE:
    resolveParameterDefNode(node);
    break;
  case SUBPROGRAM_HEADER_NODE:
    resolveSubprogramHeaderNode(node);
    break;
  case SUBPROGRAM_DEF_NODE:
    resolveSubprogramDefNode(node);
    break;
  case ASSIGNMENT_NODE:
    resolveAssignmentNode(node);
    break;
  case FUNCTION_CALL_NODE:
    resolveFunctionCallNode(node);
    break;
  case IF_NODE:
    resolveIfNode(node);
    break;
  case WHILE_NODE:
    resolveWhileNode(node);
    break;
  case EXPRESSION_NODE:
    resolveExpressionNode(node);
    break;
  case LITERAL_NODE:
    resolveLiteralNode(node);
    break;
  default:
    myPrint("UNKNOWN NODE\n");
    break;
  }
}

void resolveASTtoLLVM(astNode_t *root) {
  file = fopen("a.out", "w"); // open file for writing
  depth = 0;
  if (file == NULL) {
    printf("Error opening file.\n");
    exit(1);
  }

  depth = 0;
  GlobalSymTable = createHT(0);
  resolveToLLVM(root);
  if (file)
    fclose(file);
}