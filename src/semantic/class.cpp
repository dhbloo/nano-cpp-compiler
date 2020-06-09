#include "../ast/node.h"
#include "../core/semantic.h"

#include <cassert>

namespace ast {

void ClassSpecifier::Analysis(SemanticContext &context) const
{
    std::shared_ptr<ClassDescriptor> classDesc;
    SymbolTable *                    symtab    = context.symtab;
    bool                             qualified = false;

    if (nameSpec) {
        nameSpec->Analysis(context);
        symtab    = context.qualifiedScope;
        qualified = true;
    }

    classDesc = symtab->QueryClass(identifier, qualified);
    if (!classDesc) {
        if (qualified)
            throw SemanticError("no class named '" + identifier + "' in '"
                                    + symtab->ScopeName() + "'",
                                srcLocation);

        classDesc            = std::make_shared<ClassDescriptor>();
        classDesc->className = identifier;
        if (classDesc->className.empty()) {
            classDesc->className = "<anonymous class>";
        }

        // Inject class name into symbol table
        if (!identifier.empty())
            symtab->AddClass(classDesc);
    }

    classDesc->memberTable =
        std::make_shared<SymbolTable>(context.symtab, classDesc.get());

    SemanticContext classContext(context);
    classContext.symtab = classDesc->memberTable.get();

    if (baseSpec)
        baseSpec->Analysis(classContext);

    memberList->Analysis(classContext);

    context.type = {classDesc};
}

void MemberList::Analysis(SemanticContext &context) const
{
    auto lastDecl = context.decl;

    // Restore point
    for (const auto &m : members) {
        try {
            // Always follow last decl from upper scope
            context.decl = {lastDecl.state};
            m->Analysis(context);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
    }

    context.decl = lastDecl;
}

void MemberDeclaration::Analysis(SemanticContext &context) const
{
    assert(context.symbolSet);
    auto &attr = context.symbolSet->attr;

    if (context.decl.isFriend)
        return;

    switch (access) {
    case Access::PRIVATE: attr = Symbol::Attribute(attr | Symbol::PRIVATE); break;
    case Access::PROTECTED: attr = Symbol::Attribute(attr | Symbol::PROTECTED); break;
    default: attr = Symbol::Attribute(attr | Symbol::PUBLIC); break;
    }
}

void MemberDefinition::Analysis(SemanticContext &context) const
{
    auto lastDecl = context.decl;

    if (declSpec)
        declSpec->Analysis(context);
    else
        context.type = {FundType::VOID};

    if (context.decl.state == DeclState::LOCALDECL)
        throw SemanticError("static data member not allowed in local class '"
                                + context.symtab->ScopeName() + "'",
                            srcLocation);

    Type decayType = context.type;
    auto savedDecl = context.decl;

    for (const auto &d : decls) {
        d->Analysis(context);
        MemberDeclaration::Analysis(context);

        context.type = decayType;
        context.decl = savedDecl;
    }

    context.decl = lastDecl;
}

void MemberDeclarator::Analysis(SemanticContext &context) const
{
    decl->Analysis(context);

    if (isPure) {
        if (context.symbolSet->attr == Symbol::VIRTUAL)
            context.symbolSet->attr = Symbol::PUREVIRTUAL;
        else
            throw SemanticError("only virtual function can be declared pure",
                                srcLocation);
    }
    else if (constInit) {
        if ((context.symbolSet->attr & ~Symbol::ACCESSMASK) != Symbol::STATIC)
            throw SemanticError("in-class initialization of data member must be static",
                                srcLocation);

        if (context.type.cv != CVQualifier::CONST)
            throw SemanticError(
                "non-const static data member must be initialized out of line",
                srcLocation);

        SemanticContext newContext(context);
        newContext.decl.state = DeclState::NODECL;

        constInit->Analysis(newContext);

        if (!newContext.expr.isConstant)
            throw SemanticError("initialize expression is not constant", srcLocation);

        if (!newContext.type.IsConvertibleTo(context.type, &newContext.expr.constant))
            throw SemanticError("cannot initialize '" + context.type.Name() + "' with "
                                    + newContext.type.Name(),
                                srcLocation);
    }
}

void MemberFunction::Analysis(SemanticContext &context) const
{
    func->Analysis(context);
    MemberDeclaration::Analysis(context);
}

void BaseSpecifier::Analysis(SemanticContext &context) const
{
    auto classDesc = context.symtab->GetCurrentClass();
    assert(classDesc);

    SymbolTable *symtab    = context.symtab;
    bool         qualified = false;
    if (nameSpec) {
        nameSpec->Analysis(context);
        symtab    = context.qualifiedScope;
        qualified = true;
    }

    // Query base class definition
    auto baseClassDesc = symtab->QueryClass(className, qualified);
    if (!baseClassDesc)
        throw SemanticError("base class '" + className + "' has incomplete type",
                            srcLocation);

    classDesc->baseClassDesc = baseClassDesc.get();
    classDesc->baseAccess    = access;
    classDesc->memberTable->SetStartOffset(baseClassDesc->memberTable->ScopeSize());
}

void CtorMemberInitializer::Analysis(SemanticContext &context) const
{
    if (isBaseCtor) {
        SymbolTable *symtab    = context.symtab;
        bool         qualified = false;
        if (nameSpec) {
            nameSpec->Analysis(context);
            symtab    = context.qualifiedScope;
            qualified = true;
        }

        auto baseClassDesc = symtab->QueryClass(identifier, qualified);
        if (!baseClassDesc) {
            throw SemanticError("no class named '" + identifier + "' in '"
                                    + symtab->ScopeName() + "'",
                                srcLocation);
        }

        auto classDesc = context.symtab->GetCurrentClass();
        bool founded   = false;
        while (classDesc->baseClassDesc) {
            classDesc = classDesc->baseClassDesc;
            if (classDesc == baseClassDesc.get()) {
                founded = true;
                break;
            }
        }

        if (!founded)
            throw SemanticError("'" + baseClassDesc->memberTable->ScopeName()
                                    + "' is not base of '" + context.symtab->ScopeName()
                                    + "'",
                                srcLocation);

        // TODO: Get constructor symbol set
    }
    else {
        // Get member symbol
        context.symbolSet = context.symtab->QuerySymbol(identifier);
        if (!context.symbolSet)
            throw SemanticError("use of undeclared identifier '" + identifier + "'",
                                srcLocation);
    }

    exprList->Analysis(context);
}

void ConversionFunctionId::Analysis(SemanticContext &context) const
{
    Type funcType = context.type;

    typeSpec->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    // TODO: conversion id is not function exception
    assert(funcType.typeClass == TypeClass::FUNCTION);

    // Set function return type to conversion type
    funcType.Function()->retType = context.type;
    context.type                 = funcType;

    IdExpression::Analysis(context);
}

std::string ConversionFunctionId::ComposedId(SemanticContext &context) const
{
    SemanticContext newContext {context};
    typeSpec->Analysis(newContext);

    if (ptrSpec) {
        ptrSpec->Analysis(newContext);
        newContext.type.ptrDescList = std::move(newContext.ptrDescList);
    }

    return "operator " + newContext.type.Name() + "()";
}

std::string OperatorFunctionId::ComposedId(SemanticContext &context) const
{
    const char *OpNameTable[] = {
        "+",  "-",  "*",  "/",  "%",  "^",  "&",  "|",  "~",   "!",  "=",   "<",   ">",
        "+=", "-=", "*=", "/=", "%=", "^=", "&=", "|=", "<<",  ">>", "<<=", ">>=", "==",
        "!=", "<=", ">=", "&&", "||", "++", "--", ",",  "->*", "->", "()",  "[]"};

    return std::string("operator") + OpNameTable[(int)overloadOp] + "()";
}

}  // namespace ast