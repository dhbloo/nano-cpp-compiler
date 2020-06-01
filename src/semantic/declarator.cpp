#include "../ast/node.h"
#include "../core/semantic.h"

#include <cassert>

namespace ast {

void PtrSpecifier::Analysis(SemanticContext &context) const
{
    context.ptrDescList.clear();

    bool hasRefenerce = false;
    for (const auto &p : ptrList) {
        ClassDescriptor *classDesc = nullptr;

        if (hasRefenerce) {
            if (p.ptrType == PtrType::REF)
                throw SemanticError("reference to reference is forbidden", srcLocation);
            else
                throw SemanticError("pointer to reference is forbidden", srcLocation);
        }

        if (p.ptrType == PtrType::CLASSPTR) {
            p.classNameSpec->Analysis(context);
            classDesc = context.specifiedScope->GetClass();
        }
        else if (p.ptrType == PtrType::REF) {
            hasRefenerce = true;
        }

        context.ptrDescList.push_back(Type::PtrDescriptor {p.ptrType, p.cv, classDesc});
    }
}

void Declarator::Analysis(SemanticContext &context) const
{
    if (innerDecl)
        innerDecl->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    // abstract declarator has no symbol
    context.symbolSet = nullptr;
}

void FunctionDeclarator::Analysis(SemanticContext &context) const
{
    if (innerDecl)
        innerDecl->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    auto funcDesc = std::make_shared<FunctionDescriptor>();

    // current context type is return type
    funcDesc->retType = context.type;
    funcDesc->hasBody = false;
    if (context.decl.isFriend)
        funcDesc->friendClass = context.symtab->GetClass();
    funcDesc->funcScope = std::make_shared<SymbolTable>(context.symtab, nullptr, funcDesc.get());
    context.symtab      = funcDesc->funcScope.get();

    // function parameters
    SemanticContext newContext(context);
    newContext.decl.state     = DeclState::MINDECL;
    newContext.decl.paramDecl = true;

    for (const auto &p : params) {
        p->Analysis(newContext);
    }

    context.type   = Type {TypeClass::FUNCTION, funcCV, {}, {}, FundType::VOID, funcDesc};
    context.symtab = context.symtab->GetParent();
}

void ArrayDeclarator::Analysis(SemanticContext &context) const
{
    if (innerDecl)
        innerDecl->Analysis(context);

    std::size_t arraySize = 0;

    if (size) {
        Type arrayType     = context.type;
        auto lastDecl      = context.decl;
        context.decl.state = DeclState::NODECL;

        size->Analysis(context);
        if (!context.expr.isConstant)
            throw SemanticError("array size is not an integral constant expression", srcLocation);

        if (!context.type.IsConvertibleTo(Type::IntType))
            throw SemanticError("'" + context.type.Name() + "' is not convertible to integral",
                                srcLocation);

        auto val = context.type.ConvertConstant(context.expr.constant, Type::IntType);
        if (val.intVal <= 0)
            throw SemanticError("array declared with non positive size", srcLocation);
        else
            arraySize = (std::size_t)val.intVal;

        context.decl = lastDecl;
        context.type = arrayType;
    }

    Type::ArrayDescriptor arrayDesc {arraySize};

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        arrayDesc.ptrDescList = std::move(context.ptrDescList);
    }

    if (!arrayDesc.ptrDescList.empty() && arrayDesc.ptrDescList.back().ptrType == PtrType::REF)
        throw SemanticError("array declared with reference to type '" + context.type.Name() + "'",
                            srcLocation);

    if (!context.type.IsComplete() && arrayDesc.ptrDescList.empty())
        throw SemanticError("array declared with incomplete element type '" + context.type.Name()
                                + "'",
                            srcLocation);

    context.type.arrayDescList.push_back(std::move(arrayDesc));
}

void IdDeclarator::Analysis(SemanticContext &context) const
{
    if (innerDecl)
        innerDecl->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    if (!context.decl.paramDecl && !context.type.IsComplete())
        throw SemanticError("variable has incomplete type '" + context.type.Name() + "'",
                            srcLocation);

    id->Analysis(context);

    if (context.decl.isTypedef)
        return;

    SymbolTable *insymtab = context.symtab;
    if (context.decl.isFriend)
        insymtab = insymtab->GetParent();

    context.symbolSet = insymtab->AddSymbol(context.newSymbol);
    if (!context.symbolSet) {
        throw SemanticError("redeclaration of '" + context.newSymbol.id + "'", srcLocation);
    }
}

void TypeId::Analysis(SemanticContext &context) const
{
    auto lastDecl      = context.decl;
    context.decl.state = DeclState::MINDECL;

    typeSpec->Analysis(context);

    if (abstractDecl)
        abstractDecl->Analysis(context);

    context.decl = lastDecl;
}

void ParameterDeclaration::Analysis(SemanticContext &context) const
{
    auto funcDesc = context.symtab->GetFunction();

    declSpec->Analysis(context);

    if (decl)
        decl->Analysis(context);

    if (!decl || !context.symbolSet) {
        context.newSymbol = {"", context.type, {}};
        context.symbolSet = context.symtab->AddSymbol(context.newSymbol);
    }

    Symbol *paramSymbol = context.symbolSet.Get();

    if (defaultExpr) {
        auto lastDecl      = context.decl;
        context.decl.state = DeclState::NODECL;

        Type paramType = context.type;

        defaultExpr->Analysis(context);

        bool passed = false;
        if (!paramType.ptrDescList.empty()) {
            Type removeRefT = paramType;
            removeRefT.ptrDescList.pop_back();
            if (removeRefT != context.type)
                passed = true;
        }

        if (!passed && !context.type.IsConvertibleTo(paramType))
            throw SemanticError("cannot initialize '" + paramType.Name() + "' with "
                                    + context.type.Name(),
                                srcLocation);

        context.decl = lastDecl;
    }

    // Add param symbol to parameter list
    funcDesc->paramList.push_back(FunctionDescriptor::Param {paramSymbol, defaultExpr != nullptr});
}

void FunctionDefinition::Analysis(SemanticContext &context) const
{
    if (declSpec)
        declSpec->Analysis(context);
    else {
        // TODO: constructor/destructor/conversion type
        context.type =
            Type {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::VOID, nullptr};
    }

    declarator->Analysis(context);

    // Get function descriptor
    assert(context.type.typeClass == TypeClass::FUNCTION);
    auto funcDesc = static_cast<FunctionDescriptor *>(context.type.typeDesc.get());

    auto lastDecl      = context.decl;
    context.decl.state = DeclState::NODECL;
    for (const auto &i : ctorInitList) {
        i->Analysis(context);
    }
    context.decl = lastDecl;

    // Enter function scope
    SemanticContext newContext(context);
    newContext.symtab = funcDesc->funcScope.get();
    newContext.stmt   = {true, false, false, false};

    funcBody->Analysis(newContext);

    // Leave function scope
    funcDesc->hasBody = true;
    if (context.printAllSymtab)
        funcDesc->funcScope->Print(context.outputStream);
}

void AssignmentInitializer::Analysis(SemanticContext &context) const
{
    Type varType = context.type;

    expr->Analysis(context);

    if (!varType.ptrDescList.empty()) {
        Type removeRefT = varType;
        removeRefT.ptrDescList.pop_back();
        if (removeRefT == context.type)
            return;
    }

    if (!context.type.IsConvertibleTo(varType))
        throw SemanticError("cannot initialize '" + varType.Name() + "' with "
                                + context.type.Name(),
                            srcLocation);
}

void ListInitializer::Analysis(SemanticContext &context) const
{
    for (const auto &i : initList) {
        i->Analysis(context);
    }
}

void ParenthesisInitializer::Analysis(SemanticContext &context) const
{
    exprList->Analysis(context);
}

}  // namespace ast