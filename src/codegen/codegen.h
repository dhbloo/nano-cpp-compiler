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
    CodeGenHelper(llvm::LLVMContext &llvmContext,
                  llvm::Module &     module,
                  llvm::IRBuilder<> &IRBuilder);

    llvm::Constant *CreateConstant(const ::Type &t, ::Constant constant);
    llvm::Constant *CreateStringConstant(std::string string);
    llvm::Constant *CreateZeroConstant(const ::Type &t = FundType::INT);

    llvm::Type *      MakeType(const ::Type &t);
    llvm::StructType *MakeClass(const ClassDescriptor *classDesc);
    llvm::Value *     ConvertType(::Type fromT, const ::Type &toT, llvm::Value *fromV);
    llvm::Value *
    CreateValue(const ::Type &fromT, const ::Type &toT, const ExprState &expr);

    void GenZeroInit(SymbolSet varSymbol);
    void
    GenAssignInit(SymbolSet varSymbol, const ::Type &exprType, const ExprState &expr);

private:
    llvm::Constant *CreateFundTypeConstant(FundType fundType, ::Constant constant);

    llvm::Type *        MakeFundType(FundType ft);
    llvm::FunctionType *MakeFunction(const FunctionDescriptor *funcDesc);
    llvm::Value *       ConvertFundType(FundType fromT, FundType toT, llvm::Value *fromV);

    llvm::LLVMContext &                                             ctx;
    llvm::Module &                                                  module;
    llvm::IRBuilder<> &                                             Builder;
    std::unordered_map<const ClassDescriptor *, llvm::StructType *> structTypeMap;
};