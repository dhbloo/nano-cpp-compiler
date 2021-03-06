#include "../ast/node.h"
#include "context.h"

namespace ast {

void TranslationUnit::Codegen(CodegenContext &context) const
{
    // Restore point
    for (const auto &n : decls) {
        context.decl = {DeclState::FULLDECL};
        try {
            n->Codegen(context);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
    }
}

void NameSpecifier::Codegen(CodegenContext &context) const
{
    SymbolTable *symtab = context.symtab;

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

    context.qualifiedScope = symtab;
}

}  // namespace ast