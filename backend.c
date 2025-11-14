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

// to be used in function
unsigned int nextLocalReg = 0;

// to be used in main
unsigned int nextGlobalReg = 0;

void resolveToLLVM(astNode_t *node);

/* ======= PRINT HELPERS =======*/

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

void printSymbol(symbol_t *symb) {

  printf("SYMBOL %s:\n", symb->srcName);
  char *vof = symb->symbType == VARIABLE_ID ? "var" : "func";
  printf("  symbType: %s\n", vof);
  char *type = getVarTypeLLVM(symb->returnType);
  printf("  returnType: %s\n", type);

  if (symb->symbType != FUNCTION_ID) {
    printf("  isPtr: %d\n", symb->isPtr);
  } else {
    printf("  paramCount: %d\n", symb->paramCount);
    printf("  Parameters:");
    for (int i = 0; i < symb->paramCount && i < 100; i++) {
      parameter_t *p = symb->params[i];
      char *pType = getVarTypeLLVM(p->type);
      printf("%s byRef=%d, ", pType, p->byReference);
    }
    printf("\n");
  }
}

/* ====== LABEL HELPERS ====== */
char *getNextReg() {
  int number = LocalSymTable == NULL ? nextGlobalReg++ : nextLocalReg++;

  char temp[32];
  int len = snprintf(temp, sizeof(temp), "%d", number);

  char *result = malloc(len + 1);
  strcpy(result, temp);
  return result;
}

/* ===== TAB SYMB HELPERS ====== */

int isScopeLocal() { return LocalSymTable != NULL; }

void insertSymbolGlobalScope(symbol_t *symb) {
  if (!insertHT(GlobalSymTable, symb->srcName, symb)) {
    printf("ERROR: possible redeclaration of %s\n", symb->srcName);
    exit(1);
  }
}

hashTable_t *getCurrentSymTab() {
  if (isScopeLocal())
    return (hashTable_t *)LocalSymTable;

  return GlobalSymTable;
}

// inserts a symbol in current scope (use only for variables)
void insertSymbolCurrScope(symbol_t *symb) {
  hashTable_t *currSymTab = getCurrentSymTab();

  if (!insertHT(currSymTab, symb->srcName, (void *)symb)) {
    printf("ERROR: possible redefinition of %s\n", symb->srcName);
    exit(1);
  }
}

/* ======= LIST ITERATOR =======*/

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

/* ===== PROGRAM NODE ===== */
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

/* ===== VARIABLE DECLARATIONS ===== */

void declareVariableLLVM(symbol_t *symb) {
  char *name = symb->srcName;
  char *type = getVarTypeLLVM(symb->returnType);

  if (isScopeLocal()) {
    myPrint("%%%s = alloca %s;\n", name, type);
  } else {
    myPrint("@%s = global %s;\n", name, type);
  }
}

// add vars to symTab and call declareVariableLLVM
// this is wrapped by printListNode
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
    symb->isGlobal = !isScopeLocal();
    symb->symbType = VARIABLE_ID;
    symb->isPtr = 1;

    insertSymbolCurrScope(symb);
    declareVariableLLVM(symb);
  }
  depth--;
}

/* ====== FUNCTIONS ====== */

// add parameters to sym tab and generate llvm
void resolveParameterDefNode(astNode_t *node) {
  paramDefNode_t *paramDefNode = node->payload.paramDefNode;

  varType_e type = paramDefNode->varType;
  char *typeStr = getVarTypeLLVM(type);
  int byRef = paramDefNode->byReference;

  listNode_t *idList = paramDefNode->idsList->payload.list;

  char *separator = "";
  for (int i = 0; i < idList->childrenCount; i++) {
    idNode_t *idNode = idList->children[i]->payload.id;
    symbol_t *symb = malloc(sizeof(symbol_t));

    // add to symbTab
    symb->srcName = idNode->id;
    symb->symbType = VARIABLE_ID;
    symb->returnType = type;
    symb->isPtr = byRef;
    symb->isGlobal = 0;
    insertSymbolCurrScope(symb);

    if (byRef) {
      myPrintSL("%s%s* %%%s", separator, typeStr, symb->srcName);
    } else {
      myPrintSL("%s%s %%%s", separator, typeStr, symb->srcName);
    }
    separator = ", ";
  }
}

// register params in SymTab and generate LLVM
void resolveHeaderParamsList(listNode_t *paramsList) {
  char *separator = "";
  for (int i = 0; i < paramsList->childrenCount; i++) {
    myPrintSL("%s", separator);
    resolveParameterDefNode(paramsList->children[i]);
    separator = ", ";
  }
}

// returns symbol for a function (name, type, params...)
symbol_t *getFunctionSymbol(subprogramHeaderNode_t *headerNode) {
  symbol_t *symb = malloc(sizeof(symbol_t));
  symb->srcName = headerNode->id->payload.id->id;
  symb->returnType = headerNode->returnType;
  symb->symbType = FUNCTION_ID;
  symb->isGlobal = 1;

  // params
  listNode_t *paramsList = headerNode->params->payload.list;
  int paramCount = 0;
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

// generate code and symbol for function declaration
void resolveSubprogramHeaderNode(astNode_t *node) {
  subprogramHeaderNode_t *headerNode = node->payload.subprogramHeader;

  myPrint("SUBPROGRAM HEADER\n");

  // insert into symtable
  symbol_t *symb = getFunctionSymbol(headerNode);
  insertSymbolGlobalScope(symb);

  // declare in LLVM
  char *type = getVarTypeLLVM(headerNode->returnType);
  myPrint("define %s @%s", type, symb->srcName);

  // declar params in LLVM and add them to local symTable
  myPrintSL("(");
  resolveHeaderParamsList(headerNode->params->payload.list);
  myPrintSL(") ");
}

void resolveSubprogramDefNode(astNode_t *node) {
  subprogramDefNode_t *subpNode = node->payload.subprogramDef;

  myPrint("SUBPROGRAM DECLARATION\n");

  LocalSymTable = createHT(0);
  nextLocalReg = 0;

  resolveToLLVM(subpNode->header);
  myPrintSL("{\n");

  depth++;
  resolveToLLVM(subpNode->declarations);
  resolveToLLVM(subpNode->codeBlock);
  depth--;

  myPrint("}\n");

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

/* ====== EXPRESSIONS =====*/

symbol_t *resolveLiteralToLLVM(astNode_t *node) {
  symbol_t *symb = malloc(sizeof(symbol_t));
  char *literal = node->payload.literal->literal;

  symb->srcName = getNextReg();
  symb->isGlobal = 0;
  symb->symbType = VARIABLE_ID;
  symb->isPtr = 0;

  // get type
  symb->returnType = INT_TYPE;
  for (const char *p = literal; *p; p++) {
    if (*p == '.' || *p == 'e' || *p == 'E')
      symb->returnType = REAL_TYPE;
  }

  char *type = getVarTypeLLVM(symb->returnType);
  myPrint("%%%s = add %s %s, 0\n", symb->srcName, type, literal);

  return symb;
}

// forward declaration
symbol_t *resolveExpressionNode(astNode_t *);

symbol_t *expressionResolutionWrapper(astNode_t *node) {
  if (!node)
    return NULL;
  switch (node->nodeType) {
  case EXPRESSION_NODE:
    return resolveExpressionNode(node);
    break;
  case ID_NODE:
    return NULL;
    break;
  case FUNCTION_CALL_NODE:
    resolveFunctionCallNode(node);
    return NULL;
    break;
  case LITERAL_NODE:
    return resolveLiteralToLLVM(node);
    break;
  default:
    return NULL;
    break;
  }
}

symbol_t *resolveExpressionNode(astNode_t *node) {
  expressionNode_t *expressionNode = node->payload.expression;

  myPrint("EXPRESSION\n");

  depth++;
  symbol_t *lOp = expressionResolutionWrapper(expressionNode->lOperand);
  myPrint("op: %s\n", expressionNode->op);
  symbol_t *rOp = expressionResolutionWrapper(expressionNode->rOperand);
  depth--;

  return NULL;
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