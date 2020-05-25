#include "node.h"

#include <algorithm>

namespace ast {

void ClassSpecifier::MoveDefaultMember()
{
    Access defaultAccess = key == STRUCT ? Access::PUBLIC : Access::PRIVATE;
    memberList->MoveDefaultTo(defaultAccess);
}

std::size_t MemberList::MemberCount() const
{
    return members.size();
}

void MemberList::Reverse()
{
    std::reverse(members.begin(), members.end());
}

void MemberList::MoveDefaultTo(Access access)
{
    // set all access in default member list
    for (auto &&m : defaultMembers) {
        m->access = access;
    }

    if (members.empty()) {
        members = std::move(defaultMembers);
    }
    else {
        members.reserve(members.size() + defaultMembers.size());
        std::move(defaultMembers.begin(), defaultMembers.end(), std::back_inserter(members));
        defaultMembers.clear();
    }
}

void ClassSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类定义: " << (key == CLASS ? "class" : "struct") << ", 名称: " << identifier
       << (memberList->MemberCount() ? "\n" : " (空类)\n");

    if (nameSpec)
        nameSpec->Print(os, indent);

    if (baseSpec)
        baseSpec->Print(os, indent);

    if (memberList->MemberCount())
        memberList->Print(os, indent);
}

void MemberList::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类成员列表:\n";

    for (std::size_t i = 0; i < members.size(); i++) {
        os << indent + 1 << "成员[" << i << "]: ";
        switch (members[i]->access) {
        case Access::PRIVATE: os << "(私有)"; break;
        case Access::PROTECTED: os << "(保护)"; break;
        case Access::PUBLIC: os << "(公有)"; break;
        case Access::DEFAULT: os << "(默认)"; break;
        }
        os << ' ';
        members[i]->Print(os, indent + 1);
    }
}

void MemberDefinition::Print(std::ostream &os, Indent indent) const
{
    os << "成员定义\n";
    if (declSpec)
        declSpec->Print(os, indent + 1);

    for (std::size_t i = 0; i < decls.size(); i++) {
        os << indent + 1 << "定义[" << i << "]:";
        decls[i]->Print(os, indent + 1);
    }
}

void MemberDeclarator::Print(std::ostream &os, Indent indent) const
{
    os << (isPure ? " (纯虚函数)\n" : "\n");
    decl->Print(os, indent + 1);
    if (constInit) {
        os << indent << "初始化:\n";
        constInit->Print(os, indent + 1);
    }
}

void MemberFunction::Print(std::ostream &os, Indent indent) const
{
    os << "成员函数\n";
    func->Print(os, indent + 1);
}

void BaseSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "基类描述: " << className << ' ';
    switch (access) {
    case Access::PRIVATE: os << "(私有继承)"; break;
    case Access::PROTECTED: os << "(保护继承)"; break;
    case Access::PUBLIC: os << "(公有继承)"; break;
    default: os << "(默认继承)";
    }
    os << '\n';
    if (nameSpec)
        nameSpec->Print(os, indent + 1);
}

void ConversionFunctionId::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型转换函数Id:\n";
    typeSpec->Print(os, indent + 1);
    if (ptrSpec)
        ptrSpec->Print(os, indent + 1);
}

void CtorMemberInitializer::Print(std::ostream &os, Indent indent) const
{
    os << indent << "成员: " << identifier << (isBaseCtor ? " (基类构造)\n" : "\n");
    if (nameSpec)
        nameSpec->Print(os, indent + 1);
    if (exprList)
        exprList->Print(os, indent + 1);
}

}  // namespace ast