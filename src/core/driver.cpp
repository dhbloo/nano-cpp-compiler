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

    int        errCnt = 0;
    yy::parser parser(ast, errCnt, errorStream, {});

    // parser.set_debug_level(isDebugMode);
    errCnt += parser() != 0;

    if (errCnt > 0) {
        errorStream << "parsing failed, " << errCnt << " error generated!\n";
        return false;
    }

    /* Semantic analysis */

    SemanticContext context {errorStream,
                             std::cout,
                             errCnt,
                             isDebugMode,
                             stringTable,
                             &globalSymtab};
    ast->Analysis(context);

    if (errCnt > 0) {
        errorStream << "semantic check failed, " << errCnt << " error generated!\n";
        return false;
    }

    return true;
}

void Driver::PrintSymbolTable(std::ostream &os) const
{
    globalSymtab.Print(os);
}