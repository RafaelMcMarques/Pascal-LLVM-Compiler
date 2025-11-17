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

// to be used in functions
unsigned int nextLocalReg = 0;
unsigned int nextLocalLabel = 0;

// to be used in main
unsigned int nextGlobalReg = 0;
unsigned int nextGlobalLabel = 0;

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
      fprintf(file, "\t");
    else
      printf("  ");
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

  return "?";
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

char *getNextLabel() {
  int number = LocalSymTable == NULL ? nextGlobalLabel++ : nextLocalLabel++;

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
    // myPrint("; statements(%d)\n", listNode->childrenCount);
    if (!PrintCodeBlock)
      return;
    break;
  case IDS_LIST:
    myPrint("IDS LIST (%d)\n", listNode->childrenCount);
    break;
  case SUBPROGRAM_LIST:
    // myPrint("; subprograms(%d)\n", listNode->childrenCount);
    break;
  case VARS_DEFINITIONS_LIST:
    // myPrint("VAR DEFS LIST (%d)\n", listNode->childrenCount);
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

  for (int i = 0; i < listNode->childrenCount; i++) {
    if (!listNode->children[i]) {
      myPrint("EMPTY CHILD NODE\n");
      continue;
    }
    resolveToLLVM(listNode->children[i]);
  }
}

void declareIOFunctionsLLVM() {
  myPrint("; I/O functions\n");

  myPrint("declare i32 @printf(ptr noundef, ...)\n");
  myPrint("declare i32 @__isoc99_scanf(ptr noundef, ...)\n");
  myPrint("@read_int = private unnamed_addr constant [3 x i8] c\"%%d\\00\", "
          "align 1\n");
  myPrint("@write_int = private unnamed_addr constant [4 x i8] "
          "c\"%%d\\0A\\00\", align 1\n");
  myPrint("@read_float = private unnamed_addr constant [3 x i8] c\"%%f\\00\", "
          "align 1\n");
  myPrint("@write_float = private unnamed_addr constant [4 x i8] "
          "c\"%%f\\0A\\00\", align 1\n");
  myPrint("\n");
}

/* ===== PROGRAM NODE ===== */
void resolveProgramNode(astNode_t *node) {
  programNode_t *programNode = node->payload.program;

  myPrint("; PROGRAM: %s\n\n", programNode->programName);

  declareIOFunctionsLLVM();

  myPrint("; global variable declarations\n");
  resolveToLLVM(programNode->varDeclarationList);
  myPrint("\n");
  resolveToLLVM(programNode->subprogramDeclarationList);
  myPrint("\n");

  myPrint("define i32 @main() {\n");
  depth++;
  resolveToLLVM(programNode->mainCodeBlock);
  depth--;
  myPrint("}\n");
}

/* ===== VARIABLE DECLARATIONS ===== */

void declareVariableLLVM(symbol_t *symb) {
  char *name = symb->srcName;
  char *type = getVarTypeLLVM(symb->returnType);

  if (isScopeLocal()) {
    myPrint("%%%s = alloca %s\n", name, type);
  } else {
    myPrint("@%s = global %s\n", name, type);
  }
}

// add vars to symTab and call declare
// this is wrapped by printListNode
void resolveVarsDefNode(astNode_t *node) {
  varsDefinitionNode_t *varsDefNode = node->payload.varsDefinition;

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

  if (!headerNode->params) {
    symb->paramCount = 0;
    return symb;
  }

  // register params
  int paramCount = 0;
  listNode_t *paramsList = headerNode->params->payload.list;
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

  // insert into symtable
  symbol_t *symb = getFunctionSymbol(headerNode);
  insertSymbolGlobalScope(symb);

  myPrint("; Subprogram %s\n", symb->srcName);

  // declare in LLVM
  char *type = getVarTypeLLVM(symb->returnType);
  myPrint("define %s @%s", type, symb->srcName);

  // declar params in LLVM and add them to local symTable
  myPrintSL("(");
  if (headerNode->params) {
    resolveHeaderParamsList(headerNode->params->payload.list);
  }
  myPrintSL(") ");
}

// resolve a function to llvm
void resolveSubprogramDefNode(astNode_t *node) {
  subprogramDefNode_t *subpNode = node->payload.subprogramDef;

  LocalSymTable = createHT(0);
  nextLocalReg = 0;

  resolveToLLVM(subpNode->header);

  depth++;
  myPrintSL("{\n");

  myPrint("; local variables\n");
  resolveToLLVM(subpNode->declarations);

  // declare return value
  char *name = subpNode->header->payload.subprogramHeader->id->payload.id->id;
  varType_e type = subpNode->header->payload.subprogramHeader->returnType;
  if (type != NO_TYPE) {
    char *strType = getVarTypeLLVM(type);
    myPrint("%%%s = alloca %s 0\n", name, strType);
  }

  // statements
  resolveToLLVM(subpNode->codeBlock);

  // return
  if (type != NO_TYPE) {
    char *strType = getVarTypeLLVM(type);
    myPrint("return %s %s\n", strType, name);
  }

  depth--;
  myPrint("}\n\n");

  destroyHT(LocalSymTable);
  LocalSymTable = NULL;
}

/* ======= STATEMENTS =======*/

// forward declaration
symbol_t *resolveExpressionNode(astNode_t *);

void resolveAssignmentNode(astNode_t *node) {
  assignmentNode_t *asNode = node->payload.assignment;

  symbol_t *var = getSymbol(asNode->variable->payload.id->id);
  myPrint("\n");
  myPrint("; Assignment (%s :=  exp)\n", var->srcName);

  symbol_t *exp = resolveExpressionNode(asNode->exp);

  if (!exp) {
    myPrint("ERROR: could not assign %s no null expression\n", var->srcName);
    exit(1);
  }
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
  depth++;
  ifNode_t *ifNode = node->payload.ifNode;

  myPrint("\n");
  myPrint("; if statement\n");
  symbol_t *exp = resolveExpressionNode(ifNode->condition);

  char *ifLabelID = getNextLabel();
  char *elseLabelID = getNextLabel();
  char *endifLabelID = getNextLabel();
  myPrint("br i1 %%%s, label %%then%s, label %%else%s\n\n", exp->srcName,
          ifLabelID, elseLabelID);

  myPrintSL("then%s: \n", ifLabelID);
  resolveToLLVM(ifNode->thenBlock);
  myPrint("\n");
  myPrint("br label %%endif%s\n\n", endifLabelID);

  myPrintSL("else%s: \n", elseLabelID);
  resolveToLLVM(ifNode->elseBlock);
  myPrint("\n");

  myPrintSL("endif%s: \n", endifLabelID);

  depth--;
}

void resolveWhileNode(astNode_t *node) {
  depth++;
  whileNode_t *whileNode = node->payload.whileNode;

  char *condLabelID = getNextLabel();
  char *bodyLabelID = getNextLabel();
  char *exitLabelID = getNextLabel();

  myPrint("\n");
  myPrint("; while statement\n");

  myPrintSL("cond%s:\n", condLabelID);
  symbol_t *exp = resolveExpressionNode(whileNode->condition);
  myPrint("br i1 %%%s, label %%body%s, label %%exit%s\n\n", exp->srcName,
          bodyLabelID, exitLabelID);

  myPrintSL("body%s:\n", bodyLabelID);
  resolveToLLVM(whileNode->codeBlock);
  myPrint("\n");
  myPrint("br label %%cond%s\n\n", condLabelID);

  myPrintSL("exit%s:\n", exitLabelID);
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

symbol_t *getReferenceArgSymb(expressionNode_t *argNode, parameter_t *param) {
  // arg tem que ser var
  if (strcmp(argNode->op, "leaf") || (argNode->lOperand->nodeType != ID_NODE))
    return NULL;

  idNode_t *idNode = argNode->lOperand->payload.id;
  symbol_t *argSymb = getSymbol(idNode->id);

  if ((argSymb->symbType != VARIABLE_ID) || !argSymb->isPtr ||
      (argSymb->returnType != param->type)) {
    return NULL; // unreachable
  }

  return argSymb;
}

// use when function in pascal is called with only its id
symbol_t *resolveParamlessFuncCall(idNode_t *idNode) {
  symbol_t *funcSym = getSymbol(idNode->id);
  char *functionName = funcSym->srcName;

  if (funcSym->paramCount) {
    printf("ERROR: cannot call function %s with no params\n", functionName);
    exit(1);
    return NULL;
  }

  // generate LLVM
  symbol_t *returnSymb = NULL;
  if (funcSym->returnType != NO_TYPE) {
    returnSymb = malloc(sizeof(symbol_t));

    returnSymb->srcName = getNextReg();
    returnSymb->isGlobal = 0;
    returnSymb->symbType = VARIABLE_ID;
    returnSymb->returnType = funcSym->returnType;
    returnSymb->isPtr = 0;

    myPrint("%%%s = call %s @%s()\n", returnSymb->srcName,
            getVarTypeLLVM(returnSymb->returnType), functionName);
  } else {
    myPrint("call void @%s()\n", functionName);
  }

  return returnSymb;
}

// forward declaration
symbol_t *resolveIdToLLVM(astNode_t *node);

void resolveIOFunctionCall(functionCallNode_t *callNode) {
  char *functionName = callNode->id->payload.id->id;

  if (!callNode->hasArguments) {
    printf("ERROR: function %s called with no arguments\n", functionName);
    exit(1);
  }

  listNode_t *argsList = callNode->arguments->payload.list;

  if (argsList->childrenCount != 1) {
    printf("ERROR: function %s called with too many arguments\n", functionName);
    exit(1);
  }

  expressionNode_t *argNode = argsList->children[0]->payload.expression;

  if (strcmp(argNode->op, "leaf") || (argNode->lOperand->nodeType != ID_NODE)) {
    printf("ERROR: function %s called with wrong type arguments\n",
           functionName);
    exit(1);
  }

  idNode_t *idNode = argNode->lOperand->payload.id;
  symbol_t *argSymb = getSymbol(idNode->id);

  if ((argSymb->symbType != VARIABLE_ID) || !argSymb->isPtr) {
    printf("ERROR: function %s called with wrong type arguments\n",
           functionName);
    exit(1);
  }

  char *scopeSymb = argSymb->isGlobal ? "@" : "%";

  // READ
  if (!strcmp(functionName, "read")) {
    char *formatStr =
        argSymb->returnType == INT_TYPE ? "@read_int" : "@read_float";
    myPrint("call i32 (ptr, ...) @__isoc99_scanf(ptr %s, ptr %s%s)\n",
            formatStr, scopeSymb, argSymb->srcName);
    return;
  }

  // WRITE
  if (!strcmp(functionName, "write")) {
    symbol_t *resolvedArgSymb = resolveIdToLLVM(argNode->lOperand);

    if (resolvedArgSymb->returnType == REAL_TYPE) {
      char *nextReg = getNextReg();
      myPrint(" %%%s = fpext float %%%s to double\n", nextReg,
              resolvedArgSymb->srcName);
      myPrint("call i32 (ptr, ...) @printf(ptr  @write_float, double %%%s)\n",
              nextReg);

    } else {
      myPrint("call i32 (ptr, ...) @printf(ptr @write_int, i32 %%%s)\n",
              resolvedArgSymb->srcName);
    }
  }
}

// can be called with id node or functionCallNode
symbol_t *resolveFunctionCallToLLVM(astNode_t *node) {
  myPrint("\n");
  myPrint("; function call\n");
  if (node->nodeType == ID_NODE) {
    return resolveParamlessFuncCall(node->payload.id);
  }

  functionCallNode_t *callNode = node->payload.functionCall;
  listNode_t *argList =
      callNode->hasArguments ? callNode->arguments->payload.list : NULL;

  int argumentCount = callNode->hasArguments ? argList->childrenCount : 0;

  char *functionName = callNode->id->payload.id->id;

  // IO function handled separetely
  if (!strcmp(functionName, "read") || !strcmp(functionName, "write")) {
    resolveIOFunctionCall(callNode);
    return NULL;
  }
  symbol_t *funcSym = getSymbol(functionName);

  if (argumentCount != funcSym->paramCount) {
    printf("ERROR: wrong number of arguments for function %s\n", functionName);
    exit(1);
    return NULL; // unreachable
  }

  symbol_t *argSymbols[argumentCount];

  // check arguments
  for (int i = 0; i < argumentCount; i++) {
    parameter_t *param = funcSym->params[i];
    expressionNode_t *argNode = argList->children[i]->payload.expression;

    if (param->byReference) {
      argSymbols[i] = getReferenceArgSymb(argNode, param);
      continue;
    }

    argSymbols[i] = resolveExpressionNode(argList->children[i]);
    if (argSymbols[i]->returnType != param->type) {
      argSymbols[i] = NULL;
    }
  }

  // generate LLVM
  symbol_t *returnSymb = NULL;
  if (funcSym->returnType != NO_TYPE) {
    returnSymb = malloc(sizeof(symbol_t));

    returnSymb->srcName = getNextReg();
    returnSymb->isGlobal = 0;
    returnSymb->symbType = VARIABLE_ID;
    returnSymb->returnType = funcSym->returnType;
    returnSymb->isPtr = 0;

    myPrint("%%%s = call %s @%s", returnSymb->srcName,
            getVarTypeLLVM(returnSymb->returnType), functionName);
  } else {
    myPrint("call void @%s", functionName);
  }

  // print arguments
  myPrintSL("(");
  char *separator = "";
  for (int i = 0; i < argumentCount; i++) {
    if (!argSymbols[i]) {
      printf("ERROR: wrong type for argument %d for function %s\n", i + 1,
             functionName);
      exit(1);
      return NULL; // unreachable
    }
    char *ptrStr = argSymbols[i]->isPtr ? "*" : "";
    char *scopeStr = argSymbols[i]->isGlobal ? "@" : "%";
    char *typeStr = getVarTypeLLVM(argSymbols[i]->returnType);
    myPrintSL("%s%s%s %s%s", separator, typeStr, ptrStr, scopeStr,
              argSymbols[i]->srcName);
    separator = ", ";
  }
  myPrintSL(")\n");

  return returnSymb;
}

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
    return resolveFunctionCallToLLVM(node);
    break;
  case LITERAL_NODE:
    return resolveLiteralToLLVM(node);
    break;
  default:
    printf("Ainda nao implementei essa exp\n");
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
  case FUNCTION_CALL_NODE:
    resolveFunctionCallToLLVM(node);
    break;
  default:
    myPrint("UNKNOWN NODE\n");
    break;
  }
}

void resolveASTtoLLVM(astNode_t *root) {
  file = fopen("a.ll", "w"); // open file for writing
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