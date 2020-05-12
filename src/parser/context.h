#pragma once

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

class ParseContext
{
public:
    enum IdType { ID, CLASS, ENUM, TYPEDEF };

    ParseContext();

    bool        PushQueryScope(std::string scopeName);
    void        EnterAnonymousScope();
    void        EnterLastAddedName();
    void        PopQueryScopes();
    void        LeaveScope();
    void        BeginTypedef();
    void        EndTypedef();
    void        AddPossibleTypedefName(std::string name);
    bool        AddName(std::string name, IdType type);
    IdType      QueryName(std::string name) const;
    std::string CurLocalScopeName() const;

private:
    struct Scope
    {
        Scope *                                                 parentScope;
        std::string                                             name;
        std::unordered_map<std::string, IdType>                 nameMap;
        std::unordered_map<std::string, std::shared_ptr<Scope>> scopeMap;
    };

    std::shared_ptr<Scope>             global;
    std::stack<std::shared_ptr<Scope>> scopeStack;
    Scope *                            localScope;
    bool                               isInTypedef;
    std::string                        lastAddedName;
};