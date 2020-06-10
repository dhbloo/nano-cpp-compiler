#include "../ast/node.h"
#include "../core/semantic.h"

#include <cassert>

namespace ast {

void AssignmentExpression::Analysis(SemanticContext &context) const
{
    left->Analysis(context);
    Type leftType = context.type;

    if (!leftType.IsRef() || leftType.RemoveRef().IsSimple(TypeClass::FUNCTION))
        throw SemanticError("left of expression is not assignable", srcLocation);

    right->Analysis(context);
    Type rightType = context.type.Decay();

    if (!rightType.IsConvertibleTo(leftType.RemoveRef())) {
        throw SemanticError("assigning to '" + leftType.RemoveRef().Name()
                                + "' from incompatible type '" + rightType.Name() + "'",
                            srcLocation);
    }

    context.type = leftType;
}

void ConditionalExpression::Analysis(SemanticContext &context) const
{
    condition->Analysis(context);
    Type condType = context.type.Decay();
    auto condExpr = context.expr;
    if (!condType.IsConvertibleTo(FundType::BOOL, &condExpr.constant))
        throw SemanticError(condType.Name() + " is not convertible to bool", srcLocation);

    trueExpr->Analysis(context);
    Type trueType = context.type;
    auto trueExpr = context.expr;

    falseExpr->Analysis(context);
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

    if (!trueType.IsConvertibleTo(commonType, &trueExpr.constant)
        || !falseType.IsConvertibleTo(commonType, &falseExpr.constant))
        throw SemanticError("operand types '" + trueType.Name() + "' and '"
                                + falseType.Name() + "' are incompatible",
                            srcLocation);

    context.type = commonType;
    context.expr.constant =
        condExpr.constant.boolVal ? trueExpr.constant : falseExpr.constant;
}

void BinaryExpression::Analysis(SemanticContext &context) const
{
    left->Analysis(context);
    SemanticContext rightContext(context);

    switch (op) {
    case BinaryOp::SUBSCRIPT:
        context.type = context.type.Decay();
        if (!context.type.IsPtr())
            throw SemanticError("subscripted value is not array or pointer", srcLocation);
        break;

    case BinaryOp::DOT:
    case BinaryOp::DOTSTAR:
        if (context.type.IsPtr() && context.type.RemovePtr().IsSimple(TypeClass::CLASS))
            throw SemanticError("member reference type '" + context.type.Name()
                                    + "' is a pointer; note: use '->' instead",
                                srcLocation);

        if (!context.type.RemoveRef().IsSimple(TypeClass::CLASS))
            throw SemanticError("member reference base type '" + context.type.Name()
                                    + "' is not a class or struct",
                                srcLocation);

        rightContext.qualifiedScope = context.type.Class()->memberTable.get();
        break;

    case BinaryOp::ARROW:
    case BinaryOp::ARROWSTAR:
        context.type = context.type.Decay();
        if (!context.type.IsPtr()) {
            if (context.type.IsSimple(TypeClass::CLASS))
                throw SemanticError("member reference type '" + context.type.Name()
                                        + "' is not a pointer; note: use '.' instead",
                                    srcLocation);
            else
                throw SemanticError("member reference type '" + context.type.Name()
                                        + "' is not a pointer",
                                    srcLocation);
        }

        if (!context.type.RemovePtr().IsSimple(TypeClass::CLASS))
            throw SemanticError("member reference base type '" + context.type.Name()
                                    + "' is not a class or struct",
                                srcLocation);

        rightContext.qualifiedScope = context.type.Class()->memberTable.get();
        break;

    default: break;
    }

    right->Analysis(rightContext);

    switch (op) {
    case BinaryOp::SUBSCRIPT:
        if (!rightContext.type.IsConvertibleTo(FundType::INT))
            throw SemanticError("array subscript is not an integer", srcLocation);
        // TODO: operator[]

        context.type =
            context.type.RemovePtr().AddPtrDesc(Type::PtrDescriptor {PtrType::REF});
        // TODO: subscript symbol set
        context.symbolSet       = {};
        context.expr.isConstant = false;
        break;

    case BinaryOp::DOT:
    case BinaryOp::DOTSTAR:
        if (!context.type.IsRef() && rightContext.type.IsRef())
            rightContext.type = rightContext.type.RemoveRef();
    case BinaryOp::ARROW:
    case BinaryOp::ARROWSTAR:
        context.type            = rightContext.type;
        context.symbolSet       = rightContext.symbolSet;
        context.expr.isConstant = false;
        break;

    case BinaryOp::COMMA:
        context.type      = rightContext.type;
        context.symbolSet = rightContext.symbolSet;
        context.expr.isConstant &= rightContext.expr.isConstant;
        context.expr.constant = rightContext.expr.constant;
        break;

    default:
        // Convert to arithmetic type
        context.type      = context.type.Decay();
        rightContext.type = rightContext.type.Decay();
        Type commonType   = context.type.ArithmeticConvert(rightContext.type);

        // TODO: more binary operand type

        if (!context.type.IsConvertibleTo(commonType, &context.expr.constant)
            || !rightContext.type.IsConvertibleTo(commonType,
                                                  &rightContext.expr.constant))
            throw SemanticError("invalid operand types '" + context.type.Name()
                                    + "' and '" + rightContext.type.Name()
                                    + "' to binary expression",
                                srcLocation);

        context.type      = commonType;
        context.symbolSet = {};

        if (context.expr.isConstant &= rightContext.expr.isConstant) {
            // TODO: binary constant calc
            context.expr.isConstant = false;
        }
        break;
    }
}

void CastExpression::Analysis(SemanticContext &context) const
{
    typeId->Analysis(context);
    Type castType = context.type;

    expr->Analysis(context);

    // TODO: check cast

    context.type = castType;
}

void UnaryExpression::Analysis(SemanticContext &context) const
{
    expr->Analysis(context);

    switch (op) {
    case UnaryOp::UNREF:
        context.type = context.type.Decay();
        if (!context.type.IsPtr())
            throw SemanticError("indirection type '" + context.type.Name()
                                    + "' is not pointer operand",
                                srcLocation);

        context.type =
            context.type.RemovePtr().AddPtrDesc(Type::PtrDescriptor {PtrType::REF});
        break;

    case UnaryOp::ADDRESSOF:
        if (context.type.IsSimple(TypeClass::FUNCTION)) {
            context.type = context.type.Decay();
        }
        else {
            if (!context.type.IsRef())
                throw SemanticError("cannot take the address of an rvalue of type '"
                                        + context.type.Name() + "'",
                                    srcLocation);

            context.type = context.type.RemoveRef();

            // if (context.type.RemoveRef().Function()->IsNonStaticMember())
            //     // member function to member function pointer
            //     context.type.AddPtrDesc(Type::PtrDescriptor {
            //         PtrType::CLASSPTR,
            //         CVQualifier::NONE,
            //         context.type.Function()->funcScope->GetCurrentClass()});
            // else
            assert(context.symbolSet);
            if (context.symbolSet.Scope()->GetCurrentClass()) {
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
    case UnaryOp::PREDEC:
        if (!context.type.IsRef())
            throw SemanticError("expression is not assignable", srcLocation);
        break;

    case UnaryOp::POSTINC:
    case UnaryOp::POSTDEC:
        if (!context.type.IsRef())
            throw SemanticError("expression is not assignable", srcLocation);

        context.type = context.type.RemoveRef();
        break;

    case UnaryOp::SIZEOF:
        context.expr.constant.intVal = context.type.Decay().Size();
        context.expr.isConstant      = true;
        context.type                 = {FundType::INT};

    case UnaryOp::LOGINOT:
        context.type = context.type.Decay();

        if (!context.type.IsConvertibleTo(FundType::BOOL, &context.expr.constant))
            throw SemanticError("invalid argumrnt type '" + context.type.Name()
                                    + "' to unary expression",
                                srcLocation);

        context.type = {FundType::BOOL};
        break;

    default:
        context.type   = context.type.Decay();
        Type arithType = context.type.ArithmeticConvert(FundType::INT);

        if (!context.type.IsConvertibleTo(arithType, &context.expr.constant))
            throw SemanticError("invalid argumrnt type '" + context.type.Name()
                                    + "' to unary expression",
                                srcLocation);
        context.type = arithType;
        break;
    }
}

void CallExpression::Analysis(SemanticContext &context) const
{
    funcExpr->Analysis(context);

    if (!context.type.RemovePtr().IsSimple(TypeClass::FUNCTION))
        throw SemanticError("called object type '" + context.type.Name()
                                + "' is not a function or function pointer",
                            srcLocation);

    params->Analysis(context);
}

void ConstructExpression::Analysis(SemanticContext &context) const
{
    type->Analysis(context);
    if (params)
        params->Analysis(context);
}

void SizeofExpression::Analysis(SemanticContext &context) const
{
    typeId->Analysis(context);

    if (!context.type.IsComplete())
        throw SemanticError("apply sizeof to incomplete type '" + context.type.Name()
                                + "'",
                            srcLocation);

    context.expr.constant.intVal = context.type.Size();
    context.expr.isConstant      = true;
    context.type                 = {FundType::INT};
}

void PlainNew::Analysis(SemanticContext &context) const
{
    throw SemanticError("unimplemented", srcLocation);

    if (placement)
        placement->Analysis(context);

    typeId->Analysis(context);
}

void InitializableNew::Analysis(SemanticContext &context) const
{
    throw SemanticError("unimplemented", srcLocation);

    if (placement)
        placement->Analysis(context);

    typeSpec->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    for (const auto &s : arraySizes) {
        s->Analysis(context);
    }

    if (initializer)
        initializer->Analysis(context);
}

void DeleteExpression::Analysis(SemanticContext &context) const
{
    throw SemanticError("unimplemented", srcLocation);

    expr->Analysis(context);
    // TODO: type check

    context.type = {FundType::VOID};
}

void IdExpression::Analysis(SemanticContext &context) const
{
    SymbolTable *symtab    = nullptr;
    bool         qualified = false;

    if (context.qualifiedScope) {
        std::swap(symtab, context.qualifiedScope);
        qualified = true;
    }
    else {
        symtab = context.symtab;
    }

    if (nameSpec) {
        SemanticContext nameContext(context);
        nameContext.symtab = symtab;

        nameSpec->Analysis(nameContext);
        symtab    = nameContext.qualifiedScope;
        qualified = true;
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
        context.type                 = context.symbolSet->type;
        context.expr.isConstant      = context.symbolSet->Attr() == Symbol::CONSTANT;
        context.expr.constant.intVal = context.symbolSet->intConstant;

        // Id expression is always a l-value (function is l-value already)
        if (!context.type.IsRef() && !context.type.IsSimple(TypeClass::FUNCTION))
            context.type.AddPtrDesc(Type::PtrDescriptor {PtrType::REF});
    }
}

std::string IdExpression::ComposedId(SemanticContext &context) const
{
    switch (stype) {
    case DESTRUCTOR: return "~" + identifier + "()";
    case CONSTRUCTOR: return identifier + "()";
    default: return identifier;
    }
}

void ThisExpression::Analysis(SemanticContext &context) const
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

void IntLiteral::Analysis(SemanticContext &context) const
{
    context.type                 = {FundType::INT};
    context.expr.constant.intVal = value;
    context.expr.isConstant      = true;
}

void FloatLiteral::Analysis(SemanticContext &context) const
{
    context.type                   = {FundType::DOUBLE};
    context.expr.constant.floatVal = value;
    context.expr.isConstant        = true;
}

void CharLiteral::Analysis(SemanticContext &context) const
{
    context.type                  = {FundType::CHAR};
    context.expr.constant.charVal = value;
    context.expr.isConstant       = true;
}

void StringLiteral::Analysis(SemanticContext &context) const
{
    context.type = {FundType::CHAR, CVQualifier::CONST};
    context.type.arrayDescList.push_back(Type::ArrayDescriptor {value.length() + 1, {}});

    context.stringTable.push_back(value);
    context.expr.constant.strIdx = context.stringTable.size() - 1;
    context.expr.isConstant      = true;
}

void BoolLiteral::Analysis(SemanticContext &context) const
{
    context.type                  = {FundType::BOOL};
    context.expr.constant.boolVal = value;
    context.expr.isConstant       = true;
}

void ExpressionList::Analysis(SemanticContext &context) const
{
    // Here symbolSet is the object(s) of function call syntax
    assert(context.symbolSet);

    if (context.symbolSet.size() == 1) {
        // TODO: class, fundtype, enum: para init
        // TODO: operator() or constructor

        if (context.type.IsSimple(TypeClass::FUNCTION)) {
            auto funcDesc = context.type.Function();

            // TODO: default parameter match
            if (funcDesc->paramList.size() != exprList.size())
                throw SemanticError("no matching function for call to '"
                                        + context.symbolSet->id + "'",
                                    srcLocation);

            for (std::size_t i = 0; i < exprList.size(); i++) {
                exprList[i]->Analysis(context);
                if (!context.type.IsConvertibleTo(funcDesc->paramList[i].symbol->type))
                    throw SemanticError("expression type '" + context.type.Name()
                                            + "' does not fit argument type '"
                                            + funcDesc->paramList[i].symbol->type.Name()
                                            + "'",
                                        srcLocation);
            }

            context.type = funcDesc->retType;
        }
        else {
            if (exprList.size() != 1)
                throw SemanticError("excess elements in scalar initializer", srcLocation);

            Type varType = context.type;
            exprList.front()->Analysis(context);

            Constant *c = context.expr.isConstant ? &context.expr.constant : nullptr;
            if (!context.type.IsConvertibleTo(varType, c))
                throw SemanticError("cannot initialize '" + varType.Name() + "' with '"
                                        + context.type.Name() + "'",
                                    srcLocation);
        }
    }
    else {
        // TODO: overload function resolution
        throw SemanticError("function overloading unimplemented", srcLocation);
    }
}

}  // namespace ast