#pragma once

#include "operator.h"
#include "typeEnum.h"

#include <cstdint>

// Constant value
union Constant {
    intmax_t intVal;
    double   floatVal;
    char     charVal;
    bool     boolVal;

    // Convert a constant from one type to another type
    Constant Convert(FundType fromType, FundType toType) const;
    // Unary operator calcuation
    Constant UnaryOpResult(FundType type, UnaryOp uop) const;
    // binary operator calcuation (two operand must be the same type)
    Constant BinaryOpResult(FundType type, BinaryOp bop, Constant con2) const;
};