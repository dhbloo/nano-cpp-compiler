#pragma once

#include "../ast/node.h"
#include "symbol.h"

class Driver
{
public:
    Driver(std::ostream &errorStream);

    bool Parse(bool isDebugMode = false);

private:
    std::ostream &errorStream;

    ast::Ptr<ast::TranslationUnit> ast;
    SymbolTable                    globalSymtab;
    SymbolTable *                  curSymtab;
};