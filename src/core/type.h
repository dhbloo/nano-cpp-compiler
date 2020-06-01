#pragma once

#include "typeEnum.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SymbolTable;
struct Symbol;
union Constant;

struct TypeDescriptor
{};

struct ClassDescriptor : TypeDescriptor
{
    std::string                  className;
    Access                       baseAccess;
    ClassDescriptor *            baseClassDesc;
    std::shared_ptr<SymbolTable> memberTable;

    std::string QualifiedName() const;
};

struct EnumDescriptor : TypeDescriptor
{
    std::string enumName;
    // FundType = int
};

struct Type
{
    struct PtrDescriptor
    {
        PtrType          ptrType;
        CVQualifier      cv;
        ClassDescriptor *classDesc;

        std::string Name() const;
        bool        operator==(const PtrDescriptor &rhs) const;
    };

    struct ArrayDescriptor
    {
        std::size_t                size;
        std::vector<PtrDescriptor> ptrDescList;

        bool operator==(const ArrayDescriptor &rhs) const;
    };

    TypeClass typeClass;

    // for FUNDTYPE, CLASS, ENUM, FUNCTION (return type)
    CVQualifier                  cv;
    std::vector<PtrDescriptor>   ptrDescList;
    std::vector<ArrayDescriptor> arrayDescList;

    FundType                        fundType;  // for FUNDTYPE
    std::shared_ptr<TypeDescriptor> typeDesc;  // for CLASS, ENUM, FUNCTION

    std::string Name(std::string innerName = "") const;
    int         TypeSize() const;  // in bytes
    bool        IsComplete() const;
    bool        IsConvertibleTo(const Type &target) const;
    Constant    ConvertConstant(Constant constant, const Type &target) const;
    bool        operator==(const Type &rhs) const;
    bool        operator!=(const Type &rhs) const;

    static const Type IntType, BoolType, CharType, FloatType, StringTypeProto;
};

struct FunctionDescriptor : TypeDescriptor
{
    struct Param
    {
        Symbol *symbol;
        bool    hasDefault;
    };

    Type             retType;
    bool             hasBody;
    ClassDescriptor *friendClass;

    std::vector<Param>           paramList;
    std::shared_ptr<SymbolTable> funcScope;
};