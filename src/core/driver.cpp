#include "driver.h"

#include "../codegen/context.h"
#include "../parser/yyparser.h"

Driver::Driver(std::ostream &errorStream) : errorStream(errorStream) {}

bool Driver::Parse(bool isDebugMode, bool printLocalTable)
{
    ast          = {};
    globalSymtab = {};
    llvmContext  = {};
    module       = {};

    /* Parser analysis */

    int        errCnt = 0;
    yy::parser parser(ast, errCnt, errorStream, {});

    // parser.set_debug_level(isDebugMode);
    errCnt += parser() != 0;

    if (errCnt > 0) {
        errorStream << "parsing failed, " << errCnt << " error generated!\n";
        return false;
    }

    /* Semantic analysis & Code generation */

    globalSymtab = std::make_unique<SymbolTable>(nullptr);
    llvmContext  = std::make_unique<llvm::LLVMContext>();
    module       = std::make_unique<llvm::Module>("NCC Module", *llvmContext);
    llvm::IRBuilder<> IRBuilder(*llvmContext);
    CodeGenHelper     cgHelper(*llvmContext, *module);

    CodegenContext context {errorStream,
                            std::cout,
                            errCnt,
                            printLocalTable,
                            *llvmContext,
                            *module,
                            &IRBuilder,
                            cgHelper,
                            globalSymtab.get()};
    ast->Codegen(context);

    if (errCnt > 0) {
        errorStream << "semantic check failed, " << errCnt << " error generated!\n";
        return false;
    }

    return true;
}

void Driver::PrintSymbolTable(std::ostream &os) const
{
    if (globalSymtab)
        globalSymtab->Print(os);
}

void Driver::PrintIR() const
{
    module->print(llvm::outs(), nullptr);
}