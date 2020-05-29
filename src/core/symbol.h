#pragma once

#include "type.h"

#include <string>
#include <unordered_map>

struct Symbol
{
    enum Attribute { NONE = 0, STATIC = 1, FNUCCONST = 2, VIRTUAL = 4, PURE = 8 };

    std::string id;
    Type        type;
    int         offset;  // in bytes
    Attribute   attr;
};

class SymbolTable
{
public:
    SymbolTable(SymbolTable *parent);

    // Remove all symbols.
    void ClearAll();

    // If symbol with same name already exists, returns false; otherwise returns true.
    // For function symbol, if symbol with same name and same signture already exists,
    // returns false; otherwise returns true.
    bool AddSymbol(Symbol symbol);

    std::shared_ptr<ClassDescriptor> QueryClass(std::string id);
    std::shared_ptr<EnumDescriptor>  QueryEnum(std::string id);

    SymbolTable *GetRoot();
    std::string  ScopeName() const;
    int          ScopeSize() const;  // in bytes

private:
    SymbolTable *                                                     parent;
    ClassDescriptor *                                                 asscioatedClass;
    std::unordered_multimap<std::string, Symbol>                      symbols;
    std::unordered_map<std::string, std::shared_ptr<ClassDescriptor>> classTypes;
    std::unordered_map<std::string, std::shared_ptr<EnumDescriptor>>  enumTypes;
};