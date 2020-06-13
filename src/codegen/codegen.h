#pragma once

#include "../core/constant.h"
#include "../core/symbol.h"
#include "../core/type.h"
#include "llvm.h"

#include <map>

struct ExprState;

class CodeGenHelper
{
public:
    CodeGenHelper(llvm::LLVMContext &llvmContext, llvm::Module &module);

    llvm::Constant *CreateConstant(const ::Type &t, ::Constant constant);
    llvm::Constant *CreateStringConstant(std::string string);
    llvm::Constant *CreateZeroConstant(const ::Type &t = FundType::INT);

    llvm::Type * MakeType(const ::Type &t);
    llvm::Value *ConvertType(llvm::IRBuilder<> &IRBuilder,
                             ::Type             fromT,
                             const ::Type &     toT,
                             llvm::Value *      fromV);

    void GenZeroInit(llvm::IRBuilder<> &IRBuilder, SymbolSet varSymbol);
    void GenAssignInit(llvm::IRBuilder<> &IRBuilder,
                       SymbolSet          varSymbol,
                       const ::Type &     exprType,
                       const ExprState &  expr);

private:
    llvm::Constant *CreateFundTypeConstant(FundType fundType, ::Constant constant);

    llvm::Type * MakeFundType(FundType ft);
    llvm::Type * MakeClass(const ClassDescriptor *classDesc);
    llvm::Type * MakeFunction(const FunctionDescriptor *funcDesc);
    llvm::Value *ConvertFundType(llvm::IRBuilder<> &IRBuilder,
                                 FundType           fromT,
                                 FundType           toT,
                                 llvm::Value *      fromV);

    llvm::LLVMContext &                                             ctx;
    llvm::Module &                                                  module;
    std::unordered_map<const ClassDescriptor *, llvm::StructType *> structTypeMap;
};