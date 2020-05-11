#include "context.h"

ParseContext::ParseContext()
{
    nameCountStack.push(0);
    isInTypedef = false;
}

void ParseContext::EnterScope()
{
    nameCountStack.push(0);
}

void ParseContext::LeaveScope()
{
    for (std::size_t i = 0; i < nameCountStack.top(); i++) {
        nameMap.erase(nameStack.top());
        nameStack.pop();
    }
    nameCountStack.pop();
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
    auto it = nameMap.find(name);
    if (it != nameMap.end()) {
        return it->second == type;
    }

    nameCountStack.top()++;
    nameStack.push(name);
    nameMap[name] = type;

    return true;
}

ParseContext::IdType ParseContext::QueryName(std::string name) const
{
    auto it = nameMap.find(name);
    if (it != nameMap.end()) {
        return it->second;
    }

    return ID;
}