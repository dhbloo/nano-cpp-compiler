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
    LOGIOR
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