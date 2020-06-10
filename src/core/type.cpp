#include "type.h"

#include "semantic.h"
#include "symbol.h"

#include <cassert>

static const int FundSizeTable[] = {0, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8};
static const int PointerSize     = 8;

Type::Type(FundType fundType, CVQualifier cv)
    : typeClass(TypeClass::FUNDTYPE)
    , cv(cv)
    , fundType(fundType)
    , typeDesc(nullptr)
{}

Type::Type(std::shared_ptr<ClassDescriptor> classDesc, CVQualifier cv)
    : typeClass(TypeClass::CLASS)
    , cv(cv)
    , typeDesc(classDesc)
    , fundType(FundType::VOID)
{}

Type::Type(std::shared_ptr<EnumDescriptor> enumDesc, CVQualifier cv)
    : typeClass(TypeClass::ENUM)
    , cv(cv)
    , typeDesc(enumDesc)
    , fundType(FundType::VOID)
{}

Type::Type(std::shared_ptr<FunctionDescriptor> funcDesc, CVQualifier cv)
    : typeClass(TypeClass::FUNCTION)
    , cv(cv)
    , typeDesc(funcDesc)
    , fundType(FundType::VOID)
{}

ClassDescriptor *Type::Class() const
{
    if (typeClass == TypeClass::CLASS)
        return static_cast<ClassDescriptor *>(typeDesc.get());
    else
        return nullptr;
}

EnumDescriptor *Type::Enum() const
{
    if (typeClass == TypeClass::ENUM)
        return static_cast<EnumDescriptor *>(typeDesc.get());
    else
        return nullptr;
}

FunctionDescriptor *Type::Function() const
{
    if (typeClass == TypeClass::FUNCTION)
        return static_cast<FunctionDescriptor *>(typeDesc.get());
    else
        return nullptr;
}

bool Type::operator==(const Type &rhs) const
{
    if (typeClass != rhs.typeClass || cv != rhs.cv || ptrDescList != rhs.ptrDescList
        || arrayDescList != rhs.arrayDescList)
        return false;

    switch (typeClass) {
    case TypeClass::FUNDTYPE: return fundType == rhs.fundType;
    case TypeClass::FUNCTION: return *Function() == *rhs.Function();
    default: return typeDesc == rhs.typeDesc;
    }
}

bool Type::operator!=(const Type &rhs) const
{
    return !operator==(rhs);
}

bool Type::PtrDescriptor::operator==(const PtrDescriptor &rhs) const
{
    if (ptrType != rhs.ptrType || cv != rhs.cv)
        return false;

    if (ptrType == PtrType::CLASSPTR && classDesc != rhs.classDesc)
        return false;

    return true;
}

bool Type::ArrayDescriptor::operator==(const ArrayDescriptor &rhs) const
{
    if (size != rhs.size || ptrDescList.size() != rhs.ptrDescList.size())
        return false;

    for (std::size_t i = 0; i < ptrDescList.size(); i++) {
        if (!(ptrDescList[i] == rhs.ptrDescList[i]))
            return false;
    }
    return true;
}

std::string Type::Name(std::string innerName) const
{
    const char *FundTypeName[] = {"void",
                                  "bool",
                                  "char",
                                  "uchar",
                                  "short",
                                  "ushort",
                                  "int",
                                  "uint",
                                  "long",
                                  "ulong",
                                  "float",
                                  "double"};

    auto AppendPtr = [](const std::string &n, const PtrDescriptor &p) {
        std::string pName = p.Name();
        if (!n.empty() && pName.back() == 't')
            return n + pName + " ";
        else
            return n + pName;
    };

    std::string name, postfix;

    // type postfix
    for (const auto &p : ptrDescList) {
        postfix = AppendPtr(postfix, p);
    }

    // inner function type name
    if (!innerName.empty()) {
        if (!postfix.empty() && postfix.back() == 't')
            postfix += " " + innerName;
        else
            postfix += innerName;
    }

    // array type description
    for (auto a = arrayDescList.crbegin(); a != arrayDescList.crend(); a++) {
        std::string arrayPrefix, arraySize;

        for (const auto &p : a->ptrDescList) {
            arrayPrefix += p.Name();
            if (arrayPrefix.back() == 't')
                arrayPrefix += " ";
        }

        if (a->size > 0)
            arraySize = std::to_string(a->size);

        if (!postfix.empty() && postfix.front() != '[')
            postfix = arrayPrefix + "(" + postfix + ")[" + arraySize + "]";
        else
            postfix = arrayPrefix + postfix + "[" + arraySize + "]";
    }

    // primary type
    switch (typeClass) {
    case TypeClass::FUNDTYPE:
        name = std::string(FundTypeName[(int)fundType]);
        if (!postfix.empty())
            name += " " + postfix;
        break;
    case TypeClass::ENUM:
        name = Enum()->enumName;
        if (!postfix.empty())
            name += " " + postfix;
        break;
    case TypeClass::CLASS:
        name = Class()->className;
        if (!postfix.empty())
            name += " " + postfix;
        break;
    default:
        if (!postfix.empty())
            name = "(" + postfix + ")";

        name += "(";
        std::size_t startIdx = Function()->IsNonStaticMember() ? 1 : 0;
        for (std::size_t i = startIdx; i < Function()->paramList.size(); i++) {
            if (i > startIdx)
                name += ", ";
            name += Function()->paramList[i].symbol->type.Name();
        }
        name += ")";

        if (cv == CVQualifier::CONST)
            name += " const";

        name = Function()->retType.Name(name);
        break;
    }

    if (cv == CVQualifier::CONST && typeClass != TypeClass::FUNCTION)
        name = "const " + name;

    return name;
}

std::string Type::PtrDescriptor::Name() const
{
    std::string name;
    switch (ptrType) {
    case PtrType::PTR: name = "*"; break;
    case PtrType::REF: name = "&"; break;
    case PtrType::CLASSPTR:
        name = " " + classDesc->memberTable->ScopeName() + "::*";
        break;
    }

    if (cv == CVQualifier::CONST)
        name += "const";

    return name;
}

int Type::Size() const
{
    if (!ptrDescList.empty())
        return PointerSize;

    int size;
    switch (typeClass) {
    case TypeClass::FUNDTYPE: size = FundSizeTable[(int)fundType]; break;
    case TypeClass::ENUM: size = FundSizeTable[(int)FundType::INT]; break;
    case TypeClass::CLASS: size = Class()->memberTable->ScopeSize(); break;
    default: size = 0; break;
    }

    for (const auto &a : arrayDescList) {
        if (!a.ptrDescList.empty())
            size = PointerSize;
        size *= a.size;
    }

    return size;
}

int Type::AlignmentSize() const
{
    return ElementType().Size();
}

bool Type::IsConvertibleTo(const Type &target, Constant *constant) const
{
    if (!target.IsComplete())
        return false;

    Type t = *this;

    // 0. identical type
    if (t == target)
        return true;

    // 1. l-value to r-value
    if (t.IsRef() && !target.IsRef() && !t.RemoveRef().IsSimple(TypeClass::FUNCTION)
        && !t.RemoveRef().IsArray()) {
        t = t.RemoveRef();
        // remove cv for non class type
        if (!t.IsSimple(TypeClass::CLASS))
            t.cv = CVQualifier::NONE;
    }
    // 2. array (reference) to pointer
    else if (t.RemoveRef().IsArray() && target.IsPtr()) {
        t = t.RemoveRef().ElementType().AddPtrDesc(PtrDescriptor {PtrType::PTR});
    }
    // 3. function (reference) to pointer / member pointer
    else if (t.RemoveRef().IsSimple(TypeClass::FUNCTION) && target.IsPtr()
             && target.RemovePtr().IsSimple(TypeClass::FUNCTION)) {
        if (!t.Function()->IsNonStaticMember())
            t = t.RemoveRef().AddPtrDesc(PtrDescriptor {PtrType::PTR});
    }
    // r-value to const l-value (creates temporary)
    else if (!t.IsRef() && target.IsRef() && target.cv == CVQualifier::CONST
             && !t.IsSimple(TypeClass::FUNCTION)) {
        t.AddPtrDesc(PtrDescriptor {PtrType::REF});
        t.cv = CVQualifier::CONST;
    }
    // function to function reference
    else if (t.IsSimple(TypeClass::FUNCTION) && target.IsRef()
             && target.RemoveRef().IsSimple(TypeClass::FUNCTION)) {
        t.AddPtrDesc(PtrDescriptor {PtrType::REF});
    }

    // 4~8, 10. numeric conversion & bool conversion
    if (t.IsSimple(TypeClass::FUNDTYPE) && target.IsSimple(TypeClass::FUNDTYPE)) {
        if (t.fundType == FundType::VOID)
            return false;
        else if (constant) {
            bool          isDecimal = false;
            std::intmax_t integral;
            double        decimal;

            switch (fundType) {
            case FundType::BOOL: integral = constant->boolVal; break;
            case FundType::CHAR:
            case FundType::UCHAR: integral = constant->charVal; break;
            case FundType::FLOAT:
            case FundType::DOUBLE:
                decimal   = constant->floatVal;
                isDecimal = true;
                break;
            default: integral = constant->intVal; break;
            }

            switch (target.fundType) {
            case FundType::BOOL:
                constant->boolVal = (bool)(isDecimal ? decimal : integral);
                break;
            case FundType::CHAR:
            case FundType::UCHAR:
                constant->charVal = (char)(isDecimal ? decimal : integral);
                break;
            case FundType::FLOAT:
            case FundType::DOUBLE:
                constant->floatVal = (double)(isDecimal ? decimal : integral);
                break;
            default:
                constant->intVal = (std::intmax_t)(isDecimal ? decimal : integral);
                break;
            }
        }
        return true;
    }
    // 4, 10. integer promotion: enum to int & bool conversion: enum to bool
    else if (t.IsSimple(TypeClass::ENUM) && target.IsSimple(TypeClass::FUNDTYPE)) {
        if (constant) {
            switch (target.fundType) {
            case FundType::BOOL: constant->boolVal = (bool)(constant->intVal); break;
            case FundType::CHAR:
            case FundType::UCHAR: constant->charVal = (char)(constant->intVal); break;
            case FundType::FLOAT:
            case FundType::DOUBLE: constant->floatVal = (double)(constant->intVal); break;
            default: break;
            }
        }
        return true;
    }
    // 9. pointer conversion
    else if (target.IsPtr()) {
        // literal '0' to pointer
        if (t.IsSimple(TypeClass::FUNDTYPE) && t.fundType == FundType::INT) {
            return constant && constant->intVal == 0;
        }
        // object pointer to void pointer
        else if (t.IsPtr() && t.RemovePtr().IsSimple(TypeClass::FUNDTYPE)
                 && target.fundType == FundType::VOID) {
            return true;
        }
        // pointer to derived class to pointer to base class
        else if (t.IsPtr() && t.RemovePtr().IsSimple(TypeClass::CLASS)
                 && target.RemovePtr().IsSimple(TypeClass::CLASS)) {
            return target.Class()->IsBaseOf(*t.Class());
        }
        // TODO: member pointer conversion
    }
    // 10. bool conversion: pointer to bool
    else if (t.IsPtr() && target.IsSimple(TypeClass::FUNDTYPE)
             && target.fundType == FundType::BOOL) {
        if (constant)
            constant->boolVal = (bool)(constant->intVal);
        return true;
    }

    // 11. qualification adjustment
    if (target.cv == CVQualifier::CONST && !t.IsSimple(TypeClass::FUNCTION)
        && !target.IsSimple(TypeClass::FUNCTION)) {
        t.cv = CVQualifier::CONST;
    }

    auto convertPtrDesc = [](std::vector<Type::PtrDescriptor> &      pdl,
                             const std::vector<Type::PtrDescriptor> &pdlTarget) {
        if (pdl.size() == pdlTarget.size()) {
            for (size_t i = 0; i < pdl.size(); i++)
                if (pdlTarget[i].cv == CVQualifier::CONST)
                    pdl[i].cv = CVQualifier::CONST;
        }
    };
    convertPtrDesc(t.ptrDescList, target.ptrDescList);

    if (t.arrayDescList.size() == target.arrayDescList.size()) {
        for (size_t i = 0; i < t.arrayDescList.size(); i++) {
            convertPtrDesc(t.arrayDescList[i].ptrDescList,
                           target.arrayDescList[i].ptrDescList);
        }
    }

    return t == target;

    // // Implicit conversion must be the same typeclass
    // // TODO: conversion operator or constructor?
    // if (typeClass != target.typeClass) {
    //     // ENUM type to INT
    //     if (typeClass == TypeClass::ENUM && target.typeClass == TypeClass::FUNDTYPE)
    //         return Type {FundType::INT}.IsConvertibleTo(target);
    //     else
    //         return false;
    // }

    // if (arrayDescList != target.arrayDescList) {
    //     // Const array/pointer convert to non-const array/pointer
    //     if (cv == CVQualifier::CONST && target.cv == CVQualifier::NONE)
    //         return false;

    //     // T [x] -> T *
    //     if (!arrayDescList.empty()) {
    //         if (!target.ptrDescList.empty()
    //             && target.ptrDescList.back().ptrType != PtrType::REF) {
    //             Type removeArrayT   = *this;
    //             Type removePointerT = target;
    //             removeArrayT.arrayDescList.pop_back();
    //             removePointerT.ptrDescList.pop_back();
    //             if (removeArrayT == removePointerT)
    //                 return true;
    //         }
    //         // T [x] -> T []
    //         else if (!target.arrayDescList.empty()
    //                  && target.arrayDescList.back().size == 0) {
    //             Type removeArrayT = *this;
    //             Type removeArrayU = target;
    //             removeArrayT.arrayDescList.pop_back();
    //             removeArrayU.arrayDescList.pop_back();
    //             if (removeArrayT == removeArrayU)
    //                 return true;
    //         }
    //     }

    //     // T * -> T []
    //     if (!ptrDescList.empty() && ptrDescList.back().ptrType != PtrType::REF
    //         && !target.arrayDescList.empty() && target.arrayDescList.back().size == 0)
    //         { Type removeArrayT   = target; Type removePointerT = *this;
    //         removeArrayT.arrayDescList.pop_back();
    //         removePointerT.ptrDescList.pop_back();
    //         if (removeArrayT == removePointerT)
    //             return true;
    //     }

    //     return false;
    // }

    // if (ptrDescList != target.ptrDescList) {
    //     // -> T
    //     if (target.ptrDescList.empty()) {
    //         // T& -> T, const T& -> T: implicit copy
    //         if (ptrDescList.size() == 1 && ptrDescList[0].ptrType == PtrType::REF)
    //             return true;
    //     }
    //     // -> const T&
    //     else if (target.ptrDescList.size() == 1
    //              && target.ptrDescList[0].ptrType == PtrType::REF
    //              && target.cv == CVQualifier::CONST) {
    //         // T -> const T&, const T -> const T&
    //         if (ptrDescList.empty())
    //             return true;
    //     }

    //     return false;
    // }
    // else if (!ptrDescList.empty()) {
    //     // Const ref/pointer convert to non-const ref/pointer
    //     if (cv == CVQualifier::CONST && target.cv == CVQualifier::NONE)
    //         return false;
    // }

    // if (typeClass == TypeClass::FUNDTYPE) {
    //     // implicit fundmental type conversion rules
    //     switch (target.fundType) {
    //     case FundType::BOOL: return true;
    //     case FundType::CHAR:
    //     case FundType::UCHAR: return fundType <= FundType::UCHAR;
    //     case FundType::SHORT:
    //     case FundType::USHORT: return fundType <= FundType::USHORT;
    //     case FundType::INT:
    //     case FundType::UINT: return fundType <= FundType::UINT;
    //     case FundType::LONG:
    //     case FundType::ULONG: return fundType <= FundType::ULONG;
    //     case FundType::FLOAT: return fundType <= FundType::FLOAT;
    //     case FundType::DOUBLE: return true;
    //     default: return false;
    //     }
    // }
    // else if (typeClass == TypeClass::FUNCTION) {
    //     // assert(target.typeClass == TypeClass::FUNCTION)
    //     return *Function() == *target.Function();
    // }
    // else {
    //     return typeDesc == target.typeDesc;
    // }
}

bool Type::IsComplete() const
{
    // Incomplete types:
    // 1. void
    // 2. class without definition
    // 3. array with unknown size

    // Reference or pointer to some type
    if (IsPtr() || IsRef())
        return true;

    if (IsArray()) {
        auto &a = arrayDescList.back();

        // Array of unknown size is incomplete type
        if (a.size == 0)
            return false;

        // Array to some type
        return ElementType().IsComplete();
    }

    switch (typeClass) {
    case TypeClass::FUNDTYPE:
        // void is incomplete type
        return fundType != FundType::VOID;
    case TypeClass::CLASS:
        // class without definition is incomplete type
        return Class()->memberTable != nullptr;
    default: return true;
    }
}

bool Type::IsSimple(TypeClass tc) const
{
    return typeClass == tc && ptrDescList.empty() && arrayDescList.empty();
}

bool Type::IsRef() const
{
    return !ptrDescList.empty() && ptrDescList.back().ptrType == PtrType::REF;
}

bool Type::IsPtr() const
{
    return !ptrDescList.empty() && ptrDescList.back().ptrType == PtrType::PTR;
}

bool Type::IsMemberPtr() const
{
    return !ptrDescList.empty() && ptrDescList.back().ptrType == PtrType::CLASSPTR;
}

bool Type::IsArray() const
{
    return ptrDescList.empty() && !arrayDescList.empty();
}

std::size_t Type::ArraySize() const
{
    return IsArray() ? arrayDescList.back().size : 0;
}

Type &Type::AddPtrDesc(Type::PtrDescriptor ptrDesc)
{
    ptrDescList.push_back(std::move(ptrDesc));
    return *this;
}

Type Type::ElementType() const
{
    Type t = *this;

    if (!t.ptrDescList.empty()) {
        if (t.ptrDescList.back().ptrType == PtrType::PTR)
            t.ptrDescList.pop_back();
    }
    else if (!t.arrayDescList.empty()) {
        t.ptrDescList = t.arrayDescList.back().ptrDescList;
        t.arrayDescList.pop_back();
    }

    return t;
}

Type Type::Decay() const
{
    Type t = *this;

    // 1. l-value to r-value
    if (t.IsRef() && !t.RemoveRef().IsSimple(TypeClass::FUNCTION)
        && !t.RemoveRef().IsArray()) {
        t = t.RemoveRef();
        // remove cv for non class type
        if (!t.IsSimple(TypeClass::CLASS))
            t.cv = CVQualifier::NONE;
    }
    // 2. array (reference) to pointer
    else if (t.RemoveRef().IsArray()) {
        t = t.RemoveRef().ElementType().AddPtrDesc(PtrDescriptor {PtrType::PTR});
    }
    // 3. function (reference) to pointer
    else if (t.RemoveRef().IsSimple(TypeClass::FUNCTION)) {
        if (!t.Function()->IsNonStaticMember())
            t = t.RemoveRef().AddPtrDesc(PtrDescriptor {PtrType::PTR});
    }

    return t;
}

Type Type::ArithmeticConvert(const Type &t2) const
{
    if (IsSimple(TypeClass::FUNDTYPE) && t2.IsSimple(TypeClass::FUNDTYPE)) {
        if (fundType == FundType::DOUBLE || t2.fundType == FundType::DOUBLE)
            return {FundType::DOUBLE};
        else if (fundType == FundType::FLOAT || t2.fundType == FundType::FLOAT)
            return {FundType::FLOAT};
        else if (fundType == FundType::ULONG || t2.fundType == FundType::ULONG)
            return {FundType::ULONG};
        else if (fundType == FundType::LONG || t2.fundType == FundType::LONG)
            return {FundType::LONG};
        else if (fundType == FundType::UINT || t2.fundType == FundType::UINT)
            return {FundType::UINT};
    }

    return {FundType::INT};
}

Type Type::RemoveCV() const
{
    Type t = *this;
    t.cv   = CVQualifier::NONE;
    return t;
}

Type Type::RemovePtr() const
{
    Type t = *this;
    if (t.IsPtr())
        t.ptrDescList.pop_back();
    return t;
}

Type Type::RemoveRef() const
{
    Type t = *this;
    if (t.IsRef())
        t.ptrDescList.pop_back();
    return t;
}

bool ClassDescriptor::IsBaseOf(const ClassDescriptor &classDesc) const
{
    for (auto p = classDesc.baseClassDesc; p; p = p->baseClassDesc) {
        if (p == this)
            return true;
    }
    return false;
}

FunctionDescriptor::FunctionDescriptor(Type retType)
    : retType(retType)
    , hasBody(false)
    , friendClass(nullptr)
{}

bool FunctionDescriptor::IsMember() const
{
    assert(defSymbol);
    return defSymbol->IsMember();
}

bool FunctionDescriptor::IsNonStaticMember() const
{
    return IsMember() && defSymbol->Attr() != Symbol::STATIC;
}

CVQualifier FunctionDescriptor::MemberCV() const
{
    assert(defSymbol);
    return defSymbol->type.cv;
}

bool FunctionDescriptor::HasSameSignatureWith(const FunctionDescriptor &func)
{
    if (paramList.size() != func.paramList.size())
        return false;

    for (size_t i = 0; i < paramList.size(); i++) {
        if (paramList[i].symbol->type != func.paramList[i].symbol->type)
            return false;
    }

    return true;
}

bool FunctionDescriptor::operator==(const FunctionDescriptor &func)
{
    return HasSameSignatureWith(func) && retType == func.retType;
}