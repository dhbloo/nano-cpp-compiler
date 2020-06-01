#pragma once

enum class FundTypePart {
    VOID     = 1024 | 2048 | 4096 | 1,
    BOOL     = 1024 | 2048 | 4096 | 2,
    SHORT    = 2048 | 4,
    INT      = 1024 | 8,
    LONG     = 2048 | 16,
    CHAR     = 1024 | 2048 | 32,
    FLOAT    = 1024 | 2048 | 4096 | 64,
    DOUBLE   = 1024 | 2048 | 4096 | 128,
    SIGNED   = 4096 | 256,
    UNSIGNED = 4096 | 512,

    HASINT  = 1024,
    HASINTS = 2048,
    HASSIGN = 4096
};

enum class FundType {
    VOID,
    BOOL,
    CHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    FLOAT,
    DOUBLE
};

enum class TypeClass { FUNDTYPE, ENUM, CLASS, FUNCTION };

enum class CVQualifier { NONE, CONST };

enum class PtrType { PTR, REF, CLASSPTR };

enum class Access { DEFAULT, PUBLIC, PROTECTED, PRIVATE };