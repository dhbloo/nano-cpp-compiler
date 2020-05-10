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
        auto t1 = static_cast<SimpleTypeSpecifier *>(this);
        auto t2 = static_cast<SimpleTypeSpecifier *>(other.get());

        if ((int)t1->fundTypePart & (int)t2->fundTypePart)
            return "cannot Combine two types";
        else
            t1->fundTypePart = FundTypePart((int)t1->fundTypePart | (int)t2->fundTypePart);
    }
    else if (typeid(*this) == typeid(TypeSpecifier) && typeid(*other) == typeid(TypeSpecifier)) {
        return "duplicate const specifier";
    }
    else if (typeid(*this) != typeid(TypeSpecifier) && typeid(*other) != typeid(TypeSpecifier)) {
        return "more than one type are specified";
    }

    return {};
}

bool ElaboratedTypeSpecifier::operator==(const ElaboratedTypeSpecifier &other)
{
    return nameSpec == other.nameSpec && typeClass == other.typeClass && typeName == other.typeName;
}

};  // namespace ast