#include "node.h"

namespace ast {

SyntaxStatus DeclSpecifier::Combine(Ptr<DeclSpecifier> other)
{
    if (isFriend && other->isFriend)
        return "duplicate friend specifier";
    else
        isFriend = isFriend || other->isFriend;

    if (isVirtual && other->isVirtual)
        return "duplicate virtual specifier";
    else
        isVirtual = isVirtual || other->isVirtual;

    if (isTypedef && other->isTypedef)
        return "duplicate typedef specifier";
    else
        isTypedef = isTypedef || other->isTypedef;

    if (typeSpec && other->typeSpec) {
        auto ss = typeSpec->Combine(std::move(other->typeSpec));
        if (ss)
            return ss;
    }
    else if (other->typeSpec)
        typeSpec = std::move(other->typeSpec);

    return {};
}

SyntaxStatus TypeSpecifier::Combine(Ptr<TypeSpecifier> other)
{
    if (typeid(*this) == typeid(SimpleTypeSpecifier)
        && typeid(*other) == typeid(SimpleTypeSpecifier)) {
        auto t  = static_cast<SimpleTypeSpecifier *>(other.get());
        auto ss = static_cast<SimpleTypeSpecifier *>(this)->Combine(t);
        if (ss)
            return ss;
    }
    else if (typeid(*this) == typeid(TypeSpecifier) && typeid(*other) == typeid(TypeSpecifier)) {
        return "duplicate const specifier";
    }
    else if (typeid(*this) != typeid(TypeSpecifier) && typeid(*other) != typeid(TypeSpecifier)) {
        return "more than one type are specified";
    }

    return {};
}

SyntaxStatus SimpleTypeSpecifier::Combine(SimpleTypeSpecifier *other)
{
    if ((int)fundTypePart & (int)other->fundTypePart)
        return "cannot Combine two types";
    else {
        fundTypePart = FundTypePart((int)fundTypePart | (int)other->fundTypePart);
        return {};
    }
}

bool ElaboratedTypeSpecifier::operator==(const ElaboratedTypeSpecifier &other)
{
    return nameSpec == other.nameSpec && typeClass == other.typeClass && typeName == other.typeName;
}

void BlockDeclaration::Print(std::ostream &os, Indent indent) const
{
    declSpec->Print(os, indent);
    for (std::size_t i = 0; i < initDeclList.size(); i++) {
        os << indent << "声明符[" << i << "]:\n";

        initDeclList[i]->declarator->Print(os, indent + 1);

        if (initDeclList[i]->initializer) {
            os << indent + 1 << "初始化:\n";
            initDeclList[i]->initializer->Print(os, indent + 2);
        }
    }
}

void DeclSpecifier::Print(std::ostream &os, Indent indent) const
{
    if (isFriend || isVirtual || isTypedef) {
        os << indent << "声明描述:";
        if (isFriend)
            os << " (friend)";
        if (isVirtual)
            os << " (virtual)";
        if (isTypedef)
            os << " (typedef)";

        os << "\n";
        typeSpec->Print(os, indent + 1);
    }
    else {
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
        {"VOID", "BOOL", "SHORT", "INT", "LONG", "CHAR", "FLOAT", "DOUBLE", "SIGNED", "UNSIGNED"};

    os << indent << "简单类型描述: ";

    if (cv == CVQualifier::CONST)
        os << "(const) ";

    for (int i = 0; i + 1 < sizeof(FUNDTYPEPART_NAME) / sizeof(FUNDTYPEPART_NAME[0]); i++) {
        int mask = 1 << i;
        if (((int)fundTypePart & mask) == mask)
            os << FUNDTYPEPART_NAME[1 + i];
    }
    if (fundTypePart == FundTypePart::VOID)
        os << FUNDTYPEPART_NAME[0];
    os << '\n';
}

void ElaboratedTypeSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "详述类型描述: ";

    switch (typeClass) {
    case TypeClass::CLASSNAME: os << "类名"; break;
    case TypeClass::ENUMNAME: os << "枚举名"; break;
    case TypeClass::TYPEDEFNAME: os << "类型别名"; break;
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
    os << indent << "枚举名: " << identifier;
    for (std::size_t i = 0; i < enumList.size(); i++) {
        os << indent + 1 << "枚举值[" << i << "], 名称: " << enumList[i].first << '\n';
        enumList[i].second->Print(os, indent + 2);
    }
}

};  // namespace ast