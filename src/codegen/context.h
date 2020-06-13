#pragma once

#include "../core/constant.h"
#include "../core/symbol.h"
#include "../parser/yylocation.h"
#include "codegen.h"
#include "llvm.h"

#include <list>
#include <ostream>

enum class DeclState : std::uint8_t {
    NODECL,     // only allow symbol reference
    PARAMDECL,  // new symbol (no class or enum definition, allow incomplete type)
    MINDECL,    // new symbol (no class or enum definition)
    LOCALDECL,  // new symbol (no static data member)
    FULLDECL    // new symbol
};

struct ExprState
{
    bool isConstant;
    union {
        Constant     constant;
        llvm::Value *value;
    };
    Constant *constOrNull() { return isConstant ? &constant : nullptr; }
};

struct CodegenContext
{
    std::ostream &errorStream;
    std::ostream &outputStream;
    int &         errCnt;
    bool          printLocalTable;

    llvm::LLVMContext &llvmContext;
    llvm::Module &     module;
    llvm::IRBuilder<> *IRBuilder;
    CodeGenHelper &    cgHelper;

    SymbolTable *                    symtab;
    Type                             type;
    SymbolSet                        symbolSet;
    Symbol                           newSymbol;
    SymbolTable *                    qualifiedScope;
    std::vector<Type::PtrDescriptor> ptrDescList;
    std::list<CodegenContext> *      secondPassContexts;

    ExprState expr;

    struct
    {
        bool               keepScope;
        bool               isSwitchLevel;
        bool               isInSwitch;
        bool               isInLoop;
        llvm::BasicBlock * breakBB;
        llvm::BasicBlock * continueBB;
        llvm::SwitchInst * switchInst;
    } stmt;

    struct
    {
        DeclState         state;
        bool              isFriend;
        bool              isTypedef;
        bool              mustComplete;
        int8_t            memberFirstPass;
        Symbol::Attribute symbolAccessAttr;
    } decl;
};

class SemanticError : std::runtime_error
{
public:
    template <typename T>
    SemanticError(T msg, yy::location loc)
        : std::runtime_error(std::move(msg))
        , location(loc)
    {}

    friend inline std::ostream &operator<<(std::ostream &os, SemanticError err)
    {
        return os << "error " << err.location << ": " << err.what() << '\n';
    }

private:
    yy::location location;
};