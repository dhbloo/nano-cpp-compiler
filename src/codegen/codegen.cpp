#include "codegen.h"

#include "../core/symbol.h"
#include "context.h"

#include <cassert>

using namespace llvm;

CodeGenHelper::CodeGenHelper(LLVMContext &llvmContext,
                             Module &     module,
                             IRBuilder<> &Builder)
    : ctx(llvmContext)
    , module(module)
    , Builder(Builder)
{}

llvm::Constant *CodeGenHelper::CreateConstant(const ::Type &t, ::Constant constant)
{
    if (t.IsSimple(TypeKind::FUNDTYPE)) {
        return CreateFundTypeConstant(t.fundType, constant);
    }
    else if (t.IsSimple(TypeKind::ENUM)) {
        return CreateFundTypeConstant(FundType::INT, constant);
    }
    else if (t.IsPtr()) {
        if (constant.intVal == 0) {
            auto pointerT = PointerType::get(MakeType(t.ElementType()), 0);
            return ConstantPointerNull::get(pointerT);
        }
        else {
            auto constAddr = CreateFundTypeConstant(FundType::LONG, constant);
            return ConstantExpr::getIntToPtr(constAddr, MakeType(t));
        }
    }
    else if (t.IsMemberPtr()) {
        throw SemanticError("34 unimp", yy::location());
        if (constant.intVal == 0) {
            auto pointerT = PointerType::get(MakeType(t.ElementType()), 0);
            return ConstantPointerNull::get(pointerT);
        }
        else {
            auto constAddr = CreateFundTypeConstant(FundType::LONG, constant);
            return ConstantExpr::getIntToPtr(constAddr, MakeType(t));
        }
    }
    else {
        assert(false);
        return nullptr;
    }
}

llvm::Constant *CodeGenHelper::CreateStringConstant(std::string string)
{
    return ConstantDataArray::getString(ctx, std::move(string), true);
}

llvm::Constant *CodeGenHelper::CreateZeroConstant(const ::Type &t)
{
    if (t.IsArray() || t.IsSimple(TypeKind::CLASS)) {
        auto p = ConstantAggregateZero::get(MakeType(t));
        assert(p);
        return p;
    }
    else {
        return CreateConstant(t, ::Constant {});
    }
}

llvm::Type *CodeGenHelper::MakeType(const ::Type &t)
{
    if (t.IsSimple(TypeKind::FUNDTYPE)) {
        return MakeFundType(t.fundType);
    }
    else if (t.IsSimple(TypeKind::ENUM)) {
        return MakeFundType(FundType::INT);
    }
    else if (t.IsSimple(TypeKind::CLASS)) {
        return MakeClass(t.Class());
    }
    else if (t.IsSimple(TypeKind::FUNCTION)) {
        return MakeFunction(t.Function());
    }
    else if (t.IsRef()) {
        return PointerType::get(MakeType(t.RemoveRef()), 0);
    }
    else if (t.IsPtr()) {
        ::Type elemT = t.RemovePtr();
        if (elemT.IsSimple(TypeKind::FUNDTYPE) && elemT.fundType == FundType::VOID)
            return PointerType::get(llvm::Type::getInt8Ty(ctx), 0);
        else
            return PointerType::get(MakeType(elemT), 0);
    }
    else if (t.IsMemberPtr()) {
        // TODO
        throw SemanticError("93 unimp", yy::location());
        return nullptr;
    }
    else if (t.IsArray()) {
        return ArrayType::get(MakeType(t.ElementType()), t.ArraySize());
    }
    else {
        assert(false);
        return nullptr;
    }
}

llvm::Value *
CodeGenHelper::ConvertType(::Type fromT, const ::Type &toT, llvm::Value *fromV)
{
    llvm::Value *toV = fromV;

    // 0. identical type
    if (fromT == toT)
        return toV;

    // 1. l-value to r-value
    if (fromT.IsRef() && !toT.IsRef() && !fromT.RemoveRef().IsSimple(TypeKind::FUNCTION)
        && !fromT.RemoveRef().IsArray()) {
        // Load variable (non function)
        fromT = fromT.RemoveRef();
        toV   = Builder.CreateAlignedLoad(toV, llvm::Align(fromT.Alignment()));

        // remove cv for non class type
        if (!fromT.IsSimple(TypeKind::CLASS))
            fromT.cv = CVQualifier::NONE;
    }
    // 2. array (reference) to pointer
    else if (fromT.RemoveRef().IsArray() && toT.IsPtr()) {
        fromT = fromT.RemoveRef().ElementType().AddPtrDesc(
            ::Type::PtrDescriptor {PtrType::PTR});

        std::array<Value *, 2> idx;
        idx[0] = CreateZeroConstant();
        idx[1] = CreateZeroConstant();
        toV    = Builder.CreateInBoundsGEP(fromV, idx);
    }
    // 3. function (reference) to pointer / member pointer
    else if (fromT.RemoveRef().IsSimple(TypeKind::FUNCTION) && toT.IsPtr()
             && toT.RemovePtr().IsSimple(TypeKind::FUNCTION)) {
        fromT = fromT.RemoveRef();
        if (!fromT.Function()->IsNonStaticMember())
            fromT.AddPtrDesc(::Type::PtrDescriptor {PtrType::PTR});
        else {
            // member function to member pointer
            fromT.AddPtrDesc(
                ::Type::PtrDescriptor {PtrType::CLASSPTR,
                                       CVQualifier::NONE,
                                       fromT.Function()->funcScope->GetCurrentClass()});
        }
    }
    // O. r-value to const l-value (creates temporary)
    else if (!fromT.IsRef() && toT.IsRef() && toT.cv == CVQualifier::CONST
             && !fromT.IsSimple(TypeKind::FUNCTION)) {
        auto tempVar = Builder.CreateAlloca(MakeType(fromT), nullptr);
        tempVar->setAlignment(llvm::Align(fromT.Alignment()));

        Builder.CreateAlignedStore(toV, tempVar, llvm::Align(fromT.Alignment()));
        toV = tempVar;

        fromT.AddPtrDesc(::Type::PtrDescriptor {PtrType::REF});
        fromT.cv = CVQualifier::CONST;
    }
    // O. function to function reference
    else if (fromT.IsSimple(TypeKind::FUNCTION) && toT.IsRef()
             && toT.RemoveRef().IsSimple(TypeKind::FUNCTION)) {
        fromT.AddPtrDesc(::Type::PtrDescriptor {PtrType::REF});
    }

    if (toT == fromT)
        return toV;

    // 4~8, 10. numeric conversion & bool conversion
    if (fromT.IsSimple(TypeKind::FUNDTYPE) && toT.IsSimple(TypeKind::FUNDTYPE)) {
        toV = ConvertFundType(fromT.fundType, toT.fundType, toV);
    }
    // 4, 10. integer promotion: enum to int (to float) & bool conversion: enum to bool
    else if (fromT.IsSimple(TypeKind::ENUM) && toT.IsSimple(TypeKind::FUNDTYPE)) {
        toV = ConvertFundType(FundType::INT, toT.fundType, toV);
    }
    // 9. pointer conversion
    else if (toT.IsPtr()) {
        if (fromT.IsSimple(TypeKind::FUNDTYPE) && fromT.fundType == FundType::INT
            && isa<ConstantInt>(toV)
            && cast<ConstantInt>(toV)->getValue().isNullValue()) {
            toV = CreateConstant(toT, ::Constant {});
        }
        // object pointer to void pointer
        else if (fromT.IsPtr() && toT.RemovePtr().IsSimple(TypeKind::FUNDTYPE)
                 && toT.fundType == FundType::VOID) {
            toV = Builder.CreatePointerCast(toV, MakeType(toT));
        }
        // pointer to derived class to pointer to base class
        else if (fromT.IsPtr() && fromT.RemovePtr().IsSimple(TypeKind::CLASS)
                 && toT.RemovePtr().IsSimple(TypeKind::CLASS)) {
            toV = Builder.CreatePointerCast(toV, MakeType(toT));
        }
    }
    // 9. member pointer conversion
    else if (toT.IsMemberPtr()) {
        // literal '0' to pointer
        if (fromT.IsSimple(TypeKind::FUNDTYPE) && fromT.fundType == FundType::INT
            && isa<ConstantInt>(toV)
            && cast<ConstantInt>(toV)->getValue().isNullValue()) {
            toV = CreateConstant(toT, ::Constant {});
        }
        // base member pointer to derived class member pointer
        else if (fromT.IsMemberPtr() && fromT.RemoveMemberPtr().IsSimple(TypeKind::CLASS)
                 && toT.RemoveMemberPtr().IsSimple(TypeKind::CLASS)) {
            toV = Builder.CreatePointerCast(toV, MakeType(toT));
        }
    }
    // 10. bool conversion: pointer to bool
    else if (fromT.IsPtr() && toT.IsSimple(TypeKind::FUNDTYPE)
             && toT.fundType == FundType::BOOL) {
        toV = Builder.CreatePtrToInt(toV, MakeType(FundType::BOOL));
    }

    return toV;
}

llvm::Value *
CodeGenHelper::CreateValue(const ::Type &fromT, const ::Type &toT, const ExprState &expr)
{
    if (expr.isConstant) {
        return CreateConstant(toT,
                              fromT == toT
                                  ? expr.constant
                                  : expr.constant.Convert(fromT.fundType, toT.fundType));
    }
    else {
        return ConvertType(fromT, toT, expr.value);
    }
}

void CodeGenHelper::GenZeroInit(SymbolSet varSymbol)
{
    if (varSymbol.Scope()->GetParent()) {
        Builder.CreateAlignedStore(CreateZeroConstant(varSymbol->type),
                                   varSymbol->value,
                                   Align(varSymbol->type.Alignment()));
    }
    else {
        auto globalVar = module.getGlobalVariable(varSymbol->id, true);
        globalVar->setInitializer(CreateZeroConstant(varSymbol->type));
    }
}

void CodeGenHelper::GenAssignInit(SymbolSet        varSymbol,
                                  const ::Type &   exprType,
                                  const ExprState &expr)
{
    if (varSymbol.Scope()->GetParent()) {
        if (expr.isConstant) {
            Builder.CreateAlignedStore(CreateConstant(varSymbol->type, expr.constant),
                                       varSymbol->value,
                                       Align(varSymbol->type.Alignment()));
        }
        else {
            Builder.CreateAlignedStore(ConvertType(exprType, varSymbol->type, expr.value),
                                       varSymbol->value,
                                       Align(varSymbol->type.Alignment()));
        }
    }
    else {
        auto globalVar = module.getGlobalVariable(varSymbol->id, true);
        if (expr.isConstant) {
            auto constant = CreateConstant(varSymbol->type, expr.constant);
            globalVar->setInitializer(constant);
        }
        else {
            throw SemanticError("226 unimplemented", yy::location());
        }
    }
}

llvm::Constant *CodeGenHelper::CreateFundTypeConstant(FundType   fundType,
                                                      ::Constant constant)
{
    switch (fundType) {
    case FundType::BOOL:
        return ConstantInt::get(ctx, APInt(1, constant.boolVal));
    case FundType::CHAR:
        return ConstantInt::get(ctx, APInt(8, constant.charVal));
    case FundType::UCHAR:
        return ConstantInt::get(ctx, APInt(8, (uint8_t)constant.charVal));
    case FundType::SHORT:
        return ConstantInt::get(ctx, APInt(16, (int16_t)constant.intVal));
    case FundType::USHORT:
        return ConstantInt::get(ctx, APInt(16, (uint16_t)constant.intVal));
    case FundType::INT:
        return ConstantInt::get(ctx, APInt(32, (int32_t)constant.intVal));
    case FundType::UINT:
        return ConstantInt::get(ctx, APInt(32, (uint32_t)constant.intVal));
    case FundType::LONG:
        return ConstantInt::get(ctx, APInt(64, (int64_t)constant.intVal));
    case FundType::ULONG:
        return ConstantInt::get(ctx, APInt(64, (uint64_t)constant.intVal));
    case FundType::FLOAT:
        return ConstantFP::get(ctx, APFloat((float)constant.floatVal));
    case FundType::DOUBLE:
        return ConstantFP::get(ctx, APFloat(constant.floatVal));
    default:
        assert(false);
        return nullptr;
    }
}

llvm::Type *CodeGenHelper::MakeFundType(FundType ft)
{
    switch (ft) {
    case FundType::BOOL:
        return llvm::Type::getInt1Ty(ctx);
    case FundType::CHAR:
    case FundType::UCHAR:
        return llvm::Type::getInt8Ty(ctx);
    case FundType::SHORT:
    case FundType::USHORT:
        return llvm::Type::getInt16Ty(ctx);
    case FundType::INT:
    case FundType::UINT:
        return llvm::Type::getInt32Ty(ctx);
    case FundType::LONG:
    case FundType::ULONG:
        return llvm::Type::getInt64Ty(ctx);
    case FundType::FLOAT:
        return llvm::Type::getFloatTy(ctx);
    case FundType::DOUBLE:
        return llvm::Type::getDoubleTy(ctx);
    default:
        return llvm::Type::getVoidTy(ctx);
    }
}

llvm::Type *CodeGenHelper::MakeClass(const ClassDescriptor *classDesc)
{
    auto it = structTypeMap.find(classDesc);
    if (it != structTypeMap.end())
        return it->second;

    std::vector<llvm::Type *> membersT;
    auto                      sortedSymbols = classDesc->memberTable->SortedSymbols();

    for (const auto &member : sortedSymbols) {
        membersT.push_back(MakeType(member->type));
    }

    StructType *structType   = StructType::create(ctx, membersT, classDesc->FullName());
    structTypeMap[classDesc] = structType;
    return structType;
}

llvm::Type *CodeGenHelper::MakeFunction(const FunctionDescriptor *funcDesc)
{
    std::vector<llvm::Type *> paramsT;

    auto resultT = MakeType(funcDesc->retType);
    for (const auto &param : funcDesc->paramList) {
        paramsT.push_back(MakeType(param.symbol->type));
    }

    return FunctionType::get(resultT, paramsT, false);
}

Value *CodeGenHelper::ConvertFundType(FundType fromT, FundType toT, Value *fromV)
{
    Value *toV = nullptr;

    switch (fromT) {
    case FundType::BOOL:

        switch (toT) {
        case FundType::BOOL:
            toV = fromV;
            break;

        case FundType::CHAR:
        case FundType::SHORT:
        case FundType::INT:
        case FundType::LONG:
        case FundType::UCHAR:
        case FundType::USHORT:
        case FundType::UINT:
        case FundType::ULONG:
            toV = Builder.CreateZExt(fromV, MakeType(toT));
            break;

        case FundType::FLOAT:
        case FundType::DOUBLE:
            toV = Builder.CreateSIToFP(fromV, MakeType(toT));
            break;

        default:
            break;
        }
        break;

    case FundType::CHAR:
    case FundType::SHORT:
    case FundType::INT:
    case FundType::LONG:

        switch (toT) {
        case FundType::BOOL:
            toV = Builder.CreateICmpNE(fromV, CreateZeroConstant(fromT));
            break;

        case FundType::CHAR:
        case FundType::SHORT:
        case FundType::INT:
        case FundType::LONG:
        case FundType::UCHAR:
        case FundType::USHORT:
        case FundType::UINT:
        case FundType::ULONG:
            toV = Builder.CreateSExtOrTrunc(fromV, MakeType(toT));
            break;

        case FundType::FLOAT:
        case FundType::DOUBLE:
            toV = Builder.CreateSIToFP(fromV, MakeType(toT));
            break;

        default:
            break;
        }
        break;

    case FundType::UCHAR:
    case FundType::USHORT:
    case FundType::UINT:
    case FundType::ULONG:

        switch (toT) {
        case FundType::BOOL:
            toV = Builder.CreateICmpNE(fromV, CreateZeroConstant(fromT));
            break;

        case FundType::CHAR:
        case FundType::SHORT:
        case FundType::INT:
        case FundType::LONG:

        case FundType::UCHAR:
        case FundType::USHORT:
        case FundType::UINT:
        case FundType::ULONG:
            toV = Builder.CreateZExtOrTrunc(fromV, MakeType(toT));
            break;

        case FundType::FLOAT:
        case FundType::DOUBLE:
            toV = Builder.CreateUIToFP(fromV, MakeType(toT));

        default:
            break;
        }
        break;

    case FundType::FLOAT:
    case FundType::DOUBLE:

        switch (toT) {
        case FundType::BOOL:
            toV = Builder.CreateFCmpUNE(fromV, CreateZeroConstant(fromT));
            break;

        case FundType::CHAR:
        case FundType::SHORT:
        case FundType::INT:
        case FundType::LONG:
            toV = Builder.CreateFPToSI(fromV, MakeType(toT));
            break;

        case FundType::UCHAR:
        case FundType::USHORT:
        case FundType::UINT:
        case FundType::ULONG:
            toV = Builder.CreateFPToUI(fromV, MakeType(toT));
            break;

        case FundType::FLOAT:
        case FundType::DOUBLE:
            toV = Builder.CreateFPCast(fromV, MakeType(toT));
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return toV;
}
