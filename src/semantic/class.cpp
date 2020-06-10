#include "../ast/node.h"
#include "../core/semantic.h"

#include <cassert>
#include <list>

namespace ast {

void ClassSpecifier::Analysis(SemanticContext &context) const
{
    std::shared_ptr<ClassDescriptor> classDesc;
    SymbolTable *                    symtab    = nullptr;
    bool                             qualified = false;

    if (nameSpec) {
        nameSpec->Analysis(context);
        std::swap(symtab, context.qualifiedScope);
        qualified = true;
    }
    else {
        symtab = context.symtab;
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
    auto                       lastDecl = context.decl;
    std::list<SemanticContext> secondPassContext;
    context.secondPassContext = &secondPassContext;

    // Restore point
    // First pass: member declarations
    for (const auto &m : members) {
        try {
            // Always follow last decl from upper scope
            context.decl                 = {lastDecl.state};
            context.decl.memberFirstPass = true;
            m->Analysis(context);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
    }

    // Second pass: member function definitions
    for (const auto &m : members) {
        if (!Is<MemberFunction>(*m))
            continue;

        try {
            // Always follow last decl from upper scope
            context.decl = {lastDecl.state};
            m->Analysis(context);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
        secondPassContext.pop_front();
    }

    context.decl              = lastDecl;
    context.secondPassContext = nullptr;
}

void MemberDeclaration::Analysis(SemanticContext &context) const
{
    if (context.decl.isFriend)
        return;

    switch (access) {
    case Access::PRIVATE: context.decl.symbolAccessAttr = Symbol::PRIVATE; break;
    case Access::PROTECTED: context.decl.symbolAccessAttr = Symbol::PROTECTED; break;
    default: context.decl.symbolAccessAttr = Symbol::PUBLIC; break;
    }
}

void MemberDefinition::Analysis(SemanticContext &context) const
{
    if (declSpec)
        declSpec->Analysis(context);
    else
        context.type = {FundType::VOID};

    if (context.decl.state == DeclState::LOCALDECL)
        throw SemanticError("static data member not allowed in local class '"
                                + context.symtab->ScopeName() + "'",
                            srcLocation);

    if (!context.decl.isTypedef)
        MemberDeclaration::Analysis(context);

    Type decayType = context.type;
    auto savedDecl = context.decl;

    for (const auto &d : decls) {
        d->Analysis(context);

        context.type = decayType;
        context.decl = savedDecl;
    }
}

void MemberDeclarator::Analysis(SemanticContext &context) const
{
    decl->Analysis(context);

    if (isPure) {
        if (context.symbolSet->Attr() == Symbol::VIRTUAL)
            context.symbolSet->SetAttr(Symbol::PUREVIRTUAL);
        else
            throw SemanticError("only virtual function can be declared pure",
                                srcLocation);
    }
    else if (constInit) {
        if (context.symbolSet->Attr() != Symbol::STATIC)
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
    MemberDeclaration::Analysis(context);
    func->Analysis(context);
}

void BaseSpecifier::Analysis(SemanticContext &context) const
{
    auto classDesc = context.symtab->GetCurrentClass();
    assert(classDesc);

    SymbolTable *symtab    = nullptr;
    bool         qualified = false;
    if (nameSpec) {
        nameSpec->Analysis(context);
        std::swap(symtab, context.qualifiedScope);
        qualified = true;
    }
    else {
        symtab = context.symtab;
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
        SymbolTable *symtab    = nullptr;
        bool         qualified = false;
        if (nameSpec) {
            nameSpec->Analysis(context);
            std::swap(symtab, context.qualifiedScope);
            qualified = true;
        }
        else {
            symtab = context.symtab;
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
        context.symbolSet = {};  // !!
        context.type      = {FundType::VOID};
    }
    else {
        // Get member symbol in class scope (current is function scope)
        context.symbolSet = context.symtab->GetParent()->QuerySymbol(identifier);
        if (!context.symbolSet)
            throw SemanticError("use of undeclared identifier '" + identifier + "'",
                                srcLocation);

        context.type = context.symbolSet->type;
    }

    exprList->Analysis(context);
}

void ConversionFunctionId::Analysis(SemanticContext &context) const
{
    Type funcType = context.type;

    if (!funcType.IsSimple(TypeClass::FUNCTION))
        throw SemanticError("function definition does not a function", srcLocation);

    typeSpec->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

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