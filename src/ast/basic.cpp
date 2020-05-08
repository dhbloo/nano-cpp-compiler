#include "node.h"

namespace ast {

void TranslationUnit::Print(std::ostream &os, Indent indent) const
{
    os << "----- Translation Unit Begin ---------------------------------------------------\n";
    for (const auto &decl : decls) {
        decl->Print(os, indent);
    }
    os << "----- Translation Unit End -----------------------------------------------------\n";
}

}  // namespace ast