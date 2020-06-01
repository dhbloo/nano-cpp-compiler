#include "driver.h"

#include "../parser/yyparser.h"
#include "semantic.h"

Driver::Driver(std::ostream &errorStream) : errorStream(errorStream), globalSymtab(nullptr) {}

bool Driver::Parse(bool isDebugMode)
{
    ast.reset();
    globalSymtab.ClearAll();
    stringTable.clear();

    /* Parser analysis */

    int        parseErrcnt = 0;
    yy::parser parser(ast, parseErrcnt, errorStream, {});

    // parser.set_debug_level(isDebugMode);
    parseErrcnt += parser() != 0;

    if (parseErrcnt > 0) {
        errorStream << "parsing failed, " << parseErrcnt << " error generated!\n";
        return false;
    }

    /* Semantic analysis */

    SemanticContext context {errorStream, std::cout, 0, isDebugMode, &globalSymtab, stringTable};
    try {
        ast->Analysis(context);

        if (context.errCnt > 0) {
            errorStream << "semantic check failed, " << context.errCnt << " error generated!\n";
            return false;
        }
    }
    catch (SemanticError error) {
        errorStream << error;
        errorStream << "semantic check failed, " << context.errCnt + 1 << " error generated!\n";
        return false;
    }

    return true;
}

void Driver::PrintSymbolTable(std::ostream &os) const
{
    globalSymtab.Print(os);
}