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
            classDesc = context.qualifiedScope->GetCurrentClass();
            assert(classDesc);
            assert(classDesc->memberTable);
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
    context.symbolSet = {};
}

void FunctionDeclarator::Analysis(SemanticContext &context) const
{
    if (innerDecl)
        innerDecl->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    // current context type is return type
    auto funcDesc = std::make_shared<FunctionDescriptor>(context.type);
    funcDesc->funcScope =
        std::make_shared<SymbolTable>(context.symtab, nullptr, funcDesc.get());
    if (context.decl.isFriend)
        funcDesc->friendClass = context.symtab->GetCurrentClass();

    // function parameters
    SemanticContext newContext(context);
    newContext.symtab = funcDesc->funcScope.get();

    for (const auto &p : params) {
        newContext.decl = {DeclState::PARAMDECL};
        p->Analysis(newContext);
    }

    context.type = {funcDesc, funcCV};
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
            throw SemanticError("array size is not an integral constant expression",
                                srcLocation);

        if (!context.type.IsConvertibleTo(FundType::INT, &context.expr.constant))
            throw SemanticError("'" + context.type.Name()
                                    + "' is not convertible to integral",
                                srcLocation);

        if (context.expr.constant.intVal <= 0)
            throw SemanticError("array declared with non positive size", srcLocation);
        else
            arraySize = (std::size_t)context.expr.constant.intVal;

        context.decl = lastDecl;
        context.type = arrayType;
    }

    Type::ArrayDescriptor arrayDesc {arraySize};

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        arrayDesc.ptrDescList = std::move(context.ptrDescList);
    }

    if (!arrayDesc.ptrDescList.empty()
        && arrayDesc.ptrDescList.back().ptrType == PtrType::REF)
        throw SemanticError("array declared with reference to type '"
                                + context.type.Name() + "'",
                            srcLocation);

    if ((context.decl.state != DeclState::PARAMDECL || context.decl.mustComplete)
        && !context.type.IsComplete() && arrayDesc.ptrDescList.empty())
        throw SemanticError("array declared with incomplete element type '"
                                + context.type.Name() + "'",
                            srcLocation);

    // Array with unknown bound decay to pointer
    if (arrayDesc.size == 0)
        context.type.AddPtrDesc(Type::PtrDescriptor {PtrType::PTR});
    else
        context.type.arrayDescList.push_back(std::move(arrayDesc));
}

void IdDeclarator::Analysis(SemanticContext &context) const
{
    SymbolTable *insymtab = context.symtab;
    if (context.decl.isFriend)
        insymtab = insymtab->GetParent();

    if (innerDecl)
        innerDecl->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    if ((context.decl.state != DeclState::PARAMDECL || context.decl.mustComplete)
        && !context.type.IsComplete())
        throw SemanticError("variable has incomplete type '" + context.type.Name() + "'",
                            srcLocation);

    id->Analysis(context);

    if (context.decl.isTypedef)
        return;

    // Record original symbol's attribute
    auto originAttr = Symbol::Attribute(context.newSymbol.attr & ~Symbol::ACCESSMASK);

    context.symbolSet = insymtab->AddSymbol(context.newSymbol);
    if (!context.symbolSet) {
        throw SemanticError("redefinition of '" + context.newSymbol.id + "'",
                            srcLocation);
    }
    else if (context.type.IsSimple(TypeClass::FUNCTION)) {
        // Due to function overloading, function type might change
        // Here symbolSet should contain exactly one symbol
        context.type = context.symbolSet->type;

        // Check if function symbol's attribute changed, which means
        // overrided function from base class using a different attribute
        auto curAttr = Symbol::Attribute(context.symbolSet->attr & ~Symbol::ACCESSMASK);
        if (originAttr && curAttr != originAttr)
            throw SemanticError("function '" + context.newSymbol.id
                                    + "' overrides a virtual function in base class",
                                srcLocation);

        if (curAttr != Symbol::STATIC && !context.decl.isFriend
            && insymtab->GetCurrentClass())
            context.symbolSet->type.Function()->isNonStaticMember = true;
    }
}

void TypeId::Analysis(SemanticContext &context) const
{
    auto lastDecl = context.decl;
    context.decl  = {DeclState::MINDECL};

    typeSpec->Analysis(context);

    if (abstractDecl)
        abstractDecl->Analysis(context);

    context.decl = lastDecl;
}

void ParameterDeclaration::Analysis(SemanticContext &context) const
{
    declSpec->Analysis(context);

    Symbol *paramSymbol = nullptr;
    if (decl) {
        decl->Analysis(context);
        paramSymbol = context.symbolSet;
    }

    // Unnamed parameter are added directly without check previous symbol
    if (!paramSymbol) {
        context.newSymbol = {"", context.type};
        paramSymbol       = context.symtab->AddSymbol(context.newSymbol);
    }

    if (defaultExpr) {
        context.decl.state = DeclState::NODECL;
        Type paramType     = context.type;

        defaultExpr->Analysis(context);

        if (!context.type.IsConvertibleTo(paramType))
            throw SemanticError("cannot initialize '" + paramType.Name() + "' with '"
                                    + context.type.Name() + "'",
                                srcLocation);
    }

    // Add param symbol to parameter list
    context.symtab->GetCurrentFunction()->paramList.push_back(
        FunctionDescriptor::Param {paramSymbol, defaultExpr != nullptr});
}

void FunctionDefinition::Analysis(SemanticContext &context) const
{
    if (declSpec)
        declSpec->Analysis(context);
    else {
        // constructor/destructor/conversion type
        context.type = {FundType::VOID};
    }

    // Do not allow incomplete type in parameter declaration
    context.decl.mustComplete = true;
    declarator->Analysis(context);

    // TODO: function decl not function type exception
    assert(context.type.typeClass == TypeClass::FUNCTION);

    // Get function descriptor
    auto funcDesc = context.type.Function();
    if (funcDesc->hasBody)
        throw SemanticError("redefinition of function '" + context.symbolSet->id + "'",
                            srcLocation);

    // Enter function scope
    SemanticContext newContext(context);
    newContext.decl.state = DeclState::NODECL;
    newContext.symtab     = funcDesc->funcScope.get();

    for (const auto &i : ctorInitList) {
        i->Analysis(newContext);
    }

    newContext.stmt = {true};
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

    if (!varType.IsRef() && !varType.IsArray())
        context.type = context.type.Decay();

    if (!context.type.IsConvertibleTo(varType))
        throw SemanticError("cannot initialize '" + varType.Name() + "' with '"
                                + context.type.Name() + "'",
                            srcLocation);
}

void ListInitializer::Analysis(SemanticContext &context) const
{
    Type varType = context.type;
    if (!varType.IsArray()) {
        if (initList.size() != 1)
            throw SemanticError("excess elements in scalar initializer", srcLocation);

        initList.front()->Analysis(context);

        if (!context.type.IsConvertibleTo(varType))
            throw SemanticError("cannot initialize '" + varType.Name() + "' with '"
                                    + context.type.Name() + "'",
                                srcLocation);
    }
    else {
        if (initList.size() > varType.ArraySize())
            throw SemanticError("excess elements in array initializer", srcLocation);

        Type elementType = varType.ElementType();

        for (size_t i = 0; i < initList.size(); i++) {
            context.type = elementType;
            initList[i]->Analysis(context);
        }
    }
}

void ParenthesisInitializer::Analysis(SemanticContext &context) const
{
    // TODO: find constructor function for class type
    exprList->Analysis(context);
}

}  // namespace ast