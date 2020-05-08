#include "node.h"

namespace ast {



void IntLiteral::Print(std::ostream &os, Indent indent) const {
    os << indent << "Int Literal: " << value << '\n';
}

void FloatLiteral::Print(std::ostream &os, Indent indent) const {
    os << indent << "Float Literal: " << value << '\n';
}

void CharLiteral::Print(std::ostream &os, Indent indent) const {
    os << indent << "Char Literal: '" << value << "'\n";
}

void StringLiteral::Print(std::ostream &os, Indent indent) const {
    os << indent << "String Literal: \"" << value << "\"\n";
}

void BoolLiteral::Print(std::ostream &os, Indent indent) const {
    os << indent << "Bool Literal: " << (value ? "true" : "false") << '\n';
}


}