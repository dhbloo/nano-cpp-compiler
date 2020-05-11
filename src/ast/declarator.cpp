#include "node.h"

namespace ast {

void PtrSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型指针修饰:\n";
    for (std::size_t i = 0; i < ptrList.size(); i++) {
        os << indent + 1 << "修饰[" << i << "]: ";
        switch (ptrList[i].ptrType) {
        case PTR: os << "指针"; break;
        case REF: os << "引用"; break;
        case CLASSPTR: os << "成员指针"; break;
        }
        os << (ptrList[i].isPtrConst ? " (const)\n" : "\n");

        if (ptrList[i].classNameSpec)
            ptrList[i].classNameSpec->Print(os, indent + 2);
    }
}

void Declarator::Print(std::ostream &os, Indent indent) const
{
    if (ptrSpec)
        ptrSpec->Print(os, indent);
}

void FunctionDeclarator::Print(std::ostream &os, Indent indent) const
{
    Declarator::Print(os, indent);
    os << indent << "函数:" << (isFuncConst ? " (成员const)\n" : "\n");
    if (retType) {
        os << indent + 1 << "函数名:\n";
        retType->Print(os, indent + 2);
    }

    for (std::size_t i = 0; i < params.size(); i++) {
        os << indent + 1 << "参数[" << i << "]:\n";
        params[i]->Print(os, indent + 2);
    }
}

void ArrayDeclarator::Print(std::ostream &os, Indent indent) const
{
    Declarator::Print(os, indent);
    os << indent << "数组:\n";
    if (elemType) {
        os << indent + 1 << "数组元素类型:\n";
        elemType->Print(os, indent + 2);
    }
    if (size) {
        os << indent + 1 << "数组大小:\n";
        size->Print(os, indent + 2);
    }
}

void IdDeclarator::Print(std::ostream &os, Indent indent) const
{
    Declarator::Print(os, indent);
    id->Print(os, indent);
}

void NestedDeclarator::Print(std::ostream &os, Indent indent) const
{
    Declarator::Print(os, indent);
    decl->Print(os, indent);
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
    os << indent << "参数描述:\n";
    os << indent + 1 << "类型描述:\n";
    declSpec->Print(os, indent + 2);

    if (decl) {
        os << indent + 1 << "描述符:\n";
        decl->Print(os, indent + 2);
    }

    if (defaultExpr) {
        os << indent + 1 << "默认值:\n";
        defaultExpr->Print(os, indent + 2);
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

}  // namespace ast