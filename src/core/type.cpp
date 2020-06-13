#include "type.h"

#include "constant.h"
#include "symbol.h"

#include <cassert>

static const int FundSizeTable[] = {0, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8};
static const int PointerSize     = 8;

Type::Type(FundType fundType, CVQualifier cv)
    : typeKind(TypeKind::FUNDTYPE)
    , cv(cv)
    , fundType(fundType)
    , typeDesc(nullptr)
{}

Type::Type(std::shared_ptr<ClassDescriptor> classDesc, CVQualifier cv)
    : typeKind(TypeKind::CLASS)
    , cv(cv)
    , typeDesc(classDesc)
    , fundType(FundType::VOID)
{}

Type::Type(std::shared_ptr<EnumDescriptor> enumDesc, CVQualifier cv)
    : typeKind(TypeKind::ENUM)
    , cv(cv)
    , typeDesc(enumDesc)
    , fundType(FundType::VOID)
{}

Type::Type(std::shared_ptr<FunctionDescriptor> funcDesc, CVQualifier cv)
    : typeKind(TypeKind::FUNCTION)
    , cv(cv)
    , typeDesc(funcDesc)
    , fundType(FundType::VOID)
{}

ClassDescriptor *Type::Class() const
{
    if (typeKind == TypeKind::CLASS)
        return static_cast<ClassDescriptor *>(typeDesc.get());
    else
        return nullptr;
}

EnumDescriptor *Type::Enum() const
{
    if (typeKind == TypeKind::ENUM)
        return static_cast<EnumDescriptor *>(typeDesc.get());
    else
        return nullptr;
}

FunctionDescriptor *Type::Function() const
{
    if (typeKind == TypeKind::FUNCTION)
        return static_cast<FunctionDescriptor *>(typeDesc.get());
    else
        return nullptr;
}

bool Type::operator==(const Type &rhs) const
{
    if (typeKind != rhs.typeKind || cv != rhs.cv || ptrDescList != rhs.ptrDescList
        || arrayDescList != rhs.arrayDescList)
        return false;

    switch (typeKind) {
    case TypeKind::FUNDTYPE:
        return fundType == rhs.fundType;
    case TypeKind::FUNCTION:
        return *Function() == *rhs.Function();
    default:
        return typeDesc == rhs.typeDesc;
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

    for (size_t i = 0; i < ptrDescList.size(); i++) {
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
    switch (typeKind) {
    case TypeKind::FUNDTYPE:
        name = std::string(FundTypeName[(int)fundType]);
        if (!postfix.empty())
            name += " " + postfix;
        break;
    case TypeKind::ENUM:
        name = Enum()->enumName;
        if (!postfix.empty())
            name += " " + postfix;
        break;
    case TypeKind::CLASS:
        name = Class()->className;
        if (!postfix.empty())
            name += " " + postfix;
        break;
    default:
        if (!postfix.empty())
            name = "(" + postfix + ")";

        name += "(";
        size_t startIdx = Function()->IsNonStaticMember() ? 1 : 0;
        for (size_t i = startIdx; i < Function()->paramList.size(); i++) {
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

    if (cv == CVQualifier::CONST && typeKind != TypeKind::FUNCTION)
        name = "const " + name;

    return name;
}

std::string Type::PtrDescriptor::Name() const
{
    std::string name;
    switch (ptrType) {
    case PtrType::PTR:
        name = "*";
        break;
    case PtrType::REF:
        name = "&";
        break;
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
    switch (typeKind) {
    case TypeKind::FUNDTYPE:
        size = FundSizeTable[(int)fundType];
        break;
    case TypeKind::ENUM:
        size = FundSizeTable[(int)FundType::INT];
        break;
    case TypeKind::CLASS:
        size = Class()->memberTable->ScopeSize();
        break;
    default:
        size = 0;
        break;
    }

    for (const auto &a : arrayDescList) {
        if (!a.ptrDescList.empty())
            size = PointerSize;
        size *= a.size;
    }

    return size;
}

int Type::Alignment() const
{
    int size = Size();
    if (size >= 8)
        return 8;
    else if (size >= 4)
        return 4;
    else if (size >= 2)
        return 2;
    else
        return 1;
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
    if (t.IsRef() && !target.IsRef() && !t.RemoveRef().IsSimple(TypeKind::FUNCTION)
        && !t.RemoveRef().IsArray()) {
        t = t.RemoveRef();
        // remove cv for non class type
        if (!t.IsSimple(TypeKind::CLASS))
            t.cv = CVQualifier::NONE;
    }
    // 2. array (reference) to pointer
    else if (t.RemoveRef().IsArray() && target.IsPtr()) {
        t = t.RemoveRef().ElementType().AddPtrDesc(PtrDescriptor {PtrType::PTR});
    }
    // 3. function (reference) to pointer / member pointer
    else if (t.RemoveRef().IsSimple(TypeKind::FUNCTION) && target.IsPtr()
             && target.RemovePtr().IsSimple(TypeKind::FUNCTION)) {
        t = t.RemoveRef();
        if (!t.Function()->IsNonStaticMember())
            t.AddPtrDesc(PtrDescriptor {PtrType::PTR});
        else
            // member function to member pointer
            t.AddPtrDesc(PtrDescriptor {PtrType::CLASSPTR,
                                        CVQualifier::NONE,
                                        t.Function()->funcScope->GetCurrentClass()});
    }
    // O. r-value to const l-value (creates temporary)
    else if (!t.IsRef() && target.IsRef() && target.cv == CVQualifier::CONST
             && !t.IsSimple(TypeKind::FUNCTION)) {
        t.AddPtrDesc(PtrDescriptor {PtrType::REF});
        t.cv = CVQualifier::CONST;
    }
    // O. function to function reference
    else if (t.IsSimple(TypeKind::FUNCTION) && target.IsRef()
             && target.RemoveRef().IsSimple(TypeKind::FUNCTION)) {
        t.AddPtrDesc(PtrDescriptor {PtrType::REF});
    }

    // 4~8, 10. numeric conversion & bool conversion
    if (t.IsSimple(TypeKind::FUNDTYPE) && target.IsSimple(TypeKind::FUNDTYPE)) {
        if (t.fundType == FundType::VOID)
            return false;
        else if (constant)
            *constant = constant->Convert(t.fundType, target.fundType);

        return true;
    }
    // 4, 10. integer promotion: enum to int (to float) & bool conversion: enum to bool
    else if (t.IsSimple(TypeKind::ENUM) && target.IsSimple(TypeKind::FUNDTYPE)) {
        if (constant) {
            switch (target.fundType) {
            case FundType::BOOL:
                constant->boolVal = (bool)(constant->intVal);
                break;
            case FundType::CHAR:
            case FundType::UCHAR:
                constant->charVal = (char)(constant->intVal);
                break;
            case FundType::FLOAT:
            case FundType::DOUBLE:
                constant->floatVal = (double)(constant->intVal);
                break;
            default:
                break;
            }
        }
        return true;
    }
    // 9. pointer conversion
    else if (target.IsPtr()) {
        // literal '0' to pointer
        if (t.IsSimple(TypeKind::FUNDTYPE) && t.fundType == FundType::INT) {
            return constant && constant->intVal == 0;
        }
        // object pointer to void* pointer
        else if (t.IsPtr() && target.RemovePtr().IsSimple(TypeKind::FUNDTYPE)
                 && target.fundType == FundType::VOID) {
            return true;
        }
        // pointer to derived class to pointer to base class
        else if (t.IsPtr() && t.RemovePtr().IsSimple(TypeKind::CLASS)
                 && target.RemovePtr().IsSimple(TypeKind::CLASS)) {
            return target.Class()->IsBaseOf(*t.Class());
        }
        // TODO: member pointer conversion
    }
    // 10. bool conversion: pointer to bool
    else if (t.IsPtr() && target.IsSimple(TypeKind::FUNDTYPE)
             && target.fundType == FundType::BOOL) {
        if (constant)
            constant->boolVal = (bool)(constant->intVal);
        return true;
    }

    // 11. qualification adjustment
    if (target.cv == CVQualifier::CONST && !t.IsSimple(TypeKind::FUNCTION)
        && !target.IsSimple(TypeKind::FUNCTION)) {
        t.cv = CVQualifier::CONST;
    }

    auto convertPtrDesc = [](auto &pdl, const auto &pdlTarget) {
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
    // if (typeKind != target.typeKind) {
    //     // ENUM type to INT
    //     if (typeKind == TypeKind::ENUM && target.typeKind == TypeKind::FUNDTYPE)
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

    // if (typeKind == TypeKind::FUNDTYPE) {
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
    // else if (typeKind == TypeKind::FUNCTION) {
    //     // assert(target.typeKind == TypeKind::FUNCTION)
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

    switch (typeKind) {
    case TypeKind::FUNDTYPE:
        // void is incomplete type
        return fundType != FundType::VOID;
    case TypeKind::CLASS:
        // class without definition is incomplete type
        return Class()->memberTable != nullptr;
    default:
        return true;
    }
}

bool Type::IsSimple(TypeKind tc) const
{
    return typeKind == tc && ptrDescList.empty() && arrayDescList.empty();
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

size_t Type::ArraySize() const
{
    return IsArray() ? arrayDescList.back().size : 0;
}

bool Type::IsConstInit() const
{
    assert(!IsSimple(TypeKind::FUNCTION));

    if (IsRef())
        return true;
    else if (IsPtr() || IsMemberPtr())
        return ptrDescList.back().cv == CVQualifier::CONST;
    else if (IsArray())
        return ElementType().IsConstInit();
    else
        return cv == CVQualifier::CONST;
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
    if (t.IsRef() && !t.RemoveRef().IsSimple(TypeKind::FUNCTION)
        && !t.RemoveRef().IsArray()) {
        t = t.RemoveRef();
        // remove cv for non class type
        if (!t.IsSimple(TypeKind::CLASS))
            t.cv = CVQualifier::NONE;
    }
    // 2. array (reference) to pointer
    else if (t.RemoveRef().IsArray()) {
        t = t.RemoveRef().ElementType().AddPtrDesc(PtrDescriptor {PtrType::PTR});
    }
    // 3. function (reference) to pointer
    else if (t.RemoveRef().IsSimple(TypeKind::FUNCTION)) {
        t = t.RemoveRef();
        if (!t.Function()->IsNonStaticMember())
            t.AddPtrDesc(PtrDescriptor {PtrType::PTR});
        else
            // member function to member pointer
            t.AddPtrDesc(PtrDescriptor {PtrType::CLASSPTR,
                                        CVQualifier::NONE,
                                        t.Function()->funcScope->GetCurrentClass()});
    }

    return t;
}

Type Type::ArithmeticConvert(Type t2) const
{
    Type t1 = *this;

    if (t1.IsSimple(TypeKind::ENUM))
        t1 = {FundType::INT};
    if (t2.IsSimple(TypeKind::ENUM))
        t2 = {FundType::INT};
    if (t1.IsSimple(TypeKind::FUNDTYPE) && t2.IsSimple(TypeKind::FUNDTYPE)) {
        if (t1.fundType == FundType::DOUBLE || t2.fundType == FundType::DOUBLE)
            return {FundType::DOUBLE};
        else if (t1.fundType == FundType::FLOAT || t2.fundType == FundType::FLOAT)
            return {FundType::FLOAT};
        else if (t1.fundType == FundType::ULONG || t2.fundType == FundType::ULONG)
            return {FundType::ULONG};
        else if (t1.fundType == FundType::LONG || t2.fundType == FundType::LONG)
            return {FundType::LONG};
        else if (t1.fundType == FundType::UINT || t2.fundType == FundType::UINT)
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

Type Type::RemoveRef() const
{
    Type t = *this;
    if (t.IsRef())
        t.ptrDescList.pop_back();
    return t;
}

Type Type::RemovePtr() const
{
    Type t = *this;
    if (t.IsPtr())
        t.ptrDescList.pop_back();
    return t;
}

std::string ClassDescriptor::FullName() const
{
    std::string      name          = className;
    ClassDescriptor *lastClassDesc = nullptr;

    for (SymbolTable *p = memberTable->GetParent(); p; p = p->GetParent()) {
        ClassDescriptor *classDesc = p->GetCurrentClass();
        if (classDesc) {
            if (classDesc != lastClassDesc) {
                name          = classDesc->className + "::" + name;
                lastClassDesc = classDesc;
            }
        }
        else
            break;
    }

    return name;
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
    , defSymbol(nullptr)
{}

bool FunctionDescriptor::IsMember() const
{
    return defSymbol && defSymbol->IsMember();
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

bool FunctionDescriptor::HasSameSignatureWith(const FunctionDescriptor &func,
                                              bool                      ignoreFirst)
{
    if (paramList.size() != func.paramList.size())
        return false;

    for (size_t i = ignoreFirst ? 1 : 0; i < paramList.size(); i++) {
        if (paramList[i].symbol->type != func.paramList[i].symbol->type)
            return false;
    }

    return true;
}

bool FunctionDescriptor::operator==(const FunctionDescriptor &func)
{
    return HasSameSignatureWith(func) && retType == func.retType;
}