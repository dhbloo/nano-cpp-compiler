CXX = g++ -std=c++11 -Os -c

yyparser: src/parser/ncc.y
	cd src/parser && bison ncc.y
	$(CXX) -o bin/yyparser.o src/parser/yyparser.cpp

yylexer: src/lexer/ncc.l yyparser
	cd src/lexer && flex ncc.l
	$(CXX) -o bin/yylexer.o src/lexer/yylexer.cpp

lextest: yylexer
	g++ -std=c++11 -Os -o bin/lextest.exe src/lexer/lextest.cpp bin/yylexer.o bin/yyparser.o


clean:
	rm bin/*.o
	rm bin/*.exe
