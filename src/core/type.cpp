#include "type.h"

#include "symbol.h"

const Type Type::IntType {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::INT, nullptr};
const Type Type::BoolType {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::BOOL, nullptr};
const Type Type::CharType {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::CHAR, nullptr};
const Type Type::FloatType {TypeClass::FUNDTYPE,
                            CVQualifier::NONE,
                            {},
                            {},
                            FundType::FLOAT,
                            nullptr};
const Type Type::StringTypeProto {TypeClass::FUNDTYPE,
                                  CVQualifier::CONST,
                                  {},
                                  {},
                                  FundType::CHAR,
                                  nullptr};

static const int FundTypeSizeTable[] = {0, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8};
static const int PointerSize         = 8;

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
        std::string arrayPrefix;
        for (const auto &p : a->ptrDescList) {
            arrayPrefix += p.Name();
            if (arrayPrefix.back() == 't')
                arrayPrefix += " ";
        }

        if (!postfix.empty() && postfix.front() != '[')
            postfix = arrayPrefix + "(" + postfix + ")[" + std::to_string(a->size) + "]";
        else
            postfix = arrayPrefix + postfix + "[" + std::to_string(a->size) + "]";
    }

    // primary type
    switch (typeClass) {
    case TypeClass::FUNDTYPE:
        name = std::string(FundTypeName[(int)fundType]);
        if (!postfix.empty())
            name += " " + postfix;
        break;
    case TypeClass::ENUM:
        name = static_cast<EnumDescriptor *>(typeDesc.get())->enumName;
        if (!postfix.empty())
            name += " " + postfix;
        break;
    case TypeClass::CLASS:
        name = static_cast<ClassDescriptor *>(typeDesc.get())->className;
        if (!postfix.empty())
            name += " " + postfix;
        break;
    default:
        auto funcDesc = static_cast<FunctionDescriptor *>(typeDesc.get());

        if (!postfix.empty())
            name = "(" + postfix + ")";

        name += "(";
        for (std::size_t i = 0; i < funcDesc->paramList.size(); i++) {
            if (i > 0)
                name += ", ";
            name += funcDesc->paramList[i].symbol->type.Name();
        }
        name += ")";

        if (cv == CVQualifier::CONST)
            name += " const";

        name = funcDesc->retType.Name(name);
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
        name = "::*";  // TODO
        break;
    }

    if (cv == CVQualifier::CONST)
        name += "const";

    return name;
}

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
        if (!a.ptrDescList.empty())
            size = PointerSize;
        size *= a.size;
    }

    return size;
}

bool Type::IsComplete() const
{
    // ref or pointer to some type
    if (!ptrDescList.empty())
        return true;

    if (!arrayDescList.empty()) {
        auto &a = arrayDescList.back();

        // array of unknown size is incomplete type
        if (a.size == 0)
            return false;

        // array to pointer to some type
        if (!a.ptrDescList.empty())
            return true;
    }

    switch (typeClass) {
    case TypeClass::FUNDTYPE:
        // void is incomplete type
        if (fundType == FundType::VOID)
            return false;
        break;
    case TypeClass::CLASS:
        // class without definition is incomplete type
        if (!static_cast<ClassDescriptor *>(typeDesc.get())->memberTable)
            return false;
        break;
    default: break;
    }

    return true;
}

bool Type::IsConvertibleTo(const Type &target) const
{
    // Implicit conversion must be the same typeclass
    // TODO: conversion operator
    if (typeClass != target.typeClass) {
        if (typeClass == TypeClass::ENUM && target.typeClass == TypeClass::FUNDTYPE)
            return Type::IntType.IsConvertibleTo(target);
        else
            return false;
    }

    if (arrayDescList != target.arrayDescList) {
        // Const array/pointer convert to non-const array/pointer
        if (cv == CVQualifier::CONST && target.cv == CVQualifier::NONE)
            return false;

        // T [x] -> T *
        if (!arrayDescList.empty()) {
            if (!target.ptrDescList.empty() && target.ptrDescList.back().ptrType != PtrType::REF) {
                Type removeArrayT   = *this;
                Type removePointerT = target;
                removeArrayT.arrayDescList.pop_back();
                removePointerT.ptrDescList.pop_back();
                if (removeArrayT == removePointerT)
                    return true;
            }
            // T [x] -> T []
            else if (!target.arrayDescList.empty() && target.arrayDescList.back().size == 0) {
                Type removeArrayT = *this;
                Type removeArrayU = target;
                removeArrayT.arrayDescList.pop_back();
                removeArrayU.arrayDescList.pop_back();
                if (removeArrayT == removeArrayU)
                    return true;
            }
        }

        // T * -> T []
        if (!ptrDescList.empty() && ptrDescList.back().ptrType != PtrType::REF
            && !target.arrayDescList.empty() && target.arrayDescList.back().size == 0) {
            Type removeArrayT   = target;
            Type removePointerT = *this;
            removeArrayT.arrayDescList.pop_back();
            removePointerT.ptrDescList.pop_back();
            if (removeArrayT == removePointerT)
                return true;
        }

        return false;
    }

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
    else if (!ptrDescList.empty()) {
        // Const ref/pointer convert to non-const ref/pointer
        if (cv == CVQualifier::CONST && target.cv == CVQualifier::NONE)
            return false;
    }

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

Constant Type::ConvertConstant(Constant constant, const Type &target) const
{
    bool          isDecimal = false;
    std::intmax_t integral;
    double        decimal;

    switch (fundType) {
    case FundType::BOOL: integral = constant.boolVal; break;
    case FundType::CHAR:
    case FundType::UCHAR: integral = constant.charVal; break;
    case FundType::FLOAT:
    case FundType::DOUBLE:
        decimal   = constant.floatVal;
        isDecimal = true;
        break;
    default: integral = constant.intVal; break;
    }

    switch (target.fundType) {
    case FundType::BOOL: constant.boolVal = (bool)(isDecimal ? decimal : integral); break;
    case FundType::CHAR:
    case FundType::UCHAR: constant.charVal = (char)(isDecimal ? decimal : integral); break;
    case FundType::FLOAT:
    case FundType::DOUBLE: constant.floatVal = (double)(isDecimal ? decimal : integral); break;
    default: constant.intVal = (std::intmax_t)(isDecimal ? decimal : integral); break;
    }

    return constant;
}

bool Type::operator==(const Type &rhs) const
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

std::string ClassDescriptor::QualifiedName() const
{
    std::string name = className;

    SymbolTable *symtab = memberTable->GetParent();
    while (symtab != symtab->GetParent()) {
        if (symtab->GetClass())
            name = symtab->GetClass()->className + "::" + name;
    }

    return name;
}