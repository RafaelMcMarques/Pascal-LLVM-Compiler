#ifndef AST_NODE_TYPES_H
#define AST_NODE_TYPES_H


/*
  This file defines the node types of the AST
  Node types are based on the grammar productions
  but not necessarily in a 1:1 relation

*/

typedef struct astNode_t astNode_t;

typedef enum astNodeType_e {
  LIST_NODE,
  PROGRAM_NODE, // root
  VARS_DEFINITION_NODE,
  ID_NODE,
  PARAMETER_DEF_NODE,
} astNodeType_e;

typedef enum listNodeType_e {
  CODE_BLOCK, 
  IDS_LIST, // vai servir para globais, locais e params
  SUBPROGRAM_LIST,
  VARS_DEFINITIONS_LIST,
  PARAM_DEF_LIST,
} listNodeType_e;


typedef enum varType_e {
  INT_TYPE,
  REAL_TYPE,
} varType_e;

typedef enum idType_e {
  VARIABLE_ID,
  FUNCTION_ID,
} idType_e;

// LIST_NODE 
typedef struct listNode_t {
  listNodeType_e listNodeType;
  int childrenCount;
  astNode_t* children[100];
} listNode_t;

// PROGRAM_NODE
typedef struct programNode_t {
  char* programName;
  astNode_t* varDeclarationList;  
  astNode_t* subprogramDeclarationList;
  astNode_t* mainCodeBlock;

} programNode_t;

//VARS DEFINITION NODE
typedef struct varsDefinitionNode_t {
  varType_e varsType;
  astNode_t* varsList;
} varsDefinitionNode_t;


// ID_NODE
typedef struct idNode_t {
  idType_e idType;
  char *id;
} idNode_t;

// PARAMETER_DEF_NODE,
typedef struct paramDefNode_t {
  varType_e varType;
  int byReference;
  astNode_t *idsList;
} paramDefNode_t;

typedef struct astNode_t {
  astNodeType_e nodeType;

  union {
    listNode_t *list;
    programNode_t *program;
    idNode_t *id;
    varsDefinitionNode_t *varsDefinition;
    paramDefNode_t *paramDefNode;

  } payload; 

} astNode_t;

#endif