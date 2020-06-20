#include "symbol.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <ostream>

Symbol::Attribute Symbol::Attr() const
{
    return Attribute(accessAttr & ~ACCESSMASK);
}

Symbol::Attribute Symbol::Access() const
{
    return Attribute(accessAttr & ACCESSMASK);
}

bool Symbol::IsMember() const
{
    return (accessAttr & ACCESSMASK) != 0;
}

void Symbol::SetAttr(Attribute attr)
{
    accessAttr = Attribute(attr | Access());
}

SymbolSet::SymbolSet(Symbol *symbol, SymbolTable *scope) : symbolScope(scope)
{
    if (symbol)
        push_back(symbol);
}

SymbolSet::SymbolSet(std::pair<It, It> symbolRange, SymbolTable *scope)
    : symbolScope(scope)
{
    for (It it = symbolRange.first; it != symbolRange.second; it++)
        push_back(&it->second);
}

SymbolTable *SymbolSet::Scope() const
{
    return symbolScope;
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
    , currentIndex(0)
    , currentOffset(0)
{}

void SymbolTable::SetStartOffset(int offset)
{
    currentOffset = offset;
}

SymbolSet SymbolTable::AddSymbol(Symbol symbol)
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

            if (symbol.type.IsSimple(TypeKind::FUNCTION)) {
                for (auto funcSym : set) {
                    if (!funcSym->type.IsSimple(TypeKind::FUNCTION)) {
                        // Allow derived class override variable from base class
                        if (isInCurScope)
                            return {};
                        else
                            continue;
                    }

                    if (funcSym->type.Function()->HasSameSignatureWith(
                            *symbol.type.Function(),
                            true)) {
                        // Found an identical function symbol (same name and signature)
                        if (isInCurScope) {
                            // Redeclaration of member function in the same scope is not
                            // allowed
                            if (funcSym->IsMember())
                                return {};
                            else {
                                // if function does not have body difinition,
                                // replace its type
                                if (!funcSym->type.Function()->hasBody)
                                    funcSym->type = symbol.type;

                                return {funcSym, this};
                            }
                        }
                        else {
                            // If the found symbol is from base class, our new symbol will
                            // override it. Check whether the found symbol is virtual, if
                            // so, make inserted symbol virtual too. Otherwise insert new
                            // symbol

                            if (funcSym->Attr() == Symbol::VIRTUAL) {
                                // Add virtual attribute for overriding virtual function,
                                // if it doesn't have virtual attribute
                                if (symbol.Attr() != Symbol::VIRTUAL)
                                    symbol.SetAttr(Symbol::VIRTUAL);

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
                    return {};
            }
        }
    }

    if (symbol.Attr() != Symbol::CONSTANT) {
        // Set symbol offset to current scope size
        // Offset alignment: alignment amount depends on size of
        // symbol type and max alignment requirement is 8 bytes.
        int alignment = symbol.type.IsArray() ? symbol.type.ElementType().Alignment()
                                              : symbol.type.Alignment();
        if (alignment == 8)
            currentOffset = (currentOffset + 7) & ~7;
        else if (alignment == 4)
            currentOffset = (currentOffset + 3) & ~3;
        else if (alignment == 2)
            currentOffset = (currentOffset + 1) & ~1;

        // Only set index and offset for variable
        if (symbol.type.IsSimple(TypeKind::FUNCTION)) {
            symbol.index  = -1;
            symbol.offset = -1;
        }
        else {
            symbol.index  = currentIndex++;
            symbol.offset = currentOffset;
            currentOffset += symbol.type.Size();
        }
    }

    auto it = symbols.insert(std::make_pair(symbol.id, symbol));
    return {&it->second, this};
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

    // Set anonymous class/enum name to its first typedef name
    if (type.IsSimple(TypeKind::CLASS) && type.Class()->className[0] == '<')
        type.Class()->className = aliasName;
    else if (type.IsSimple(TypeKind::ENUM) && type.Enum()->enumName[0] == '<')
        type.Enum()->enumName = aliasName;
    return true;
}

SymbolSet SymbolTable::QuerySymbol(std::string id, bool qualified)
{
    for (SymbolTable *p = this; p; p = p->parent) {
        auto range = p->symbols.equal_range(id);
        if (range.first != symbols.end())
            return {range, p};

        if (p->classDesc) {
            for (ClassDescriptor *pc = p->classDesc->baseClassDesc; pc;
                 pc                  = pc->baseClassDesc) {
                range = pc->memberTable->symbols.equal_range(id);
                if (range.first != symbols.end())
                    return {range, pc->memberTable.get()};
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
    return parent;
}

SymbolTable *SymbolTable::GetRoot()
{
    SymbolTable *symtab = this;
    while (symtab->GetParent())
        symtab = symtab->GetParent();
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
    if (classDesc)
        return classDesc->FullName();

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

std::vector<Symbol *> SymbolTable::SortedSymbols()
{
    std::vector<Symbol *> sortedSymbols;

    for (auto it = symbols.begin(); it != symbols.end(); it++) {
        if (it->second.Attr() != Symbol::CONSTANT)
            sortedSymbols.push_back(&it->second);
    }

    std::sort(sortedSymbols.begin(), sortedSymbols.end(), [](auto a, auto b) {
        return a->index < b->index;
    });

    return std::move(sortedSymbols);
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
        os << "标识符            类型                                      "
              "属性/访问 偏移/值\n";

        auto sortedSymbols = const_cast<SymbolTable *>(this)->SortedSymbols();

        for (auto it = symbols.begin(); it != symbols.end(); it++) {
            if (it->second.Attr() == Symbol::CONSTANT)
                sortedSymbols.push_back(const_cast<Symbol *>(&it->second));
        }

        for (auto symbolPtr : sortedSymbols) {
            const Symbol &sym = *symbolPtr;

            os << setw(17) << sym.id << ' ' << setw(41) << sym.type.Name() << ' ';

            switch (sym.Attr()) {
            case Symbol::STATIC:
                os << "静态 ";
                break;
            case Symbol::VIRTUAL:
                os << "虚   ";
                break;
            case Symbol::PUREVIRTUAL:
                os << "纯虚 ";
                break;
            case Symbol::CONSTANT:
                os << "常量 ";
                break;
            default:
                os << "     ";
                break;
            }

            switch (sym.Access()) {
            case Symbol::PUBLIC:
                os << "PUB  ";
                break;
            case Symbol::PRIVATE:
                os << "PRI  ";
                break;
            case Symbol::PROTECTED:
                os << "PRO  ";
                break;
            default:
                if (sym.type.typeKind == TypeKind::FUNCTION
                    && sym.type.Function()->friendClass)
                    os << "FRI  ";
                else
                    os << "     ";
                break;
            }

            if (sym.Attr() == Symbol::CONSTANT)
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
                case Access::PRIVATE:
                    os << "(私有继承)";
                    break;
                case Access::PROTECTED:
                    os << "(保护继承)";
                    break;
                case Access::PUBLIC:
                    os << "(公有继承)";
                    break;
                default:
                    os << "(默认继承)";
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