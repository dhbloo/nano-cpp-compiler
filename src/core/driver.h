#pragma once

#include "../ast/node.h"
#include "../codegen/llvm.h"
#include "symbol.h"

class Driver
{
public:
    Driver(std::ostream &errorStream);

    bool Parse(bool isDebugMode = false, bool printLocalTable = false);
    void Optimize();
    void PrintSymbolTable(std::ostream &os) const;
    void PrintIR() const;

private:
    std::ostream &errorStream;

    ast::Ptr<ast::TranslationUnit>     ast;
    std::unique_ptr<SymbolTable>       globalSymtab;
    std::unique_ptr<llvm::LLVMContext> llvmContext;
    std::unique_ptr<llvm::Module>      module;
};