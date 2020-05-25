#pragma once

enum class FundTypePart {
    VOID     = 512 | 1024 | 2048 | 0,
    BOOL     = 512 | 1024 | 2048 | 1,
    SHORT    = 1024 | 2,
    INT      = 512 | 4,
    LONG     = 1024 | 8,
    CHAR     = 512 | 1024 | 16,
    FLOAT    = 512 | 1024 | 2048 | 32,
    DOUBLE   = 512 | 1024 | 2048 | 64,
    SIGNED   = 2048 | 128,
    UNSIGNED = 2048 | 256,

    HASINT   = 512,
    HASINTS  = 1024,
    HASSIGN  = 2048
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