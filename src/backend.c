#include "../include/backend.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int PrintCodeBlock = 1;

static int identDepth = 0;

static FILE *file = NULL;

// to be used in functions
int NextLocalReg = 1, NextIfLabel = 1, NextWhileLabel = 1;

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
  for (int i = 0; i < identDepth; i++) {
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

varType_e getOperationResultType(varType_e left, varType_e right,
                                 const char *op) {

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

    printf("ERROR: Invalid div or mod for real type\n"); // div/mod em real é
                                                         // inválido
    exit(1);
    return 0; // wont reach
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

  printf("ERROR: invalid operation\n");
  exit(1);
}

/* ====== LABEL HELPERS ====== */
char *getNextReg() {
  int number = NextLocalReg++;

  char temp[32];
  int len = snprintf(temp, sizeof(temp), "tmp%d", number);

  char *result = malloc(len + 1);
  strcpy(result, temp);
  return result;
}

char *getNextIfLabel() {
  int number = NextIfLabel++;

  char temp[32];
  int len = snprintf(temp, sizeof(temp), "%d", number);

  char *result = malloc(len + 1);
  strcpy(result, temp);
  return result;
}

char *getNextWhileLabel() {
  int number = NextWhileLabel++;

  char temp[32];
  int len = snprintf(temp, sizeof(temp), "%d", number);

  char *result = malloc(len + 1);
  strcpy(result, temp);
  return result;
}

void enterNewScope() {
  NextIfLabel = 1;
  NextWhileLabel = 1;
  NextLocalReg = 1;
  initLocalSymTab();
  return;
}

void leaveScope() {
  NextIfLabel = 1;
  NextWhileLabel = 1;
  NextLocalReg = 1;
  destroyLocalSymTab();
  return;
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
  myPrintSL("\n");
}

/* ===== PROGRAM NODE ===== */
void resolveProgramNode(programNode_t *programNode) {

  myPrint("; PROGRAM: %s\n\n", programNode->programName);

  declareIOFunctionsLLVM();

  myPrint("; global variable declarations\n");
  resolveVarsDefsList(programNode->varDeclarationList->payload.list);
  myPrintSL("\n");

  resolveSubprogramList(programNode->subprogramDeclarationList->payload.list);
  myPrintSL("\n");

  myPrint("define i32 @main() {\n");
  identDepth++;

  myPrintSL("entry:\n");
  // TODO: change
  resolveCodeBlockNode(programNode->mainCodeBlock->payload.list);
  myPrint("ret i32 0\n");

  identDepth--;
  myPrint("}\n");
}

/* ===== VARIABLE DECLARATIONS ===== */

void resolveVarsDefsList(listNode_t *listNode) {
  for (int i = 0; i < listNode->childrenCount; i++) {
    resolveVarsDefNode(listNode->children[i]->payload.varsDefinition);
  }
}

void resolveVarsDefNode(varsDefinitionNode_t *varsDefNode) {

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

    char *name = symb->srcName;
    char *type = getVarTypeLLVM(symb->returnType);

    if (!symb->isGlobal) {
      myPrint("%%%s = alloca %s\n", name, type);
    } else {
      char *initValue = symb->returnType == INT_TYPE ? "0" : "0.0";
      myPrint("@%s = global %s %s\n", name, type, initValue);
    }
  }
}

/* ====== SUBPROGRAMS ====== */

void resolveSubprogramList(listNode_t *subprogramList) {
  for (int i = 0; i < subprogramList->childrenCount; i++) {
    resolveSubprogramDefNode(
        subprogramList->children[i]->payload.subprogramDef);
  }
}

// add parameters to sym tab and generate llvm
void resolveParameterDefNode(paramDefNode_t *paramDefNode) {

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

// iterate all params definitions
void resolveHeaderParamsDefsList(listNode_t *paramsList) {
  char *separator = "";
  for (int i = 0; i < paramsList->childrenCount; i++) {
    myPrintSL("%s", separator);
    resolveParameterDefNode(paramsList->children[i]->payload.paramDefNode);
    separator = ", ";
  }
}

// generate code and symbol for function declaration
void resolveSubprogramHeaderNode(subprogramHeaderNode_t *headerNode) {

  // insert into symtable
  symbol_t *symb = createFunctionSymbol(headerNode);
  insertSymbolGlobalScope(symb);

  myPrint("; Subprogram %s\n", symb->srcName);

  // declare in LLVM
  char *type = getVarTypeLLVM(symb->returnType);

  myPrint("define %s @%s", type, symb->srcName);

  // declar params in LLVM and add them to local symTable
  myPrintSL("(");
  if (headerNode->params) {
    resolveHeaderParamsDefsList(headerNode->params->payload.list);
  }
  myPrintSL(") ");
}

// resolve a function to llvm
void resolveSubprogramDefNode(subprogramDefNode_t *subpNode) {
  enterNewScope();

  resolveSubprogramHeaderNode(subpNode->header->payload.subprogramHeader);

  identDepth++;
  myPrintSL("{\n");
  myPrintSL("entry:\n");

  myPrint("; local variables\n");
  resolveVarsDefsList(subpNode->declarations->payload.list);

  // declare return value
  char *name = subpNode->header->payload.subprogramHeader->id->payload.id->id;
  varType_e type = subpNode->header->payload.subprogramHeader->returnType;
  if (type != NO_TYPE) {
    symbol_t *returnSymb = malloc(sizeof(symbol_t));
    returnSymb->srcName = name;
    returnSymb->symbType = VARIABLE_ID;
    returnSymb->isPtr = 1;
    returnSymb->returnType = type;
    returnSymb->isGlobal = 0;
    insertSymbolCurrScope(returnSymb);

    char *strType = getVarTypeLLVM(type);
    myPrint("%%%s = alloca %s\n", name, strType);
  }

  // statements
  resolveCodeBlockNode(subpNode->codeBlock->payload.list);

  // return
  if (type != NO_TYPE) {
    char *strType = getVarTypeLLVM(type);
    char *reg = getNextReg();
    myPrint("%%%s = load %s, %s* %%%s\n", reg, strType, strType, name);
    myPrint("ret %s %%%s\n", strType, reg);
  } else {
    myPrint("ret void\n");
  }

  identDepth--;
  myPrint("}\n\n");

  leaveScope();
}

/* ======= STATEMENTS =======*/
void resolveCodeBlockNode(listNode_t *statementList) {
  for (int i = 0; i < statementList->childrenCount; i++) {
    astNode_t *childNode = statementList->children[i];
    switch (childNode->nodeType) {
    case (ASSIGNMENT_NODE):
      resolveAssignmentNode(childNode->payload.assignment);
      break;
    case (IF_NODE):
      resolveIfNode(childNode->payload.ifNode);
      break;
    case (WHILE_NODE):
      resolveWhileNode(childNode->payload.whileNode);
      break;
    case (FUNCTION_CALL_NODE):
      resolveFunctionCallNode(childNode->payload.functionCall);
      break;
    default:
      printf("ERROR: unknown statement\n");
      exit(1);
      break;
    }
  }
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

void resolveAssignmentNode(assignmentNode_t *asNode) {

  symbol_t *var = getSymbol(asNode->variable->payload.id->id);
  myPrintSL("\n");
  myPrint("; Assignment (%s :=  exp)\n", var->srcName);

  symbol_t *exp = resolveExpressionNode(asNode->exp->payload.expression);

  if (!exp) {
    myPrint("ERROR: could not assign %s no null expression\n", var->srcName);
    exit(1);
  }
  if (var->symbType == FUNCTION_ID) {
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
    if (exp->returnType == INT_TYPE && var->returnType == REAL_TYPE) {
      exp = convertSymbToType(exp, REAL_TYPE);
      typeStrE = getVarTypeLLVM(exp->returnType);
    } else {
      printf("ERROR: cannot assign %s to %s\n", typeStrE, typeStrV);
      exit(1);
    }
  }

  if (var->isGlobal) {
    myPrint("store %s %%%s, %s* @%s\n", typeStrE, exp->srcName, typeStrV,
            var->srcName);
  } else {
    myPrint("store %s %%%s, %s* %%%s\n", typeStrE, exp->srcName, typeStrV,
            var->srcName);
  }
}

void resolveIfNode(ifNode_t *ifNode) {

  myPrintSL("\n");
  myPrint("; if statement\n");
  symbol_t *exp = resolveExpressionNode(ifNode->condition->payload.expression);

  char *ifLabelID = getNextIfLabel();
  myPrint("br i1 %%%s, label %%then%s, label %%else%s\n\n", exp->srcName,
          ifLabelID, ifLabelID);

  myPrintSL("then%s:\n", ifLabelID);
  resolveCodeBlockNode(ifNode->thenBlock->payload.list);
  myPrintSL("\n");
  myPrint("br label %%endif%s\n\n", ifLabelID);

  myPrintSL("else%s:\n", ifLabelID);
  resolveCodeBlockNode(ifNode->elseBlock->payload.list);
  myPrint("br label %%endif%s\n\n", ifLabelID);
  myPrintSL("\n");

  myPrintSL("endif%s:\n", ifLabelID);
}

void resolveWhileNode(whileNode_t *whileNode) {

  char *whileLabelID = getNextWhileLabel();

  myPrintSL("\n");
  myPrint("; while statement\n");

  myPrint("br label %%cond%s\n", whileLabelID);
  myPrintSL("cond%s:\n", whileLabelID);
  symbol_t *exp =
      resolveExpressionNode(whileNode->condition->payload.expression);
  myPrint("br i1 %%%s, label %%body%s, label %%exit%s\n\n", exp->srcName,
          whileLabelID, whileLabelID);

  myPrintSL("body%s:\n", whileLabelID);
  resolveCodeBlockNode(whileNode->codeBlock->payload.list);
  myPrintSL("\n");
  myPrint("br label %%cond%s\n", whileLabelID);

  myPrintSL("exit%s:\n", whileLabelID);
}

/* ====== EXPRESSIONS =====*/

// puts a literal into a register
symbol_t *resolveLiteralNode(literalNode_t *literalNode) {
  symbol_t *symb = malloc(sizeof(symbol_t));
  char *literal = literalNode->literal;

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
  symbol_t *funcSym = getGlobalSymbol(idNode->id);
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

void resolveIOFunctionCall(functionCallNode_t *callNode) {
  char *functionName = callNode->id->payload.id->id;

  if (!callNode->hasArguments) {
    printf("ERROR: function %s called with no arguments\n", functionName);
    exit(1);
  }

  listNode_t *argsList = callNode->arguments->payload.list;

  for (int i = 0; i < argsList->childrenCount; i++) {
    expressionNode_t *argNode = argsList->children[i]->payload.expression;

    if (strcmp(argNode->op, "leaf") ||
        (argNode->lOperand->nodeType != ID_NODE)) {
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
    }

    // WRITE
    if (!strcmp(functionName, "write")) {
      symbol_t *resolvedArgSymb = resolveIdNode(argNode->lOperand->payload.id);

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
}

// can be called with id node or functionCallNode
symbol_t *resolveFunctionCallNode(functionCallNode_t *callNode) {
  myPrintSL("\n");
  myPrint("; function call\n");

  listNode_t *argList =
      callNode->hasArguments ? callNode->arguments->payload.list : NULL;

  int argumentCount = callNode->hasArguments ? argList->childrenCount : 0;

  char *functionName = callNode->id->payload.id->id;

  // IO function handled separetely
  if (!strcmp(functionName, "read") || !strcmp(functionName, "write")) {
    resolveIOFunctionCall(callNode);
    return NULL;
  }

  // functions are always global
  symbol_t *funcSym = getGlobalSymbol(functionName);

  if (funcSym->symbType != FUNCTION_ID) {
    printf("ERROR: %s is not a function\n", functionName);
    exit(1);
    return NULL;
  }

  if (argumentCount != funcSym->paramCount) {
    printf("ERROR: wrong number of arguments for function %s\n", functionName);
    printf("gave %d, expected %d\n", argumentCount, funcSym->paramCount);
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
    argSymbols[i] =
        resolveExpressionNode(argList->children[i]->payload.expression);
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
symbol_t *resolveIdNode(idNode_t *idNode) {
  char *id = idNode->id;

  symbol_t *symb = getSymbol(id);

  symbol_t *newSymb = malloc(sizeof(symbol_t));

  // this is a paramless function call
  if (symb->symbType == FUNCTION_ID) {
    return resolveParamlessFuncCall(idNode);
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

// generates code for operations
symbol_t *resolveOperation(symbol_t *lOp, char *op, symbol_t *rOp) {
  varType_e resultType;

  // printSymbol(rOp);

  char *llvmInstruction;
  if (rOp)
    resultType = getOperationResultType(lOp->returnType, rOp->returnType, op);
  else
    resultType = lOp->returnType;

  llvmInstruction = getLLVMOp(resultType, op);

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
    return resolveExpressionNode(node->payload.expression);
    break;
  case ID_NODE:
    return resolveIdNode(node->payload.id);
    break;
  case FUNCTION_CALL_NODE:
    return resolveFunctionCallNode(node->payload.functionCall);
    break;
  case LITERAL_NODE:
    return resolveLiteralNode(node->payload.literal);
    break;
  default:
    printf("ERROR: unknown expression\n");
    exit(1);
    return NULL; // wont reach
  }
}

// resolves an expression
symbol_t *resolveExpressionNode(expressionNode_t *expressionNode) {

  symbol_t *lOp = expressionResolutionWrapper(expressionNode->lOperand);
  symbol_t *rOp = expressionResolutionWrapper(expressionNode->rOperand);

  if (strcmp(expressionNode->op, "leaf") == 0) {
    return lOp;
  }

  return resolveOperation(lOp, expressionNode->op, rOp);
}

void resolveASTtoLLVM(astNode_t *root) {
  file = fopen("a.out", "w"); // open file for writing
  if (file == NULL) {
    printf("Error opening file.\n");
    exit(1);
  }

  initGlobalSymTab();

  resolveProgramNode(root->payload.program);

  destroyGlobalSymTab();
  if (file)
    fclose(file);
}