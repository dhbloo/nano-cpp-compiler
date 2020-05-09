#include "node.h"

namespace ast {

SyntaxStatus DeclSpecifier::combine(Ptr<DeclSpecifier> other)
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

    if (typeSpec && other->typeSpec)
        return "more then one type specifier";
    else if (other->typeSpec)
        typeSpec = std::move(other->typeSpec);

    return {};
}

};  // namespace ast