compilador: compilador.y compilador.l frontend.c
	bison -d compilador.y
	flex compilador.l
	clang -o compilador compilador.tab.c lex.yy.c frontend.c -lfl

clean:
	rm -f lex.yy.c compilador compilador.tab.c compilador.tab.h
