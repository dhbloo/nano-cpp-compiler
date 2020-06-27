#include "driver.h"

#include "../codegen/context.h"
#include "../parser/yyparser.h"
#include "../pass/MipsAsmGen/MipsAssemblyGenPass.h"

#include <sstream>

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
    CodeGenHelper     cgHelper(*llvmContext, *module, IRBuilder);

    CodegenContext context {errorStream,
                            std::cout,
                            errCnt,
                            printLocalTable,
                            *llvmContext,
                            *module,
                            IRBuilder,
                            cgHelper,
                            globalSymtab.get()};
    ast->Codegen(context);

    if (errCnt > 0) {
        errorStream << "semantic check failed, " << errCnt << " error generated!\n";
        return false;
    }

    return true;
}

void Driver::Optimize()
{
    llvm::legacy::FunctionPassManager fpm(module.get());

    // Promote allocas to registers.
    fpm.add(llvm::createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    fpm.add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    fpm.add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    fpm.add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    fpm.add(llvm::createCFGSimplificationPass());

    fpm.doInitialization();
    for (auto &function : module->functions()) {
        fpm.run(function);
    }
    fpm.doFinalization();
}

std::string Driver::PrintSymbolTable() const
{
    std::stringstream ss;
    if (globalSymtab)
        globalSymtab->Print(ss);

    ss.flush();
    return std::move(ss.str());
}

std::string Driver::PrintIR() const
{
    std::string              IR;
    llvm::raw_string_ostream ss(IR);

    module->print(ss, nullptr);
    ss.flush();

    return std::move(IR);
}

bool Driver::EmitAssemblyCode(std::string filename) const
{
    auto targetTriple = "mips-unknown-linux-gnu";
    // auto targetTriple = llvm::sys::getDefaultTargetTriple();

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    auto        target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        errorStream << "Target not found: " << error << '\n';
        return false;
    }

    auto CPU      = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto                RM = llvm::Optional<llvm::Reloc::Model>();
    auto                targetMachine =
        target->createTargetMachine(targetTriple, CPU, Features, opt, RM);

    module->setDataLayout(targetMachine->createDataLayout());
    module->setTargetTriple(targetTriple);

    std::error_code      ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_Text);

    if (ec) {
        errorStream << "Could not open file: " << ec.message() << '\n';
        return false;
    }

    llvm::legacy::PassManager pm;

    if (targetMachine->addPassesToEmitFile(pm, dest, nullptr, llvm::CGFT_AssemblyFile)) {
        errorStream << "Failed to add Emit File pass\n";
        return false;
    }

    pm.run(*module);
    dest.flush();
    return true;
}

bool Driver::EmitSimpleMipsCode(std::string filename) const
{
    llvm::legacy::PassManager pm;
    auto                      mipsPass = createMipsAssemblyGenPass();

    pm.add(mipsPass);
    pm.run(*module);

    std::error_code      ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_Text);

    if (ec) {
        errorStream << "Could not open file: " << ec.message() << '\n';
        return false;
    }

    mipsPass->print(dest, nullptr);
    dest.flush();
    return true;
}