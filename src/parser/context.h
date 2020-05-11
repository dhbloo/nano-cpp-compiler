#pragma once

#include <stack>
#include <string>
#include <unordered_map>

class ParseContext
{
public:
    enum IdType { ID, CLASS, ENUM, TYPEDEF };

    ParseContext();

    void   EnterScope();
    void   LeaveScope();
    void   BeginTypedef();
    void   EndTypedef();
    void   AddPossibleTypedefName(std::string name);
    bool   AddName(std::string name, IdType type);
    IdType QueryName(std::string name) const;

private:
    std::unordered_map<std::string, IdType> nameMap;
    std::stack<std::string>                 nameStack;
    std::stack<std::size_t>                 nameCountStack;
    bool isInTypedef;
};