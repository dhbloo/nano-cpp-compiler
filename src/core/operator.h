#pragma once

enum class UnaryOp {
    UNREF,
    ADDRESSOF,
    POSI,
    NEG,
    NOT,
    LOGINOT,
    PREINC,
    PREDEC,
    POSTINC,
    POSTDEC,
    SIZEOF
};

enum class BinaryOp {
    SUBSCRIPT,
    DOT,
    ARROW,
    DOTSTAR,
    ARROWSTAR,
    MUL,
    DIV,
    MOD,
    ADD,
    SUB,
    SHL,
    SHR,
    GT,
    LT,
    LE,
    GE,
    EQ,
    NE,
    AND,
    XOR,
    OR,
    LOGIAND,
    LOGIOR,
    COMMA
};

enum class AssignOp {
    ASSIGN,
    SELFMUL,
    SELFDIV,
    SELFMOD,
    SELFADD,
    SELFSUB,
    SELFSHR,
    SELFSHL,
    SELFAND,
    SELFXOR,
    SELFOR
};

enum class Operator {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    XOR,
    AND,
    OR,
    NOT,
    LOGINOT,
    ASSIGN,
    LT,
    GT,
    SELFADD,
    SELFSUB,
    SELFMUL,
    SELFDIV,
    SELFMOD,
    SELFXOR,
    SELFAND,
    SELFOR,
    SHL,
    SHR,
    SELFSHL,
    SELFSHR,
    EQ,
    NE,
    LE,
    GE,
    LOGIAND,
    LOGIOR,
    SELFINC,
    SELFDEC,
    COMMA,
    ARROWSTAR,
    ARROW,
    CALL,
    SUBSCRIPT,
    NIL
};

inline Operator ToOverloadOp(UnaryOp uop)
{
    switch (uop) {
    case UnaryOp::ADDRESSOF:
        return Operator::AND;
    case UnaryOp::POSI:
        return Operator::ADD;
    case UnaryOp::NEG:
        return Operator::SUB;
    case UnaryOp::NOT:
        return Operator::NOT;
    case UnaryOp::LOGINOT:
        return Operator::LOGINOT;
    case UnaryOp::PREINC:
        return Operator::SELFINC;
    case UnaryOp::PREDEC:
        return Operator::SELFDEC;
    case UnaryOp::POSTINC:
        return Operator::SELFINC;
    case UnaryOp::POSTDEC:
        return Operator::SELFDEC;
    default:
        return Operator::NIL;
    }
}

inline Operator ToOverloadOp(BinaryOp bop)
{
    switch (bop) {
    case BinaryOp::SUBSCRIPT:
        return Operator::SUBSCRIPT;
    case BinaryOp::ARROW:
        return Operator::ARROW;
    case BinaryOp::ARROWSTAR:
        return Operator::ARROWSTAR;
    case BinaryOp::MUL:
        return Operator::MUL;
    case BinaryOp::DIV:
        return Operator::DIV;
    case BinaryOp::MOD:
        return Operator::MOD;
    case BinaryOp::ADD:
        return Operator::ADD;
    case BinaryOp::SUB:
        return Operator::SUB;
    case BinaryOp::SHL:
        return Operator::SHL;
    case BinaryOp::SHR:
        return Operator::SHR;
    case BinaryOp::GT:
        return Operator::GT;
    case BinaryOp::LT:
        return Operator::LT;
    case BinaryOp::LE:
        return Operator::LE;
    case BinaryOp::GE:
        return Operator::GE;
    case BinaryOp::EQ:
        return Operator::EQ;
    case BinaryOp::NE:
        return Operator::NE;
    case BinaryOp::AND:
        return Operator::AND;
    case BinaryOp::XOR:
        return Operator::XOR;
    case BinaryOp::OR:
        return Operator::OR;
    case BinaryOp::LOGIAND:
        return Operator::LOGIAND;
    case BinaryOp::LOGIOR:
        return Operator::LOGIOR;
    case BinaryOp::COMMA:
        return Operator::COMMA;
    default:
        return Operator::NIL;
    }
}

inline Operator ToOverloadOp(AssignOp aop)
{
    switch (aop) {
    case AssignOp::ASSIGN:
        return Operator::ASSIGN;
    case AssignOp::SELFMUL:
        return Operator::SELFMUL;
    case AssignOp::SELFDIV:
        return Operator::SELFDIV;
    case AssignOp::SELFMOD:
        return Operator::SELFMOD;
    case AssignOp::SELFADD:
        return Operator::SELFADD;
    case AssignOp::SELFSUB:
        return Operator::SELFSUB;
    case AssignOp::SELFSHR:
        return Operator::SELFSHR;
    case AssignOp::SELFSHL:
        return Operator::SELFSHL;
    case AssignOp::SELFAND:
        return Operator::SELFAND;
    case AssignOp::SELFXOR:
        return Operator::SELFXOR;
    default:
        return Operator::SELFOR;
    }
}