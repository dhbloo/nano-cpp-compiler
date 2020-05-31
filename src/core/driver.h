#pragma once

#include "../ast/node.h"
#include "symbol.h"

class Driver
{
public:
    Driver(std::ostream &errorStream);

    bool Parse(bool isDebugMode = false);
    void PrintSymbolTable(std::ostream& os) const;

private:
    std::ostream &errorStream;

    ast::Ptr<ast::TranslationUnit> ast;
    SymbolTable                    globalSymtab;
    std::vector<std::string>       stringTable;
};