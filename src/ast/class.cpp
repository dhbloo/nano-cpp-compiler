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
       << (members ? "\n" : " (空类)\n");

    if (nameSpec)
        nameSpec->Print(os, indent + 1);

    if (baseSpec)
        baseSpec->Print(os, indent + 1);

    if (members)
        members->Print(os, indent + 1);
}

void MemberList::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类成员列表:\n";
    for (std::size_t i = 0; i < publicMember.size(); i++) {
        os << indent + 1 << "公有成员[" << i << "]:\n";
        publicMember[i]->Print(os, indent + 2);
    }
    for (std::size_t i = 0; i < protectedMember.size(); i++) {
        os << indent + 1 << "保护成员[" << i << "]:\n";
        protectedMember[i]->Print(os, indent + 2);
    }
    for (std::size_t i = 0; i < privateMember.size(); i++) {
        os << indent + 1 << "私有成员[" << i << "]:\n";
        privateMember[i]->Print(os, indent + 2);
    }
    for (std::size_t i = 0; i < defaultMember.size(); i++) {
        os << indent + 1 << "默认成员[" << i << "]:\n";
        defaultMember[i]->Print(os, indent + 2);
    }
}

void MemberVariable::Print(std::ostream &os, Indent indent) const
{
    os << indent << "成员变量:\n";
    declSpec->Print(os, indent + 1);
    for (std::size_t i = 0; i < decls.size(); i++) {
        os << indent + 1 << "变量[" << i << "]:" << (decls[i]->isPure ? " (纯虚函数)\n" : "\n");
        decls[i]->decl->Print(os, indent + 2);
        if (decls[i]->constInit) {
            os << indent + 1 << "初始化:\n";
            decls[i]->constInit->Print(os, indent + 2);
        }
    }
}

void MemberFunction::Print(std::ostream &os, Indent indent) const
{
    os << indent << "成员函数:\n";
    func->Print(os, indent + 1);
}

void BaseSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "基类描述: " << className << ' ';
    switch (access) {
    case Access::PRIVATE: os << "(私有继承)";
    case Access::PROTECTED: os << "(保护继承)";
    case Access::PUBLIC: os << "(公有继承)";
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

void OperatorFunctionId::Print(std::ostream &os, Indent indent) const
{
    const char *OP_NAME[] = {
        "ADD",     "SUB",     "MUL",       "DIV",     "MOD",    "XOR",      "AND",     "OR",
        "NOT",     "LOGINOT", "ASSIGN",    "LT",      "GT",     "SELFADD",  "SELFSUB", "SELFMUL",
        "SELFDIV", "SELFMOD", "SELFXOR",   "SELFAND", "SELFOR", "SHL",      "SHR",     "SELFSHL",
        "SELFSHR", "EQ",      "NE",        "LE",      "GE",     "LOGIAND",  "LOGIOR",  "SELFINC",
        "SELFDEC", "COMMA",   "ARROWSTAR", "ARROW",   "CALL",   "SUBSCRIPT"};

    os << indent << "运算符函数Id: " << OP_NAME[(int)overloadOp]
       << (isGlobal ? " (global)\n" : "\n");
}

}  // namespace ast