#include "../ast/node.h"
#include "../core/semantic.h"

#include <cassert>

namespace ast {

std::string ConversionFunctionId::ComposedId(SemanticContext &context) const
{
    SemanticContext newContext {context};
    typeSpec->Analysis(newContext);

    if (ptrSpec)
        ptrSpec->Analysis(newContext);

    return "operator " + newContext.type.Name() + "()";
}

std::string OperatorFunctionId::ComposedId(SemanticContext &context) const
{
    const char *OpNameTable[] = {"+",  "-",  "*",  "/",   "%",   "^",  "&",  "|",  "~",  "!",
                                 "=",  "<",  ">",  "+=",  "-=",  "*=", "/=", "%=", "^=", "&=",
                                 "|=", "<<", ">>", "<<=", ">>=", "==", "!=", "<=", ">=", "&&",
                                 "||", "++", "--", ",",   "->*", "->", "()", "[]"};

    return std::string("operator") + OpNameTable[(int)overloadOp] + "()";
}

void ClassSpecifier::Analysis(SemanticContext &context) const
{
    std::shared_ptr<ClassDescriptor> classDesc;
    SymbolTable *                    symtab    = context.symtab;
    bool                             qualified = false;

    if (nameSpec) {
        nameSpec->Analysis(context);
        symtab    = context.specifiedScope;
        qualified = true;
    }

    classDesc = symtab->QueryClass(identifier, qualified);
    if (!classDesc) {
        if (qualified)
            throw SemanticError("no class named '" + identifier + "' in '" + symtab->ScopeName()
                                    + "'",
                                srcLocation);

        classDesc            = std::make_shared<ClassDescriptor>();
        classDesc->className = identifier;

        // Inject class name into symbol table
        symtab->AddClass(classDesc);
    }

    classDesc->memberTable = std::make_shared<SymbolTable>(context.symtab, classDesc.get());
    context.symtab         = classDesc->memberTable.get();

    if (baseSpec)
        baseSpec->Analysis(context);

    memberList->Analysis(context);

    context.symtab = context.symtab->GetParent();
    context.type   = Type {TypeClass::CLASS, CVQualifier::NONE, {}, {}, FundType::VOID, classDesc};
}

void MemberList::Analysis(SemanticContext &context) const
{
    for (const auto &m : members) {
        m->Analysis(context);
    }
}

void MemberDeclaration::Analysis(SemanticContext &context) const
{
    assert(context.symbolSet);
    auto &attr = context.symbolSet.Get()->attr;

    if (context.decl.isFriend) {
        attr = Symbol::Attribute(attr | Symbol::FRIEND);
        return;
    }

    switch (access) {
    case Access::PRIVATE: attr = Symbol::Attribute(attr | Symbol::PRIVATE); break;
    case Access::PROTECTED: attr = Symbol::Attribute(attr | Symbol::PROTECTED); break;
    case Access::PUBLIC: attr = Symbol::Attribute(attr | Symbol::PUBLIC); break;
    default: break;
    }
}

void MemberDefinition::Analysis(SemanticContext &context) const
{
    auto lastDecl = context.decl;

    if (declSpec)
        declSpec->Analysis(context);
    else {
        context.type =
            Type {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::VOID, nullptr};
    }

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
    if (isPure) {
        if (context.decl.symAttr == Symbol::VIRTUAL)
            context.decl.symAttr = Symbol::PUREVIRTUAL;
        else
            throw SemanticError("only virtual function can be declared pure", srcLocation);
    }

    decl->Analysis(context);

    if (constInit) {
        context.decl.state = DeclState::NODECL;

        Type varType = context.type;

        constInit->Analysis(context);

        if (!context.expr.isConstant)
            throw SemanticError("initialize expression is not constant", srcLocation);

        if (!context.type.IsConvertibleTo(varType))
            throw SemanticError("cannot initialize '" + varType.Name() + "' with "
                                    + context.type.Name(),
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
    auto classDesc = context.symtab->GetClass();
    assert(classDesc);

    SymbolTable *symtab    = context.symtab;
    bool         qualified = false;
    if (nameSpec) {
        nameSpec->Analysis(context);
        symtab    = context.specifiedScope;
        qualified = true;
    }

    // Query base class definition
    auto baseClassDesc = symtab->QueryClass(className, qualified);
    if (!baseClassDesc)
        throw SemanticError("base class '" + className + "' has incomplete type", srcLocation);

    classDesc->baseClassDesc = baseClassDesc.get();
    classDesc->baseAccess    = access;
}

void CtorMemberInitializer::Analysis(SemanticContext &context) const
{
    if (isBaseCtor) {
        SymbolTable *symtab    = context.symtab;
        bool         qualified = false;
        if (nameSpec) {
            nameSpec->Analysis(context);
            symtab    = context.specifiedScope;
            qualified = true;
        }

        auto baseClassDesc = symtab->QueryClass(identifier, qualified);
        if (!baseClassDesc) {
            throw SemanticError("no class named '" + identifier + "' in '" + symtab->ScopeName()
                                    + "'",
                                srcLocation);
        }

        auto classDesc = context.symtab->GetClass();
        bool founded   = false;
        while (classDesc->baseClassDesc) {
            classDesc = classDesc->baseClassDesc;
            if (classDesc == baseClassDesc.get()) {
                founded = true;
                break;
            }
        }

        if (!founded)
            throw SemanticError("'" + baseClassDesc->QualifiedName() + "' is not base of '"
                                    + context.symtab->GetClass()->QualifiedName() + "'",
                                srcLocation);

        // TODO: Get constructor symbol set
    }
    else {
        // Get member symbol
        context.symbolSet = context.symtab->QuerySymbol(identifier);
        if (!context.symbolSet)
            throw SemanticError("use of undeclared identifier '" + identifier + "'", srcLocation);
    }

    exprList->Analysis(context);
}

}  // namespace ast