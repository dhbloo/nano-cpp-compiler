#pragma once

#include "../ast/node.h"
#include "../llvm.h"
#include "symbol.h"

class Driver
{
public:
    Driver(std::ostream &errorStream);

    bool        Parse(bool isDebugMode = false, bool printLocalTable = false);
    void        Optimize();
    std::string PrintSymbolTable() const;
    std::string PrintIR() const;
    bool        EmitAssemblyCode(std::string filename) const;
    bool        EmitSimpleMipsCode(std::string filename) const;

private:
    std::ostream &errorStream;

    ast::Ptr<ast::TranslationUnit>     ast;
    std::unique_ptr<SymbolTable>       globalSymtab;
    std::unique_ptr<llvm::LLVMContext> llvmContext;
    std::unique_ptr<llvm::Module>      module;
};