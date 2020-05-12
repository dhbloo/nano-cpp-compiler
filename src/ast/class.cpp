#include "node.h"

#include <algorithm>

namespace ast {

void ClassSpecifier::MoveDefaultMember()
{
    Access access = key == STRUCT ? Access::PUBLIC : Access::PRIVATE;
    members->MoveDefaultTo(access);
}

std::size_t MemberList::MemberCount() const
{
    return publicMember.size() + protectedMember.size() + privateMember.size();
}

void MemberList::Reverse()
{
    std::reverse(privateMember.begin(), privateMember.end());
    std::reverse(protectedMember.begin(), protectedMember.end());
    std::reverse(publicMember.begin(), publicMember.end());
}

void MemberList::MoveDefaultTo(Access access)
{
    PtrVec<MemberDeclaration> *dst;

    switch (access) {
    case Access::PRIVATE: dst = &privateMember; break;
    case Access::PROTECTED: dst = &protectedMember; break;
    case Access::PUBLIC: dst = &publicMember; break;
    default: return;
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

void ClassSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类定义: " << (key == CLASS ? "class" : "struct") << ", 名称: " << identifier
       << (members->MemberCount() ? "\n" : " (空类)\n");

    if (nameSpec)
        nameSpec->Print(os, indent);

    if (baseSpec)
        baseSpec->Print(os, indent);

    if (members->MemberCount())
        members->Print(os, indent);
}

void MemberList::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类成员列表:\n";

    for (std::size_t i = 0; i < publicMember.size(); i++) {
        os << indent + 1 << "公有成员[" << i << "]: ";
        publicMember[i]->Print(os, indent + 1);
    }
    for (std::size_t i = 0; i < protectedMember.size(); i++) {
        os << indent + 1 << "保护成员[" << i << "]: ";
        protectedMember[i]->Print(os, indent + 1);
    }
    for (std::size_t i = 0; i < privateMember.size(); i++) {
        os << indent + 1 << "私有成员[" << i << "]: ";
        privateMember[i]->Print(os, indent + 1);
    }
}

void MemberDefinition::Print(std::ostream &os, Indent indent) const
{
    os << "成员定义\n";
    if (declSpec)
        declSpec->Print(os, indent + 1);

    for (std::size_t i = 0; i < decls.size(); i++) {
        os << indent + 1 << "定义[" << i << "]:" << (decls[i]->isPure ? " (纯虚函数)\n" : "\n");
        decls[i]->decl->Print(os, indent + 2);
        if (decls[i]->constInit) {
            os << indent + 1 << "初始化:\n";
            decls[i]->constInit->Print(os, indent + 2);
        }
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