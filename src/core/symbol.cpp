#include "symbol.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <ostream>

SymbolSet::SymbolSet(Symbol *symbol)
{
    assert(symbol);
    push_back(symbol);
}

SymbolSet::SymbolSet(std::pair<It, It> symbolRange)
{
    for (It it = symbolRange.first; it != symbolRange.second; it++)
        push_back(&it->second);
}

Symbol *SymbolSet::operator->() const
{
    assert(size() > 0);
    return front();
}

SymbolSet::operator Symbol *() const
{
    return size() > 0 ? front() : nullptr;
}

SymbolTable::SymbolTable(SymbolTable *       parent,
                         ClassDescriptor *   classDesc,
                         FunctionDescriptor *funcDesc)
    : parent(parent)
    , classDesc(classDesc)
    , funcDesc(funcDesc)
    , currentOffset(0)
{}

void SymbolTable::ClearAll()
{
    symbols.clear();
    classTypes.clear();
    enumTypes.clear();
}

void SymbolTable::SetStartOffset(int offset)
{
    currentOffset = offset;
}

Symbol *SymbolTable::AddSymbol(Symbol symbol)
{
    if (!symbol.id.empty()) {
        SymbolSet set = QuerySymbol(symbol.id, true);

        if (set) {
            // Check if the found symbol(set) is from base class
            auto curScopeSymbolRange = symbols.equal_range(symbol.id);
            auto isInCurScope        = false;
            for (auto it = curScopeSymbolRange.first; it != curScopeSymbolRange.second;
                 it++) {
                if (&it->second == set) {
                    isInCurScope = true;
                    break;
                }
            }

            if (symbol.type.IsSimple(TypeClass::FUNCTION)) {
                for (auto funcSym : set) {
                    if (!funcSym->type.IsSimple(TypeClass::FUNCTION)) {
                        // Allow derived class override variable from base class
                        if (isInCurScope)
                            return nullptr;
                        else
                            continue;
                    }

                    if (funcSym->type.cv == symbol.type.cv
                        && funcSym->type.Function()->HasSameSignatureWith(
                            *symbol.type.Function())) {
                        // Found an identical function symbol (same name and signature)
                        if (isInCurScope) {
                            // Redeclaration of member function in the same scope is not
                            // allowed
                            if (funcSym->attr & Symbol::ACCESSMASK)
                                return nullptr;
                            else
                                return funcSym;
                        }
                        else {
                            // If the found symbol is from base class, our new symbol will
                            // override it. Check whether the found symbol is virtual, if
                            // so, make inserted symbol virtual too. Otherwise insert new
                            // symbol
                            auto overrideSymbolAttr =
                                Symbol::Attribute(funcSym->attr & ~Symbol::ACCESSMASK);

                            if (overrideSymbolAttr == Symbol::VIRTUAL) {
                                // Add virtual attribute for overriding virtual function,
                                // if it doesn't have virtual attribute
                                auto newSymbolAttr =
                                    Symbol::Attribute(symbol.attr & ~Symbol::ACCESSMASK);

                                if (newSymbolAttr != Symbol::VIRTUAL)
                                    symbol.attr =
                                        Symbol::Attribute(symbol.attr & Symbol::ACCESSMASK
                                                          | Symbol::VIRTUAL);

                                break;
                            }
                        }
                    }
                }

                // If haven't found any function with same name and signiture,
                // insert new one
            }
            else {
                // Allow derived class override variable from base class
                if (isInCurScope)
                    return nullptr;
            }
        }
    }

    if (symbol.attr != Symbol::CONSTANT) {
        // Set symbol offset to current scope size
        // Offset alignment: alignment amount depends on size of
        // symbol type and max alignment requirement is 8 bytes.
        int alignment = symbol.type.AlignmentSize();
        if (alignment >= 8)
            currentOffset = (currentOffset + 7) & ~7;
        else if (alignment >= 4)
            currentOffset = (currentOffset + 3) & ~3;
        else if (alignment >= 2)
            currentOffset = (currentOffset + 1) & ~1;

        symbol.offset = currentOffset;
        currentOffset += symbol.type.Size();
    }

insert:
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
    for (SymbolTable *p = this; p; p = p->parent) {
        auto range = p->symbols.equal_range(id);
        if (range.first != symbols.end())
            return SymbolSet {range};

        if (p->classDesc) {
            for (ClassDescriptor *pc = p->classDesc->baseClassDesc; pc;
                 pc                  = pc->baseClassDesc) {
                range = pc->memberTable->symbols.equal_range(id);
                if (range.first != symbols.end())
                    return SymbolSet {range};
            }
        }

        if (qualified)
            break;
    }

    return {};
}

std::shared_ptr<ClassDescriptor> SymbolTable::QueryClass(std::string id, bool qualified)
{
    auto it = classTypes.find(id);
    if (it != classTypes.end())
        return it->second;

    if (!qualified) {
        for (SymbolTable *p = parent; p; p = p->parent) {
            it = p->classTypes.find(id);
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
            it = p->enumTypes.find(id);
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
            it = p->typedefs.find(id);
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

ClassDescriptor *SymbolTable::GetCurrentClass()
{
    for (SymbolTable *p = this; p; p = p->parent) {
        if (p->classDesc)
            return p->classDesc;
    }
    return nullptr;
}

FunctionDescriptor *SymbolTable::GetCurrentFunction()
{
    for (SymbolTable *p = this; p; p = p->parent) {
        if (p->funcDesc)
            return p->funcDesc;
    }
    return nullptr;
}

std::string SymbolTable::ScopeName() const
{
    if (classDesc) {
        std::string name = classDesc->className;

        for (SymbolTable *p = parent; p; p = p->parent) {
            if (p->classDesc)
                name = p->classDesc->className + "::" + name;
            else
                break;
        }

        return name;
    }

    if (parent)
        return "local(" + std::to_string(ScopeLevel()) + ")";
    else
        return "global";
}

int SymbolTable::ScopeLevel() const
{
    int level = 0;
    for (const SymbolTable *symtab = parent; symtab; symtab = symtab->parent) {
        level++;
    }
    return level;
}

int SymbolTable::ScopeSize() const
{
    return currentOffset;
}

void SymbolTable::Print(std::ostream &os) const
{
    using std::setw;

    os << std::left;
    os << std::string(80, '=') << '\n';
    os << "符号表 " << ScopeName() << ", 大小: " << ScopeSize() << '\n';
    os << std::string(80, '-') << '\n';

    if (!symbols.empty()) {
        os << "符号, 共 " << symbols.size() << " 个\n";
        os << "标识符          类型                                        "
              "属性/访问 偏移/值\n";

        std::vector<const Symbol *> sortedSymbols;
        for (auto it = symbols.cbegin(); it != symbols.cend(); it++) {
            sortedSymbols.push_back(&it->second);
        }

        std::sort(sortedSymbols.begin(),
                  sortedSymbols.end(),
                  [](const Symbol *a, const Symbol *b) {
                      if ((a->attr & ~Symbol::ACCESSMASK) == Symbol::CONSTANT)
                          return false;
                      if ((b->attr & ~Symbol::ACCESSMASK) == Symbol::CONSTANT)
                          return true;
                      return a->offset < b->offset;
                  });

        for (auto symbolPtr : sortedSymbols) {
            const Symbol &sym = *symbolPtr;

            os << setw(15) << sym.id << ' ' << setw(43) << sym.type.Name() << ' ';

            switch (sym.attr & ~Symbol::ACCESSMASK) {
            case Symbol::STATIC: os << "静态 "; break;
            case Symbol::VIRTUAL: os << "虚   "; break;
            case Symbol::PUREVIRTUAL: os << "纯虚 "; break;
            case Symbol::CONSTANT: os << "常量 "; break;
            default: os << "     "; break;
            }

            switch (sym.attr & Symbol::ACCESSMASK) {
            case Symbol::PUBLIC: os << "PUB  "; break;
            case Symbol::PRIVATE: os << "PRI  "; break;
            case Symbol::PROTECTED: os << "PRO  "; break;
            default:
                if (sym.type.typeClass == TypeClass::FUNCTION
                    && sym.type.Function()->friendClass)
                    os << "FRI  ";
                else
                    os << "     ";
                break;
            }

            if (sym.attr == Symbol::CONSTANT)
                // Enum constant
                os << sym.intConstant;
            else
                os << sym.offset;
            os << '\n';
        }
        os << '\n';
    }

    if (!classTypes.empty()) {
        os << "类定义, 共 " << classTypes.size() << " 个\n";
        for (auto it = classTypes.cbegin(); it != classTypes.cend(); it++) {
            os << std::string(80, '=') << '\n';
            os << "类: " << it->first;
            if (it->second->baseClassDesc) {
                os << ", 基类: " << it->second->baseClassDesc->className << ' ';

                switch (it->second->baseAccess) {
                case Access::PRIVATE: os << "(私有继承)"; break;
                case Access::PROTECTED: os << "(保护继承)"; break;
                case Access::PUBLIC: os << "(公有继承)"; break;
                default: os << "(默认继承)";
                }
            }
            os << '\n';

            if (it->second->memberTable)
                it->second->memberTable->Print(os);
        }
        os << '\n';
    }

    if (!enumTypes.empty()) {
        os << "枚举名, 共 " << enumTypes.size() << " 个\n";
        for (auto it = enumTypes.cbegin(); it != enumTypes.cend(); it++) {
            os << '\t' << it->first << '\n';
        }
        os << '\n';
    }

    if (!typedefs.empty()) {
        os << "Typedef 类型别名, 共 " << typedefs.size() << " 个\n";
        for (auto it = typedefs.cbegin(); it != typedefs.cend(); it++) {
            os << '\t' << it->first << " = " << it->second.Name() << '\n';
        }
    }
}