%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int yylex();
extern FILE *yyin;
extern int yylineno;
void yyerror(const char *s);

%}

%code requires {
  #include "AST_node_types.h"
  #include "frontend.h"
  #include "ast.h"
  #include "backend.h"

  typedef struct astNode_t astNode_t;
  typedef enum lexemeType_e lexemeType_e;
}

%union {
  // for non-terminals
  astNode_t *node;

  // for terminals
  char *lexeme;
  varType_e varType;
}

// Faz com que os erros sintaticos sejam bem detalhados
%define parse.error detailed


// Lista de TOKENS da gramatica. No caso, apenas simbolos terminais
%token PROGRAM VAR 
%token INTEIRO REAL
%token FUNCTION PROCEDURE BEGIN_TOKEN END 
%token IF THEN ELSE WHILE DO 
%token<lexeme> NUM ID OPERADOR_ATRIBUICAO OPERADOR_RELACIONAL MAIS MENOS OR OPERADOR_MULTIPLICATIVO
%token ABRE_PARENTESES FECHA_PARENTESES
%token PONTO_FINAL PONTO_VIRGULA VIRGULA DOIS_PONTOS

%type <node> LISTA_DE_IDENTIFICADORES PROGRAMA DECLARACOES 
%type <node> LISTA_DE_PARAMETROS ARGUMENTOS CABECALHO_DE_SUBPROGRAMA
%type <node> DECLARACAO_DE_SUBPROGRAMA DECLARACOES_DE_SUBPROGRAMAS
%type <node> ENUNCIADO VARIAVEL CHAMADA_DE_PROCEDIMENTO EXPRESSAO
%type <node> LISTA_DE_EXPRESSOES ENUNCIADO_COMPOSTO ENUNCIADOS_OPCIONAIS
%type <node> LISTA_DE_ENUNCIADOS FATOR TERMO EXPRESSAO_SIMPLES
%type <varType> TIPO
%type <lexeme> SINAL


// Define a associatividade dessas operacoes. A precedencia ja esta incluida na gramatica
%left MAIS MENOS OR

%%

//program meuPrograma (Input, Output);
PROGRAMA: 
PROGRAM ID
ABRE_PARENTESES LISTA_DE_IDENTIFICADORES FECHA_PARENTESES PONTO_VIRGULA
DECLARACOES
DECLARACOES_DE_SUBPROGRAMAS 
ENUNCIADO_COMPOSTO
PONTO_FINAL {
  astNode_t *node = createProgramNode($2, $7, $8, $9);
  printAST(node, "output.txt");
  resolveASTtoLLVM(node);
}
;

// Input, Output
LISTA_DE_IDENTIFICADORES: 
ID {
  astNode_t *idsListNode = createListNode(IDS_LIST);
  astNode_t *idNode = createIdNode($1);  
  addChildToListNode(idsListNode, idNode);
  $$ = idsListNode;
}
| LISTA_DE_IDENTIFICADORES VIRGULA ID {
  astNode_t *idNode = createIdNode($3);
  addChildToListNode($1, idNode);
  $$ = $1;
} 
;

// .....
// var a,b,c: integer;
DECLARACOES: 
DECLARACOES VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS
TIPO 
PONTO_VIRGULA {
  astNode_t *varsDefinition = createVarsDefinitionNode($5, $3);
  addChildToListNode($1, varsDefinition);
  $$ = $1;
} 
/*==*/
| /* empty */ {
  $$ = createListNode(VARS_DEFINITIONS_LIST);
}
;

TIPO: INTEIRO  {$$ = INT_TYPE; } 
    | REAL { $$ = REAL_TYPE; }
    ;

// Subprogramas podem ser funcoes (com ret) ou procedimentos (sem ret)
DECLARACOES_DE_SUBPROGRAMAS: 
DECLARACOES_DE_SUBPROGRAMAS DECLARACAO_DE_SUBPROGRAMA PONTO_VIRGULA {
  addChildToListNode($1, $2);
  $$ = $1;
}
| /* empty */ {
  astNode_t *listNode = createListNode(SUBPROGRAM_LIST);
  $$ = listNode;
}
;

DECLARACAO_DE_SUBPROGRAMA: 
CABECALHO_DE_SUBPROGRAMA DECLARACOES ENUNCIADO_COMPOSTO {
  astNode_t* subprogramDef = createSubprogramDefNode($1, $2, $3);
  $$ = subprogramDef;
} 
;
// Procedure nao tem tipo
CABECALHO_DE_SUBPROGRAMA: 
FUNCTION ID ARGUMENTOS DOIS_PONTOS TIPO PONTO_VIRGULA {
  astNode_t *idNode = createIdNode2($2, FUNCTION_ID);
  astNode_t *header = createSubprogramHeaderNode(idNode, $5, $3);
  $$ = header;
}
| PROCEDURE ID ARGUMENTOS PONTO_VIRGULA  {
  astNode_t *idNode = createIdNode2($2, FUNCTION_ID);
  astNode_t *header = createSubprogramHeaderNode(idNode, NO_TYPE, $3);
  $$ = header;
}
;

// (a,b: integer)
ARGUMENTOS: ABRE_PARENTESES LISTA_DE_PARAMETROS FECHA_PARENTESES {
  $$ = $2;
}
| {$$ = NULL; } /* empty */ 
;

// (a: integer): por valor
// (var a: integer): por ref
LISTA_DE_PARAMETROS: 
LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO  {
  astNode_t* paramDefsList = createListNode(PARAM_DEF_LIST);
  astNode_t* paramDefNode = createParamDefNode($3, 0, $1);
  addChildToListNode(paramDefsList, paramDefNode);
  $$ = paramDefsList;
}
| VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO {
  astNode_t* paramDefsList = createListNode(PARAM_DEF_LIST);
  astNode_t* paramDefNode = createParamDefNode($4, 1, $2);
  addChildToListNode(paramDefsList, paramDefNode);
  $$ = paramDefsList;
}
| LISTA_DE_PARAMETROS PONTO_VIRGULA LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO {
  astNode_t *paramDefNode = createParamDefNode($5, 0, $3);
  addChildToListNode($1, paramDefNode);
  $$ = $1;
}
| LISTA_DE_PARAMETROS PONTO_VIRGULA VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO {
  astNode_t *paramDefNode = createParamDefNode($6, 1, $4);
  addChildToListNode($1, paramDefNode);
  $$ = $1;
}
;

// um enunciado composto pode conter enunciados compostos
// sempre wrapped por BEGIN e END
ENUNCIADO_COMPOSTO: 
BEGIN_TOKEN ENUNCIADOS_OPCIONAIS END {
  $$ = $2;
}
;

// posso ter BEGIN END; (vazio)
ENUNCIADOS_OPCIONAIS: 
LISTA_DE_ENUNCIADOS {
  $$ = $1;
}
| /* empty */ {
  $$ = NULL;
}
;

LISTA_DE_ENUNCIADOS: 
ENUNCIADO {
  astNode_t *list = createListNode(STATEMENT_LIST);
  addChildToListNode(list, $1);
  $$ = list;
}
| LISTA_DE_ENUNCIADOS PONTO_VIRGULA ENUNCIADO  {
  addChildToListNode($1, $3);
  $$ = $1;
}
;

/* id = exp
func(a)
IF EXP
WHILE */
ENUNCIADO: 
VARIAVEL OPERADOR_ATRIBUICAO EXPRESSAO {
  astNode_t* assignment = createAssignmentNode($1, $3);
  $$ = assignment;
}
| CHAMADA_DE_PROCEDIMENTO {
  $$ = $1;
}
| ENUNCIADO_COMPOSTO {
  $$ = $1;
}
| IF EXPRESSAO THEN ENUNCIADO ELSE ENUNCIADO  {
  astNode_t *ifNode = createIfNode($2, $4, $6);
  $$ = ifNode;
}
| WHILE EXPRESSAO DO ENUNCIADO {
  astNode_t *whileNode = createWhileNode($2, $4);
  $$ = whileNode;
}
  ;

VARIAVEL: ID {
  astNode_t *idNode = createIdNode2($1, VARIABLE_ID);
  $$ = idNode;
}
;

CHAMADA_DE_PROCEDIMENTO: 
ID {
  astNode_t *idNode = createIdNode2($1, FUNCTION_ID);
  astNode_t *functionCall = createFunctionCallNode(idNode, 0, NULL);
  $$ = functionCall;
}
| ID ABRE_PARENTESES LISTA_DE_EXPRESSOES FECHA_PARENTESES  {
  astNode_t *idNode = createIdNode2($1, FUNCTION_ID);
  astNode_t *functionCall = createFunctionCallNode(idNode, 1, $3);
  $$ = functionCall;
}
;

LISTA_DE_EXPRESSOES: 
EXPRESSAO {
  astNode_t *listNode = createListNode(EXPRESSION_LIST);
  addChildToListNode(listNode, $1);
  $$ = listNode;
}
| LISTA_DE_EXPRESSOES VIRGULA EXPRESSAO {
  addChildToListNode($1, $3);
  $$ = $1;
}
;

// Operadores relacionais: =, <>, >, >=, <, <=
EXPRESSAO:
EXPRESSAO_SIMPLES {
  $$ = $1; // expression
}
| EXPRESSAO_SIMPLES OPERADOR_RELACIONAL EXPRESSAO_SIMPLES {
  $$ = createExpressionNode($2, $1, $3); // relational expression
}
;

// expressao simples eh uma conta com valor numerico
EXPRESSAO_SIMPLES: 
TERMO {
  $$ = $1;
}
| SINAL TERMO {
  astNode_t *expNode = createExpressionNode($1, $2, NULL);
  $$ = expNode;
} 
| EXPRESSAO_SIMPLES MAIS EXPRESSAO_SIMPLES {
  astNode_t *expNode = createExpressionNode($2, $1, $3);
  $$ = expNode;
}
| EXPRESSAO_SIMPLES MENOS EXPRESSAO_SIMPLES {
  astNode_t *expNode = createExpressionNode($2, $1, $3);
  $$ = expNode;
} 
| EXPRESSAO_SIMPLES OR EXPRESSAO_SIMPLES {
  astNode_t *expNode = createExpressionNode($2, $1, $3);
  $$ = expNode;
} 
;

// resultado de uma operacao multiplicativa (*, /, div, mod, and)
TERMO: 
FATOR {
  $$ = $1;
}
| TERMO OPERADOR_MULTIPLICATIVO FATOR {
  astNode_t *expNode = createExpressionNode($2, $1, $3);
  $$ = expNode;
}
;

FATOR: 
ID {
  astNode_t* idNode = createIdNode2($1, UNKNOWN);
  $$ = idNode;
}
| ID ABRE_PARENTESES LISTA_DE_EXPRESSOES FECHA_PARENTESES {
  astNode_t *idNode = createIdNode2($1, FUNCTION_ID);
  astNode_t *callNode = createFunctionCallNode(idNode, 1, $3);
  $$ = callNode;
}
| NUM {
  astNode_t *literalNode = createLiteralNode($1);
  $$ = literalNode;
}
| ABRE_PARENTESES EXPRESSAO FECHA_PARENTESES {
  $$ = $2;
} 
;

SINAL: 
MAIS {
  $$ = $1;
}
| MENOS {
  $$ = $1;
}
;


%%

int main(int argc, char ** argv) {
    if (argc == 2) {
        yyin = fopen(argv[1], "r");
        yylineno=1;
        yyparse();
    } else {
        fprintf(stderr, "Arquivo de entrada nao foi fornecido\n");
    }    
    return 0;
}

void yyerror(const char *s) {
  fprintf(stderr, "Erro na linha %d: %s\n", yylineno,s);
  exit(1);
}

