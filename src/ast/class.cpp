#include "../core/semantic.h"
#include "node.h"

#include <algorithm>
#include <cassert>

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

void ClassSpecifier::Analysis(SemanticContext &context) const
{
    std::shared_ptr<ClassDescriptor> classDesc;
    SymbolTable *                    symtab    = context.symtab;
    bool                             qualified = false;

    if (nameSpec) {
        nameSpec->Analysis(context);
        symtab    = context.specifiedScope;
        qualified = true;
    }

    classDesc = symtab->QueryClass(identifier, qualified);
    if (!classDesc) {
        if (qualified)
            throw SemanticError("no class named '" + identifier + "' in '" + symtab->ScopeName()
                                    + "'",
                                srcLocation);

        classDesc            = std::make_shared<ClassDescriptor>();
        classDesc->className = identifier;

        // Inject class name into symbol table
        symtab->AddClass(classDesc);
    }

    classDesc->memberTable = std::make_shared<SymbolTable>(context.symtab);
    context.symtab         = classDesc->memberTable.get();

    if (baseSpec)
        baseSpec->Analysis(context);

    memberList->Analysis(context);

    context.symtab = context.symtab->GetParent();
    context.type   = Type {TypeClass::CLASS, CVQualifier::NONE, {}, {}, FundType::VOID, classDesc};
}

void MemberList::Analysis(SemanticContext &context) const
{
    for (const auto &m : members) {
        m->Analysis(context);
    }
}

void MemberDefinition::Analysis(SemanticContext &context) const
{
    auto lastDecl = context.decl;

    if (declSpec)
        declSpec->Analysis(context);
    else {
        context.type =
            Type {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::VOID, nullptr};
    }

    if (context.decl.state == DeclState::LOCALDECL)
        throw SemanticError("static data member not allowed in local class", srcLocation);

    Type decayType = context.type;
    auto savedDecl = context.decl;

    for (const auto &d : decls) {
        d->Analysis(context);

        context.type = decayType;
        context.decl = savedDecl;
    }

    context.decl = lastDecl;
}

void MemberDeclarator::Analysis(SemanticContext &context) const
{
    if (isPure)
        context.decl.symAttr = Symbol::Attribute(context.decl.symAttr | Symbol::PURE);

    decl->Analysis(context);

    if (constInit) {
        Type varType = context.type;

        constInit->Analysis(context);

        if (!context.type.IsConvertibleTo(varType))
            throw SemanticError("cannot initialize '" + varType.Name() + "' with "
                                    + context.type.Name(),
                                srcLocation);
    }
}

void MemberFunction::Analysis(SemanticContext &context) const
{
    func->Analysis(context);
}

void BaseSpecifier::Analysis(SemanticContext &context) const
{
    auto classDesc = context.symtab->GetClass();
    assert(classDesc);

    SymbolTable *symtab    = context.symtab;
    bool         qualified = false;
    if (nameSpec) {
        nameSpec->Analysis(context);
        symtab    = context.specifiedScope;
        qualified = true;
    }

    auto baseClassDesc = symtab->QueryClass(className, qualified);
    assert(baseClassDesc);
    /*if (!baseClassDesc) {
        throw SemanticError("no class named '" + className + "' in '" + symtab->ScopeName() + "'",
                            srcLocation);
    }*/

    classDesc->baseClassDesc = baseClassDesc.get();
    classDesc->baseAccess    = access;
}

void CtorMemberInitializer::Analysis(SemanticContext &context) const
{
    if (isBaseCtor) {
        SymbolTable *symtab    = context.symtab;
        bool         qualified = false;
        if (nameSpec) {
            nameSpec->Analysis(context);
            symtab    = context.specifiedScope;
            qualified = true;
        }

        auto baseClassDesc = symtab->QueryClass(identifier, qualified);
        if (!baseClassDesc) {
            throw SemanticError("no class named '" + identifier + "' in '" + symtab->ScopeName()
                                    + "'",
                                srcLocation);
        }

        auto classDesc = context.symtab->GetClass();
        bool founded   = false;
        while (classDesc->baseClassDesc) {
            classDesc = classDesc->baseClassDesc;
            if (classDesc == baseClassDesc.get()) {
                founded = true;
                break;
            }
        }

        if (!founded)
            throw SemanticError("'" + baseClassDesc->QualifiedName() + "' is not base of '"
                                    + context.symtab->GetClass()->QualifiedName() + "'",
                                srcLocation);

        // TODO: Get constructor symbol set
    }
    else {
        // Get member symbol
        context.symbolSet = context.symtab->QuerySymbol(identifier);
        if (!context.symbolSet)
            throw SemanticError("use of undeclared identifier '" + identifier + "'", srcLocation);
    }

    exprList->Analysis(context);
}

std::string ConversionFunctionId::ComposedId(SemanticContext &context) const
{
    SemanticContext newContext {context};
    typeSpec->Analysis(newContext);

    if (ptrSpec)
        ptrSpec->Analysis(newContext);

    return "operator " + newContext.type.Name() + "()";
}

std::string OperatorFunctionId::ComposedId(SemanticContext &context) const
{
    const char *OpNameTable[] = {"+",  "-",  "*",  "/",   "%",   "^",  "&",  "|",  "~",  "!",
                                 "=",  "<",  ">",  "+=",  "-=",  "*=", "/=", "%=", "^=", "&=",
                                 "|=", "<<", ">>", "<<=", ">>=", "==", "!=", "<=", ">=", "&&",
                                 "||", "++", "--", ",",   "->*", "->", "()", "[]"};

    return std::string("operator") + OpNameTable[(int)overloadOp] + "()";
}

}  // namespace ast