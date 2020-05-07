
yyparser: src/parser/ncc.y
	cd src/parser && bison ncc.y

yylexer: src/lexer/ncc.l yyparser
	cd src/lexer && flex ncc.l
	g++ -std=c++11 -c -o bin/yylexer.o src/lexer/yylexer.cpp

lextest: yylexer
	g++ -O3 -o bin/yylexer.exe src/lexer/yylexer.cpp


clean:
	rm bin/*.o
	rm bin/*.exe
