%option yylineno 

%{
#include "compilador.tab.h" // para utilizar os TOKENS criados pelo BISON
#include "AST_node_types.h"
#include "frontend.h"
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
("="|"<>"|">"|">="|"<"|"<=") { 
  yylval.lexeme = strdup(yytext);
  return OPERADOR_RELACIONAL; 
}
"+" { 
  yylval.lexeme = strdup(yytext);
  return MAIS; 
}
"-" { 
  yylval.lexeme = strdup(yytext);
  return MENOS; 
}
"or" {
  yylval.lexeme = strdup(yytext);
  return OR; 
}
("*"|"/"|"div"|"mod"|"and") { 
  yylval.lexeme = strdup(yytext);
  return OPERADOR_MULTIPLICATIVO; 
}
{id} { 
  yylval.lexeme = strdup(yytext); 
  return ID; 
}
{numero} {
    yylval.lexeme = strdup(yytext);
    return NUM; 
}
 

[ \t] {/*ignorar espacos em branco, incluindo tabs*/}
\n { /* return EOL */}

%%

// Sem codigo aqui, flex sera usado pelo bison e nao 'standalone'
