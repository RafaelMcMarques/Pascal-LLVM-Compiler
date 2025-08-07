# Trabalho - subconjunto do Pascal

Construir um compilador de um subconjunto da linguagem Pascal, definido abaixo, para a representação intermediária do LLVM ([LLVM-IR](https://llvm.org/docs/LangRef.html), ou LLVM Assembly Language).

## Regras e requisitos - **IMPORTANTE**

- O trabalho pode ser feito em grupos de dois alunos. O trabalho deve ser apresentado pelo grupo. Não apresentar implica em nota zero.
    - O aluno que não souber responder as perguntas, ou explicar o código inteiro, poderá receber uma nota inferior que o colega.

- Os critérios para a nota serão:
    - Funcionamento correto do código para todos os testes realizados pelo professor (20%).
    - Compreensão e domínio do código produzido (40%)
    - Domínio geral do tema, incluindo capacidade de implementar mudanças/melhorias propostas pelo professor, como implementar outras features na linguagem (40%)

- Não deixe para submeter (fazer push) apenas no último momento. Faça o trabalho e submeta de forma incremental, já usando o repositório criado pelo GitHub Classroom. Dessa forma, submissões parciais poderão receber alguma nota. **Quando o prazo encerrar, não serão aceitas novas submissões (push) mesmo com redução da nota!**

- Arquivos plagiados, copiados, serão zerados. Não importa quem fez e quem copiou, ambos serão zerados. Portanto, não deixe o seu trabalho disponível de forma pública.

- Utilize a versão 15 ou superior do LLVM, onde [*Opaque Pointer*](https://llvm.org/docs/OpaquePointers.html) são suportados por padrão, preferencialmente use a versão 19 ou 20.

- Não é permitido o uso da [biblioteca LLVM](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html) para gerar código LLVM-IR, ou qualquer outra biblioteca para isso. Portanto, é necessário gerar o código de forma manual.
    - Não utilize bibliotecas extras, além das bibliotecas padrões de C, como Boost, pois podem complicar a compilação e validação pelo professor. Qualquer estrutura de dados, como a utilizada para tabela de símbolos, pode ser facilmente implementada pelo aluno (listas ligadas, tabelas hash e etc.).

- O compilador deve ler um arquivo de entrada (um único arquivo), que será o arquivo com o código Pascal. Esse arquivo será passado ao compilador como argumento na linha de comando (o único argumento)

- Um arquivo de saída, com as instruções LLVM-IR, será criado no diretório de trabalho atual. Esse arquivo sempre será nomeado `a.out`.

- O compilador deve retornar um valor diferente de zero quando encontrar qualquer tipo de erro. E retornar zero apenas em uma compilação bem sucedida.

- Recursão e passagem por referência são permitidas. 

- As funções `read` e `write` devem ser providas pelo compilador, e podem ser substituídas por uma chamada ao scanf e printf. Não é necessário ler e imprimir caracteres na tela, trabalhe apenas com números inteiros e de ponto flutuante. Mais detalhes em como usar/chamar essas funções pode ser obtido [aqui](./read_write_llvm.md).

- A gramática não permite a construção com apenas IF-THEN, impedindo o problema de *dangling else*. Portanto, toda construção de condicional deve conter tanto os enunciados do THEN como os do ELSE.

- Outro ponto é que não existe uma distinção sintática entre uma variável simples e uma chamada de função sem parâmetros. Ambos são gerados pela produção `FATOR: ID`.

## Exemplos de código

Alguns exemplos de código podem ser vistos [aqui](./exemplos_pascal).

## Ferramentas

O compilador deve, obrigatoriamente, usar as ferramentas [Flex](https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/) e [Bison](https://www.gnu.org/software/bison/manual/html_node/). 

Um arquivo inicial para o Flex [compilador.l](./compilador.l) e Bison [compilador.y](./compilador.y) já está incluído neste repositório. 

Um arquivo Makefile também foi incluído, e pode ser modificado de acordo com a organização do seu projeto. Porém, o arquivo executável não pode ser alterado (chamado `compilador`
).

## Gramática

A seguinte gramática já está apropriada para o Bison:

```
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

DECLARACAO_DE_SUBPROGRAMA: CABECALHO_DE_SUBPROGRAMA DECLARACOES ENUNCIADO_COMPOSTO 
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
         | IF EXPRESSAO THEN ENUNCIADO ELSE ENUNCIADO
         | WHILE EXPRESSAO DO ENUNCIADO 
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
```

### Tokens terminais -> lexemas correspondentes ou padrão (expressão regular)

| Token | Lexema |
| ----- | ------ |
| PROGRAM | program|
| ID | `[a-zA-Z][a-zA-Z1-9]*` |
| PONTO\_FINAL | . |
| VAR | var |
| INTEIRO | integer |
| REAL | real |
| FUNCTION | function |
| PROCEDURE | procedure |
|ABRE\_PARENTESES | ( |
| FECHA\_PARENTESES | ) |
| PONTO\_VIRGULA | ; |
| VIRGULA | , |
| DOIS\_PONTOS| : |
| BEGIN\_TOKEN | begin |
| END | end |
| IF | if |
| THEN | then |
| ELSE | else |
| WHILE | while |
| DO | do |
| OPERADOR\_ATRIBUICAO | := |
| OPERADOR\_RELACIONAL | =, <>, >, >=, <, <= |
| MAIS | + |
| MENOS | - |
| OR | or |
| OPERADOR\_MULTIPLICATIVO | \*, /, div, mod, and |
| NUM | `[0-9]+(.[0-9]+)?([Ee][+-]?{digitos})?` |

Obs.: a vírgula não faz parte dos lexemas de OPERADOR\_RELACIONAL e OPERADOR\_MULTIPLICATIVO, mas apenas divide os lexemas que podem ser classificados como um token de cada tipo.

## Operadores aritméticos, tipos e cast implícito

Os operadores `and`, `mod` e `div` só podem operar sobre `integer`, embora o resultado da operação pode ser armazenado em variáveis do tipo `real` (cast implícito).

Operações que misturam `integer` com `real` devem ter como resultado um `real`, esse resultado só pode ser armazenado em variáveis do tipo `real` não sendo permitido um cast implícito.

Essas regras, a princípio, fazem com que o compilador tenha o mesmo comportamento que o Free Pascal Compiler. Dessa forma, é possível compilar códigos exemplos com o `fpc` e comparar os resultados com o compilador implementado.


