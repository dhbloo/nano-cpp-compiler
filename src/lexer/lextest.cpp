#include "../parser/yyparser.h"

#include <iostream>
#include <string>

const int   EnumStart   = yy::parser::token::IDENTIFIER;
const char *EnumTable[] = {
    "IDENTIFIER", "CLASSNAME", "ENUMNAME",   "TYPEDEFNAME", "INTVAL",  "FLOATVAL",
    "CHARVAL",    "STRVAL",    "BOOLVAL",    "COLONCOLON",  "DOTSTAR", "SELFADD",
    "SELFSUB",    "SELFMUL",   "SELFDIV",    "SELFMOD",     "SELFXOR", "SELFAND",
    "SELFOR",     "SHIFTLEFT", "SHIFTRIGHT", "SELFSHL",     "SELFSHR", "EQ",
    "NE",         "LE",        "GE",         "LOGIAND",     "LOGIOR",  "SELFINC",
    "SELFDEC",    "ARROWSTAR", "ARROW",      "BOOL",        "BREAK",   "CASE",
    "CHAR",       "CLASS",     "CONST",      "CONTINUE",    "DEFAULT", "DELETE",
    "DO",         "DOUBLE",    "ELSE",       "ENUM",        "FLOAT",   "FOR",
    "FRIEND",     "IF",        "INT",        "LONG",        "NEW",     "OPERATOR",
    "PRIVATE",    "PROTECTED", "PUBLIC",     "RETURN",      "SHORT",   "SIGNED",
    "SIZEOF",     "STATIC",    "STRUCT",     "SWITCH",      "THIS",    "TYPEDEF",
    "UNSIGNED",   "VIRTUAL",   "VOID",       "WHILE"};

bool HasSemanticValue(int t)
{
    return t >= yy::parser::token::IDENTIFIER && t <= yy::parser::token::BOOLVAL;
}

std::string SemanticValueToString(int t, const YYSTYPE &v)
{
    switch (t) {
    case yy::parser::token::IDENTIFIER:
        return v.as<std::string>();
    case yy::parser::token::INTVAL:
        return std::to_string(v.as<intmax_t>());
    case yy::parser::token::FLOATVAL:
        return std::to_string(v.as<double>());
    case yy::parser::token::CHARVAL:
        return std::string("\'") + v.as<char>() + "\'";
    case yy::parser::token::STRVAL:
        return "\"" + v.as<std::string>() + "\"";
    case yy::parser::token::BOOLVAL:
        return v.as<bool>() ? "true" : "false";
    default:
        return "";
    }
}

std::string EnumName(int t)
{
    constexpr int Count = sizeof(EnumTable) / sizeof(decltype(EnumTable[0]));
    if (t >= 0 && t < EnumStart) {
        return std::string(1, char(t));
    }
    else if (t >= EnumStart && t < EnumStart + Count) {
        return EnumTable[t - EnumStart];
    }
    else {
        return "Unknown Enum";
    }
}

int main(int argc, char *argv[])
{
    int          token;
    YYSTYPE      value;
    YYLTYPE      location;
    size_t       index = 1;
    ParseContext pc;

    std::cout << std::string(80, '-') << '\n';
    std::cout << "Index\t | Token Type\t\t | Semantic Value\t\t\n";
    std::cout << std::string(80, '-') << '\n';

    do {
        try {
            token = yylex(&value, &location, pc);
        }
        catch (yy::parser::syntax_error e) {
            std::cerr << "error at: " << location << "\n\t" << e.what() << '\n';
            continue;
        }

        std::printf("%7d. | %-21s | ", index++, EnumName(token).c_str());
        if (HasSemanticValue(token))
            std::cout << SemanticValueToString(token, value);
        std::cout << '\n';

    } while (token != 0);
}