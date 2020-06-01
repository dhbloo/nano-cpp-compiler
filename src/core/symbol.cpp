#include "symbol.h"

#include <cassert>
#include <iomanip>
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
    assert(operator bool());
    return single ? single : &begin->second;
}

std::size_t SymbolSet::Count() const
{
    if (single)
        return 1;
    else {
        std::size_t cnt = 0;
        for (It cur = begin; cur != end; cur++)
            cnt++;
        return cnt;
    }
}

SymbolSet::operator bool() const
{
    return single ? true : begin != end;
}

SymbolTable::SymbolTable(SymbolTable *       parent,
                         ClassDescriptor *   classDesc,
                         FunctionDescriptor *funcDesc)
    : parent(parent)
    , classDesc(classDesc)
    , funcDesc(funcDesc)
    , curSize(0)
{}

void SymbolTable::ClearAll()
{
    symbols.clear();
    classTypes.clear();
    enumTypes.clear();
}

Symbol *SymbolTable::AddSymbol(Symbol symbol)
{
    if (!symbol.id.empty()) {
        SymbolSet set = QuerySymbol(symbol.id, true);

        if (set) {
            // TODO: function match
            /*if (set.Count() == 1) {
                Type &type = set.Get()->type;
                if (type.typeClass == TypeClass::FUNCTION
                    && !static_cast<FunctionDescriptor *>(type.typeDesc.get())->hasBody) {
                    return set.Get();
                }
            }*/
            return nullptr;
        }
    }

    if (symbol.type.typeClass == TypeClass::FUNCTION)
        assert(symbol.type.typeDesc);

    if (symbol.attr != Symbol::CONSTANT) {
        // set symbol offset to current scope size
        // TODO: check alignment
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
    else if (!qualified) {
        for (SymbolTable *p = parent; p; p = p->parent) {
            it = p->symbols.equal_range(id);
            if (it.first != symbols.end())
                return SymbolSet {it};
        }
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

ClassDescriptor *SymbolTable::GetClass()
{
    for (SymbolTable *p = this; p; p = p->parent) {
        if (p->classDesc)
            return p->classDesc;
    }
    return nullptr;
}

FunctionDescriptor *SymbolTable::GetFunction()
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
        return classDesc->className;

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
    using std::setw;

    os << std::left;
    os << std::string(80, '=') << '\n';
    os << "符号表 " << ScopeName() << '\n';
    os << std::string(80, '-') << '\n';

    if (!symbols.empty()) {
        os << "符号, 共 " << symbols.size() << " 个\n";
        os << "标识符          类型                                    属性/访问 偏移/值\n";
        for (auto it = symbols.cbegin(); it != symbols.cend(); it++) {
            const Symbol &sym = it->second;

            os << setw(15) << sym.id << ' ' << setw(39) << sym.type.Name() << ' ';

            if (sym.attr != Symbol::NORMAL) {
                switch (sym.attr & ~Symbol::ACCESSMASK) {
                case Symbol::STATIC: os << "静态  "; break;
                case Symbol::VIRTUAL: os << "虚    "; break;
                case Symbol::PUREVIRTUAL: os << "纯虚  "; break;
                case Symbol::CONSTANT: os << "常量  "; break;
                default: os << "      "; break;
                }

                switch (sym.attr & Symbol::ACCESSMASK) {
                case Symbol::PUBLIC: os << "PUB "; break;
                case Symbol::PRIVATE: os << "PRI "; break;
                case Symbol::PROTECTED: os << "PRO "; break;
                case Symbol::FRIEND: os << "FRI "; break;
                default: os << "    "; break;
                }
            }
            else
                os << "(无)      ";

            if (sym.attr == Symbol::CONSTANT) {
                os << " ";
            }
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

                switch (it->second->baseClassDesc->baseAccess) {
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