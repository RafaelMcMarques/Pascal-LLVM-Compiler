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

char *getLLVMOp(varType_e type, char *op) {
  /* --- Aritméticos --- */
  if (strcmp(op, "+") == 0)
    return (type == INT_TYPE ? "add" : "fadd");

  if (strcmp(op, "-") == 0)
    return (type == INT_TYPE ? "sub" : "fsub");

  if (strcmp(op, "*") == 0)
    return (type == INT_TYPE ? "mul" : "fmul");

  /* "/" = divisão real */
  if (strcmp(op, "/") == 0)
    return "fdiv";

  /* "div" = divisão inteira */
  if (strcmp(op, "div") == 0)
    return "sdiv";

  if (strcmp(op, "mod") == 0)
    return "srem";

  /* --- Booleanos --- */
  if (strcmp(op, "and") == 0)
    return "and";

  if (strcmp(op, "or") == 0)
    return "or";

  /* --- Comparações --- */
  if (strcmp(op, "=") == 0)
    return (type == INT_TYPE ? "icmp eq" : "fcmp oeq");

  if (strcmp(op, "<>") == 0)
    return (type == INT_TYPE ? "icmp ne" : "fcmp one");

  if (strcmp(op, ">") == 0)
    return (type == INT_TYPE ? "icmp sgt" : "fcmp ogt");

  if (strcmp(op, ">=") == 0)
    return (type == INT_TYPE ? "icmp sge" : "fcmp oge");

  if (strcmp(op, "<") == 0)
    return (type == INT_TYPE ? "icmp slt" : "fcmp olt");

  if (strcmp(op, "<=") == 0)
    return (type == INT_TYPE ? "icmp sle" : "fcmp ole");
}

varType_e getResultType(varType_e left, varType_e right, const char *op) {

  /* --- Operadores aritméticos básicos (+ - *) --- */
  if (!strcmp(op, "+") || !strcmp(op, "-") || !strcmp(op, "*")) {
    if (left == REAL_TYPE || right == REAL_TYPE)
      return REAL_TYPE; // promoção automática

    return INT_TYPE; // ambos inteiros
  }

  /* --- "/" sempre vira real --- */
  if (!strcmp(op, "/")) {
    return REAL_TYPE;
  }

  /* --- "div" e "mod" só funcionam com inteiros --- */
  if (!strcmp(op, "div") || !strcmp(op, "mod")) {
    if (left == INT_TYPE && right == INT_TYPE)
      return INT_TYPE;

    printf("Invalid div or mod for real type\n"); // div/mod em real é inválido
    exit(1);
  }

  /* --- Booleanos ("and", "or") --- */
  if (!strcmp(op, "and") || !strcmp(op, "or")) {
    return INT_TYPE; // boolean em Pascal → representado como int
  }

  /* --- Comparações: sempre retornam boolean (int) --- */
  if (!strcmp(op, "=") || !strcmp(op, "<>") || !strcmp(op, ">") ||
      !strcmp(op, ">=") || !strcmp(op, "<") || !strcmp(op, "<=")) {
    return INT_TYPE;
  }

  printf("INVALID OPERATION\n");
  exit(1);
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

symbol_t *getSymbol(char *id) {
  symbol_t *symb = NULL;

  if (isScopeLocal()) {
    printf("local\n");
    symb = (symbol_t *)getFromHT(LocalSymTable, id);
  }

  if (!symb) {
    printf("global\n");
    symb = (symbol_t *)getFromHT(GlobalSymTable, id);
  }

  if (!symb) {
    printf("Undefined reference to %s\n", id);
    exit(1);
    return NULL; // wont reach
  }

  return symb;
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
    symb->isPtr = 1;
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
  symb->isPtr = 1;
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
  char *type = getVarTypeLLVM(symb->returnType);
  myPrint("define %s @%s", type, symb->srcName);

  // declar params in LLVM and add them to local symTable
  myPrintSL("(");
  resolveHeaderParamsList(headerNode->params->payload.list);
  myPrintSL(") ");
}

// resolve a function to llvm
void resolveSubprogramDefNode(astNode_t *node) {
  subprogramDefNode_t *subpNode = node->payload.subprogramDef;

  myPrint("SUBPROGRAM DECLARATION\n");

  LocalSymTable = createHT(0);
  nextLocalReg = 0;

  resolveToLLVM(subpNode->header);
  myPrintSL("{\n");

  depth++;
  resolveToLLVM(subpNode->declarations);

  // declare return value
  char *name = subpNode->header->payload.subprogramHeader->id->payload.id->id;
  varType_e type = subpNode->header->payload.subprogramHeader->returnType;
  if (type != NO_TYPE) {
    char *strType = getVarTypeLLVM(type);
    myPrint("%%%s = alloca %s 0;\n", name, strType);
  }

  resolveToLLVM(subpNode->codeBlock);
  depth--;

  myPrint("}\n");

  destroyHT(LocalSymTable);
  LocalSymTable = NULL;
}

/* ======= STATEMENTS =======*/

// forward declaration
symbol_t *resolveExpressionNode(astNode_t *);

void resolveAssignmentNode(astNode_t *node) {
  assignmentNode_t *asNode = node->payload.assignment;

  symbol_t *exp = resolveExpressionNode(asNode->exp);
  if (!exp) {
    printf("Ainda nao implementei essa exp\n");
    return;
  }

  symbol_t *var = getSymbol(asNode->variable->payload.id->id);

  if (!isScopeLocal() && var->symbType == FUNCTION_ID) {
    printf("ERROR: cannot assign value to symbol %s\n", var->srcName);
    exit(1);
  }
  if (!var->isPtr) {
    printf("ERROR: cannot assign value to non-pointer symbol %s\n",
           var->srcName);
    exit(1);
  }

  char *typeStrE = getVarTypeLLVM(exp->returnType);
  char *typeStrV = getVarTypeLLVM(var->returnType);
  if (exp->returnType != var->returnType) {
    printf("ERROR: cannot assign %s to %s\n", typeStrE, typeStrV);
    exit(1);
  }

  if (var->isGlobal) {
    myPrint("store %s %%%s, %s* @%s\n", typeStrE, exp->srcName, typeStrV,
            var->srcName);
  } else {
    myPrint("store %s %%%s, %s* %%%s\n", typeStrE, exp->srcName, typeStrV,
            var->srcName);
  }
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

// puts a literal into a register
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

// can be called with id node or functionCallNode
symbol_t *resolveFunctionCallToLLVM(astNode_t *node) { return NULL; }

// loads variable into register or calls function and stores return value in
// register
symbol_t *resolveIdToLLVM(astNode_t *node) {
  char *id = node->payload.id->id;

  symbol_t *symb = getSymbol(id);

  symbol_t *newSymb = malloc(sizeof(symbol_t));

  // function call
  if (symb->symbType == FUNCTION_ID) {
    if (!isScopeLocal()) {
      // this is a function call without params
      return resolveFunctionCallToLLVM(node);
    } // else it is referencing the return value
  }

  // no need to load
  if (!symb->isPtr) {
    return symb;
  }

  // load to register
  newSymb->srcName = getNextReg();
  newSymb->isGlobal = 0;
  newSymb->symbType = VARIABLE_ID;
  newSymb->returnType = symb->returnType;
  newSymb->isPtr = 0;

  char *typeStr = getVarTypeLLVM(symb->returnType);

  if (symb->isGlobal) {
    myPrint("%%%s = load %s, %s* @%s\n", newSymb->srcName, typeStr, typeStr,
            symb->srcName);
  } else {
    myPrint("%%%s = load %s, %s* %%%s\n", newSymb->srcName, typeStr, typeStr,
            symb->srcName);
  }
  return newSymb;
}

// converts the type of a value in a register to match resultType
symbol_t *convertSymbToType(symbol_t *op, varType_e resultType) {
  if (op->returnType == resultType)
    return op;

  symbol_t *newS = malloc(sizeof(symbol_t));

  newS->srcName = getNextReg();
  newS->isGlobal = 0;
  newS->isPtr = 0;
  newS->returnType = resultType;
  newS->symbType = VARIABLE_ID;

  // int to real
  if (op->returnType == INT_TYPE) {
    myPrint("%%%s = sitofp i32 %%%s to float\n", newS->srcName, op->srcName);
  } else {
    myPrint("%%%s = fptosi float %%%s to int\n", newS->srcName, op->srcName);
  }

  return newS;
}

// generates code for operations
symbol_t *resolveOperationToLLVM(symbol_t *lOp, char *op, symbol_t *rOp) {
  varType_e resultType;

  printf("%s\n", op);
  printSymbol(rOp);

  char *llvmInstruction;
  if (rOp)
    resultType = getResultType(lOp->returnType, rOp->returnType, op);
  else
    resultType = lOp->returnType;

  llvmInstruction = getLLVMOp(resultType, op);
  printf("resolve op\n");

  symbol_t *symb = malloc(sizeof(symbol_t));
  symb->srcName = getNextReg();
  symb->returnType = resultType;
  symb->isGlobal = 0;
  symb->isPtr = 0;
  symb->symbType = VARIABLE_ID;

  char *strType = getVarTypeLLVM(resultType);

  lOp = convertSymbToType(lOp, resultType);

  // a = -x
  if (!rOp) {
    myPrint("%%%s = %s %s 0, %%%s\n", symb->srcName, llvmInstruction, strType,
            lOp->srcName);
    return symb;
  }

  rOp = convertSymbToType(rOp, resultType);
  myPrint("%%%s = %s %s %%%s, %%%s\n", symb->srcName, llvmInstruction, strType,
          lOp->srcName, rOp->srcName);

  return symb;
}

// calls correct function to handle expression
symbol_t *expressionResolutionWrapper(astNode_t *node) {
  if (!node)
    return NULL;
  switch (node->nodeType) {
  case EXPRESSION_NODE:
    return resolveExpressionNode(node);
    break;
  case ID_NODE:
    return resolveIdToLLVM(node);
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

// resolves an expression
symbol_t *resolveExpressionNode(astNode_t *node) {
  expressionNode_t *expressionNode = node->payload.expression;

  symbol_t *lOp = expressionResolutionWrapper(expressionNode->lOperand);
  symbol_t *rOp = expressionResolutionWrapper(expressionNode->rOperand);

  if (strcmp(expressionNode->op, "leaf") == 0) {
    return lOp;
  }

  return resolveOperationToLLVM(lOp, expressionNode->op, rOp);
}

/* ======= START AND NODE WRAPPER =======*/

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
    resolveLiteralToLLVM(node);
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