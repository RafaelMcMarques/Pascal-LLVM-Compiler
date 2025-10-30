#ifndef AST_NODE_TYPES_H
#define AST_NODE_TYPES_H


/*
  This file defines the node types of the AST
  Node types are based on the grammar productions
  but not necessarily in a 1:1 relation

*/

typedef struct astNode_t astNode_t;

typedef enum astNodeType_e {
  PROGRAM_NODE, // root
  LIST_NODE,
  VAR_DELCARATION_NODE,
} astNodeType_e;

typedef enum listNodeType_e {
  CODE_BLOCK, 
  VAR_DECLARATIONS,
  SUBPROGRAM_DECLARATIONS,
} listNodeType_e;

typedef enum varType_e {
  INT_TYPE,
  REAL_TYPE,
} varType_e;

typedef struct listNode_t {
  listNodeType_e listNodeType;
  int childCount;
  astNode_t* children[100];
} listNode_t;

typedef struct programNode_t {
  char* programName;
  listNode_t* varDeclarationList;  
  listNode_t* subprogramDeclarationList;
  listNode_t* mainCodeBlock;

} programNode_t;

typedef struct varDeclarationNode_t {
  varType_e varType;
  char varName[100];
} varDeclarationNode_t;

typedef struct astNode_t {
  astNodeType_e NodeType;

  union {
    programNode_t *program;
    listNode_t *list;

  } payload; 

} astNode_t;

#endif