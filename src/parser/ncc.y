/* Nano-cpp parser generator */

%{

#include <cstdint>
#include <string>

%}

%code provides {
    typedef yy::parser::semantic_type YYSTYPE;
    typedef yy::parser::location_type YYLTYPE;

    extern int yylex(YYSTYPE * yylval_param, YYLTYPE * yylloc_param);
}

/* bison declarations */
%require "3.2"
%language "c++"
%locations
%defines "yyparser.h"
%output "yyparser.cpp"
%define api.location.file "yylocation.h"
%define api.value.type variant
//%define api.value.automove
%define parse.trace
%define parse.error verbose

/* Identifier */
%token <std::string> IDENTIFIER

/* Literal */
%token <std::intmax_t> INTVAL
%token <double> FLOATVAL
%token <char> CHARVAL
%token <std::string> STRVAL
%token <bool> BOOLVAL

/* Operator or punctuator */
%token COLONCOLON   "::"
%token DOTSTAR      ".*"
%token SELFADD      "+="
%token SELFSUB      "-="
%token SELFMUL      "*="
%token SELFDIV      "/="
%token SELFMOD      "%="
%token SELFXOR      "^="
%token SELFAND      "&="
%token SELFOR       "|="
%token SHIFTLEFT    "<<"
%token SHIFTRIGHT   ">>"
%token SELFSHL      ">>="
%token SELFSHR      "<<="
%token EQ           "=="
%token NE           "!="
%token LE           "<="
%token GE           ">="
%token LOGIAND      "&&"
%token LOGIOR       "||"
%token SELFINC      "++"
%token SELFDEC      "--"
%token ARROWSTAR    "->*"
%token ARROW        "->"

/* Keyword */
%token BOOL         BREAK           CASE            CHAR            CLASS
%token CONST        CONTINUE        DEFAULT         DELETE          DO
%token DOUBLE       ELSE            ENUM            FALSE           FLOAT
%token FOR          FRIEND          IF              INT             LONG
%token NEW          OPERATOR        PRIVATE         PROTECTED       PUBLIC            
%token REGISTER     RETURN          SHORT           SIGNED          SIZEOF
%token STATIC       STRUCT          SWITCH          THIS            TRUE
%token TYPEDEF      UNSIGNED        VIRTUAL         VOID            WHILE

/* End of file */
%token ENDOFFILE

/* Operator associativity */
%left "::" '.' "->" ".*" "->*" '*' '/' '%' '+' '-' "<<" ">>" 
%left '<' '>' "<=" ">=" "==" "!=" '&' '^' '|' "&&" "||" ','
%right '=' "+=" "-=" "*=" "/=" "%=" "^=" "&=" "|=" ">>=" "<<="


/* non-terminals */


%start translation_unit

%%

/* ------------------------------------------------------------------------- *
 * 0. Lexical element
 * ------------------------------------------------------------------------- */

identifier:
    IDENTIFIER
;

literal:
    int_literal
|   char_literal
|   float_literal
|   string_literal
|   boolean_literal
;

int_literal:
    INTVAL
;

char_literal:
    CHARVAL
;

float_literal:
    FLOATVAL;
;

string_literal:
    STRVAL
;

boolean_literal:
    BOOLVAL
;


/* ------------------------------------------------------------------------- *
 * 1. Context dependent keywords
 * ------------------------------------------------------------------------- */

typedef_name:
    TYPEDEF identifier
;

class_name:
    CLASS identifier
;

enum_name:
    ENUM identifier
;

/* ------------------------------------------------------------------------- *
 * 2. Basic concept
 * ------------------------------------------------------------------------- */

translation_unit:
    declaration_seq_opt ENDOFFILE
;

/* ------------------------------------------------------------------------- *
 * 3. Expression
 * ------------------------------------------------------------------------- */

primary_expression:
    literal
|   THIS
|   '(' expression ')'
|   id_expression
;

id_expression:
    unqualified_id
|   qualified_id
;

unqualified_id:
    identifier
|   operator_function_id
|   conversion_function_id
|   '~' class_name
;

qualified_id:
    COLONCOLON_opt nested_name_specifier unqualified_id
|   "::" identifier
|   "::" operator_function_id
;

nested_name_specifier:
    class_name "::"
|   nested_name_specifier class_name "::"
;

postfix_expression:
    primary_expression
|   postfix_expression '[' expression ']'
|   postfix_expression '(' expression_list_opt ')'
|   simple_type_specifier '(' expression_list_opt ')'
|   postfix_expression '.' id_expression
|   postfix_expression "->" id_expression
|   postfix_expression '.' pseudo_destructor_name
|   postfix_expression "->" pseudo_destructor_name
|   postfix_expression "++"
|   postfix_expression "--"
;

expression_list:
    assignment_expression
|   expression_list ',' assignment_expression
;

pseudo_destructor_name:
    COLONCOLON_opt nested_name_specifier_opt type_name_COLONCOLON_opt '~' type_name
;

unary_expression:
    postfix_expression
|   "++" cast_expression
|   "--" cast_expression
|   unary_operator cast_expression
|   SIZEOF unary_expression
|   SIZEOF '(' type_id ')'
|   new_expression
|   delete_expression
;

unary_operator:
    '*' 
|   '&' 
|   '+' 
|   '-' 
|   '!' 
|   '~'
;

new_expression:
    COLONCOLON_opt NEW new_placememt_opt new_type_id new_initializer_opt
|   COLONCOLON_opt NEW '(' type_id ')' new_placememt_opt
;

new_placememt:
    '(' expression_list ')'
;

new_type_id:
    type_specifier_seq new_declarator_opt
;

new_declarator:
    ptr_operator new_declarator_opt
|   direct_new_declarator
;

direct_new_declarator:
    '[' expression ']' 
|   direct_new_declarator '[' constant_expression ']'
;
    
new_initializer:
    '(' expression_list_opt ')'
;

delete_expression:
    COLONCOLON_opt DELETE cast_expression
|   COLONCOLON_opt DELETE '[' ']' cast_expression
;

cast_expression:
    unary_expression
|   '(' type_id ')' cast_expression
;

pm_expression:
    cast_expression
|   pm_expression ".*" cast_expression
|   pm_expression "->*" cast_expression
;

multiplicative_expression:
    pm_expression
|   multiplicative_expression '*' pm_expression
|   multiplicative_expression '/' pm_expression
|   multiplicative_expression '%' pm_expression
;

additive_expression:
    multiplicative_expression
|   additive_expression '+' multiplicative_expression
|   additive_expression '-' multiplicative_expression
;

shift_expression:
    additive_expression
|   shift_expression "<<" additive_expression
|   shift_expression ">>" additive_expression
;

relational_expression:
    shift_expression
|   relational_expression '<' shift_expression
|   relational_expression '>' shift_expression
|   relational_expression "<=" shift_expression
|   relational_expression ">=" shift_expression
;

equality_expression:
    relational_expression
|   equality_expression "==" relational_expression
|   equality_expression "!=" relational_expression
;

and_expression:
    equality_expression
|   and_expression '&' equality_expression
;

exclusive_or_expression:
    and_expression
|   exclusive_or_expression '^' and_expression
;

inclusive_or_expression:
    exclusive_or_expression
|   inclusive_or_expression '|' exclusive_or_expression
;

logical_and_expression:
    inclusive_or_expression
|   logical_and_expression "&&" inclusive_or_expression
;
    
logical_or_expression:
    logical_and_expression
|   logical_or_expression "||" logical_and_expression
;

conditional_expression:
    logical_or_expression
|   logical_or_expression '?' expression ':' assignment_expression
;

assignment_expression:
    conditional_expression
|   logical_or_expression assignment_operator assignment_expression
;

assignment_operator:
    '=' 
|   "*=" 
|   "/=" 
|   "%="
|   "+="
|   "-=" 
|   ">>=" 
|   "<<=" 
|   "&=" 
|   "~=" 
|   "|="
;

expression:
    assignment_expression
|   expression ',' assignment_expression
;

constant_expression:
    conditional_expression
;

/* ------------------------------------------------------------------------- *
 * 4. Statement
 * ------------------------------------------------------------------------- */

statement:
    labeled_statement
|   expression_statement
|   compound_statement
|   selection_statement
|   iteration_statement
|   jump_statement
|   declaration_statement
;

labeled_statement:
    CASE constant_expression ':' statement
|   DEFAULT ':' statement
;

expression_statement:
    expression_opt ';'
;

compound_statement:
    '{' statement_seq_opt '}'
;

statement_seq:
    statement
|   statement_seq statement
;

selection_statement:
    IF '(' condition ')' statement else_statement_opt
|   SWITCH '(' condition ')' statement
;

condition:
    expression
|   type_specifier_seq declarator '=' assignment_expression
;
    
iteration_statement:
    WHILE '(' condition ')' statement
|   DO statement WHILE '(' expression ')' ';'
|   FOR '(' for_init_statement condition_opt ';' expression_opt ')' statement
;

for_init_statement:
    expression_statement
|   simple_declaration
;

jump_statement:
    BREAK ';'
|   CONTINUE ';'
|   RETURN expression_opt ';'
;

declaration_statement:
    block_declaration
;

/* ------------------------------------------------------------------------- *
 * 5. Declaration
 * ------------------------------------------------------------------------- */

declaration_seq:
    declaration
|   declaration_seq declaration
;
    
declaration:
    block_declaration
|   function_definition
;
    
block_declaration:
    simple_declaration
;

simple_declaration:
    decl_specifier_seq_opt init_declarator_list_opt ';'
;
    
decl_specifier:
    type_specifier
|   function_specifier
|   FRIEND
|   TYPEDEF
;
    
decl_specifier_seq:
    decl_specifier
|   decl_specifier_seq decl_specifier
;
    
function_specifier:
    VIRTUAL
;

type_specifier:
    simple_type_specifier
|   class_specifier
|   enum_specifier
|   elaborated_type_specifier
|   cv_qualifier
;
    
simple_type_specifier:
    COLONCOLON_opt nested_name_specifier_opt type_name
|   CHAR
|   BOOL
|   SHORT
|   INT
|   LONG
|   SIGNED
|   UNSIGNED
|   FLOAT
|   DOUBLE
|   VOID
;
    
type_name:
    class_name
|   enum_name
|   typedef_name
;
    
elaborated_type_specifier:
    class_key COLONCOLON_opt nested_name_specifier_opt identifier
|   ENUM COLONCOLON_opt nested_name_specifier_opt identifier
;
    
enum_specifier:
    ENUM identifier_opt '{' enumerator_list_opt '}'
;
    
enumerator_list:
    enumerator_definition 
|   enumerator_list ',' enumerator_definition
;
    
enumerator_definition:
    enumerator
|   enumerator '=' constant_expression
;
    
enumerator:
    identifier
;

/* ------------------------------------------------------------------------- *
 * 6. Declarator
 * ------------------------------------------------------------------------- */

init_declarator_list:
    init_declarator
|   init_declarator_list ',' init_declarator
;

init_declarator:
    declarator initializer_opt
;
    
declarator:
    direct_declarator
|   ptr_operator declarator
;

direct_declarator:
    declarator_id
|   direct_declarator '(' parameter_declaration_list_opt ')' cv_qualifier_opt
|   direct_declarator '[' constant_expression_opt ']' '(' declarator ')'
;

ptr_operator:
    '*' cv_qualifier_opt
|   '&'
|   COLONCOLON_opt nested_name_specifier '*' cv_qualifier_opt
;
    
cv_qualifier:
    CONST
;

declarator_id:
    id_expression
|   COLONCOLON_opt nested_name_specifier_opt type_name
;

type_id:
    type_specifier_seq abstract_declarator_opt
;

type_specifier_seq:
    type_specifier type_specifier_seq_opt
;
    
abstract_declarator:
    ptr_operator abstract_declarator_opt
|   direct_abstract_declarator
;

direct_abstract_declarator:
    direct_abstract_declarator_opt '(' parameter_declaration_list_opt ')' cv_qualifier_opt
|   direct_abstract_declarator_opt '[' constant_expression_opt ']' '(' abstract_declarator ')'
;

parameter_declaration_list:
    parameter_declaration
|   parameter_declaration_list ',' parameter_declaration
;

parameter_declaration:
    decl_specifier_seq declarator
|   decl_specifier_seq declarator '=' assignment_expression
|   decl_specifier_seq abstract_declarator_opt
|   decl_specifier_seq abstract_declarator_opt '=' assignment_expression
;
    
function_definition:
    decl_specifier_seq_opt declarator ctor_initializer_opt function_body
;

function_body:
    compound_statement
;

initializer:
    '=' initializer_clause
|   '(' expression_list ')'
;

initializer_clause:
    assignment_expression
|   '{' initializer_list COMMA_opt '}'
|   '{' '}'
;

initializer_list:
    initializer_clause
|   initializer_list ',' initializer_clause
;

/* ------------------------------------------------------------------------- *
 * 7. Class
 * ------------------------------------------------------------------------- */

class_specifier:
    class_head '{' member_specification_opt '}'
;

class_head:
    class_key identifier_opt base_clause_opt
|   class_key nested_name_specifier identifier base_clause_opt
;

class_key:
    CLASS
|   STRUCT
;
    
member_specification:
    member_declaration member_specification_opt
|   access_specifier ':' member_specification_opt
;

member_declaration:
    decl_specifier_seq_opt member_declarator_list_opt ';'
|   function_definition COMMA_opt
|   COLONCOLON_opt nested_name_specifier unqualified_id ';'
;

member_declarator_list:
    member_declarator
|   member_declarator_list ',' member_declarator
;

member_declarator:
    declarator pure_specifier_opt
|   declarator constant_initializer
;
    
pure_specifier:
    '=' '0'
;

constant_initializer:
    '=' constant_expression
;

/* ------------------------------------------------------------------------- *
 * 8. Derived class
 * ------------------------------------------------------------------------- */

base_clause:
    ':' base_specifier
;

base_specifier:
    COLONCOLON_opt nested_name_specifier_opt class_name
|   access_specifier COLONCOLON_opt nested_name_specifier_opt class_name
;

access_specifier:
    PRIVATE
|   PROTECTED
|   PUBLIC
;

/* ------------------------------------------------------------------------- *
 * 9. Special member function
 * ------------------------------------------------------------------------- */

conversion_function_id:
    operator conversion_type_id
;

conversion_type_id:
    type_specifier_seq conversion_declarator_opt
;

conversion_declarator:
    ptr_operator conversion_declarator_opt
;

ctor_initializer:
    ':' mem_initializer_list
;

mem_initializer_list:
    mem_initializer
|   mem_initializer ',' mem_initializer_list
;

mem_initializer:
    mem_initializer_id '(' expression_list_opt ')'
;

mem_initializer_id:
    COLONCOLON_opt nested_name_specifier_opt class_name
|   identifier
;

/* ------------------------------------------------------------------------- *
 * 10. Overloading
 * ------------------------------------------------------------------------- */

operator_function_id:
    OPERATOR operator
;
    
operator:
    '+' 
|   '-' 
|   '*' 
|   '/' 
|   '%' 
|   '^' 
|   '&' 
|   '|' 
|   '~'
|   '!' 
|   '=' 
|   '<' 
|   '>' 
|   "+=" 
|   "-=" 
|   "*=" 
|   "/=" 
|   "%="
|   "~=" 
|   "&=" 
|   "|=" 
|   "<<" 
|   ">>" 
|   ">>=" 
|   "<<=" 
|   "==" 
|   "!="
|   "<=" 
|   ">=" 
|   "&&" 
|   "||" 
|   "++" 
|   "--" 
|   ','
|   "->*"
|   "->"
|   "()" 
|   "[]"
;

/* ------------------------------------------------------------------------- *
 * A. Optional epsilon definition
 * ------------------------------------------------------------------------- */

COLONCOLON_opt: | "::" ;
COMMA_opt: | ',';

declaration_seq_opt: | declaration_seq;
expression_list_opt: | expression_list ;
nested_name_specifier_opt: | nested_name_specifier;
type_name_COLONCOLON_opt: | type_name "::";
new_placememt_opt: | new_placememt;
new_initializer_opt: | new_initializer;
new_declarator_opt: | new_declarator;
expression_opt: | expression;
statement_seq_opt: | statement_seq;
else_statement_opt: | ELSE statement;
condition_opt: | condition;
decl_specifier_seq_opt: | decl_specifier_seq;
init_declarator_list_opt: | init_declarator_list;
identifier_opt: | identifier;
enumerator_list_opt: | enumerator_list;
initializer_opt: | initializer;
parameter_declaration_list_opt: | parameter_declaration_list;
cv_qualifier_opt: | cv_qualifier;
constant_expression_opt: | constant_expression;
abstract_declarator_opt: | abstract_declarator;
type_specifier_seq_opt: | type_specifier_seq;
direct_abstract_declarator_opt: | direct_abstract_declarator;
ctor_initializer_opt: | ctor_initializer;
member_specification_opt: | member_specification;
base_clause_opt: | base_clause;
member_declarator_list_opt: | member_declarator_list;
pure_specifier_opt: | pure_specifier;
conversion_declarator_opt: | conversion_declarator;


%%

namespace yy {

void parser::error(const location_type& l, const std::string& msg) {
    std::cerr << msg << " at location " << l << '\n';
}

}