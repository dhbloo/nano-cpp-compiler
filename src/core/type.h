#pragma once

#include "typeEnum.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

union Constant;
struct SymbolTable;
struct Symbol;
struct ClassDescriptor;
struct EnumDescriptor;
struct FunctionDescriptor;
struct TypeDescriptor
{};

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
        size_t                     size;
        std::vector<PtrDescriptor> ptrDescList;

        bool operator==(const ArrayDescriptor &rhs) const;
    };

    // Type catagory
    TypeKind typeKind;

    // for FUNDTYPE, CLASS, ENUM, FUNCTION
    CVQualifier                  cv;
    std::vector<PtrDescriptor>   ptrDescList;
    std::vector<ArrayDescriptor> arrayDescList;

    FundType                        fundType;  // for FUNDTYPE
    std::shared_ptr<TypeDescriptor> typeDesc;  // for CLASS, ENUM, FUNCTION

    // Constructor for FUNDTYPE type
    Type(FundType fundType = FundType::VOID, CVQualifier cv = CVQualifier::NONE);
    // Constructor for CLASS type
    Type(std::shared_ptr<ClassDescriptor> classDesc, CVQualifier cv = CVQualifier::NONE);
    // Constructor for ENUM type
    Type(std::shared_ptr<EnumDescriptor> enumDesc, CVQualifier cv = CVQualifier::NONE);
    // Constructor for FUNCTION type
    Type(std::shared_ptr<FunctionDescriptor> funcDesc,
         CVQualifier                         cv = CVQualifier::NONE);

    ClassDescriptor *   Class() const;
    EnumDescriptor *    Enum() const;
    FunctionDescriptor *Function() const;

    bool operator==(const Type &rhs) const;
    bool operator!=(const Type &rhs) const;

    /* Type general infomation */

    // Returns type readable name
    std::string Name(std::string innerName = "") const;
    // Returns sizeof(type) in bytes
    int Size() const;
    // Returns alignment requirements in bytes (1, 2, 4, 8)
    int Alignment() const;
    // Returns if the type can be implicitly converted to target type
    // If true and there is a constant conversion, convert the given
    // constant (if not null)
    bool IsConvertibleTo(const Type &target, Constant *constant = nullptr) const;

    /* Type querys */

    // Returns if the type is complete
    bool IsComplete() const;
    // Returns if the type is not reference, pointer or array, and it
    // belongs to type class
    bool IsSimple(TypeKind typeKind) const;
    // Returns if the type is a reference (T&)
    bool IsRef() const;
    // Returns if the type is a pointer or member pointer (T*, T C::*)
    bool IsPtr() const;
    // Returns if the type is a class member pointer
    bool IsMemberPtr() const;
    // Returns if the type is an array (T[])
    bool IsArray() const;
    // Returns array size if the type is an array, otherwise returns 0
    size_t ArraySize() const;
    // Returns if the type is a constant and must be explicit initialized.
    // Eg. ref, const ptr, const element array, const variable
    bool IsConstInit() const;

    /* Type modifiers */

    Type &AddPtrDesc(PtrDescriptor ptrDesc);

    // If the type is T[] or T*, return type T, otherwise retern the original type
    Type ElementType() const;
    // Applies lvalue-to-rvalue (removes cv-qualifiers for non-class type),
    // array-to-pointer, and function-to-pointer implicit conversions to the type
    Type Decay() const;
    // Convert two arithmetic type using 'usual arithmetic conversions' rules
    Type ArithmeticConvert(Type t2) const;

    Type RemoveCV() const;
    Type RemoveRef() const;
    Type RemovePtr() const;
    Type RemoveMemberPtr() const;
};

struct ClassDescriptor : TypeDescriptor
{
    std::string                    className;
    Access                         baseAccess;
    ClassDescriptor *              baseClassDesc;
    std::vector<ClassDescriptor *> friendClassTo;
    std::shared_ptr<SymbolTable>   memberTable;

    std::string FullName() const;
    bool        IsBaseOf(const ClassDescriptor &classDesc) const;
};

struct EnumDescriptor : TypeDescriptor
{
    std::string enumName;
    // FundType = int
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
    Symbol *         defSymbol;
    ClassDescriptor *friendClass;

    std::vector<Param>           paramList;
    std::shared_ptr<SymbolTable> funcScope;

    FunctionDescriptor(Type retType);
    // Returns true if the function is defined as a class member
    bool IsMember() const;
    // Returns true if the function is defined as a non-static class member
    bool IsNonStaticMember() const;
    // Returns the cv qualifier of a member function, if the function is not
    // a member, always returns CVQualifier::NONE
    CVQualifier MemberCV() const;
    // Returns true if both parameter lists are the same
    bool HasSameSignatureWith(const FunctionDescriptor &func, bool ignoreFirst = false);
    // Returns true if both signatures and return types are the same
    bool operator==(const FunctionDescriptor &func);
};