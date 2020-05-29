#pragma once

#include "../parser/yylocation.h"
#include "symbol.h"

#include <ostream>

struct SemanticContext
{
    std::ostream &errorStream;
    int           errCnt;
    SymbolTable * symtab;

    Type type;
    // union {
    SymbolTable *specifiedScope;

    struct
    {
        bool isAssignable;
        bool isConstant;

    } expr;

    struct
    {
        bool isInSwitch;
        bool isInLoop;
        bool isInFunction;
    } stmt;
    //};
};

class SemanticError : std::runtime_error
{
public:
    template <typename T>
    SemanticError(T msg, yy::location loc) : std::runtime_error(std::move(msg))
                                           , location(loc)
    {}

    friend inline std::ostream &operator<<(std::ostream &os, SemanticError err)
    {
        return os << err.what() << " at location " << err.location << '\n';
    }

private:
    yy::location location;
};