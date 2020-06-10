#include "node.h"

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

void Declarator::MergePtrSpec(Ptr<PtrSpecifier> ptrSpec)
{
    Declarator *decl = this;

    while (decl->innerDecl) {
        decl = decl->innerDecl.get();
    }

    decl->ptrSpec = Merge(std::move(ptrSpec), std::move(decl->ptrSpec));
}

bool Declarator::IsFunctionDecl() const
{
    return false;
}

bool FunctionDeclarator::IsFunctionDecl() const
{
    return true;
}

bool IdDeclarator::IsFunctionDecl() const
{
    return !ptrSpec && innerDecl ? innerDecl->IsFunctionDecl() : false;
}

bool Declarator::IsTypeConversionDecl() const
{
    return false;
}

bool IdDeclarator::IsTypeConversionDecl() const
{
    return Is<ConversionFunctionId>(*id);
}

void PtrSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型指针修饰:\n";
    for (size_t i = 0; i < ptrList.size(); i++) {
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

    for (size_t i = 0; i < params.size(); i++) {
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

    for (size_t i = 0; i < ctorInitList.size(); i++) {
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
    for (size_t i = 0; i < initList.size(); i++) {
        os << indent + 1 << "表项[" << i << "]:\n";
        initList[i]->Print(os, indent + 2);
    }
}

void ParenthesisInitializer::Print(std::ostream &os, Indent indent) const
{
    os << indent << "括号初始化:";
    exprList->Print(os, indent + 1);
}

}  // namespace ast