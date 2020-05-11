#include "yyparser.h"

#include <iostream>
#include <cstring>

int main(int argc, char *argv[])
{
    bool debug = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = true;
    }

    for (;;) {
        ast::Ptr<ast::TranslationUnit> root;
        yy::parser parser(root);
        parser.set_debug_level(debug);

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