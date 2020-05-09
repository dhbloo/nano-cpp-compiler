#include "yyparser.h"

#include <iostream>

int main(int argc, char *argv[])
{
    for (;;) {
        ast::Ptr<ast::TranslationUnit> root;
        yy::parser parser(root);

        if (parser() != 0) {
            std::cerr << "Parsing failed!\n\n";
        } else {
            root->Print(std::cout);
        }

        char peek = getc(stdin);
        if (feof(stdin))
            break;
        else
            ungetc(peek, stdin);
    }
}