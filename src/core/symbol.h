#pragma once

#include "type.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace llvm {
class Value;
}

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
        struct
        {
            int          index;   // index of member in a scope
            int          offset;  // in bytes
            llvm::Value *value;   // llvm value (for local and global var)
        };
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

    // Set current offset to some value
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

    SymbolTable *       GetParent();
    SymbolTable *       GetRoot();
    ClassDescriptor *   GetCurrentClass();
    FunctionDescriptor *GetCurrentFunction();
    std::string         ScopeName() const;
    int                 ScopeLevel() const;
    int                 ScopeSize() const;  // in bytes

    // Get a sorted list of symbol, according to their index of insertion.
    // Note: Constant are skipped.
    std::vector<Symbol *> SortedSymbols();
    void                  Print(std::ostream &os) const;

private:
    SymbolTable *                                                     parent;
    ClassDescriptor *                                                 classDesc;
    FunctionDescriptor *                                              funcDesc;
    std::unordered_multimap<std::string, Symbol>                      symbols;
    std::unordered_map<std::string, std::shared_ptr<ClassDescriptor>> classTypes;
    std::unordered_map<std::string, std::shared_ptr<EnumDescriptor>>  enumTypes;
    std::unordered_map<std::string, Type>                             typedefs;

    int currentIndex;
    int currentOffset;
};