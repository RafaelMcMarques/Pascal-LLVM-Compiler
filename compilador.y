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

PROGRAMA: PROGRAM ID ABRE_PARENTESES LISTA_DE_IDENTIFICADORES FECHA_PARENTESES PONTO_VIRGULA
        DECLARACOES 
        DECLARACOES_DE_SUBPROGRAMAS 
        ENUNCIADO_COMPOSTO 
        PONTO_FINAL
        ;

LISTA_DE_IDENTIFICADORES: ID 
                        | LISTA_DE_IDENTIFICADORES VIRGULA ID 
                        ;

DECLARACOES: DECLARACOES VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO PONTO_VIRGULA 
           | /* empty */ 
           ;

TIPO: INTEIRO 
    | REAL 
    ;

DECLARACOES_DE_SUBPROGRAMAS: DECLARACOES_DE_SUBPROGRAMAS DECLARACAO_DE_SUBPROGRAMA PONTO_VIRGULA
                           | /* empty */
                           ;

DECLARACAO_DE_SUBPROGRAMA: CABECALHO_DE_SUBPROGRAMA DECLARACOES 
                         ;

CABECALHO_DE_SUBPROGRAMA: FUNCTION ID ARGUMENTOS DOIS_PONTOS TIPO PONTO_VIRGULA 
                        | PROCEDURE ID ARGUMENTOS PONTO_VIRGULA 
                        ;

ARGUMENTOS: ABRE_PARENTESES LISTA_DE_PARAMETROS FECHA_PARENTESES 
          | /* empty */ 
          ;

LISTA_DE_PARAMETROS: LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO 
                   | VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO
                   | LISTA_DE_PARAMETROS PONTO_VIRGULA LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO
                   | LISTA_DE_PARAMETROS PONTO_VIRGULA VAR LISTA_DE_IDENTIFICADORES DOIS_PONTOS TIPO
                   ;

ENUNCIADO_COMPOSTO: BEGIN_TOKEN ENUNCIADOS_OPCIONAIS END
                  ;

ENUNCIADOS_OPCIONAIS: LISTA_DE_ENUNCIADOS
                    | /* empty */
                    ;

LISTA_DE_ENUNCIADOS: ENUNCIADO
                   | LISTA_DE_ENUNCIADOS PONTO_VIRGULA ENUNCIADO
                   ;

ENUNCIADO: VARIAVEL OPERADOR_ATRIBUICAO EXPRESSAO 
         | CHAMADA_DE_PROCEDIMENTO
         | ENUNCIADO_COMPOSTO
         | IF EXPRESSAO 
         | WHILE 
         ;

VARIAVEL: ID 
        ;

CHAMADA_DE_PROCEDIMENTO: ID 
                    | ID ABRE_PARENTESES LISTA_DE_EXPRESSOES FECHA_PARENTESES 
                    ;

LISTA_DE_EXPRESSOES: EXPRESSAO 
                   | LISTA_DE_EXPRESSOES VIRGULA EXPRESSAO  
                   ;

EXPRESSAO: EXPRESSAO_SIMPLES
         | EXPRESSAO_SIMPLES OPERADOR_RELACIONAL EXPRESSAO_SIMPLES   
         ;

EXPRESSAO_SIMPLES: TERMO
                 | SINAL TERMO   
                 | EXPRESSAO_SIMPLES MAIS EXPRESSAO_SIMPLES 
                 | EXPRESSAO_SIMPLES MENOS EXPRESSAO_SIMPLES 
                 | EXPRESSAO_SIMPLES OR EXPRESSAO_SIMPLES  
                 ;

TERMO: FATOR
     | TERMO OPERADOR_MULTIPLICATIVO FATOR  
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

