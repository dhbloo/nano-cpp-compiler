#pragma once

#include "type.h"

#include <string>
#include <unordered_map>
#include <vector>

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
        ACCESSMASK = PUBLIC | PRIVATE | PROTECTED
    };

    std::string id;
    Type        type;
    Attribute   accessAttr;

    union {
        // constant value
        int intConstant;

        // non constant symbol offset
        int offset;  // in bytes
    };

    Attribute Attr() const;
    Attribute Access() const;
    bool      IsMember() const;
    void      SetAttr(Attribute attr);
};

class SymbolSet : public std::vector<Symbol *>
{
    SymbolTable *symbolScope = nullptr;

public:
    using It = std::unordered_multimap<std::string, Symbol>::iterator;

    SymbolSet() = default;
    SymbolSet(Symbol *symbol, SymbolTable *scope);
    SymbolSet(std::pair<It, It> symbolRange, SymbolTable *scope);
    SymbolTable *Scope() const;
    Symbol *     operator->() const;
                 operator Symbol *() const;
};

class SymbolTable
{
public:
    SymbolTable(SymbolTable *       parent,
                ClassDescriptor *   classDesc = nullptr,
                FunctionDescriptor *funcDesc  = nullptr);

    // Remove all symbols.
    void ClearAll();
    void SetStartOffset(int offset);

    // If symbol with same name already exists, returns null; otherwise returns
    // pointer to the inserted symbol. For function symbol, if a function symbol
    // with same name and same signture already exists, returns pointer to that
    // symbol; if a symbol of other type exists, returns null; otherwise returns
    // pointer to the inserted symbol.
    SymbolSet AddSymbol(Symbol symbol);
    bool      AddClass(std::shared_ptr<ClassDescriptor> classDesc);
    bool      AddEnum(std::shared_ptr<EnumDescriptor> enumDesc);
    bool      AddTypedef(std::string aliasName, Type type);

    SymbolSet                        QuerySymbol(std::string id, bool qualified = false);
    std::shared_ptr<ClassDescriptor> QueryClass(std::string id, bool qualified = false);
    std::shared_ptr<EnumDescriptor>  QueryEnum(std::string id, bool qualified = false);
    Type *                           QueryTypedef(std::string id, bool qualified = false);

    // Note: parent of root is still root
    SymbolTable *       GetParent();
    SymbolTable *       GetRoot();
    ClassDescriptor *   GetCurrentClass();
    FunctionDescriptor *GetCurrentFunction();
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

    int currentOffset;
};