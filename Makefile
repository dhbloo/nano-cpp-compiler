CXX = g++ -std=c++11 -O0 -g

CORE_SRC = driver symbol type
AST_SRC = basic expression declaration class statement declarator

OBJ_DIR = bin
OBJ = $(CORE_SRC:%=$(OBJ_DIR)/%.o) \
	$(AST_SRC:%=$(OBJ_DIR)/ast_%.o) \
	$(AST_SRC:%=$(OBJ_DIR)/sem_%.o)
YY = $(OBJ_DIR)/yylexer.o $(OBJ_DIR)/yyparser.o $(OBJ_DIR)/context.o

$(OBJ_DIR)/yyparser.o: src/parser/ncc.y
	cd src/parser && bison ncc.y --report=state
	$(CXX) -c -o $(OBJ_DIR)/yyparser.o src/parser/yyparser.cpp

$(OBJ_DIR)/yylexer.o: src/lexer/ncc.l $(OBJ_DIR)/yyparser.o
	cd src/lexer && flex ncc.l
	$(CXX) -c -o $(OBJ_DIR)/yylexer.o src/lexer/yylexer.cpp

$(OBJ_DIR)/context.o: src/parser/context.cpp src/parser/context.h
	$(CXX) -c -o $(OBJ_DIR)/context.o src/parser/context.cpp

$(OBJ_DIR)/%.o: src/core/%.cpp
	$(CXX) -c -o $@ $<

$(OBJ_DIR)/ast_%.o: src/ast/%.cpp $(OBJ_DIR)/yyparser.o src/ast/node.h
	$(CXX) -c -o $@ $<

$(OBJ_DIR)/sem_%.o: src/semantic/%.cpp $(OBJ_DIR)/yyparser.o src/ast/node.h
	$(CXX) -c -o $@ $<

$(OBJ_DIR)/lextest.exe: $(YY) $(OBJ) src/lexer/lextest.cpp
	$(CXX) -o $@ $<

$(OBJ_DIR)/parsetest.exe: $(YY) $(OBJ) src/parser/parsetest.cpp
	$(CXX) -o $@ $<

$(OBJ_DIR)/ncc.exe: $(YY) $(OBJ) src/core/ncc.cpp
	$(CXX) -o $@ $<

.PHONY: clean

lextest: $(OBJ_DIR)/lextest.exe

parsetest: $(OBJ_DIR)/parsetest.exe

ncc: $(OBJ_DIR)/ncc.exe

clean:
	-rm $(OBJ_DIR)/*.o
	-rm $(OBJ_DIR)/*.exe