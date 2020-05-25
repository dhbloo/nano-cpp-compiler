#pragma once

#include "typeEnum.h"

#include <string>
#include <unordered_map>
#include <vector>

struct TypeDescriptor
{
    std::string typeName;  // class or enum name
};

struct Type
{
    struct PtrDescriptor
    {
        PtrType          ptrType;
        CVQualifier      cv;
        ClassDescriptor *classDesc;
    };

    TypeClass typeClass;

    // for FUNDTYPE, CLASS, ENUM, FUNCTION (return type)
    CVQualifier                cv;
    std::vector<PtrDescriptor> ptrDescList;

    FundType          fundType;   // for FUNDTYPE, FUNCTION (return type)
    TypeDescriptor *  typeDesc;   // for CLASS, ENUM
    std::vector<Type> paramList;  // for FUNCTION
};

struct ClassDescriptor : TypeDescriptor
{
    struct MemberDescriptor
    {
        Type type;
        bool isStatic, isFuncConst, isFuncVirtual, isFuncPure;
    };

    Access                                            baseAccess;
    ClassDescriptor *                                 baseClassDesc;
    std::unordered_map<std::string, MemberDescriptor> memberDescs;
};

struct EnumDescriptor : TypeDescriptor
{
    // FundType = int
};