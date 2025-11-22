#ifndef BACKEND_H
#define BACKEND_H

#include <stdlib.h>
#include "ast.h"
#include "symbol_table.h"

void resolveASTtoLLVM(astNode_t *root);



void resolveProgramNode(programNode_t *programNode);

void resolveSubprogramList(listNode_t* subprogramList);

void resolveVarsDefsList(listNode_t *listNode);

void resolveVarsDefNode(varsDefinitionNode_t *varsDefNode);

void resolveParameterDefNode(paramDefNode_t *paramDefNode);

void resolveHeaderParamsDefsList(listNode_t *paramsList);

void resolveSubprogramHeaderNode(subprogramHeaderNode_t *headerNode);

void resolveSubprogramDefNode(subprogramDefNode_t *subpNode);

void resolveCodeBlockNode(listNode_t *statementList);

void resolveAssignmentNode(assignmentNode_t *asNode);

void resolveIfNode(ifNode_t *ifNode);

void resolveWhileNode(whileNode_t *whileNode);

symbol_t *resolveExpressionNode(expressionNode_t *expressionNode);

symbol_t *resolveLiteralNode(literalNode_t *literalNode);

symbol_t *resolveParamlessFuncCall(idNode_t *idNode);

void resolveIOFunctionCall(functionCallNode_t *callNode);

symbol_t *resolveFunctionCallNode(functionCallNode_t *callNode);

symbol_t *resolveIdNode(idNode_t *idNode);

symbol_t *resolveOperation(symbol_t *lOp, char *op, symbol_t *rOp);

symbol_t *resolveExpressionNode(expressionNode_t *expressionNode);

#endif