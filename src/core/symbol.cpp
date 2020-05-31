#include "symbol.h"

#include <ostream>

SymbolSet::SymbolSet() : single(nullptr), begin()
{
    end = begin;
}

SymbolSet::SymbolSet(Symbol *symbol) : single(symbol), begin()
{
    end = begin;
}

SymbolSet::SymbolSet(std::pair<It, It> symbols)
    : single(nullptr)
    , begin(symbols.first)
    , end(symbols.second)
{}

Symbol *SymbolSet::Get()
{
    return single;
}

SymbolSet::operator bool() const
{
    return single ? true : begin != end;
}

SymbolTable::SymbolTable(SymbolTable *parent) : parent(parent), curSize(0) {}

void SymbolTable::ClearAll()
{
    symbols.clear();
    classTypes.clear();
    enumTypes.clear();
}

Symbol *SymbolTable::AddSymbol(Symbol symbol)
{
    if (QuerySymbol(symbol.id, true))
        return nullptr;

    if (!(symbol.attr & Symbol::CONSTANT)) {
        // set symbol offset to current scope size
        symbol.offset = curSize;
        curSize += symbol.type.TypeSize();
    }

    auto it = symbols.insert(std::make_pair(symbol.id, symbol));
    return &it->second;
}

bool SymbolTable::AddClass(std::shared_ptr<ClassDescriptor> classDesc)
{
    if (QueryClass(classDesc->className, true))
        return false;

    classTypes.insert(std::make_pair(classDesc->className, classDesc));
    return true;
}

bool SymbolTable::AddEnum(std::shared_ptr<EnumDescriptor> enumDesc)
{
    if (QueryEnum(enumDesc->enumName, true))
        return false;

    enumTypes.insert(std::make_pair(enumDesc->enumName, enumDesc));
    return true;
}

bool SymbolTable::AddTypedef(std::string aliasName, Type type)
{
    if (QueryTypedef(aliasName, true))
        return false;

    typedefs.insert(std::make_pair(aliasName, type));
    return true;
}

SymbolSet SymbolTable::QuerySymbol(std::string id, bool qualified)
{
    auto it = symbols.equal_range(id);
    if (it.first != symbols.end())
        return SymbolSet {it};

    if (!qualified) {
        for (SymbolTable *p = parent; p; p = p->parent) {
            it = p->symbols.equal_range(id);
            if (it.first != symbols.end())
                return SymbolSet {it};
        }
    }

    return SymbolSet {it};
}

std::shared_ptr<ClassDescriptor> SymbolTable::QueryClass(std::string id, bool qualified)
{
    auto it = classTypes.find(id);
    if (it != classTypes.end())
        return it->second;

    if (!qualified) {
        for (SymbolTable *p = parent; p; p = p->parent) {
            it = classTypes.find(id);
            if (it != classTypes.end())
                return it->second;
        }
    }

    return nullptr;
}

std::shared_ptr<EnumDescriptor> SymbolTable::QueryEnum(std::string id, bool qualified)
{
    auto it = enumTypes.find(id);
    if (it != enumTypes.end())
        return it->second;

    if (!qualified) {
        for (SymbolTable *p = parent; p; p = p->parent) {
            it = enumTypes.find(id);
            if (it != enumTypes.end())
                return it->second;
        }
    }

    return nullptr;
}

Type *SymbolTable::QueryTypedef(std::string id, bool qualified)
{
    auto it = typedefs.find(id);
    if (it != typedefs.end())
        return &it->second;

    if (!qualified) {
        for (SymbolTable *p = parent; p; p = p->parent) {
            it = typedefs.find(id);
            if (it != typedefs.end())
                return &it->second;
        }
    }

    return nullptr;
}

SymbolTable *SymbolTable::GetParent()
{
    return parent ? parent : this;
}

SymbolTable *SymbolTable::GetRoot()
{
    SymbolTable *symtab = GetParent();
    while (GetParent() != symtab) {
        symtab = GetParent();
    }
    return symtab;
}

ClassDescriptor *SymbolTable::GetClass()
{
    return asscioatedClass;
}

std::string SymbolTable::ScopeName() const
{
    if (asscioatedClass)
        return asscioatedClass->className;

    if (!parent)
        return "global";
    else
        return "local(" + std::to_string(ScopeLevel()) + ")";
}

int SymbolTable::ScopeLevel() const
{
    int level = 0;
    for (const SymbolTable *symtab = this; symtab; symtab = symtab->parent) {
        level++;
    }
    return level;
}

int SymbolTable::ScopeSize() const
{
    return curSize;
}

void SymbolTable::Print(std::ostream &os) const
{
    for (auto it = symbols.cbegin(); it != symbols.cend(); it++) {
        const Symbol &sym = it->second;

        os << sym.id << ", " << sym.type.Name() << ", " << sym.attr << ", ";
        if (sym.attr & Symbol::CONSTANT) {
            os << "constant";
        }
        else {
            os << sym.offset;
        }
        os << '\n';
    }
}