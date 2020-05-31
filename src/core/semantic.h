#pragma once

#include "../parser/yylocation.h"
#include "symbol.h"

#include <ostream>

enum class DeclState : std::uint8_t { NODECL, FULLDECL, LOCALDECL, MINDECL };

struct SemanticContext
{
    std::ostream &            errorStream;
    std::ostream &            outputStream;
    int                       errCnt;
    bool                      printAllSymtab;
    SymbolTable *             symtab;
    std::vector<std::string> &stringTable;

    Type                             type;
    SymbolSet                        symbolSet;
    Symbol                           newSymbol;
    std::vector<Type::PtrDescriptor> ptrDescList;

    SymbolTable *specifiedScope;

    struct
    {
        bool     isAssignable;
        bool     isConstant;
        Constant constant;
    } expr;

    struct
    {
        bool keepScope;
        bool isSwitchLevel;
        bool isInSwitch;
        bool isInLoop;
    } stmt;

    struct
    {
        DeclState         state;
        bool              isFriend;
        bool              isTypedef;
        Symbol::Attribute symAttr;
    } decl;
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
        return os << "error " << err.location << ": " << err.what() << '\n';
    }

private:
    yy::location location;
};