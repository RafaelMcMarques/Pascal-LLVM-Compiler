#ifndef BACKEND_H
#define BACKEND_H

#include <stdlib.h>
#include "ast.h"
#include "symbol_table.h"

void resolveASTtoLLVM(astNode_t *root);

#endif