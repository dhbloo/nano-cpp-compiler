#include "constant.h"

Constant Constant::Convert(FundType fromType, FundType toType) const
{
    bool     isDecimal = false;
    intmax_t integral;
    double   decimal;
    Constant constant;

    switch (fromType) {
    case FundType::BOOL:
        integral = boolVal;
        break;
    case FundType::CHAR:
    case FundType::UCHAR:
        integral = charVal;
        break;
    case FundType::FLOAT:
    case FundType::DOUBLE:
        decimal   = floatVal;
        isDecimal = true;
        break;
    default:
        integral = intVal;
        break;
    }

    switch (toType) {
    case FundType::BOOL:
        constant.boolVal = isDecimal ? (bool)decimal : (bool)integral;
        break;
    case FundType::CHAR:
    case FundType::UCHAR:
        constant.charVal = isDecimal ? (char)decimal : (char)integral;
        break;
    case FundType::FLOAT:
    case FundType::DOUBLE:
        constant.floatVal = isDecimal ? (double)decimal : (double)integral;
        break;
    default:
        constant.intVal = isDecimal ? (intmax_t)decimal : (intmax_t)integral;
        break;
    }

    return constant;
}

Constant Constant::UnaryOpResult(FundType type, UnaryOp uop) const
{
    bool     isDecimal;
    Constant constant = *this;

    switch (type) {
    case FundType::FLOAT:
    case FundType::DOUBLE:
        isDecimal = true;
        break;
    default:
        isDecimal = false;
        break;
    }

    switch (uop) {
    case UnaryOp::NEG:
        if (isDecimal)
            constant.floatVal = -floatVal;
        else
            constant.intVal = -intVal;
        break;
    case UnaryOp::NOT:
        constant.intVal = ~intVal;
        break;
    case UnaryOp::LOGINOT:
        constant.intVal = isDecimal ? !intVal : !floatVal;
        break;
    default:
        break;
    }

    return constant;
}

Constant Constant::BinaryOpResult(FundType type, BinaryOp bop, Constant con2) const
{
    bool     isDecimal;
    Constant constant = *this;

    switch (type) {
    case FundType::FLOAT:
    case FundType::DOUBLE:
        isDecimal = true;
        break;
    default:
        isDecimal = false;
        break;
    }

    switch (bop) {
    case BinaryOp::MUL:
        if (isDecimal)
            constant.floatVal = floatVal * con2.floatVal;
        else
            constant.intVal = intVal * con2.intVal;
        break;
    case BinaryOp::DIV:
        if (isDecimal)
            constant.floatVal = floatVal / con2.floatVal;
        else if (con2.intVal != 0)
            constant.intVal = intVal / con2.intVal;
        else
            constant.intVal = 0;  // undefined behaviour
        break;
    case BinaryOp::MOD:
        if (con2.intVal != 0)
            constant.intVal = intVal % con2.intVal;
        else
            constant.intVal = 0;  // undefined behaviour
        break;
    case BinaryOp::ADD:
        if (isDecimal)
            constant.floatVal = floatVal + con2.floatVal;
        else
            constant.intVal = intVal + con2.intVal;
        break;
    case BinaryOp::SUB:
        if (isDecimal)
            constant.floatVal = floatVal - con2.floatVal;
        else
            constant.intVal = intVal - con2.intVal;
        break;
    case BinaryOp::SHL:
        constant.intVal = intVal << con2.intVal;
        break;
    case BinaryOp::SHR:
        constant.intVal = intVal >> con2.intVal;
        break;
    case BinaryOp::GT:
        if (isDecimal)
            constant.floatVal = floatVal > con2.floatVal;
        else
            constant.intVal = intVal > con2.intVal;
        break;
    case BinaryOp::LT:
        if (isDecimal)
            constant.floatVal = floatVal < con2.floatVal;
        else
            constant.intVal = intVal < con2.intVal;
        break;
    case BinaryOp::LE:
        if (isDecimal)
            constant.floatVal = floatVal <= con2.floatVal;
        else
            constant.intVal = intVal <= con2.intVal;
        break;
    case BinaryOp::GE:
        if (isDecimal)
            constant.floatVal = floatVal >= con2.floatVal;
        else
            constant.intVal = intVal >= con2.intVal;
        break;
    case BinaryOp::EQ:
        if (isDecimal)
            constant.floatVal = floatVal == con2.floatVal;
        else
            constant.intVal = intVal == con2.intVal;
        break;
    case BinaryOp::NE:
        if (isDecimal)
            constant.floatVal = floatVal != con2.floatVal;
        else
            constant.intVal = intVal != con2.intVal;
        break;
    case BinaryOp::AND:
        constant.intVal = intVal & con2.intVal;
        break;
    case BinaryOp::XOR:
        constant.intVal = intVal ^ con2.intVal;
        break;
    case BinaryOp::OR:
        constant.intVal = intVal | con2.intVal;
        break;
    case BinaryOp::LOGIAND:
        if (isDecimal)
            constant.floatVal = floatVal && con2.floatVal;
        else
            constant.intVal = intVal && con2.intVal;
        break;
    case BinaryOp::LOGIOR:
        if (isDecimal)
            constant.floatVal = floatVal || con2.floatVal;
        else
            constant.intVal = intVal || con2.intVal;
        break;
    default:
        break;
    }

    return constant;
}