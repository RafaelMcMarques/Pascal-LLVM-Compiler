%{
#include <stdio.h>
#include <stdlib.h>

int yylex();
extern FILE *yyin;
extern int yylineno;
void yyerror(const char *s);

%}

// Faz com que os erros sintaticos sejam bem detalhados
%define parse.error detailed


// Lista de TOKENS da gramatica. No caso, apenas simbolos terminais
%token PROGRAM VAR 
%token INTEIRO REAL
%token FUNCTION PROCEDURE BEGIN_TOKEN END 
%token IF THEN ELSE WHILE DO 
%token NUM ID OPERADOR_ATRIBUICAO OPERADOR_RELACIONAL MAIS MENOS OR OPERADOR_MULTIPLICATIVO
%token ABRE_PARENTESES FECHA_PARENTESES
%token PONTO_FINAL PONTO_VIRGULA VIRGULA DOIS_PONTOS


// Define a associatividade dessas operacoes. A precedencia ja esta incluida na gramatica
%left MAIS MENOS OR

%%

//program meuPrograma (Input, Output);
PROGRAMA: PROGRAM ID {printf("Program id OK\n");}
        ABRE_PARENTESES LISTA_DE_IDENTIFICADORES FECHA_PARENTESES PONTO_VIRGULA
        { printf("(input, output) OK\n"); }
        DECLARACOES
        { printf("Declaracoes OK\n"); }
        DECLARACOES_DE_SUBPROGRAMAS 
        { printf("Declaracoes de subprogramas OK\n");
          printf("Iniciando funcao MAIN\n"); }
        ENUNCIADO_COMPOSTO //funcao main
        PONTO_FINAL
        { printf("Programa OK"); }
        ;

// Input, Output
LISTA_DE_IDENTIFICADORES: ID 
                        | LISTA_DE_IDENTIFICADORES VIRGULA ID 
                        ;

// .....
// var a,b,c: integer;
DECLARACOES: DECLARACOES VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS
            { printf("Declaracao de variveis OK\n"); }
            TIPO 
            { printf("Declaracao tipo vars OK\n"); }
            PONTO_VIRGULA 
           | /* empty */ 
           ;

TIPO: INTEIRO 
    | REAL 
    ;

// Subprogramas podem ser funcoes (com ret) ou procedimentos (sem ret)
DECLARACOES_DE_SUBPROGRAMAS: DECLARACOES_DE_SUBPROGRAMAS DECLARACAO_DE_SUBPROGRAMA PONTO_VIRGULA
                           | /* empty */
                           ;

DECLARACAO_DE_SUBPROGRAMA: CABECALHO_DE_SUBPROGRAMA DECLARACOES ENUNCIADO_COMPOSTO
                         ;

// Procedure nao tem tipo
CABECALHO_DE_SUBPROGRAMA: FUNCTION ID ARGUMENTOS DOIS_PONTOS TIPO PONTO_VIRGULA 
                        | PROCEDURE ID ARGUMENTOS PONTO_VIRGULA 
                        ;

// (a,b: integer)
ARGUMENTOS: ABRE_PARENTESES LISTA_DE_PARAMETROS FECHA_PARENTESES 
          | /* empty */ 
          ;

// (a: integer): por valor
// (var a: integer): por ref
LISTA_DE_PARAMETROS: LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO 
                   | VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO
                   | LISTA_DE_PARAMETROS PONTO_VIRGULA LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO
                   | LISTA_DE_PARAMETROS PONTO_VIRGULA VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO
                   ;

// um enunciado composto pode conter enunciados compostos
// sempre wrapped por BEGIN e END
ENUNCIADO_COMPOSTO: BEGIN_TOKEN 
                    { printf("=== Dentro de enun composto\n"); }
                    ENUNCIADOS_OPCIONAIS END
                    { printf("=== Fora de enun composto\n"); }
                  ;

// posso ter BEGIN END; (vazio)
ENUNCIADOS_OPCIONAIS: LISTA_DE_ENUNCIADOS
                    | /* empty */
                    ;

LISTA_DE_ENUNCIADOS: ENUNCIADO 
                   | LISTA_DE_ENUNCIADOS PONTO_VIRGULA ENUNCIADO 
                   ;

/* id = exp
IF EXP
WHILE */
// TODO: PONTOS E VIRGULAS ESTAO CERTOS AQUI?
ENUNCIADO: VARIAVEL OPERADOR_ATRIBUICAO EXPRESSAO 
        { printf("Atribuicao\n"); }
         | CHAMADA_DE_PROCEDIMENTO
        { printf("Chamada de func\n"); }
         | ENUNCIADO_COMPOSTO
         | IF EXPRESSAO THEN ENUNCIADO ELSE ENUNCIADO 
        { printf("IF\n"); }
         | WHILE EXPRESSAO DO ENUNCIADO
        { printf("WHILE\n"); }
         ;

VARIAVEL: ID 
        ;

CHAMADA_DE_PROCEDIMENTO: ID 
                    | ID ABRE_PARENTESES LISTA_DE_EXPRESSOES FECHA_PARENTESES 
                    ;

LISTA_DE_EXPRESSOES: EXPRESSAO 
                   | LISTA_DE_EXPRESSOES VIRGULA EXPRESSAO  
                   ;

// Operadores relacionais: =, <>, >, >=, <, <=
EXPRESSAO: EXPRESSAO_SIMPLES
         | EXPRESSAO_SIMPLES OPERADOR_RELACIONAL EXPRESSAO_SIMPLES   
            {printf("Operacao relacional\n");}
         ;

// expressao simples eh uma conta com valor numerico
EXPRESSAO_SIMPLES: TERMO
                 | SINAL TERMO   
                {printf("Operacao sinal\n");}
                 | EXPRESSAO_SIMPLES MAIS EXPRESSAO_SIMPLES 
                {printf("Operacao mais\n");}
                 | EXPRESSAO_SIMPLES MENOS EXPRESSAO_SIMPLES 
                {printf("Operacao menos\n");}
                 | EXPRESSAO_SIMPLES OR EXPRESSAO_SIMPLES  
                {printf("Operacao or\n");}

                 ;

// resultado de uma operacao multiplicativa (*, /, div, mod, and)
TERMO: FATOR
     | TERMO OPERADOR_MULTIPLICATIVO FATOR  
    {printf("Operacao multiplicativa\n");}
     ;

FATOR: ID 
     | ID ABRE_PARENTESES LISTA_DE_EXPRESSOES FECHA_PARENTESES  
     | NUM 
     | ABRE_PARENTESES EXPRESSAO FECHA_PARENTESES 
     ;

SINAL: MAIS
     | MENOS
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

