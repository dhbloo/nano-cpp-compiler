/* Nano-cpp lexical analyzer */

%{

#include <string>
#include <cstdlib>
#include "../parser/yyparser.h"
#include "../parser/context.h"

static int yycolumn;

#define YY_USER_INIT yycolumn = 1;
#define YY_USER_ACTION {                                \
    yylloc->initialize(nullptr, yylineno, yycolumn);    \
    yylloc->columns(yyleng - 1);                        \
    yycolumn += yyleng;                                 \
}

static bool lookup_escape_char(char& c);
static char convert_escape_char(const char* text);
static std::string convert_escape_string(const char* text);

#define YY_DECL int yylex(YYSTYPE * yylval_param, YYLTYPE * yylloc_param, const ParseContext& pc)

%}

/* flex options */
%option yylineno
%option noyywrap nounput noinput
%option warn nodefault
%option 8bit batch fast
%option bison-bridge bison-locations
%option outfile="yylexer.cpp" header-file="yylexer.h"

/* lexical definitions */

ws                      [ \f\v\t\r\a]

identifier              [_a-zA-Z][_a-zA-Z0-9]*
decimal_literal         0|([1-9][0-9]*)
hexadecimal_literal     (0x|0X)[a-fA-F0-9]+
float_literal           [-+]?(([0-9]*\.[0-9]+)|([0-9]+\.))(f|F)?

char_literal            \'([^\\\'\n]|(\\.))\'
string_literal          \"((\\.)|[^"\\\n])*\"
char_lit                \'([^\\\'\n]|(\\.))
string_lit              \"((\\.)|[^"\\\n])*

boolean_literal         "false"|"true"

%x C_COMMENT
%x LINE_COMMENT

%%

<INITIAL>"/*"           { BEGIN(C_COMMENT); }
<C_COMMENT>"*/"         { BEGIN(INITIAL); }
<C_COMMENT>\n           { }
<C_COMMENT>.            { }
<C_COMMENT><<EOF>>      { throw yy::parser::syntax_error(*yylloc, "unclosed comment"); }

<INITIAL>"//"           { BEGIN(LINE_COMMENT); }
<LINE_COMMENT>\n        { BEGIN(INITIAL); }
<LINE_COMMENT>.         { }

\#.*\n                  { throw yy::parser::syntax_error(*yylloc, "preprocessing unsupported: " + std::string(yytext, yyleng)); }

{decimal_literal}       { char* p_end;
                          yylval->emplace<std::intmax_t>(std::strtoll(yytext, &p_end, 10)); 
                          return yy::parser::token::INTVAL; }
{hexadecimal_literal}   { char* p_end;
                          yylval->emplace<std::intmax_t>(std::strtoll(yytext, &p_end, 16)); 
                          return yy::parser::token::INTVAL; }
{float_literal}         { char* p_end;
                          yylval->emplace<double>(std::strtod(yytext, &p_end)); 
                          return yy::parser::token::FLOATVAL; }
{char_literal}          { yylval->emplace<char>(convert_escape_char(yytext)); 
                          return yy::parser::token::CHARVAL; }
{string_literal}        { yylval->emplace<std::string>(convert_escape_string(yytext)); 
                          return yy::parser::token::STRVAL; }
{char_lit}              { throw yy::parser::syntax_error(*yylloc, "unterminated character: " + std::string(yytext, yyleng)); }
{string_lit}            { throw yy::parser::syntax_error(*yylloc, "unterminated string: " + std::string(yytext, yyleng)); }

"false"                 { yylval->emplace<bool>(false); 
                          return yy::parser::token::BOOLVAL; }
"true"                  { yylval->emplace<bool>(true); 
                          return yy::parser::token::BOOLVAL; }

"bool"                  { return yy::parser::token::BOOL; }
"break"                 { return yy::parser::token::BREAK; }
"case"                  { return yy::parser::token::CASE; }
"char"                  { return yy::parser::token::CHAR; }
"class"                 { return yy::parser::token::CLASS; }
"const"                 { return yy::parser::token::CONST; }
"continue"              { return yy::parser::token::CONTINUE; }
"default"               { return yy::parser::token::DEFAULT; }
"delete"                { return yy::parser::token::DELETE; }
"do"                    { return yy::parser::token::DO; }
"double"                { return yy::parser::token::DOUBLE; }
"else"                  { return yy::parser::token::ELSE; }
"enum"                  { return yy::parser::token::ENUM; }
"float"                 { return yy::parser::token::FLOAT; }
"for"                   { return yy::parser::token::FOR; }
"friend"                { return yy::parser::token::FRIEND; }
"if"                    { return yy::parser::token::IF; }
"int"                   { return yy::parser::token::INT; }
"long"                  { return yy::parser::token::LONG; }
"new"                   { return yy::parser::token::NEW; }
"operator"              { return yy::parser::token::OPERATOR; }
"private"               { return yy::parser::token::PRIVATE; }
"protected"             { return yy::parser::token::PROTECTED; }
"public"                { return yy::parser::token::PUBLIC; }
"return"                { return yy::parser::token::RETURN; }
"short"                 { return yy::parser::token::SHORT; }
"signed"                { return yy::parser::token::SIGNED; }
"sizeof"                { return yy::parser::token::SIZEOF; }
"static"                { return yy::parser::token::STATIC; }
"struct"                { return yy::parser::token::STRUCT; }
"switch"                { return yy::parser::token::SWITCH; }
"this"                  { return yy::parser::token::THIS; }
"typedef"               { return yy::parser::token::TYPEDEF; }
"unsigned"              { return yy::parser::token::UNSIGNED; }
"virtual"               { return yy::parser::token::VIRTUAL; }
"void"                  { return yy::parser::token::VOID; }
"while"                 { return yy::parser::token::WHILE; }

"{"                     { return '{'; }
"}"                     { return '}'; }
"["                     { return '['; }
"]"                     { return ']'; }
"("                     { return '('; }
")"                     { return ')'; }
";"                     { return ';'; }
":"                     { return ':'; }
"?"                     { return '?'; }
"::"                    { return yy::parser::token::COLONCOLON; }
"."                     { return '.'; }
".*"                    { return yy::parser::token::DOTSTAR; }
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"%"                     { return '%'; }
"^"                     { return '^'; }
"&"                     { return '&'; }
"|"                     { return '|'; }
"~"                     { return '~'; }
"!"                     { return '!'; }
"="                     { return '='; }
"<"                     { return '<'; }
">"                     { return '>'; }
"+="                    { return yy::parser::token::SELFADD; }
"-="                    { return yy::parser::token::SELFSUB; }
"*="                    { return yy::parser::token::SELFMUL; }
"/="                    { return yy::parser::token::SELFDIV; }
"%="                    { return yy::parser::token::SELFMOD; }
"^="                    { return yy::parser::token::SELFXOR; }
"&="                    { return yy::parser::token::SELFAND; }
"|="                    { return yy::parser::token::SELFOR; }
"<<"                    { return yy::parser::token::SHIFTLEFT; }
">>"                    { return yy::parser::token::SHIFTRIGHT; }
"<<="                   { return yy::parser::token::SELFSHL; }
">>="                   { return yy::parser::token::SELFSHR; }
"=="                    { return yy::parser::token::EQ; }
"!="                    { return yy::parser::token::NE; }
"<="                    { return yy::parser::token::LE; }
">="                    { return yy::parser::token::GE; }
"&&"                    { return yy::parser::token::LOGIAND; }
"||"                    { return yy::parser::token::LOGIOR; }
"++"                    { return yy::parser::token::SELFINC; }
"--"                    { return yy::parser::token::SELFDEC; }
","                     { return ','; }
"->*"                   { return yy::parser::token::ARROWSTAR; }
"->"                    { return yy::parser::token::ARROW; }

{identifier}            { yylval->emplace<std::string>(yytext);
                          switch (pc.QueryName(yytext)) {
                            case ParseContext::CLASS:
                                return yy::parser::token::CLASSNAME;
                            case ParseContext::ENUM:
                                return yy::parser::token::ENUMNAME;
                            case ParseContext::TYPEDEF:
                                return yy::parser::token::TYPEDEFNAME;
                            default:
                                return yy::parser::token::IDENTIFIER;
                          }
                        }

{ws}+                   { /* Throw away whitespace */ }
\n+                     { yycolumn = 1; yylloc->initialize(nullptr, yylineno, yycolumn); }
.                       { throw yy::parser::syntax_error(*yylloc, "invalid character: " + std::string(yytext, yyleng)); }


%%

static bool lookup_escape_char(char& c) {
    const char escape[] = {'\'', '\"', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v', '0'};
    const char real[] = {'\'', '\"', '\\', '\a', '\b', '\f', '\n', '\r', '\t', '\v', '\0'};

    for (int i = 0; i < sizeof(escape); i++) {
        if (c == escape[i]) {
            c = real[i];
            return true;
        }
    }

    // unknown escape character
    return false;
}

static char convert_escape_char(const char* text) {
    if (text[1] == '\\') {
        char c = text[2];
        if (!lookup_escape_char(c))
            std::cerr << "warning: unknown escape sequence '\\" << c << "'\n";
        return c;
    } else {
        return text[1];
    }
}

static std::string convert_escape_string(const char* text) {
    std::string str{text + 1};
    str = str.substr(0, str.length() - 1);

    for (auto it = str.begin(); it != str.end(); it++) {
        if (*it == '\\' && (it + 1) != str.end()) {
            char c = *(it + 1);
            if (!lookup_escape_char(c)) {
                std::cerr << "warning: unknown escape sequence '\\" << c << "'\n";
            }

            str.replace(it, it + 2, 1, c);
        }
    }
    return str;
}