# Compilador e flags
CC = clang
CFLAGS = -Iinclude

# Fontes
SRC = src/frontend.c src/ast.c src/symbol_table.c src/backend.c

# Arquivos gerados por Bison/Flex
PARSER = compilador.tab.c
LEXER  = lex.yy.c

# Alvo principal
compilador: $(PARSER) $(LEXER) $(SRC)
	$(CC) $(CFLAGS) -o compilador $(PARSER) $(LEXER) $(SRC) -lfl

# Gerar parser (Bison)
$(PARSER): compilador.y
	bison -d compilador.y

# Gerar lexer (Flex)
$(LEXER): compilador.l
	flex compilador.l

# Limpeza
clean:
	rm -f $(PARSER) compilador.tab.h $(LEXER) compilador

