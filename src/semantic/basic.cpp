#include "../ast/node.h"
#include "../core/semantic.h"

namespace ast {

void Node::Analysis(SemanticContext &context) const {}

void TranslationUnit::Analysis(SemanticContext &context) const
{
    // Restore point
    for (const auto &n : decls) {
        context.decl = {DeclState::FULLDECL};
        try {
            n->Analysis(context);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
    }
}

void NameSpecifier::Analysis(SemanticContext &context) const
{
    SymbolTable *&symtab = context.qualifiedScope;
    symtab               = context.symtab;

    if (isGlobal)
        symtab = symtab->GetRoot();

    for (const auto &p : path) {
        auto classDesc = symtab->QueryClass(p);
        if (!classDesc)
            throw SemanticError("no class named '" + p + "' in '" + symtab->ScopeName()
                                    + "'",
                                srcLocation);

        symtab = classDesc->memberTable.get();

        if (!symtab)
            throw SemanticError("incomplete class '" + classDesc->className
                                    + "' named in nested name specifier",
                                srcLocation);
    }
}

}  // namespace ast