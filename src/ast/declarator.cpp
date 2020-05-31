#include "../core/semantic.h"
#include "node.h"

#include <cassert>

namespace ast {

Ptr<PtrSpecifier> Merge(Ptr<PtrSpecifier> p1, Ptr<PtrSpecifier> p2)
{
    if (!p1)
        return std::move(p2);
    else if (!p2)
        return std::move(p1);
    else {
        p1->ptrList.insert(p1->ptrList.end(),
                           std::make_move_iterator(p2->ptrList.begin()),
                           std::make_move_iterator(p2->ptrList.end()));

        return std::move(p1);
    }
}

void Declarator::Append(Ptr<Declarator> decl)
{
    if (innerDecl)
        innerDecl->Append(std::move(decl));
    else
        innerDecl = std::move(decl);
}

void PtrSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型指针修饰:\n";
    for (std::size_t i = 0; i < ptrList.size(); i++) {
        os << indent + 1 << "修饰[" << i << "]: ";
        switch (ptrList[i].ptrType) {
        case PtrType::PTR: os << "指针"; break;
        case PtrType::REF: os << "引用"; break;
        case PtrType::CLASSPTR: os << "成员指针"; break;
        }
        os << (ptrList[i].cv == CVQualifier::CONST ? " (const)\n" : "\n");

        if (ptrList[i].ptrType == PtrType::CLASSPTR)
            ptrList[i].classNameSpec->Print(os, indent + 2);
    }
}

void Declarator::Print(std::ostream &os, Indent indent) const
{
    if (ptrSpec)
        ptrSpec->Print(os, indent);
    if (innerDecl)
        innerDecl->Print(os, indent);
}

void FunctionDeclarator::Print(std::ostream &os, Indent indent) const
{
    if (ptrSpec)
        ptrSpec->Print(os, indent);

    os << indent << "函数原型: ";
    if (funcCV == CVQualifier::CONST)
        os << "(成员const) ";
    if (params.empty())
        os << "(无参数)\n";
    else
        os << params.size() << " 参数\n";

    if (innerDecl) {
        os << indent + 1 << "函数返回声明:\n";
        innerDecl->Print(os, indent + 2);
    }

    for (std::size_t i = 0; i < params.size(); i++) {
        os << indent + 1 << "参数[" << i << "]:\n";
        params[i]->Print(os, indent + 2);
    }
}

void ArrayDeclarator::Print(std::ostream &os, Indent indent) const
{
    if (ptrSpec)
        ptrSpec->Print(os, indent);

    os << indent << "数组:" << (size ? "\n" : " (未知大小)\n");
    if (innerDecl) {
        os << indent + 1 << "数组元素声明:\n";
        innerDecl->Print(os, indent + 2);
    }
    if (size) {
        os << indent + 1 << "数组大小:\n";
        size->Print(os, indent + 2);
    }
}

void IdDeclarator::Print(std::ostream &os, Indent indent) const
{
    if (ptrSpec)
        ptrSpec->Print(os, indent);

    id->Print(os, indent);
    if (innerDecl)
        innerDecl->Print(os, indent + 1);
}

void TypeId::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型Id:\n";
    typeSpec->Print(os, indent + 1);
    if (abstractDecl) {
        abstractDecl->Print(os, indent + 1);
    }
}

void ParameterDeclaration::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型描述:\n";
    declSpec->Print(os, indent + 1);

    if (decl) {
        os << indent << "描述符:\n";
        decl->Print(os, indent + 1);
    }

    if (defaultExpr) {
        os << indent << "默认值:\n";
        defaultExpr->Print(os, indent + 1);
    }
}

void FunctionDefinition::Print(std::ostream &os, Indent indent) const
{
    os << indent << "函数定义:\n";

    if (declSpec) {
        os << indent + 1 << "函数返回类型:\n";
        declSpec->Print(os, indent + 2);
    }

    declarator->Print(os, indent + 1);

    for (std::size_t i = 0; i < ctorInitList.size(); i++) {
        os << indent + 1 << "构造函数初始化[" << i << "]:\n";
        ctorInitList[i]->Print(os, indent + 2);
    }

    os << indent + 1 << "函数体:\n";
    funcBody->Print(os, indent + 2);
}

void AssignmentInitializer::Print(std::ostream &os, Indent indent) const
{
    os << indent << "赋值初始化:\n";
    expr->Print(os, indent + 1);
}

void ListInitializer::Print(std::ostream &os, Indent indent) const
{
    os << indent << "列表初始化:\n";
    for (std::size_t i = 0; i < initList.size(); i++) {
        os << indent + 1 << "表项[" << i << "]:\n";
        initList[i]->Print(os, indent + 2);
    }
}

void ParenthesisInitializer::Print(std::ostream &os, Indent indent) const
{
    os << indent << "括号初始化:\n";
    exprList->Print(os, indent + 1);
}

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

    auto funcDesc = std::make_shared<FunctionDescriptor>();

    // current context type is return type
    funcDesc->retType   = context.type;
    funcDesc->funcScope = std::make_shared<SymbolTable>(context.symtab);
    context.symtab      = funcDesc->funcScope.get();

    // function parameters
    auto lastDecl      = context.decl;
    context.decl.state = DeclState::MINDECL;
    for (const auto &p : params) {
        p->Analysis(context);
    }
    context.decl = lastDecl;

    context.type   = Type {TypeClass::FUNCTION, funcCV, {}, {}, FundType::VOID, funcDesc};
    context.symtab = context.symtab->GetParent();

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }
}

void ArrayDeclarator::Analysis(SemanticContext &context) const
{
    if (innerDecl)
        innerDecl->Analysis(context);

    auto lastDecl         = context.decl;
    context.decl.state    = DeclState::NODECL;
    std::size_t arraySize = 0;

    if (size) {
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
    }

    context.decl = lastDecl;
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

    id->Analysis(context);

    SymbolTable *insymtab = context.symtab;
    if (context.decl.isFriend)
        insymtab = insymtab->GetParent();

    context.symbolSet = insymtab->AddSymbol(context.newSymbol);
    if (!context.symbolSet)
        throw SemanticError("redeclaration of '" + context.newSymbol.id + "'", srcLocation);
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
    assert(context.type.typeClass == TypeClass::FUNCTION);
    auto funcDesc = static_cast<FunctionDescriptor *>(context.type.typeDesc.get());

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

        if (!context.type.IsConvertibleTo(paramType))
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
    auto lastDecl = context.decl;

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
    auto funcDesc      = static_cast<FunctionDescriptor *>(context.type.typeDesc.get());
    context.decl.state = DeclState::NODECL;

    for (const auto &i : ctorInitList) {
        i->Analysis(context);
    }

    // Enter function scope
    auto lastStmt  = context.stmt;
    context.symtab = funcDesc->funcScope.get();
    context.stmt   = {true, false, false, false};

    funcBody->Analysis(context);

    // Leave function scope
    context.symtab = context.symtab->GetParent();
    context.stmt   = lastStmt;

    context.decl = lastDecl;
}

void AssignmentInitializer::Analysis(SemanticContext &context) const
{
    Type varType = context.type;

    expr->Analysis(context);

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