#pragma once

enum class FundTypePart {
    VOID     = 0,
    BOOL     = 1,
    SHORT    = 2,
    INT      = 4,
    LONG     = 8,
    CHAR     = 16,
    FLOAT    = 32,
    DOUBLE   = 64,
    SIGNED   = 128,
    UNSIGNED = 256
};

enum class FundType {
    VOID,
    BOOL,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    CHAR,
    SCHAR,
    UCHAR,
    FLOAT,
    DOUBLE
};

enum class CVQualifier {
    NONE,
    CONST
};