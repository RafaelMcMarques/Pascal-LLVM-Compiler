%option yylineno 

%{
#include "compilador.tab.h" // para utilizar os TOKENS criados pelo BISON

%}

id [a-zA-Z][a-zA-Z1-9]*
digitos     [0-9]+
numero      {digitos}(\.{digitos})?([Ee][+-]?{digitos})?

%%

"program" { return PROGRAM; }
"." { return PONTO_FINAL; }
"var" { return VAR; }
"integer" { return INTEIRO; }
"real" { return REAL; }
"function" { return FUNCTION; }
"procedure" { return PROCEDURE; }
"(" { return ABRE_PARENTESES; } 
")" { return FECHA_PARENTESES; }
";" { return PONTO_VIRGULA; }
"," { return VIRGULA; }
":" { return DOIS_PONTOS; }
"begin" { return BEGIN_TOKEN; }
"end" { return END; }
"if" { return IF; }
"then" { return THEN; }
"else" { return ELSE; }
"while" { return WHILE; }
"do" { return DO; }
":=" { return OPERADOR_ATRIBUICAO; }
("="|"<>"|">"|">="|"<"|"<=") { return OPERADOR_RELACIONAL; }
"+" { return MAIS; }
"-" { return MENOS; }
"or" { return OR; }
("*"|"/"|"div"|"mod"|"and") { return OPERADOR_MULTIPLICATIVO; }
{id} { return ID; }
{numero} { return NUM; }
 

[ \t] {/*ignorar espacos em branco, incluindo tabs*/}
\n { /* return EOL */}

%%

// Sem codigo aqui, flex sera usado pelo bison e nao 'standalone'
