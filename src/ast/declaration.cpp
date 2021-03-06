#include "node.h"

namespace ast {

SyntaxStatus
Combine(Ptr<DeclSpecifier> n1, Ptr<DeclSpecifier> n2, Ptr<DeclSpecifier> &out)
{
    if (n1->declAttr != DeclSpecifier::NONE && n2->declAttr != DeclSpecifier::NONE)
        return "connot use more than one specifier here";

    if (n1->declAttr == DeclSpecifier::NONE)
        n1->declAttr = n2->declAttr;

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

SyntaxStatus
Combine(Ptr<TypeSpecifier> n1, Ptr<TypeSpecifier> n2, Ptr<TypeSpecifier> &out)
{
    if (Is<SimpleTypeSpecifier>(*n1) && Is<SimpleTypeSpecifier>(*n2)) {
        Ptr<SimpleTypeSpecifier> t1 {static_cast<SimpleTypeSpecifier *>(n1.release())};
        Ptr<SimpleTypeSpecifier> t2 {static_cast<SimpleTypeSpecifier *>(n2.release())};
        Ptr<SimpleTypeSpecifier> tout;

        auto ss = Combine(std::move(t1), std::move(t2), tout);
        out     = std::move(tout);
        if (ss)
            return ss;
    }
    else if (Is<TypeSpecifier>(*n1) && Is<TypeSpecifier>(*n2)) {
        return "duplicate const specifier";
    }
    else if (!Is<TypeSpecifier>(*n1) && !Is<TypeSpecifier>(*n2)) {
        return "more than one type are specified";
    }
    else if (Is<TypeSpecifier>(*n1)) {
        n2->cv = n1->cv;
        out    = std::move(n2);
    }
    else {
        n1->cv = n2->cv;
        out    = std::move(n1);
    }

    return {};
}

SyntaxStatus Combine(Ptr<SimpleTypeSpecifier>  n1,
                     Ptr<SimpleTypeSpecifier>  n2,
                     Ptr<SimpleTypeSpecifier> &out)
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
    return nameSpec == n2.nameSpec && typeKind == n2.typeKind && typeName == n2.typeName;
}

void BlockDeclaration::Print(std::ostream &os, Indent indent) const
{
    declSpec->Print(os, indent);
    for (size_t i = 0; i < initDeclList.size(); i++) {
        os << indent << "声明符[" << i << "]:\n";

        initDeclList[i].declarator->Print(os, indent + 1);

        if (initDeclList[i].initializer) {
            initDeclList[i].initializer->Print(os, indent + 1);
        }
    }
}

void DeclSpecifier::Print(std::ostream &os, Indent indent) const
{
    if (declAttr != NONE) {
        os << indent << "声明描述:";
        switch (declAttr) {
        case STATIC:
            os << " (static)";
            break;
        case FRIEND:
            os << " (friend)";
            break;
        case VIRTUAL:
            os << " (virtual)";
            break;
        default:
            os << " (typedef)";
            break;
        }

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

    for (int i = 1; i <= sizeof(FUNDTYPEPART_NAME) / sizeof(FUNDTYPEPART_NAME[0]); i++) {
        int mask = 1 << i;
        if (((int)fundTypePart & mask) == mask)
            os << FUNDTYPEPART_NAME[i - 1] << ' ';
    }

    os << (fundTypePart == FundTypePart::VOID ? "VOID\n" : "\n");
}

void ElaboratedTypeSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "详述类型描述: ";

    switch (typeKind) {
    case CLASSNAME:
        os << "(类)";
        break;
    case ENUMNAME:
        os << "(枚举)";
        break;
    case TYPEDEFNAME:
        os << "(类型别名)";
        break;
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
    os << indent << "枚举名: " << (identifier.empty() ? "(匿名枚举)" : identifier)
       << '\n';
    for (size_t i = 0; i < enumList.size(); i++) {
        os << indent << "枚举值[" << i << "], 名称: " << enumList[i].first << '\n';

        if (enumList[i].second)
            enumList[i].second->Print(os, indent + 1);
    }
}

};  // namespace ast