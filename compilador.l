%option yylineno 

%{
#include "compilador.tab.h" // para utilizar os TOKENS criados pelo BISON

%}

%%

[ \t] {/*ignorar espacos em branco, incluindo tabs*/}

%%

// Sem codigo aqui, flex sera usado pelo bison e nao 'standalone'
