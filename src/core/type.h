#pragma once

#include "typeEnum.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SymbolTable;

struct TypeDescriptor
{};

struct Type
{
    struct PtrDescriptor
    {
        PtrType          ptrType;
        CVQualifier      cv;
        ClassDescriptor *classDesc;
    };

    struct ArrayDescriptor
    {
        int                        size;
        std::vector<PtrDescriptor> ptrDescList;
    };

    TypeClass typeClass;

    // for FUNDTYPE, CLASS, ENUM, FUNCTION (return type)
    CVQualifier                  cv;
    std::vector<PtrDescriptor>   ptrDescList;
    std::vector<ArrayDescriptor> arrayDescList;

    FundType                        fundType;  // for FUNDTYPE, FUNCTION (return type)
    std::shared_ptr<TypeDescriptor> typeDesc;  // for CLASS, ENUM, FUNCTION

    std::string Name() const;
    int         TypeSize() const;  // in bytes
    bool        IsConvertibleTo(const Type &target);
    bool        operator==(const Type &rhs);
};

struct ClassDescriptor : TypeDescriptor
{
    std::string                  className;
    Access                       baseAccess;
    ClassDescriptor *            baseClassDesc;
    std::shared_ptr<SymbolTable> memberTable;
};

struct EnumDescriptor : TypeDescriptor
{
    std::string enumName;
    // FundType = int
};

struct FunctionDescriptor : TypeDescriptor
{
    struct Parameter
    {
        Type type;
    };

    std::vector<Parameter> paramList;
};