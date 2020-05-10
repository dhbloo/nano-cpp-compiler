#include "node.h"

namespace ast {

void ClassSpecifier::MoveDefaultMember()
{
    Access access = key == STRUCT ? Access::PUBLIC : Access::PRIVATE;
    members->MoveDefaultTo(access);
}

void MemberList::MoveDefaultTo(Access access)
{
    PtrVec<MemberDeclaration> *dst;

    switch (access) {
    case Access::PRIVATE: dst = &privateMember;
    case Access::PROTECTED: dst = &protectedMember;
    case Access::PUBLIC: dst = &publicMember;
    default: throw std::exception("unknown access");
    }

    if (dst->empty()) {
        *dst = std::move(defaultMember);
    }
    else {
        dst->reserve(dst->size() + defaultMember.size());
        std::move(defaultMember.begin(), defaultMember.end(), std::back_inserter(*dst));
        defaultMember.clear();
    }
}

}  // namespace ast