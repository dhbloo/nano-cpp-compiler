/* Nano-cpp parser generator */

%code requires {
    #include "../ast/node.h"
}

%code {
    #include <cstdint>
    #include <string>
    #include <iostream>

    using namespace ast;
}

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
%define api.value.automove
%define parse.trace
%define parse.error verbose

%parse-param { ast::Ptr<ast::TranslationUnit>& astRoot }


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

/* Operator associativity */
%left "::" '.' "->" ".*" "->*" '*' '/' '%' '+' '-' "<<" ">>" 
%left '<' '>' "<=" ">=" "==" "!=" '&' '^' '|' "&&" "||" ','
%right '=' "+=" "-=" "*=" "/=" "%=" "^=" "&=" "|=" ">>=" "<<="


/* non-terminals */
%type<ast::Ptr<ast::LiteralExpression>> literal
%type<ast::Ptr<ast::IntLiteral>> int_literal
%type<ast::Ptr<ast::CharLiteral>> char_literal
%type<ast::Ptr<ast::FloatLiteral>> float_literal
%type<ast::Ptr<ast::StringLiteral>> string_literal
%type<ast::Ptr<ast::BoolLiteral>> boolean_literal

%type<ast::Ptr<ast::Expression>> primary_expression postfix_expression unary_expression new_expression
%type<ast::Ptr<ast::Expression>> delete_expression cast_expression pm_expression multiplicative_expression
%type<ast::Ptr<ast::Expression>> additive_expression shift_expression relational_expression equality_expression
%type<ast::Ptr<ast::Expression>> and_expression exclusive_or_expression inclusive_or_expression logical_and_expression
%type<ast::Ptr<ast::Expression>> logical_or_expression conditional_expression assignment_expression constant_expression
%type<ast::Ptr<ast::Expression>> expression expression_opt
%type<ast::Ptr<ast::IdExpression>> id_expression unqualified_id qualified_id
%type<UnaryOp> unary_operator

%type<ast::Ptr<ast::NameSpecifier>> name_specifier nested_name_specifier

%type<ast::PtrVec<ast::Declaration>> declaration_seq
%type<ast::Ptr<ast::Declaration>> declaration
%type<ast::Ptr<ast::BlockDeclaration>> block_declaration simple_declaration
%type<ast::Ptr<ast::DeclSpecifier>> decl_specifier_seq decl_specifier function_specifier
%type<ast::Ptr<ast::TypeSpecifier>> type_specifier
%type<FundTypePart> simple_type_specifier
%type<ast::Ptr<ast::ElaboratedTypeSpecifier>> elaborated_type_specifier type_name
%type<ast::Ptr<ast::EnumSpecifier>> enum_specifier enumerator_list
%type<ast::EnumSpecifier::Enumerator> enumerator_definition

%type<ast::Ptr<ast::FunctionDefinition>> function_definition

%type<ast::Ptr<ast::ConversionFunctionId>> conversion_function_id conversion_type_id

%type<ast::Ptr<ast::OperatorFunctionId>> operator_function_id
%type<OverloadOperator> operator

%type<bool> COLONCOLON_opt
%type<std::string> identifier identifier_opt enumerator typedef_name class_name enum_name

%start translation_unit

%%

/* ------------------------------------------------------------------------- *
 * 0. Lexical element
 * ------------------------------------------------------------------------- */

identifier:
    IDENTIFIER                  { $$ = $1; }
;

literal:
    int_literal                 { $$ = $1; }
|   char_literal                { $$ = $1; }
|   float_literal               { $$ = $1; }
|   string_literal              { $$ = $1; }
|   boolean_literal             { $$ = $1; }
;

int_literal:
    INTVAL
        { $$ = MkNode<IntLiteral>(); $$->value = $1; }
;

char_literal:
    CHARVAL
        { $$ = MkNode<CharLiteral>(); $$->value = $1; }
;

float_literal:
    FLOATVAL
        { $$ = MkNode<FloatLiteral>(); $$->value = $1; }
;

string_literal:
    STRVAL
        { $$ = MkNode<StringLiteral>(); $$->value = $1; }
;

boolean_literal:
    BOOLVAL
        { $$ = MkNode<BoolLiteral>(); $$->value = $1; }
;


/* ------------------------------------------------------------------------- *
 * 1. Context dependent keywords
 * ------------------------------------------------------------------------- */

typedef_name:
    TYPEDEF identifier          { $$ = $2; }
;

class_name:
    CLASS identifier            { $$ = $2; }
;

enum_name:
    ENUM identifier             { $$ = $2; }
;

/* ------------------------------------------------------------------------- *
 * 2. Basic concept
 * ------------------------------------------------------------------------- */

translation_unit:
        { astRoot = MkNode<TranslationUnit>(); }
|   declaration_seq
        { astRoot = MkNode<TranslationUnit>(); astRoot->decls = $1; }
;

/* ------------------------------------------------------------------------- *
 * 3. Expression
 * ------------------------------------------------------------------------- */

primary_expression:
    literal                     { $$ = $1; }
|   THIS                        { $$ = MkNode<ThisExpression>(); }
|   '(' expression ')'          { $$ = $2; }
|   id_expression               { $$ = $1; }
;

id_expression:
    unqualified_id              { $$ = $1; }
|   qualified_id                { $$ = $1; }
;

unqualified_id:
    identifier
        { $$ = MkNode<IdExpression>(); $$->identifier = $1; }
|   operator_function_id
        { $$ = $1; }
|   conversion_function_id
        { $$ = $1; }
|   '~' class_name
        {
            $$ = MkNode<IdExpression>(); 
            $$->identifier = $2; 
            $$->isDestructor = true;
        }
;

qualified_id:
    name_specifier unqualified_id
        { $$ = $2; $$->nameSpec = $1; }
|   "::" identifier
        {
            $$ = MkNode<IdExpression>(); 
            $$->identifier = $2;
            $$->nameSpec = MkNode<NameSpecifier>(); 
            $$->nameSpec->isGlobal = true;
        }
|   "::" operator_function_id
        { $$ = $2; $$->isGlobal = true; }
;

name_specifier:
    "::"
        { $$ = MkNode<NameSpecifier>(); $$->isGlobal = true; }
|   COLONCOLON_opt nested_name_specifier
        { $$ = $2; $$->isGlobal = $1; }
;

nested_name_specifier:
    class_name "::"
        { $$ = MkNode<NameSpecifier>(); $$->path.push_back($1); }
|   nested_name_specifier class_name "::"
        { $$ = $1; $$->path.push_back($2); }
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
    name_specifier_opt type_name_COLONCOLON_opt '~' type_name
;

unary_expression:
    postfix_expression          
        { $$ = $1; }
|   unary_operator cast_expression
        { 
            $$ = MkNode<UnaryExpression>();
            $$->op = $1;
            $$->expr = $2;
        }
|   SIZEOF unary_expression
        {
            $$ = MkNode<UnaryExpression>();
            $$->op = UnaryOp::SIZEOF;
            $$->expr = $2;
        }
|   SIZEOF '(' type_id ')'
        { $$ = MkNode<SizeofExpression>(); $$->typeId = $3; }
|   new_expression
        { $$ = $1; }
|   delete_expression
        { $$ = $1; }
;

unary_operator:
    '*'                         { $$ = UnaryOp::UNREF; }
|   '&'                         { $$ = UnaryOp::ADDRESSOF; }
|   '+'                         { $$ = UnaryOp::POSI; }
|   '-'                         { $$ = UnaryOp::NEG; }
|   '!'                         { $$ = UnaryOp::LOGINOT; }
|   '~'                         { $$ = UnaryOp::NOT; }
|   "++"                        { $$ = UnaryOp::PREINC; }
|   "--"                        { $$ = UnaryOp::PREDEC; }
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
    IF '(' condition ')' statement else_statement
|   SWITCH '(' condition ')' statement
;

else_statement:
|   ELSE statement;

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
    declaration_seq             { $$ = $1; }
|   declaration_seq declaration
        { $$ = $1; $$->decls.push_back($2); }
;
    
declaration:
    block_declaration           { $$ = $1; }
|   function_definition         { $$ = $1; }
;
    
block_declaration:
    simple_declaration          { $$ = $1; }
;

simple_declaration:
    decl_specifier_seq_opt init_declarator_list_opt ';'
        { 
            $$ = MkNode<BlockDeclaration>();
            $$->declSpec = $1;
            $$->initDeclList = $2;
        }
;
    
decl_specifier:
    type_specifier
        { $$ = MkNode<DeclSpecifier>(); $$->typeSpec = $1; }
|   function_specifier          
        { $$ = $1; }
|   FRIEND
        { $$ = MkNode<DeclSpecifier>(); $$->isFriend = true; }
|   TYPEDEF
        { $$ = MkNode<DeclSpecifier>(); $$->isTypedef = true; }
;
    
decl_specifier_seq:
    decl_specifier
        { $$ = $1; }
|   decl_specifier_seq decl_specifier
        { 
            $$ = $1;
            auto ss = $$->combine($2);
            if (ss)
                throw syntax_error(yylloc, ss.moveMsg());
        }
;
    
function_specifier:
    VIRTUAL
        { $$ = MkNode<DeclSpecifier>(); $$->isVirtual = true; }
;

type_specifier:
    simple_type_specifier       
        { $$ = MkNode<SimpleTypeSpecifier>(); $$->fundTypePart = $1; }
|   class_specifier
        { $$ = MkNode<ClassTypeSpecifier>(); $$->classType = $1; }
|   enum_specifier
        { $$ = MkNode<EnumTypeSpecifier>(); $$->enumType = $1; }
|   elaborated_type_specifier
        { $$ = $1; }
|   cv_qualifier
        { $$ = MkNode<TypeSpecifier>(); $$->cv = $1; }
;
    
simple_type_specifier:
    CHAR                        { $$ = FundTypePart::CHAR; }
|   BOOL                        { $$ = FundTypePart::BOOL; }
|   SHORT                       { $$ = FundTypePart::SHORT; }
|   INT                         { $$ = FundTypePart::INT; }
|   LONG                        { $$ = FundTypePart::LONG; }
|   SIGNED                      { $$ = FundTypePart::SIGNED; }
|   UNSIGNED                    { $$ = FundTypePart::UNSIGNED; }
|   FLOAT                       { $$ = FundTypePart::FLOAT; }
|   DOUBLE                      { $$ = FundTypePart::DOUBLE; }
|   VOID                        { $$ = FundTypePart::CHAR; }
;
    
type_name:
    class_name
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::CLASSNAME;
            $$->typeName = $1;
        }
|   enum_name
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::ENUMNAME;
            $$->typeName = $1;
        }
|   typedef_name
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::TYPEDEFNAME;
            $$->typeName = $1;
        }
;
    
elaborated_type_specifier:
    name_specifier_opt type_name
        { $$ = $2; $$->nameSpec = $1; }
|   class_key name_specifier_opt identifier
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::CLASSNAME;
            $$->typeName = $3;
            $$->nameSpec = $2;
        }
|   ENUM name_specifier_opt identifier
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::ENUMNAME;
            $$->typeName = $3;
            $$->nameSpec = $2;
        }
;
    
enum_specifier:
    ENUM identifier_opt '{' '}'
        {
            $$ = MkNode<EnumSpecifier>();
            $$->identifier = $2;
        }
|   ENUM identifier_opt '{' enumerator_list '}'
        {
            $$ = $4;
            $$->identifier = $2;
        }
;
    
enumerator_list:
    enumerator_definition 
        { $$ = MkNode<EnumSpecifier>(); $$->enumList.push_back($1); }
|   enumerator_list ',' enumerator_definition
        { $$ = $1; $$->enumList.push_back($3); }
;
    
enumerator_definition:
    enumerator
        { $$.first = $1; }
|   enumerator '=' constant_expression
        { $$.first = $1; $$.second = $2; }
;
    
enumerator:
    identifier                  { $$ = $1; }
;

/* ------------------------------------------------------------------------- *
 * 6. Declarator
 * ------------------------------------------------------------------------- */

init_declarator_list:
    init_declarator
        { return nullptr; /* TODO */ }
|   init_declarator_list ',' init_declarator
        { return nullptr; /* TODO */ }
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
|   name_specifier '*' cv_qualifier_opt
;
    
cv_qualifier:
    CONST
;

declarator_id:
    id_expression
|   name_specifier_opt type_name
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
|   name_specifier unqualified_id ';'
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
    name_specifier_opt class_name
|   access_specifier name_specifier_opt class_name
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
    operator conversion_type_id { $$ = $1; }
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
    name_specifier_opt class_name
|   identifier
;

/* ------------------------------------------------------------------------- *
 * 10. Operator overloading
 * ------------------------------------------------------------------------- */

operator_function_id:
    OPERATOR operator
        { $$ = MkNode<OperatorFunctionId>(); $$->overloadOp = $2; }
;

operator:
    '+'                         { $$ = OverloadOperator::ADD; }
|   '-'                         { $$ = OverloadOperator::SUB; }
|   '*'                         { $$ = OverloadOperator::MUL; }
|   '/'                         { $$ = OverloadOperator::DIV; }
|   '%'                         { $$ = OverloadOperator::MOD; }
|   '^'                         { $$ = OverloadOperator::XOR; }
|   '&'                         { $$ = OverloadOperator::AND; }
|   '|'                         { $$ = OverloadOperator::OR; }
|   '~'                         { $$ = OverloadOperator::NOT; }
|   '!'                         { $$ = OverloadOperator::LOGINOT; }
|   '='                         { $$ = OverloadOperator::ASSIGN; }
|   '<'                         { $$ = OverloadOperator::LT; }
|   '>'                         { $$ = OverloadOperator::GT; }
|   "+="                        { $$ = OverloadOperator::SELFADD; }
|   "-="                        { $$ = OverloadOperator::SELFSUB; }
|   "*="                        { $$ = OverloadOperator::SELFMUL; }
|   "/="                        { $$ = OverloadOperator::SELFDIV; }
|   "%="                        { $$ = OverloadOperator::SELFMOD; }
|   "~="                        { $$ = OverloadOperator::SELFXOR; }
|   "&="                        { $$ = OverloadOperator::SELFAND; }
|   "|="                        { $$ = OverloadOperator::SELFOR; }
|   "<<"                        { $$ = OverloadOperator::SHL; }
|   ">>"                        { $$ = OverloadOperator::SHR; }
|   "<<="                       { $$ = OverloadOperator::SELFSHL; }
|   ">>="                       { $$ = OverloadOperator::SELFSHR; }
|   "=="                        { $$ = OverloadOperator::EQ; }
|   "!="                        { $$ = OverloadOperator::NE; }
|   "<="                        { $$ = OverloadOperator::LE; }
|   ">="                        { $$ = OverloadOperator::GE; }
|   "&&"                        { $$ = OverloadOperator::LOGIAND; }
|   "||"                        { $$ = OverloadOperator::LOGIOR; }
|   "++"                        { $$ = OverloadOperator::SELFINC; }
|   "--"                        { $$ = OverloadOperator::SELFDEC; }
|   ','                         { $$ = OverloadOperator::COMMA; }
|   "->*"                       { $$ = OverloadOperator::ARROWSTAR; }
|   "->"                        { $$ = OverloadOperator::ARROW; }
|   "()"                        { $$ = OverloadOperator::CALL; }
|   "[]"                        { $$ = OverloadOperator::SUBSCRIPT; }
;

/* ------------------------------------------------------------------------- *
 * A. Optional epsilon definition
 * ------------------------------------------------------------------------- */

COLONCOLON_opt: | "::" ;
COMMA_opt: | ',';

expression_list_opt: | expression_list ;
name_specifier_opt: | name_specifier;
type_name_COLONCOLON_opt: | type_name "::";
new_placememt_opt: | new_placememt;
new_initializer_opt: | new_initializer;
new_declarator_opt: | new_declarator;

expression_opt:                 { $$ = nullptr; }
|   expression                  { $$ = $1; }
;

statement_seq_opt:              { $$ = nullptr; }
|   statement_seq               { $$ = $1; }
;

condition_opt:                  { $$ = nullptr; }
|   condition                   { $$ = $1; }
;

decl_specifier_seq_opt:         { $$ = nullptr; }
|   decl_specifier_seq          { $$ = $1; }
;

init_declarator_list_opt:       { $$ = nullptr; }
|   init_declarator_list        { $$ = $1; }
;

identifier_opt: | identifier;
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