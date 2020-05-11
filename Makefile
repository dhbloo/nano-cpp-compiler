CXX = g++ -std=c++11 -Os

yyparser: src/parser/ncc.y
	cd src/parser && bison ncc.y --report=state
	$(CXX) -c -o bin/yyparser.o src/parser/yyparser.cpp

yylexer: src/lexer/ncc.l yyparser
	cd src/lexer && flex ncc.l
	$(CXX) -c -o bin/yylexer.o src/lexer/yylexer.cpp

ast: src/ast/node.h src/ast/basic.cpp src/ast/expression.cpp src/ast/declaration.cpp \
		src/ast/class.cpp src/ast/statement.cpp src/ast/declarator.cpp
	$(CXX) -c -o bin/ast_basic.o src/ast/basic.cpp
	$(CXX) -c -o bin/ast_expression.o src/ast/expression.cpp
	$(CXX) -c -o bin/ast_declaration.o src/ast/declaration.cpp
	$(CXX) -c -o bin/ast_declarator.o src/ast/declarator.cpp
	$(CXX) -c -o bin/ast_class.o src/ast/class.cpp
	$(CXX) -c -o bin/ast_statement.o src/ast/statement.cpp

lextest: yylexer
	$(CXX) -o bin/lextest.exe src/lexer/lextest.cpp bin/yylexer.o bin/yyparser.o

parsetest: yylexer yyparser ast
	$(CXX) -o bin/parsetest.exe src/parser/parsetest.cpp bin/yylexer.o bin/yyparser.o \
		bin/ast_basic.o bin/ast_expression.o bin/ast_declaration.o bin/ast_class.o \
		bin/ast_statement.o bin/ast_declarator.o


clean:
	rm bin/*.o
	rm bin/*.exe
