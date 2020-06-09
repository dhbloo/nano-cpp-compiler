#include "context.h"

ParseContext::ParseContext()
    : global(new Scope {nullptr, ""})
    , localScope(nullptr)
    , isInTypedef(false)
{
    scopeStack.push(global);
}

bool ParseContext::PushQueryScope(std::string scopeName)
{
    Scope *scope = localScope ? localScope : scopeStack.top().get();
    for (; scope; scope = scope->parentScope) {
        auto it = scope->scopeMap.find(scopeName);
        if (it != scope->scopeMap.end()) {
            localScope = it->second.get();
            return true;
        }
    }
    return false;
}

void ParseContext::EnterLocalScope()
{
    scopeStack.push(std::shared_ptr<Scope> {new Scope {scopeStack.top().get(), ""}});
}

void ParseContext::EnterLastAddedName()
{
    std::shared_ptr<Scope> newScope {new Scope {scopeStack.top().get(), lastAddedName}};
    if (!lastAddedName.empty())
        scopeStack.top()->scopeMap[lastAddedName] = newScope;
    scopeStack.push(newScope);
}

void ParseContext::PopQueryScopes()
{
    localScope = nullptr;
}

void ParseContext::LeaveScope()
{
    scopeStack.pop();
}

void ParseContext::BeginTypedef()
{
    isInTypedef = true;
}

void ParseContext::EndTypedef()
{
    isInTypedef = false;
}

void ParseContext::AddPossibleTypedefName(std::string name)
{
    if (isInTypedef)
        AddName(std::move(name), TYPEDEF);
}

bool ParseContext::AddName(std::string name, IdType type)
{
    if (!name.empty()) {
        for (Scope *scope = scopeStack.top().get(); scope; scope = scope->parentScope) {
            auto it = scope->nameMap.find(name);
            if (it != scope->nameMap.end()) {
                if (it->second == type) {
                    lastAddedName = name;
                    return true;
                }
                else
                    return false;
            }
        }

        scopeStack.top()->nameMap[name] = type;
    }

    lastAddedName = name;
    return true;
}

ParseContext::IdType ParseContext::QueryName(std::string name) const
{
    if (localScope) {
        auto it = localScope->nameMap.find(name);
        if (it != localScope->nameMap.end())
            return it->second;
        return ID;
    }

    for (Scope *scope = scopeStack.top().get(); scope; scope = scope->parentScope) {
        auto it = scope->nameMap.find(name);
        if (it != scope->nameMap.end())
            return it->second;
    }

    return ID;
}

std::string ParseContext::CurLocalScopeName() const
{
    for (Scope *scope = scopeStack.top().get(); scope; scope = scope->parentScope) {
        if (!scope->name.empty())
            return scope->name;
    }
    return scopeStack.size() > 1 ? "(local)" : "(global)";
}