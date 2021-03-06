CXX = g++ -std=c++14 -O0 -g

LLVM_HEADER = -I`llvm-config --includedir`
LLVM_LIB = `llvm-config --ldflags` `llvm-config --system-libs --libs`

HEADER = src/core/operator.h src/core/typeEnum.h src/core/symbol.h \
		src/core/type.h src/core/constant.h \
		src/ast/node.h src/codegen/context.h src/codegen/codegen.h
CORE_SRC = driver symbol type constant
AST_SRC = basic expression declaration class statement declarator
CODEGEN_SRC = codegen

OBJ_DIR = bin
OBJ = $(CORE_SRC:%=$(OBJ_DIR)/%.o) \
	$(AST_SRC:%=$(OBJ_DIR)/ast_%.o) \
	$(AST_SRC:%=$(OBJ_DIR)/gen_%.o) \
	$(CODEGEN_SRC:%=$(OBJ_DIR)/cg_%.o) \
	$(OBJ_DIR)/yylexer.o \
	$(OBJ_DIR)/yyparser.o \
	$(OBJ_DIR)/context.o \
	$(OBJ_DIR)/mipsgenpass.o

$(OBJ_DIR): 
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/yyparser.o: $(OBJ_DIR) src/parser/ncc.y src/ast/node.h
	cd src/parser && bison ncc.y --report=state
	$(CXX) -c -o $(OBJ_DIR)/yyparser.o src/parser/yyparser.cpp

$(OBJ_DIR)/yylexer.o: $(OBJ_DIR) src/lexer/ncc.l $(OBJ_DIR)/yyparser.o
	cd src/lexer && flex ncc.l
	$(CXX) -c -o $(OBJ_DIR)/yylexer.o src/lexer/yylexer.cpp

$(OBJ_DIR)/context.o: src/parser/context.cpp src/parser/context.h
	$(CXX) -c -o $(OBJ_DIR)/context.o src/parser/context.cpp

$(OBJ_DIR)/%.o: src/core/%.cpp src/core/%.h $(OBJ_DIR)/yyparser.o $(HEADER)
	$(CXX) -c -o $@ $< $(LLVM_HEADER)

$(OBJ_DIR)/ast_%.o: src/ast/%.cpp $(OBJ_DIR)/yyparser.o $(HEADER)
	$(CXX) -c -o $@ $<

$(OBJ_DIR)/gen_%.o: src/codegen/%.cpp $(OBJ_DIR)/yyparser.o $(HEADER) 
	$(CXX) -c -o $@ $< $(LLVM_HEADER)

$(OBJ_DIR)/cg_%.o: src/codegen/%.cpp $(HEADER) 
	$(CXX) -c -o $@ $< $(LLVM_HEADER)

$(OBJ_DIR)/mipsgenpass.o: src/pass/MipsAsmGen/MipsAssemblyGenPass.cpp
	$(CXX) -c -o $@ $< $(LLVM_HEADER)

$(OBJ_DIR)/lextest.exe: $(OBJ) src/lexer/lextest.cpp
	$(CXX) -o $@ $^ $(LLVM_LIB)

$(OBJ_DIR)/parsetest.exe: $(OBJ) src/parser/parsetest.cpp
	$(CXX) -o $@ $^ $(LLVM_LIB)

$(OBJ_DIR)/ncc.exe: $(OBJ) src/core/ncc.cpp
	$(CXX) -o $@ $^ $(LLVM_LIB)

.PHONY: clean

lextest: $(OBJ_DIR)/lextest.exe

parsetest: $(OBJ_DIR)/parsetest.exe

ncc: $(OBJ_DIR)/ncc.exe

clean:
	-rm $(OBJ_DIR)/*.o
	-rm $(OBJ_DIR)/*.exe
	-rmdir $(OBJ_DIR)