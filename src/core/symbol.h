#pragma once

#include "type.h"

#include <string>
#include <unordered_map>

union Constant {
    // constant value
    std::intmax_t intVal;
    double        floatVal;
    char          charVal;
    bool          boolVal;
    std::size_t   strIdx;
};

struct Symbol
{
    enum Attribute {
        NORMAL = 0,

        STATIC   = 1,
        VIRTUAL  = 2,
        PURE     = 4,
        CONSTANT = 8
    };

    std::string id;
    Type        type;
    Attribute   attr;

    union {
        // constant value
        Constant constant;

        // non constant symbol offset
        struct
        {
            int offset;  // in bytes
        };
    };
};

struct SymbolSet
{
    using It = std::unordered_multimap<std::string, Symbol>::iterator;

    Symbol *single;
    It      begin, end;

    SymbolSet();
    SymbolSet(Symbol *symbol);
    SymbolSet(std::pair<It, It> symbols);
    Symbol *Get();

    operator bool() const;
};

class SymbolTable
{
public:
    SymbolTable(SymbolTable *parent);

    // Remove all symbols.
    void ClearAll();

    // If symbol with same name already exists, returns null; otherwise returns
    // pointer to inserted symbol. For function symbol, if symbol with same name
    // and same signture already exists, returns null; otherwise returns pointer
    // to inserted symbol.
    Symbol *AddSymbol(Symbol symbol);
    bool    AddClass(std::shared_ptr<ClassDescriptor> classDesc);
    bool    AddEnum(std::shared_ptr<EnumDescriptor> enumDesc);
    bool    AddTypedef(std::string aliasName, Type type);

    SymbolSet                        QuerySymbol(std::string id, bool qualified = false);
    std::shared_ptr<ClassDescriptor> QueryClass(std::string id, bool qualified = false);
    std::shared_ptr<EnumDescriptor>  QueryEnum(std::string id, bool qualified = false);
    Type *                           QueryTypedef(std::string id, bool qualified = false);

    SymbolTable *    GetParent();
    SymbolTable *    GetRoot();
    ClassDescriptor *GetClass();
    std::string      ScopeName() const;
    int              ScopeLevel() const;
    int              ScopeSize() const;  // in bytes

    void Print(std::ostream &os) const;

private:
    SymbolTable *                                                     parent;
    ClassDescriptor *                                                 asscioatedClass;
    std::unordered_multimap<std::string, Symbol>                      symbols;
    std::unordered_map<std::string, std::shared_ptr<ClassDescriptor>> classTypes;
    std::unordered_map<std::string, std::shared_ptr<EnumDescriptor>>  enumTypes;
    std::unordered_map<std::string, Type>                             typedefs;

    int curSize;
};