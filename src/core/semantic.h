#pragma once

#include "../parser/yylocation.h"
#include "symbol.h"

#include <ostream>

enum class DeclState : std::uint8_t {
    NODECL,     // only allow symbol reference
    PARAMDECL,  // new symbol (no class or enum definition, allow incomplete type)
    MINDECL,    // new symbol (no class or enum definition)
    LOCALDECL,  // new symbol (no static data member)
    FULLDECL    // new symbol
};

union Constant {
    // constant value
    std::intmax_t intVal;
    double        floatVal;
    char          charVal;
    bool          boolVal;
    std::size_t   strIdx;
};

struct SemanticContext
{
    std::ostream &            errorStream;
    std::ostream &            outputStream;
    int &                     errCnt;
    bool                      printAllSymtab;
    std::vector<std::string> &stringTable;

    SymbolTable *                    symtab;
    Type                             type;
    SymbolSet                        symbolSet;
    Symbol                           newSymbol;
    SymbolTable *                    qualifiedScope;
    std::vector<Type::PtrDescriptor> ptrDescList;

    struct
    {
        bool     isConstant;
        bool     qualified;
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
        bool              mustComplete;
        Symbol::Attribute symAttr;
    } decl;
};

class SemanticError : std::runtime_error
{
public:
    template <typename T>
    SemanticError(T msg, yy::location loc)
        : std::runtime_error(std::move(msg))
        , location(loc)
    {}

    friend inline std::ostream &operator<<(std::ostream &os, SemanticError err)
    {
        return os << "error " << err.location << ": " << err.what() << '\n';
    }

private:
    yy::location location;
};