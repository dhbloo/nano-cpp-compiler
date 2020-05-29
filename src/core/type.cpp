#include "type.h"

#include "symbol.h"

static const int FundTypeSizeTable[] = {0, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8};
static const int PointerSize         = 8;

int Type::TypeSize() const
{
    int size;

    if (!ptrDescList.empty())
        size = PointerSize;
    else {
        switch (typeClass) {
        case TypeClass::FUNDTYPE: size = FundTypeSizeTable[(int)fundType]; break;
        case TypeClass::ENUM: size = FundTypeSizeTable[(int)FundType::INT]; break;
        case TypeClass::CLASS:
            size = static_cast<ClassDescriptor *>(typeDesc.get())->memberTable->ScopeSize();
            break;
        default: size = 0; break;
        }
    }

    for (const auto &a : arrayDescList) {
        size *= a.size;
    }

    return size;
}

bool Type::IsConvertibleTo(const Type &target)
{
    // Implicit conversion must be the same typeclass
    // TODO: conversion operator
    if (typeClass != target.typeClass)
        return false;

    // Const cannot be convert to non-const
    if (cv == CVQualifier::CONST && target.cv == CVQualifier::NONE)
        return false;

    if (ptrDescList != target.ptrDescList) {
        // -> T
        if (target.ptrDescList.empty()) {
            // T& -> T, const T& -> T: implicit copy
            if (ptrDescList.size() == 1 && ptrDescList[0].ptrType == PtrType::REF)
                return true;
        }
        // -> const T&
        else if (target.ptrDescList.size() == 1 && target.ptrDescList[0].ptrType == PtrType::REF
                 && target.cv == CVQualifier::CONST) {
            // T -> const T&, const T -> const T&
            if (ptrDescList.empty())
                return true;
        }

        return false;
    }

    if (arrayDescList != target.arrayDescList)
        return false;

    if (typeClass == TypeClass::FUNDTYPE) {
        // implicit fundmental type conversion rules
        switch (target.fundType) {
        case FundType::BOOL: return true;
        case FundType::CHAR:
        case FundType::UCHAR: return fundType <= FundType::UCHAR;
        case FundType::SHORT:
        case FundType::USHORT: return fundType <= FundType::USHORT;
        case FundType::INT:
        case FundType::UINT: return fundType <= FundType::UINT;
        case FundType::LONG:
        case FundType::ULONG: return fundType <= FundType::ULONG;
        case FundType::FLOAT: return fundType <= FundType::FLOAT;
        case FundType::DOUBLE: return true;
        default: return false;
        }
    }
    else {
        return typeDesc == target.typeDesc;
    }
}

bool Type::operator==(const Type &rhs)
{
    if (typeClass != rhs.typeClass || cv != rhs.cv || ptrDescList != rhs.ptrDescList
        || arrayDescList != rhs.arrayDescList)
        return false;

    switch (typeClass) {
    case TypeClass::FUNDTYPE: return fundType == rhs.fundType;
    case TypeClass::FUNCTION: return fundType == rhs.fundType && typeDesc == rhs.typeDesc;
    default: return typeDesc == rhs.typeDesc;
    }
}