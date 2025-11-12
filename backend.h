#ifndef BACKEND_H
#define BACKEND_H

#include <stdlib.h>
#include "AST_node_types.h"

void resolveASTtoLLVM(astNode_t *root);

#endif