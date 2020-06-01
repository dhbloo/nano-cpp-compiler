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
        NORMAL,
        STATIC,
        VIRTUAL,
        PUREVIRTUAL,
        CONSTANT,

        PUBLIC     = 8,
        PRIVATE    = 16,
        PROTECTED  = 32,
        FRIEND     = 64,
        ACCESSMASK = PUBLIC | PRIVATE | PROTECTED | FRIEND,
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
    Symbol *    Get();
    std::size_t Count() const;

    operator bool() const;
};

class SymbolTable
{
public:
    SymbolTable(SymbolTable *       parent,
                ClassDescriptor *   classDesc = nullptr,
                FunctionDescriptor *funcDesc  = nullptr);

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

    SymbolTable *       GetParent();
    SymbolTable *       GetRoot();
    ClassDescriptor *   GetClass();
    FunctionDescriptor *GetFunction();
    std::string         ScopeName() const;
    int                 ScopeLevel() const;
    int                 ScopeSize() const;  // in bytes

    void Print(std::ostream &os) const;

private:
    SymbolTable *                                                     parent;
    ClassDescriptor *                                                 classDesc;
    FunctionDescriptor *                                              funcDesc;
    std::unordered_multimap<std::string, Symbol>                      symbols;
    std::unordered_map<std::string, std::shared_ptr<ClassDescriptor>> classTypes;
    std::unordered_map<std::string, std::shared_ptr<EnumDescriptor>>  enumTypes;
    std::unordered_map<std::string, Type>                             typedefs;

    int curSize;
};