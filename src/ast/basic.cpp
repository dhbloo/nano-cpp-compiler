#include "../core/semantic.h"
#include "node.h"

namespace ast {

void TranslationUnit::Print(std::ostream &os, Indent indent) const
{
    os << indent << "---------- 翻译单元起始 -------------------------------------------------\n";
    for (std::size_t i = 0; i < decls.size(); i++) {
        os << indent << "全局声明[" << i << "]:\n";
        decls[i]->Print(os, indent + 1);
    }
    os << indent << "---------- 翻译单元结束 -------------------------------------------------\n";
}

void NameSpecifier::Print(std::ostream &os, Indent indent) const
{
    os << indent << "名称限定: " << (isGlobal ? "(全局)\n" : "\n");
    for (std::size_t i = 0; i < path.size(); i++)
        os << indent + 1 << "路径[" << i << "]: " << path[i] << '\n';
}

void Node::Analysis(SemanticContext &context) const {}

void TranslationUnit::Analysis(SemanticContext &context) const
{
    for (const auto &n : decls) {
        n->Analysis(context);
    }
}

void NameSpecifier::Analysis(SemanticContext &context) const
{
    SymbolTable *&symtab = context.specifiedScope;
    symtab               = context.symtab;

    if (isGlobal)
        symtab = symtab->GetRoot();

    for (const auto &p : path) {
        auto classDesc = symtab->QueryClass(p);
        if (!classDesc)
            throw SemanticError("No " + p + " class under " + symtab->ScopeName(), srcLocation);

        symtab = classDesc->memberTable.get();
    }
}

}  // namespace ast