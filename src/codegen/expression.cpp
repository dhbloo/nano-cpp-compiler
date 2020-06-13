#include "../ast/node.h"
#include "context.h"

#include <cassert>

namespace ast {

void AssignmentExpression::Codegen(CodegenContext &context) const
{
    left->Codegen(context);
    Type      leftType   = context.type;
    Type      lValueType = leftType.RemoveRef();
    SymbolSet varSymbol  = context.symbolSet;
    auto      lValue     = context.expr.value;

    if (!leftType.IsRef() || lValueType.IsSimple(TypeKind::FUNCTION)
        || context.expr.isConstant)
        throw SemanticError("left of expression is not assignable", srcLocation);
    else if (lValueType.IsConstInit()) {
        if (varSymbol)
            throw SemanticError("cannot assign to variable '" + varSymbol->id
                                    + "' with const-qualified type '" + lValueType.Name()
                                    + "'",
                                srcLocation);
        else
            throw SemanticError("left of expression is not assignable", srcLocation);
    }

    right->Codegen(context);

    if (!context.type.IsConvertibleTo(lValueType, context.expr.constOrNull()))
        throw SemanticError("assigning to '" + lValueType.Name()
                                + "' from incompatible type '" + context.type.Name()
                                + "'",
                            srcLocation);

    if (context.expr.isConstant) {
        context.expr.isConstant = false;
        context.expr.value =
            context.cgHelper.CreateConstant(context.type, context.expr.constant);
    }

    context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                      context.type,
                                                      lValueType,
                                                      context.expr.value);
    context.IRBuilder->CreateAlignedStore(context.expr.value,
                                          lValue,
                                          llvm::Align(lValueType.Alignment()));
    context.type       = leftType;
    context.symbolSet  = varSymbol;
    context.expr.value = lValue;
}

void ConditionalExpression::Codegen(CodegenContext &context) const
{
    condition->Codegen(context);
    Type condType = context.type.Decay();
    auto condExpr = context.expr;
    if (!condType.IsConvertibleTo(FundType::BOOL, condExpr.constOrNull()))
        throw SemanticError(condType.Name() + " is not convertible to bool", srcLocation);

    trueExpr->Codegen(context);
    Type trueType = context.type;
    auto trueExpr = context.expr;

    falseExpr->Codegen(context);
    Type falseType = context.type;
    auto falseExpr = context.expr;

    context.expr.isConstant =
        condExpr.isConstant && trueExpr.isConstant && falseExpr.isConstant;

    if (trueType == falseType) {
        context.expr.constant =
            condExpr.constant.boolVal ? trueExpr.constant : falseExpr.constant;
        return;
    }

    trueType  = trueType.Decay();
    falseType = falseType.Decay();

    if (trueType == falseType) {
        context.type            = trueType;
        context.expr.isConstant = false;
        return;
    }

    // TODO: more conditional expr implicit conversion

    // Convert to arithmetic type
    Type commonType = trueType.ArithmeticConvert(falseType);

    if (!trueType.IsConvertibleTo(commonType, trueExpr.constOrNull())
        || !falseType.IsConvertibleTo(commonType, falseExpr.constOrNull()))
        throw SemanticError("operand types '" + trueType.Name() + "' and '"
                                + falseType.Name() + "' are incompatible",
                            srcLocation);

    context.type = commonType;
    context.expr.constant =
        condExpr.constant.boolVal ? trueExpr.constant : falseExpr.constant;
}

void BinaryExpression::Codegen(CodegenContext &context) const
{
    left->Codegen(context);
    Type           leftType = context.type;
    CodegenContext rightCtx(context);

    switch (op) {
    case BinaryOp::SUBSCRIPT:
        // TODO: operator[]
        assert(!context.expr.isConstant);
        leftType = leftType.Decay();
        if (!leftType.IsPtr())
            throw SemanticError("subscripted value is not array or pointer", srcLocation);

        context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                          context.type,
                                                          leftType,
                                                          context.expr.value);
        break;

    case BinaryOp::DOT:
    case BinaryOp::DOTSTAR:
        assert(!context.expr.isConstant);
        if (leftType.IsPtr() && leftType.RemovePtr().IsSimple(TypeKind::CLASS))
            throw SemanticError("member reference type '" + leftType.Name()
                                    + "' is a pointer; note: use '->' instead",
                                srcLocation);

        if (!leftType.RemoveRef().IsSimple(TypeKind::CLASS))
            throw SemanticError("member reference base type '" + leftType.Name()
                                    + "' is not a class or struct",
                                srcLocation);

        rightCtx.symtab = leftType.Class()->memberTable.get();
        break;

    case BinaryOp::ARROW:
    case BinaryOp::ARROWSTAR:
        assert(!context.expr.isConstant);
        leftType = leftType.Decay();

        if (!leftType.IsPtr()) {
            if (leftType.IsSimple(TypeKind::CLASS))
                throw SemanticError("member reference type '" + leftType.Name()
                                        + "' is not a pointer; note: use '.' instead",
                                    srcLocation);
            else
                throw SemanticError("member reference type '" + leftType.Name()
                                        + "' is not a pointer",
                                    srcLocation);
        }

        if (!leftType.RemovePtr().IsSimple(TypeKind::CLASS))
            throw SemanticError("member reference base type '" + leftType.Name()
                                    + "' is not a class or struct",
                                srcLocation);

        context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                          context.type,
                                                          leftType,
                                                          context.expr.value);
        context.type =
            context.type.RemovePtr().AddPtrDesc(Type::PtrDescriptor {PtrType::REF});
        rightCtx.symtab = leftType.Class()->memberTable.get();
        break;

    default:
        if (leftType.IsSimple(TypeKind::CLASS))
            throw SemanticError("unimp", srcLocation);

        leftType = leftType.Decay();
        if (!context.expr.isConstant) {
            context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                              context.type,
                                                              leftType,
                                                              context.expr.value);
        }
        break;
    }

    right->Codegen(rightCtx);

    switch (op) {
    case BinaryOp::SUBSCRIPT: {
        if (!rightCtx.type.IsConvertibleTo(FundType::INT, rightCtx.expr.constOrNull()))
            throw SemanticError("array subscript is not an integer", srcLocation);

        if (rightCtx.expr.isConstant) {
            rightCtx.expr.value =
                context.cgHelper.CreateConstant(FundType::INT, rightCtx.expr.constant);
        }
        else {
            rightCtx.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                               rightCtx.type,
                                                               FundType::INT,
                                                               rightCtx.expr.value);
        }

        std::array<llvm::Value *, 2> idx;
        idx[0] = context.cgHelper.CreateZeroConstant(FundType::INT);
        idx[1] = rightCtx.expr.value;

        context.expr.value =
            context.IRBuilder->CreateGEP(context.expr.value, rightCtx.expr.value);
        context.type =
            leftType.RemovePtr().AddPtrDesc(Type::PtrDescriptor {PtrType::REF});
        break;
    }

    case BinaryOp::DOTSTAR:
    case BinaryOp::ARROWSTAR:
        throw SemanticError("unimp", srcLocation);

    case BinaryOp::DOT:
    case BinaryOp::ARROW: {
        assert(!rightCtx.expr.isConstant);
        assert(rightCtx.symbolSet);

        std::array<llvm::Value *, 2> idx;
        idx[0] = context.cgHelper.CreateZeroConstant(FundType::INT);
        idx[1] = context.cgHelper.CreateConstant(FundType::INT,
                                                 ::Constant {rightCtx.symbolSet->index});

        context.expr.value = context.IRBuilder->CreateGEP(context.expr.value, idx);

        // If object is r-value, then its member is set to r-value
        if (!leftType.IsRef() && rightCtx.type.IsRef()) {
            context.type        = rightCtx.type.RemoveRef();
            rightCtx.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                               rightCtx.type,
                                                               context.type,
                                                               context.expr.value);
        }
        else {
            context.type = rightCtx.type;
        }
        context.symbolSet = {};
        break;
    }

    case BinaryOp::COMMA:
        context.type      = rightCtx.type;
        context.symbolSet = rightCtx.symbolSet;
        if (context.expr.isConstant &= rightCtx.expr.isConstant) {
            context.expr.constant = rightCtx.expr.constant;
        }
        else {
            if (rightCtx.expr.isConstant) {
                rightCtx.expr.value =
                    context.cgHelper.CreateConstant(rightCtx.type,
                                                    rightCtx.expr.constant);
            }
            context.expr.value = rightCtx.expr.value;
        }
        break;

    case BinaryOp::MOD:
    case BinaryOp::SHL:
    case BinaryOp::SHR:
    case BinaryOp::AND:
    case BinaryOp::XOR:
    case BinaryOp::OR:
        if (leftType.IsSimple(TypeKind::FUNDTYPE)
            && (leftType.fundType == FundType::FLOAT
                || leftType.fundType == FundType::DOUBLE))
            throw SemanticError("invalid argument type '" + leftType.Name()
                                    + "' to unary expression",
                                srcLocation);

        if (rightCtx.type.Decay().IsSimple(TypeKind::FUNDTYPE)
            && (rightCtx.type.Decay().fundType == FundType::FLOAT
                || rightCtx.type.Decay().fundType == FundType::DOUBLE))
            throw SemanticError("invalid argument type '" + rightCtx.type.Decay().Name()
                                    + "' to unary expression",
                                srcLocation);

    default:
        // Convert to arithmetic type
        auto rightType = rightCtx.type.Decay();
        if (!rightCtx.expr.isConstant) {
            rightCtx.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                               rightCtx.type,
                                                               rightType,
                                                               rightCtx.expr.value);
        }

        Type commonType = leftType.ArithmeticConvert(rightType);

        // TODO: more binary operand type(pointer...)
        if (!leftType.IsConvertibleTo(commonType, context.expr.constOrNull())
            || !rightType.IsConvertibleTo(commonType, rightCtx.expr.constOrNull()))
            throw SemanticError("invalid operand types '" + leftType.Name() + "' and '"
                                    + rightType.Name() + "' to binary expression",
                                srcLocation);

        if (context.expr.isConstant && rightCtx.expr.isConstant) {
            context.expr.constant =
                context.expr.constant.BinaryOpResult(commonType.fundType,
                                                     op,
                                                     rightCtx.expr.constant);
        }
        else {
            if (context.expr.isConstant) {
                context.expr.value =
                    context.cgHelper.CreateConstant(leftType, context.expr.constant);
                context.expr.isConstant = false;
            }
            else if (rightCtx.expr.isConstant) {
                rightCtx.expr.value =
                    context.cgHelper.CreateConstant(rightType, rightCtx.expr.constant);
            }

            context.expr.value  = context.cgHelper.ConvertType(*context.IRBuilder,
                                                              leftType,
                                                              commonType,
                                                              context.expr.value);
            rightCtx.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                               rightType,
                                                               commonType,
                                                               rightCtx.expr.value);

            bool isDecimal = commonType.fundType == FundType::FLOAT
                             || commonType.fundType == FundType::DOUBLE;

            switch (commonType.fundType) {
            case FundType::BOOL:
            case FundType::CHAR:
            case FundType::SHORT:
            case FundType::INT:
            case FundType::LONG:

                switch (op) {
                case BinaryOp::MUL:
                    context.expr.value =
                        context.IRBuilder->CreateMul(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::DIV:
                    context.expr.value =
                        context.IRBuilder->CreateSDiv(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::MOD:
                    context.expr.value =
                        context.IRBuilder->CreateSRem(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::ADD:
                    context.expr.value =
                        context.IRBuilder->CreateAdd(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::SUB:
                    context.expr.value =
                        context.IRBuilder->CreateSub(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::SHL:
                    context.expr.value =
                        context.IRBuilder->CreateShl(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::SHR:
                    context.expr.value =
                        context.IRBuilder->CreateAShr(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::GT:
                    context.expr.value =
                        context.IRBuilder->CreateICmpSGT(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::LT:
                    context.expr.value =
                        context.IRBuilder->CreateICmpSLT(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::LE:
                    context.expr.value =
                        context.IRBuilder->CreateICmpSLE(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::GE:
                    context.expr.value =
                        context.IRBuilder->CreateICmpSGE(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::EQ:
                    context.expr.value =
                        context.IRBuilder->CreateICmpEQ(context.expr.value,
                                                        rightCtx.expr.value);
                    break;
                case BinaryOp::NE:
                    context.expr.value =
                        context.IRBuilder->CreateICmpNE(context.expr.value,
                                                        rightCtx.expr.value);
                    break;
                case BinaryOp::XOR:
                    context.expr.value =
                        context.IRBuilder->CreateXor(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::AND:
                case BinaryOp::LOGIAND:
                    context.expr.value =
                        context.IRBuilder->CreateAnd(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::OR:
                case BinaryOp::LOGIOR:
                default:
                    context.expr.value = context.IRBuilder->CreateOr(context.expr.value,
                                                                     rightCtx.expr.value);
                    break;
                }
                break;

            case FundType::UCHAR:
            case FundType::USHORT:
            case FundType::UINT:
            case FundType::ULONG:
                switch (op) {
                case BinaryOp::MUL:
                    context.expr.value =
                        context.IRBuilder->CreateMul(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::DIV:
                    context.expr.value =
                        context.IRBuilder->CreateUDiv(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::MOD:
                    context.expr.value =
                        context.IRBuilder->CreateURem(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::ADD:
                    context.expr.value =
                        context.IRBuilder->CreateAdd(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::SUB:
                    context.expr.value =
                        context.IRBuilder->CreateSub(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::SHL:
                    context.expr.value =
                        context.IRBuilder->CreateShl(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::SHR:
                    context.expr.value =
                        context.IRBuilder->CreateLShr(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::GT:
                    context.expr.value =
                        context.IRBuilder->CreateICmpUGT(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::LT:
                    context.expr.value =
                        context.IRBuilder->CreateICmpULT(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::LE:
                    context.expr.value =
                        context.IRBuilder->CreateICmpULE(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::GE:
                    context.expr.value =
                        context.IRBuilder->CreateICmpUGE(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::EQ:
                    context.expr.value =
                        context.IRBuilder->CreateICmpEQ(context.expr.value,
                                                        rightCtx.expr.value);
                    break;
                case BinaryOp::NE:
                    context.expr.value =
                        context.IRBuilder->CreateICmpNE(context.expr.value,
                                                        rightCtx.expr.value);
                    break;
                case BinaryOp::XOR:
                    context.expr.value =
                        context.IRBuilder->CreateXor(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::AND:
                case BinaryOp::LOGIAND:
                    context.expr.value =
                        context.IRBuilder->CreateAnd(context.expr.value,
                                                     rightCtx.expr.value);
                    break;
                case BinaryOp::OR:
                case BinaryOp::LOGIOR:
                default:
                    context.expr.value = context.IRBuilder->CreateOr(context.expr.value,
                                                                     rightCtx.expr.value);
                    break;
                }
                break;

            case FundType::FLOAT:
            case FundType::DOUBLE:
                switch (op) {
                case BinaryOp::MUL:
                    context.expr.value =
                        context.IRBuilder->CreateFMul(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::DIV:
                    context.expr.value =
                        context.IRBuilder->CreateFDiv(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::ADD:
                    context.expr.value =
                        context.IRBuilder->CreateFAdd(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::SUB:
                    context.expr.value =
                        context.IRBuilder->CreateFSub(context.expr.value,
                                                      rightCtx.expr.value);
                    break;
                case BinaryOp::GT:
                    context.expr.value =
                        context.IRBuilder->CreateFCmpUGT(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::LT:
                    context.expr.value =
                        context.IRBuilder->CreateFCmpULT(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::LE:
                    context.expr.value =
                        context.IRBuilder->CreateFCmpULE(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::GE:
                    context.expr.value =
                        context.IRBuilder->CreateFCmpUGE(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::EQ:
                    context.expr.value =
                        context.IRBuilder->CreateFCmpUEQ(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                case BinaryOp::NE:
                default:
                    context.expr.value =
                        context.IRBuilder->CreateFCmpUNE(context.expr.value,
                                                         rightCtx.expr.value);
                    break;
                }
                break;

            default:
                break;
            }
        }

        switch (op) {
        case BinaryOp::GT:
        case BinaryOp::LT:
        case BinaryOp::LE:
        case BinaryOp::GE:
        case BinaryOp::EQ:
        case BinaryOp::NE:
        case BinaryOp::LOGIAND:
        case BinaryOp::LOGIOR:
            context.type = {FundType::BOOL};
            break;
        default:
            context.type = commonType;
            break;
        }
        context.symbolSet = {};
        break;
    }
}

void CastExpression::Codegen(CodegenContext &context) const
{
    typeId->Codegen(context);
    Type castType = context.type;

    expr->Codegen(context);

    if (!context.type.IsConvertibleTo(castType, context.expr.constOrNull())) {
        // TODO: check cast
    }

    context.type = castType;
}

void UnaryExpression::Codegen(CodegenContext &context) const
{
    expr->Codegen(context);
    Type exprType = context.type;

    switch (op) {
    case UnaryOp::UNREF:
        context.type = exprType.Decay();
        if (!context.type.IsPtr())
            throw SemanticError("indirection type '" + context.type.Name()
                                    + "' is not pointer operand",
                                srcLocation);

        assert(!context.expr.isConstant);
        context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                          exprType,
                                                          context.type,
                                                          context.expr.value);
        context.type =
            context.type.RemovePtr().AddPtrDesc(Type::PtrDescriptor {PtrType::REF});
        break;

    case UnaryOp::ADDRESSOF:
        if (exprType.IsSimple(TypeKind::FUNCTION) || exprType.IsArray()) {
            context.type.AddPtrDesc(Type::PtrDescriptor {PtrType::PTR});
        }
        else {
            if (!exprType.IsRef())
                throw SemanticError("cannot take the address of an rvalue of type '"
                                        + exprType.Name() + "'",
                                    srcLocation);

            assert(!context.expr.isConstant);
            context.type = exprType.RemoveRef();
            /*context.expr.value =
                context.IRBuilder->CreateGEP(context.expr.value,
                                             context.cgHelper.CreateZeroConstant());*/

            assert(context.symbolSet);
            if (context.symbolSet.Scope()
                && context.symbolSet.Scope()->GetCurrentClass()) {
                context.type.AddPtrDesc(
                    Type::PtrDescriptor {PtrType::CLASSPTR,
                                         CVQualifier::NONE,
                                         context.symbolSet.Scope()->GetCurrentClass()});
                context.qualifiedScope = nullptr;
            }
            else
                context.type.AddPtrDesc(Type::PtrDescriptor {PtrType::PTR});
        }
        break;

    case UnaryOp::PREINC:
    case UnaryOp::PREDEC: {
        if (!exprType.IsRef())
            throw SemanticError("expression is not assignable", srcLocation);

        exprType = exprType.RemoveRef();
        if (exprType.IsConstInit())
            throw SemanticError("cannot assign to variable '" + context.symbolSet->id
                                    + "' with const-qualified type '" + exprType.Name()
                                    + "'",
                                srcLocation);

        assert(!context.expr.isConstant);
        auto rvalue = context.cgHelper.ConvertType(*context.IRBuilder,
                                                   context.type,
                                                   exprType,
                                                   context.expr.value);

        if (exprType.IsSimple(TypeKind::FUNDTYPE)) {
            auto oneConstant =
                context.cgHelper.CreateConstant(exprType.fundType, Constant {1});

            switch (exprType.fundType) {
            case FundType::FLOAT:
            case FundType::DOUBLE:
                if (op == UnaryOp::PREINC)
                    rvalue = context.IRBuilder->CreateFAdd(rvalue, oneConstant);
                else
                    rvalue = context.IRBuilder->CreateFSub(rvalue, oneConstant);
                break;

            default:
                if (op == UnaryOp::PREINC)
                    rvalue = context.IRBuilder->CreateAdd(rvalue, oneConstant);
                else
                    rvalue = context.IRBuilder->CreateSub(rvalue, oneConstant);
                break;
            }
        }
        else if (exprType.IsSimple(TypeKind::ENUM)) {
            if (op == UnaryOp::PREINC)
                throw SemanticError("cannot increment expression of enum '"
                                        + exprType.Name() + "'",
                                    srcLocation);
            else
                throw SemanticError("cannot decrement expression of enum '"
                                        + exprType.Name() + "'",
                                    srcLocation);
        }
        else if (exprType.IsSimple(TypeKind::CLASS)) {
            throw SemanticError("unimplemented", srcLocation);
        }
        else if (exprType.IsPtr()) {
            auto oneOffset = context.cgHelper.CreateConstant(FundType::INT, Constant {1});
            rvalue         = context.IRBuilder->CreateGEP(rvalue, oneOffset);
        }
        else {
            if (op == UnaryOp::PREINC)
                throw SemanticError("cannot increment value of type '" + exprType.Name()
                                        + "'",
                                    srcLocation);
            else
                throw SemanticError("cannot decrement value of type '" + exprType.Name()
                                        + "'",
                                    srcLocation);
        }

        context.IRBuilder->CreateAlignedStore(rvalue,
                                              context.expr.value,
                                              llvm::Align(exprType.Alignment()));
        break;
    }
    case UnaryOp::POSTINC:
    case UnaryOp::POSTDEC: {
        if (!exprType.IsRef())
            throw SemanticError("expression is not assignable", srcLocation);

        exprType = exprType.RemoveRef();
        if (exprType.IsConstInit())
            throw SemanticError("cannot assign to variable '" + context.symbolSet->id
                                    + "' with const-qualified type '" + exprType.Name()
                                    + "'",
                                srcLocation);

        assert(!context.expr.isConstant);
        auto rvalue = context.cgHelper.ConvertType(*context.IRBuilder,
                                                   context.type,
                                                   exprType,
                                                   context.expr.value);

        if (exprType.IsSimple(TypeKind::FUNDTYPE)) {
            auto oneConstant =
                context.cgHelper.CreateConstant(exprType.fundType, Constant {1});

            switch (exprType.fundType) {
            case FundType::FLOAT:
            case FundType::DOUBLE:
                if (op == UnaryOp::POSTINC)
                    rvalue = context.IRBuilder->CreateFAdd(rvalue, oneConstant);
                else
                    rvalue = context.IRBuilder->CreateFSub(rvalue, oneConstant);
                break;

            default:
                if (op == UnaryOp::POSTINC)
                    rvalue = context.IRBuilder->CreateAdd(rvalue, oneConstant);
                else
                    rvalue = context.IRBuilder->CreateSub(rvalue, oneConstant);
                break;
            }
        }
        else if (exprType.IsSimple(TypeKind::ENUM)) {
            if (op == UnaryOp::POSTINC)
                throw SemanticError("cannot increment expression of enum '"
                                        + exprType.Name() + "'",
                                    srcLocation);
            else
                throw SemanticError("cannot decrement expression of enum '"
                                        + exprType.Name() + "'",
                                    srcLocation);
        }
        else if (exprType.IsSimple(TypeKind::CLASS)) {
            throw SemanticError("unimplemented", srcLocation);
        }
        else if (exprType.IsPtr()) {
            auto oneOffset = context.cgHelper.CreateConstant(FundType::INT, Constant {1});
            rvalue         = context.IRBuilder->CreateGEP(rvalue, oneOffset);
        }
        else {
            if (op == UnaryOp::POSTINC)
                throw SemanticError("cannot increment value of type '" + exprType.Name()
                                        + "'",
                                    srcLocation);
            else
                throw SemanticError("cannot decrement value of type '" + exprType.Name()
                                        + "'",
                                    srcLocation);
        }

        context.IRBuilder->CreateAlignedStore(rvalue,
                                              context.expr.value,
                                              llvm::Align(exprType.Alignment()));
        context.expr.value = rvalue;
        context.type       = exprType;
        break;
    }
    case UnaryOp::SIZEOF:
        context.expr.constant.intVal = exprType.Decay().Size();
        context.expr.isConstant      = true;
        context.type                 = {FundType::INT};

    case UnaryOp::LOGINOT:
        exprType = exprType.Decay();

        if (!context.expr.isConstant) {
            context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                              context.type,
                                                              exprType,
                                                              context.expr.value);
        }

        if (!exprType.IsConvertibleTo(FundType::BOOL, context.expr.constOrNull()))
            throw SemanticError("invalid argument type '" + exprType.Name()
                                    + "' to unary expression",
                                srcLocation);

        if (context.expr.isConstant) {
            context.expr.constant =
                context.expr.constant.UnaryOpResult(FundType::BOOL, UnaryOp::LOGINOT);
        }
        else {
            context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                              exprType,
                                                              FundType::BOOL,
                                                              context.expr.value);
            context.expr.value = context.IRBuilder->CreateNot(context.expr.value);
        }

        context.type = {FundType::BOOL};
        break;

    case UnaryOp::NOT:
        if (exprType.Decay().IsSimple(TypeKind::FUNDTYPE)
            && (exprType.fundType == FundType::FLOAT
                || exprType.fundType == FundType::DOUBLE))
            throw SemanticError("invalid argument type '" + exprType.Decay().Name()
                                    + "' to unary expression",
                                srcLocation);
    default:
        exprType       = exprType.Decay();
        Type arithType = exprType.ArithmeticConvert(FundType::INT);

        if (!exprType.IsConvertibleTo(arithType, context.expr.constOrNull()))
            throw SemanticError("invalid argument type '" + exprType.Name()
                                    + "' to unary expression",
                                srcLocation);

        if (context.expr.isConstant) {
            context.expr.constant =
                context.expr.constant.UnaryOpResult(arithType.fundType, op);
        }
        else {
            context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                              context.type,
                                                              arithType,
                                                              context.expr.value);

            switch (op) {
            case UnaryOp::NOT:
                context.expr.value = context.IRBuilder->CreateNot(context.expr.value);
                break;
            case UnaryOp::NEG:
                context.expr.value = context.IRBuilder->CreateNeg(context.expr.value);
                break;
            default:
                break;
            }
        }
        context.type = arithType;
        break;
    }
}

void CallExpression::Codegen(CodegenContext &context) const
{
    funcExpr->Codegen(context);

    context.type = context.type.RemoveRef().RemovePtr();
    if (!context.type.IsSimple(TypeKind::FUNCTION))
        throw SemanticError("called object type '" + context.type.Name()
                                + "' is not a function or function pointer",
                            srcLocation);

    params->Codegen(context);
}

void ConstructExpression::Codegen(CodegenContext &context) const
{
    type->Codegen(context);
    if (params)
        params->Codegen(context);
}

void SizeofExpression::Codegen(CodegenContext &context) const
{
    typeId->Codegen(context);

    if (!context.type.IsComplete())
        throw SemanticError("apply sizeof to incomplete type '" + context.type.Name()
                                + "'",
                            srcLocation);

    context.expr.constant.intVal = context.type.Size();
    context.expr.isConstant      = true;
    context.type                 = {FundType::INT};
}

void PlainNew::Codegen(CodegenContext &context) const
{
    throw SemanticError("unimplemented", srcLocation);

    if (placement)
        placement->Codegen(context);

    typeId->Codegen(context);
}

void InitializableNew::Codegen(CodegenContext &context) const
{
    throw SemanticError("unimplemented", srcLocation);

    if (placement)
        placement->Codegen(context);

    typeSpec->Codegen(context);

    if (ptrSpec) {
        ptrSpec->Codegen(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    for (const auto &s : arraySizes) {
        s->Codegen(context);
    }

    if (initializer)
        initializer->Codegen(context);
}

void DeleteExpression::Codegen(CodegenContext &context) const
{
    throw SemanticError("unimplemented", srcLocation);

    expr->Codegen(context);
    // TODO: type check

    context.type = {FundType::VOID};
}

void IdExpression::Codegen(CodegenContext &context) const
{
    SymbolTable *symtab    = context.symtab;
    bool         qualified = false;

    if (nameSpec) {
        nameSpec->Codegen(context);
        symtab                 = context.qualifiedScope;
        context.qualifiedScope = nullptr;
        qualified              = true;
    }

    // Get composed identifier
    std::string composedId = ComposedId(context);

    if (context.decl.isTypedef) {
        // Inject typename name into symbol table
        if (!symtab->AddTypedef(composedId, context.type))
            throw SemanticError("redeclaration type alias of '" + composedId + "'",
                                srcLocation);

        context.symbolSet = {};
        return;
    }

    if (context.decl.state != DeclState::NODECL) {
        if (qualified) {
            context.symbolSet = symtab->QuerySymbol(composedId, qualified);
            if (!context.symbolSet)
                throw SemanticError("no member named '" + composedId + "' in '"
                                        + symtab->ScopeName() + "'",
                                    srcLocation);
            context.type = context.symbolSet->type;
        }
        else {
            // Record new symbol
            context.newSymbol = {composedId, context.type, context.decl.symbolAccessAttr};
            // maybe unused (id declarator already sets symbolSet)
            context.symbolSet = {};
        }
    }
    else {
        context.symbolSet = symtab->QuerySymbol(composedId, qualified);
        if (!context.symbolSet) {
            switch (stype) {
            case DESTRUCTOR:
                // TODO: gen default destructor
                context.newSymbol = {composedId, context.type};
                break;
            case CONSTRUCTOR:
                // TODO: gen default constructor?
                context.newSymbol = {composedId, context.type};
                break;
            default:
                if (qualified)
                    throw SemanticError("no member named '" + composedId + "' in '"
                                            + symtab->ScopeName() + "'",
                                        srcLocation);
                else
                    throw SemanticError("use of undeclared identifier '" + composedId
                                            + "'",
                                        srcLocation);
                break;
            }
            assert(false);
            context.symbolSet = {&context.newSymbol, symtab};
        }

        // IdExpression's type is always the first symbol's type (if multiple symbols)
        context.type            = context.symbolSet->type;
        context.expr.isConstant = context.symbolSet->Attr() == Symbol::CONSTANT;
        if (context.expr.isConstant)
            context.expr.constant.intVal = context.symbolSet->intConstant;
        else
            context.expr.value = context.symbolSet->value;

        // Id expression is always a l-value (except for function, array and constant)
        if (!context.type.IsRef()) {
            if (!context.type.IsSimple(TypeKind::FUNCTION) && !context.type.IsArray()
                && !context.expr.isConstant) {
                assert(context.symbolSet.size() == 1);
                context.type.AddPtrDesc(Type::PtrDescriptor {PtrType::REF});
            }
        }
        else {
            if (context.type.RemoveRef().IsSimple(TypeKind::FUNCTION)
                || context.type.RemoveRef().IsArray()) {
                context.type = context.type.RemoveRef();
            }
            else {
                assert(context.symbolSet.size() == 1);
                // Load reference
                context.expr.value = context.IRBuilder->CreateAlignedLoad(
                    context.expr.value,
                    llvm::Align(context.type.Alignment()),
                    context.symbolSet->id);
            }
        }
    }
}

std::string IdExpression::ComposedId(CodegenContext &context) const
{
    switch (stype) {
    case DESTRUCTOR:
        return "~" + identifier + "()";
    case CONSTRUCTOR:
        return identifier + "()";
    default:
        return identifier;
    }
}

void ThisExpression::Codegen(CodegenContext &context) const
{
    auto funcDesc = context.symtab->GetCurrentFunction();
    assert(funcDesc);

    if (!funcDesc->IsNonStaticMember())
        throw SemanticError(
            "invalid use of 'this' outside of a non-static member function",
            srcLocation);

    context.symbolSet = {funcDesc->paramList.front().symbol, funcDesc->funcScope.get()};
    context.type      = context.symbolSet->type;
    context.expr.isConstant = false;
}

void IntLiteral::Codegen(CodegenContext &context) const
{
    context.type                 = {FundType::INT};
    context.expr.constant.intVal = value;
    context.expr.isConstant      = true;
}

void FloatLiteral::Codegen(CodegenContext &context) const
{
    context.type                   = {FundType::DOUBLE};
    context.expr.constant.floatVal = value;
    context.expr.isConstant        = true;
}

void CharLiteral::Codegen(CodegenContext &context) const
{
    context.type                  = {FundType::CHAR};
    context.expr.constant.charVal = value;
    context.expr.isConstant       = true;
}

void StringLiteral::Codegen(CodegenContext &context) const
{
    context.type = {FundType::CHAR, CVQualifier::CONST};
    context.type.arrayDescList.push_back(Type::ArrayDescriptor {value.length() + 1, {}});

    context.expr.isConstant = false;
    context.expr.value      = context.cgHelper.CreateStringConstant(value);
}

void BoolLiteral::Codegen(CodegenContext &context) const
{
    context.type                  = {FundType::BOOL};
    context.expr.constant.boolVal = value;
    context.expr.isConstant       = true;
}

void ExpressionList::Codegen(CodegenContext &context) const
{
    // Here symbolSet is the object(s) of function call syntax
    assert(context.symbolSet);

    if (context.symbolSet.size() == 1) {
        // TODO: class, fundtype, enum: para init
        // TODO: operator() or constructor

        if (context.type.IsSimple(TypeKind::FUNCTION)) {
            auto   funcDesc = context.type.Function();
            auto   function = llvm::cast<llvm::Function>(funcDesc->defSymbol->value);
            size_t startIdx = funcDesc->IsNonStaticMember();

            // TODO: default parameter match
            if (funcDesc->paramList.size() != exprList.size() + startIdx)
                throw SemanticError("no matching function for call to '"
                                        + context.symbolSet->id + "'",
                                    srcLocation);

            std::vector<llvm::Value *> argValues;
            // TODO: first param for non-static member function

            for (size_t i = startIdx; i < funcDesc->paramList.size(); i++) {
                auto argSymbol = funcDesc->paramList[i].symbol;

                exprList[i - startIdx]->Codegen(context);

                if (!context.type.IsConvertibleTo(argSymbol->type,
                                                  context.expr.constOrNull()))
                    throw SemanticError("expression type '" + context.type.Name()
                                            + "' does not fit argument type '"
                                            + argSymbol->type.Name() + "'",
                                        srcLocation);

                if (context.expr.isConstant) {
                    context.expr.value =
                        context.cgHelper.CreateConstant(context.type,
                                                        context.expr.constant);
                }
                else {
                    context.expr.value = context.cgHelper.ConvertType(*context.IRBuilder,
                                                                      context.type,
                                                                      argSymbol->type,
                                                                      context.expr.value);
                }
                argValues.push_back(context.expr.value);
            }

            context.expr.value = context.IRBuilder->CreateCall(function, argValues);
            context.type       = funcDesc->retType;
        }
        else if (context.type.IsSimple(TypeKind::CLASS)) {
            throw SemanticError("unimp", srcLocation);
        }
        else {
            if (exprList.size() != 1)
                throw SemanticError("excess elements in scalar initializer", srcLocation);

            Type varType = context.type;
            exprList.front()->Codegen(context);

            if (!context.type.IsConvertibleTo(varType, context.expr.constOrNull()))
                throw SemanticError("cannot initialize '" + varType.Name() + "' with '"
                                        + context.type.Name() + "'",
                                    srcLocation);

            context.cgHelper.GenAssignInit(*context.IRBuilder,
                                           context.symbolSet,
                                           varType,
                                           context.expr);
        }
    }
    else {
        // TODO: overload function resolution
        throw SemanticError("function overloading unimplemented", srcLocation);
    }
}

}  // namespace ast