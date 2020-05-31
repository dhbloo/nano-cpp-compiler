CXX = g++ -std=c++11 -Og -g

yyparser: src/parser/ncc.y
	cd src/parser && bison ncc.y --report=state
	$(CXX) -c -o bin/yyparser.o src/parser/yyparser.cpp
	$(CXX) -c -o bin/context.o src/parser/context.cpp

yylexer: src/lexer/ncc.l yyparser
	cd src/lexer && flex ncc.l
	$(CXX) -c -o bin/yylexer.o src/lexer/yylexer.cpp

ast: yyparser src/ast/node.h src/ast/basic.cpp src/ast/expression.cpp src/ast/declaration.cpp \
		src/ast/class.cpp src/ast/statement.cpp src/ast/declarator.cpp
	$(CXX) -c -o bin/ast_basic.o src/ast/basic.cpp
	$(CXX) -c -o bin/ast_expression.o src/ast/expression.cpp
	$(CXX) -c -o bin/ast_declaration.o src/ast/declaration.cpp
	$(CXX) -c -o bin/ast_declarator.o src/ast/declarator.cpp
	$(CXX) -c -o bin/ast_class.o src/ast/class.cpp
	$(CXX) -c -o bin/ast_statement.o src/ast/statement.cpp

core: src/core/driver.cpp src/core/symbol.cpp src/core/type.cpp
	$(CXX) -c -o bin/driver.o src/core/driver.cpp
	$(CXX) -c -o bin/symbol.o src/core/symbol.cpp
	$(CXX) -c -o bin/type.o src/core/type.cpp

lextest: yylexer yyparser ast
	$(CXX) -o bin/lextest.exe src/lexer/lextest.cpp bin/yylexer.o bin/yyparser.o \
		bin/context.o bin/ast_basic.o bin/ast_expression.o bin/ast_declaration.o \
		bin/ast_class.o bin/ast_statement.o bin/ast_declarator.o

parsetest: yylexer yyparser ast core src/parser/parsetest.cpp
	$(CXX) -o bin/parsetest.exe src/parser/parsetest.cpp bin/yylexer.o bin/yyparser.o \
		bin/context.o bin/ast_basic.o bin/ast_expression.o bin/ast_declaration.o \
		bin/ast_class.o bin/ast_statement.o bin/ast_declarator.o \
		bin/driver.o bin/symbol.o bin/type.o

semantic: yylexer yyparser ast core src/core/ncc.cpp
	$(CXX) -o bin/ncc.exe src/core/ncc.cpp bin/yylexer.o bin/yyparser.o \
		bin/context.o bin/ast_basic.o bin/ast_expression.o bin/ast_declaration.o \
		bin/ast_class.o bin/ast_statement.o bin/ast_declarator.o \
		bin/driver.o bin/symbol.o bin/type.o

clean:
	rm bin/*.o
	rm bin/*.exe
