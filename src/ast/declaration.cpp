#include "node.h"

namespace ast {

SyntaxStatus Combine(Ptr<DeclSpecifier> n1, Ptr<DeclSpecifier> n2, Ptr<DeclSpecifier> &out)
{
    if (n1->isStatic && n2->isStatic)
        return "duplicate static specifier";
    else
        n1->isStatic = n1->isStatic || n2->isStatic;

    if (n1->isFriend && n2->isFriend)
        return "duplicate friend specifier";
    else
        n1->isFriend = n1->isFriend || n2->isFriend;

    if (n1->isVirtual && n2->isVirtual)
        return "duplicate virtual specifier";
    else
        n1->isVirtual = n1->isVirtual || n2->isVirtual;

    if (n1->isTypedef && n2->isTypedef)
        return "duplicate typedef specifier";
    else
        n1->isTypedef = n1->isTypedef || n2->isTypedef;

    if (n1->typeSpec && n2->typeSpec) {
        auto ss = Combine(std::move(n1->typeSpec), std::move(n2->typeSpec), n1->typeSpec);
        if (ss)
            return ss;
    }
    else if (n2->typeSpec)
        n1->typeSpec = std::move(n2->typeSpec);

    out = std::move(n1);
    return {};
}

SyntaxStatus Combine(Ptr<TypeSpecifier> n1, Ptr<TypeSpecifier> n2, Ptr<TypeSpecifier> &out)
{
    if (typeid(*n1) == typeid(SimpleTypeSpecifier) && typeid(*n2) == typeid(SimpleTypeSpecifier)) {
        Ptr<SimpleTypeSpecifier> t1 {static_cast<SimpleTypeSpecifier *>(n1.release())};
        Ptr<SimpleTypeSpecifier> t2 {static_cast<SimpleTypeSpecifier *>(n2.release())};
        Ptr<SimpleTypeSpecifier> tout;

        auto ss = Combine(std::move(t1), std::move(t2), tout);
        out     = std::move(tout);
        if (ss)
            return ss;
    }
    else if (typeid(*n1) == typeid(TypeSpecifier) && typeid(*n2) == typeid(TypeSpecifier)) {
        return "duplicate const specifier";
    }
    else if (typeid(*n1) != typeid(TypeSpecifier) && typeid(*n2) != typeid(TypeSpecifier)) {
        return "more than one type are specified";
    }
    else if (typeid(*n1) == typeid(TypeSpecifier)) {
        n2->cv = n1->cv;
        out    = std::move(n2);
    }
    else {
        n1->cv = n2->cv;
        out    = std::move(n1);
    }

    return {};
}

SyntaxStatus
Combine(Ptr<SimpleTypeSpecifier> n1, Ptr<SimpleTypeSpecifier> n2, Ptr<SimpleTypeSpecifier> &out)
{
    if ((int)n1->fundTypePart & (int)n2->fundTypePart)
        return "cannot combine two types";
    else {
        n1->fundTypePart = FundTypePart((int)n1->fundTypePart | (int)n2->fundTypePart);

        out = std::move(n1);
        return {};
    }
}

bool ElaboratedTypeSpecifier::operator==(const ElaboratedTypeSpecifier &n2)
{
    return nameSpec == n2.nameSpec && typeClass == n2.typeClass && typeName == n2.typeName;
}

void BlockDeclaration::Print(std::ostream &os, Indent indent) const
{
    declSpec->Print(os, indent);
    for (std::size_t i = 0; i < initDeclList.size(); i++) {
        os << indent << "声明符[" << i << "]:\n";

        initDeclList[i].declarator->Print(os, indent + 1);

        if (initDeclList[i].initializer) {
            initDeclList[i].initializer->Print(os, indent + 1);
        }
    }
}

void DeclSpecifier::Print(std::ostream &os, Indent indent) const
{
    if (isFriend || isVirtual || isTypedef || isStatic) {
        os << indent << "声明描述:";
        if (isStatic)
            os << " (static)";
        if (isFriend)
            os << " (friend)";
        if (isVirtual)
            os << " (virtual)";
        if (isTypedef)
            os << " (typedef)";

        os << "\n";
        if (typeSpec)
            typeSpec->Print(os, indent + 1);
    }
    else if (typeSpec) {
        typeSpec->Print(os, indent);
    }
}

void TypeSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型描述:" << (cv == CVQualifier::CONST ? " (const)\n" : "\n");
}

void SimpleTypeSpecifier::Print(std::ostream &os, Indent indent) const
{
    const char *FUNDTYPEPART_NAME[] =
        {"BOOL", "SHORT", "INT", "LONG", "CHAR", "FLOAT", "DOUBLE", "SIGNED", "UNSIGNED"};

    os << indent << "简单类型描述: ";

    if (cv == CVQualifier::CONST)
        os << "(const) ";

    for (int i = 0; i < sizeof(FUNDTYPEPART_NAME) / sizeof(FUNDTYPEPART_NAME[0]); i++) {
        int mask = 1 << i;
        if (((int)fundTypePart & mask) == mask)
            os << FUNDTYPEPART_NAME[i] << ' ';
    }

    os << (fundTypePart == FundTypePart::VOID ? "VOID\n" : "\n");
}

void ElaboratedTypeSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "详述类型描述: ";

    switch (typeClass) {
    case TypeClass::CLASSNAME: os << "(类)"; break;
    case TypeClass::ENUMNAME: os << "(枚举)"; break;
    case TypeClass::TYPEDEFNAME: os << "(类型别名)"; break;
    }

    os << ' ' << typeName << (cv == CVQualifier::CONST ? " (const)\n" : "\n");
    if (nameSpec)
        nameSpec->Print(os, indent + 1);
}

void ClassTypeSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类描述:" << (cv == CVQualifier::CONST ? " (const)\n" : "\n");
    classType->Print(os, indent + 1);
}

void EnumTypeSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "枚举描述:" << (cv == CVQualifier::CONST ? " (const)\n" : "\n");
    enumType->Print(os, indent + 1);
}

void EnumSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "枚举名: " << identifier << '\n';
    for (std::size_t i = 0; i < enumList.size(); i++) {
        os << indent << "枚举值[" << i << "], 名称: " << enumList[i].first << '\n';

        if (enumList[i].second)
            enumList[i].second->Print(os, indent + 1);
    }
}

};  // namespace ast