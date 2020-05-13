/* Nano-cpp parser generator */

%code requires {
    #include "../ast/node.h"
    #include "context.h"
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

    extern int yylex(YYSTYPE * yylval_param, YYLTYPE * yylloc_param, const ParseContext& pc);
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
%parse-param { int& errcnt }
%param { ParseContext pc }


/* Identifier */
%token <std::string> IDENTIFIER CLASSNAME ENUMNAME TYPEDEFNAME

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
%token DOUBLE       ELSE            ENUM            FLOAT
%token FOR          FRIEND          IF              INT             LONG
%token NEW          OPERATOR        PRIVATE         PROTECTED       PUBLIC            
%token RETURN       SHORT           SIGNED          SIZEOF
%token STATIC       STRUCT          SWITCH          THIS            
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

%type<ast::Ptr<ast::Expression>> primary_expression postfix_expression unary_expression equality_expression
%type<ast::Ptr<ast::Expression>> cast_expression pm_expression multiplicative_expression logical_and_expression
%type<ast::Ptr<ast::Expression>> additive_expression shift_expression relational_expression constant_expression 
%type<ast::Ptr<ast::Expression>> constant_expression_opt and_expression exclusive_or_expression inclusive_or_expression 
%type<ast::Ptr<ast::Expression>> logical_or_expression conditional_expression assignment_expression assignment_expression_opt
%type<ast::Ptr<ast::Expression>> expression expression_opt condition condition_opt constant_initializer 
%type<ast::Ptr<ast::IdExpression>> id_expression unqualified_id qualified_id
%type<ast::Ptr<ast::NewExpression>> new_expression
%type<ast::Ptr<ast::DeleteExpression>> delete_expression
%type<ast::Ptr<ast::ExpressionList>> expression_list expression_list_opt
%type<ast::Ptr<ast::ExpressionList>> new_placememt new_placememt_opt new_initializer new_initializer_opt
%type<ast::Ptr<ast::InitializableNew>> new_type_id new_declarator
%type<ast::PtrVec<ast::Expression>> direct_new_declarator
%type<ast::Ptr<ast::NameSpecifier>> nested_name_specifier
%type<AssignOp> assignment_operator
%type<UnaryOp> unary_operator

%type<ast::Ptr<ast::Statement>> statement labeled_statement selection_statement iteration_statement else_statement
%type<ast::Ptr<ast::ExpressionStatement>> expression_statement
%type<ast::Ptr<ast::CompoundStatement>> compound_statement statement_seq function_body
%type<ast::Ptr<ast::ForStatement>> for_init_statement
%type<ast::Ptr<ast::JumpStatement>> jump_statement
%type<ast::Ptr<ast::DeclerationStatement>> declaration_statement

%type<ast::PtrVec<ast::Declaration>> declaration_seq
%type<ast::Ptr<ast::Declaration>> declaration
%type<ast::Ptr<ast::BlockDeclaration>> block_declaration simple_declaration
%type<ast::Ptr<ast::DeclSpecifier>> decl_specifier_seq decl_specifier function_specifier
%type<ast::Ptr<ast::TypeSpecifier>> type_specifier type_specifier_seq
%type<ast::Ptr<ast::ElaboratedTypeSpecifier>> elaborated_type_specifier simple_typename_specifier forward_class_specifier type_name
%type<ast::Ptr<ast::EnumSpecifier>> enum_specifier enumerator_list
%type<ast::EnumSpecifier::Enumerator> enumerator_definition
%type<FundTypePart> simple_type_specifier
%type<CVQualifier> cv_qualifier

%type<std::vector<ast::BlockDeclaration::InitDecl>> init_declarator_list
%type<ast::BlockDeclaration::InitDecl> init_declarator
%type<ast::Ptr<ast::PtrSpecifier>> ptr_operator_list conversion_declarator conversion_declarator_opt
%type<ast::PtrSpecifier::PtrOp> ptr_operator
%type<ast::Ptr<ast::Declarator>> declarator direct_declarator abstract_declarator direct_abstract_declarator
%type<ast::Ptr<ast::Declarator>> abstract_declarator_opt
%type<ast::Ptr<ast::IdDeclarator>> declarator_id
%type<ast::Ptr<ast::TypeId>> type_id
%type<ast::PtrVec<ast::ParameterDeclaration>> parameter_declaration_list
%type<ast::Ptr<ast::ParameterDeclaration>> parameter_declaration
%type<ast::Ptr<ast::FunctionDefinition>> function_definition
%type<ast::Ptr<ast::Initializer>> initializer initializer_opt
%type<ast::Ptr<ast::ClauseInitializer>> initializer_clause
%type<ast::Ptr<ast::ListInitializer>> initializer_list

%type<ast::Ptr<ast::ClassSpecifier>> class_specifier class_head
%type<ast::ClassSpecifier::Key> class_key
%type<ast::Ptr<ast::MemberList>> member_specification
%type<ast::Ptr<ast::MemberDeclaration>> member_declaration
%type<ast::PtrVec<ast::MemberDeclarator>> member_declarator_list
%type<ast::Ptr<ast::MemberDeclarator>> member_declarator
%type<ast::Ptr<ast::BaseSpecifier>> base_clause base_clause_opt base_specifier
%type<Access> access_specifier

%type<ast::Ptr<ast::ConversionFunctionId>> conversion_function_id conversion_type_id
%type<ast::PtrVec<ast::CtorMemberInitializer>> ctor_initializer ctor_initializer_opt mem_initializer_list
%type<ast::Ptr<ast::CtorMemberInitializer>> mem_initializer mem_initializer_id
%type<ast::Ptr<ast::OperatorFunctionId>> operator_function_id
%type<Operator> operator

%type<bool> cv_qualifier_opt pure_specifier_opt
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
    TYPEDEFNAME                 { $$ = $1; }
;

class_name:
    CLASSNAME                   { $$ = $1; }
;

enum_name:
    ENUMNAME                    { $$ = $1; }
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
        { 
            $$ = MkNode<IdExpression>();
            $$->identifier = $1;
            pc.AddPossibleTypedefName($$->identifier);
        }
|   operator_function_id
        { $$ = $1; }
|   conversion_function_id
        { $$ = $1; }
|   '~' class_name
        {
            $$ = MkNode<IdExpression>();
            $$->identifier = $2; 
            $$->stype = IdExpression::DESTRUCTOR;
        }
;

qualified_id:
    nested_name_specifier unqualified_id
        { 
            $$ = $2; 
            $$->nameSpec = $1; 
            $$->nameSpec->isGlobal = false; 
            pc.PopQueryScopes();
        }
|   "::" nested_name_specifier unqualified_id
        { 
            $$ = $3; 
            $$->nameSpec = $2; 
            $$->nameSpec->isGlobal = true; 
            pc.PopQueryScopes();
        }
|   "::" identifier
        {
            $$ = MkNode<IdExpression>(); 
            $$->identifier = $2;
            $$->nameSpec = MkNode<NameSpecifier>(); 
            $$->nameSpec->isGlobal = true;
        }
|   "::" operator_function_id
        { auto e = $2; e->isGlobal = true; $$ = std::move(e); }
;

nested_name_specifier:
    class_name "::"
        { 
            $$ = MkNode<NameSpecifier>(); 
            $$->path.push_back($1);
            pc.PushQueryScope($$->path.back());
        }
|   nested_name_specifier class_name "::"
        { 
            $$ = $1; 
            $$->path.push_back($2);
            pc.PushQueryScope($$->path.back());
        }
;

postfix_expression:
    primary_expression
        { $$ = $1; }
|   postfix_expression '[' expression ']'
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::SUBSCRIPT;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   postfix_expression '(' expression_list_opt ')'
        {
            auto e = MkNode<CallExpression>();
            e->funcExpr = $1;
            e->params = $3;
            $$ = std::move(e);
        }
|   simple_typename_specifier '{' expression_list_opt '}'
        {
            auto e = MkNode<ConstructExpression>();
            e->type = $1;
            e->params = $3;
            $$ = std::move(e);
        }
|   postfix_expression '.' unqualified_id
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::DOT;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   postfix_expression '.' nested_name_specifier unqualified_id
        {
            auto r = $4;
            r->nameSpec = $3;
            r->nameSpec->isGlobal = false;

            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::DOT;
            e->left = $1;
            e->right = std::move(r);
            $$ = std::move(e);
            pc.PopQueryScopes();
        }
|   postfix_expression '.' "::" nested_name_specifier unqualified_id
        {
            auto r = $5;
            r->nameSpec = $4;
            r->nameSpec->isGlobal = true;
            
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::DOT;
            e->left = $1;
            e->right = std::move(r);
            $$ = std::move(e);
            pc.PopQueryScopes();
        }
|   postfix_expression "->" unqualified_id
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::ARROW;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   postfix_expression "++"
        {
            auto e = MkNode<UnaryExpression>();
            e->op = UnaryOp::POSTINC;
            e->expr = $1;
            $$ = std::move(e);
        }
|   postfix_expression "--"
        {
            auto e = MkNode<UnaryExpression>();
            e->op = UnaryOp::POSTDEC;
            e->expr = $1;
            $$ = std::move(e);
        }
;

expression_list:
    assignment_expression
        { $$ = MkNode<ExpressionList>(); $$->exprList.push_back($1); }
|   expression_list ',' assignment_expression
        { $$ = $1; $$->exprList.push_back($3); }
;

unary_expression:
    postfix_expression          
        { $$ = $1; }
|   unary_operator cast_expression
        { 
            auto e = MkNode<UnaryExpression>();
            e->op = $1;
            e->expr = $2;
            $$ = std::move(e);
        }
|   SIZEOF unary_expression
        {
            auto e = MkNode<UnaryExpression>();
            e->op = UnaryOp::SIZEOF;
            e->expr = $2;
            $$ = std::move(e);
        }
|   SIZEOF '(' type_id ')'
        {
            auto e = MkNode<SizeofExpression>();
            e->typeId = $3; 
            $$ = std::move(e);
        }
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
|   '~'                         { $$ = UnaryOp::NOT; }
|   '!'                         { $$ = UnaryOp::LOGINOT; }
|   "++"                        { $$ = UnaryOp::PREINC; }
|   "--"                        { $$ = UnaryOp::PREDEC; }
;

new_expression:
    NEW new_placememt_opt new_type_id new_initializer_opt
        {
            auto e = $3;
            e->initializer = $4;
            e->placement = $2;
            $$ = std::move(e);
        }
|   NEW '(' type_id ')' new_placememt_opt
        {
            auto e = MkNode<PlainNew>();
            e->typeId = $3;
            e->placement = $5;
            $$ = std::move(e);
        }
;

new_placememt:
    '(' expression_list ')'     { $$ = $2; }
;

new_type_id:
    type_specifier_seq
        { $$ = MkNode<InitializableNew>(); $$->typeSpec = $1; }
|   type_specifier_seq new_declarator
        { $$ = $2; $$->typeSpec = $1; }
;

new_declarator:
    ptr_operator_list direct_new_declarator
        { 
            $$ = MkNode<InitializableNew>();
            $$->ptrSpec = $1;
            $$->arraySizes = $2;
        }
|   ptr_operator_list
        { 
            $$ = MkNode<InitializableNew>();
            $$->ptrSpec = $1;
        }
|   direct_new_declarator
        { 
            $$ = MkNode<InitializableNew>();
            $$->arraySizes = $1;
        }
;

direct_new_declarator:
    '[' expression ']' 
        { $$.push_back($2); }
|   direct_new_declarator '[' constant_expression ']'
        { $$ = $1; $$.push_back($3); }
;
    
new_initializer:
    '(' ')'
        { $$ = MkNode<ExpressionList>(); }
|   '(' expression_list ')'
        { $$ = $2; }
;

delete_expression:
    DELETE cast_expression
        {
            $$ = MkNode<DeleteExpression>();
            $$->isArray = false;
            $$->expr = $2;
        }
|   DELETE '[' ']' cast_expression
        {
            $$ = MkNode<DeleteExpression>();
            $$->isArray = true;
            $$->expr = $4;
        }
;

cast_expression:
    unary_expression            
        { $$ = $1; }
|   '(' type_id ')' cast_expression
        {
            auto e = MkNode<CastExpression>();
            e->typeId = $2;
            e->expr = $4;
            $$ = std::move(e);
        }
;

pm_expression:
    cast_expression             
        { $$ = $1; }
|   pm_expression ".*" cast_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::DOTSTAR;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   pm_expression "->*" cast_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::ARROWSTAR;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

multiplicative_expression:
    pm_expression
        { $$ = $1; }
|   multiplicative_expression '*' pm_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::MUL;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   multiplicative_expression '/' pm_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::DIV;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   multiplicative_expression '%' pm_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::MOD;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

additive_expression:
    multiplicative_expression
        { $$ = $1; }
|   additive_expression '+' multiplicative_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::ADD;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   additive_expression '-' multiplicative_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::SUB;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

shift_expression:
    additive_expression
        { $$ = $1; }
|   shift_expression "<<" additive_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::SHL;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   shift_expression ">>" additive_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::SHR;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

relational_expression:
    shift_expression
        { $$ = $1; }
|   relational_expression '<' shift_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::LT;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   relational_expression '>' shift_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::GT;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   relational_expression "<=" shift_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::LE;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   relational_expression ">=" shift_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::GE;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

equality_expression:
    relational_expression
        { $$ = $1; }
|   equality_expression "==" relational_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::EQ;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
|   equality_expression "!=" relational_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::NE;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

and_expression:
    equality_expression
        { $$ = $1; }
|   and_expression '&' equality_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::AND;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

exclusive_or_expression:
    and_expression
        { $$ = $1; }
|   exclusive_or_expression '^' and_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::XOR;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

inclusive_or_expression:
    exclusive_or_expression
        { $$ = $1; }
|   inclusive_or_expression '|' exclusive_or_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::OR;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

logical_and_expression:
    inclusive_or_expression
        { $$ = $1; }
|   logical_and_expression "&&" inclusive_or_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::LOGIAND;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;
    
logical_or_expression:
    logical_and_expression
        { $$ = $1; }
|   logical_or_expression "||" logical_and_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::LOGIOR;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

conditional_expression:
    logical_or_expression
        { $$ = $1; }
|   logical_or_expression '?' expression ':' assignment_expression
        {
            auto e = MkNode<ConditionalExpression>();
            e->condition = $1;
            e->trueExpr = $3;
            e->falseExpr = $5;
            $$ = std::move(e);
        }
;

assignment_expression:
    conditional_expression
        { $$ = $1; }
|   logical_or_expression assignment_operator assignment_expression
        {
            auto e = MkNode<AssignmentExpression>();
            e->op = $2;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

assignment_operator:
    '='                         { $$ = AssignOp::ASSIGN; }
|   "*="                        { $$ = AssignOp::SELFMUL; }
|   "/="                        { $$ = AssignOp::SELFDIV; }
|   "%="                        { $$ = AssignOp::SELFMOD; }
|   "+="                        { $$ = AssignOp::SELFADD; }
|   "-="                        { $$ = AssignOp::SELFSUB; }
|   ">>="                       { $$ = AssignOp::SELFSHR; }
|   "<<="                       { $$ = AssignOp::SELFSHL; }
|   "&="                        { $$ = AssignOp::SELFAND; }
|   "^="                        { $$ = AssignOp::SELFXOR; }
|   "|="                        { $$ = AssignOp::SELFOR; }
;

expression:
    assignment_expression
        { $$ = $1; }
|   expression ',' assignment_expression
        {
            auto e = MkNode<BinaryExpression>();
            e->op = BinaryOp::COMMA;
            e->left = $1;
            e->right = $3;
            $$ = std::move(e);
        }
;

constant_expression:
    conditional_expression      { $$ = $1; }
;

/* ------------------------------------------------------------------------- *
 * 4. Statement
 * ------------------------------------------------------------------------- */

statement:
    labeled_statement           { $$ = $1; }
|   expression_statement        { $$ = $1; }
|   compound_statement          { $$ = $1; }
|   selection_statement         { $$ = $1; }
|   iteration_statement         { $$ = $1; }
|   jump_statement              { $$ = $1; }
|   declaration_statement       { $$ = $1; }
;

labeled_statement:
    CASE constant_expression ':' statement
        {
            auto e = MkNode<CaseStatement>();
            e->constant = $2;
            e->stmt = $4;
            $$ = std::move(e);
        }
|   DEFAULT ':' statement
        {
            auto e = MkNode<DefaultStatement>();
            e->stmt = $3;
            $$ = std::move(e);
        }
;

expression_statement:
    expression_opt ';'
        { $$ = MkNode<ExpressionStatement>(); $$->expr = $1; }
;

compound_statement:
    '{' '}'
        { $$ = MkNode<CompoundStatement>(); }
|   '{'
        { pc.EnterAnonymousScope(); } 
    statement_seq '}'
        { $$ = $3; pc.LeaveScope(); }
;

statement_seq:
    statement
        { $$ = MkNode<CompoundStatement>(); $$->stmts.push_back($1); }
|   statement_seq statement
        { $$ = $1; $$->stmts.push_back($2); }
|   error ';'
        { pc.PopQueryScopes(); }
|   statement_seq error ';'
        { $$ = $1; pc.PopQueryScopes(); }
;

selection_statement:
    IF '(' condition ')' statement else_statement
        {
            auto e = MkNode<IfStatement>();
            e->condition = $3;
            e->trueStmt = $5;
            e->falseStmt = $6;
            $$ = std::move(e);
        }
|   SWITCH '(' condition ')' statement
        {
            auto e = MkNode<SwitchStatement>();
            e->condition = $3;
            e->stmt = $5;
            $$ = std::move(e);
        }
;

else_statement:                 { $$ = nullptr; }
|   ELSE statement              { $$ = $2; }
;

condition:
    expression                  { $$ = $1; }
;
    
iteration_statement:
    WHILE '(' condition ')' statement
        {
            auto e = MkNode<WhileStatement>();
            e->condition = $3;
            e->stmt = $5;
            $$ = std::move(e);
        }
|   DO statement WHILE '(' expression ')' ';'
        {
            auto e = MkNode<DoStatement>();
            e->condition = $5;
            e->stmt = $2;
            $$ = std::move(e);
        }
|   FOR '(' for_init_statement condition_opt ';' expression_opt ')' statement
        {
            auto e = $3;
            e->condition = $4;
            e->iterExpr = $6;
            e->stmt = $8;
            $$ = std::move(e);
        }
;

for_init_statement:
    expression_statement
        {
            $$ = MkNode<ForStatement>();
            $$->initType = ForStatement::EXPR;
            $$->exprInit = $1;
        }
|   simple_declaration
        {
            $$ = MkNode<ForStatement>();
            $$->initType = ForStatement::DECL;
            $$->declInit = $1;
        }
;

jump_statement:
    BREAK ';'
        { $$ = MkNode<JumpStatement>(); $$->type = JumpStatement::BREAK; }
|   CONTINUE ';'
        { $$ = MkNode<JumpStatement>(); $$->type = JumpStatement::CONTINUE; }
|   RETURN expression_opt ';'
        {
            $$ = MkNode<JumpStatement>(); 
            $$->type = JumpStatement::RETURN; 
            $$->retExpr = $2;
        }
;

declaration_statement:
    block_declaration
        { $$ = MkNode<DeclerationStatement>(); $$->decl = $1; }
;

/* ------------------------------------------------------------------------- *
 * 5. Declaration
 * ------------------------------------------------------------------------- */

declaration_seq:
    declaration
        { $$.push_back($1); }
|   declaration_seq declaration
        { $$ = $1; $$.push_back($2); }
|   error
        { pc.PopQueryScopes(); }
|   error ';'
        { pc.PopQueryScopes(); yyclearin; yyerrok; }
|   declaration_seq error
        { $$ = $1; pc.PopQueryScopes(); }
|   declaration_seq error ';'
        { $$ = $1; pc.PopQueryScopes(); yyclearin; yyerrok; }
;
    
declaration:
    block_declaration           { $$ = $1; }
|   function_definition         { $$ = $1; }
;
    
block_declaration:
    simple_declaration          { $$ = $1; }
;

simple_declaration:
    decl_specifier_seq ';'
        { 
            $$ = MkNode<BlockDeclaration>(); 
            $$->declSpec = $1; 
            pc.EndTypedef();
        }
|   decl_specifier_seq init_declarator_list ';'
        { 
            $$ = MkNode<BlockDeclaration>();
            $$->declSpec = $1;
            $$->initDeclList = $2;
            pc.EndTypedef();
        }
;
    
decl_specifier:
    type_specifier
        { $$ = MkNode<DeclSpecifier>(); $$->typeSpec = $1; }
|   function_specifier          
        { $$ = $1; }
|   STATIC
        { $$ = MkNode<DeclSpecifier>(); $$->isStatic = true; }
|   FRIEND
        { $$ = MkNode<DeclSpecifier>(); $$->isFriend = true; }
|   TYPEDEF
        { 
            $$ = MkNode<DeclSpecifier>(); 
            $$->isTypedef = true; 
            pc.BeginTypedef();
        }
;
    
decl_specifier_seq:
    decl_specifier
        { $$ = $1; }
|   decl_specifier_seq decl_specifier
        { 
            $$ = $1;
            auto ss = Combine(std::move($$), $2, $$);
            if (ss)
                throw syntax_error(@$, ss.moveMsg());
        }
;
    
function_specifier:
    VIRTUAL
        { $$ = MkNode<DeclSpecifier>(); $$->isVirtual = true; }
;

type_specifier:
    simple_type_specifier
        { 
            auto e = MkNode<SimpleTypeSpecifier>(); 
            e->fundTypePart = $1; 
            $$ = std::move(e);
        }
|   class_specifier
        { 
            auto e = MkNode<ClassTypeSpecifier>(); 
            e->classType = $1; 
            $$ = std::move(e);
        }
|   enum_specifier
        { 
            auto e = MkNode<EnumTypeSpecifier>(); 
            e->enumType = $1; 
            $$ = std::move(e);
        }
|   elaborated_type_specifier
        { $$ = $1; }
|   cv_qualifier
        { 
            auto e = MkNode<TypeSpecifier>(); 
            e->cv = $1; 
            $$ = std::move(e);
        }
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
|   VOID                        { $$ = FundTypePart::VOID; }
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

simple_typename_specifier:
    type_name
        { 
            $$ = $1; 
        }
|   nested_name_specifier type_name
        { 
            $$ = $2; 
            $$->nameSpec = $1; 
            pc.PopQueryScopes();
        }
;
    
elaborated_type_specifier:
    simple_typename_specifier
        { $$ = $1; }
|   forward_class_specifier
        { $$ = $1; }
|   ENUM enum_name
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::ENUMNAME;
            $$->typeName = $2;
            pc.PopQueryScopes();
        }
|   ENUM nested_name_specifier enum_name
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::ENUMNAME;
            $$->typeName = $3;
            $$->nameSpec = $2;
            pc.PopQueryScopes();
        }
;
    
enum_specifier:
    ENUM identifier_opt '{' '}'
        {
            $$ = MkNode<EnumSpecifier>();
            $$->identifier = $2;

            if (!$$->identifier.empty())
                if (!pc.AddName($$->identifier, ParseContext::ENUM))
                    throw syntax_error(@3, "redeclear of identifier " + $$->identifier);
        }
|   ENUM identifier_opt '{' enumerator_list '}'
        {
            $$ = $4;
            $$->identifier = $2;

            if (!$$->identifier.empty())
                if (!pc.AddName($$->identifier, ParseContext::ENUM))
                    throw syntax_error(@3, "redeclear of identifier " + $$->identifier);
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
        { $$.first = $1; $$.second = $3; }
;
    
enumerator:
    identifier                  { $$ = $1; }
;

/* ------------------------------------------------------------------------- *
 * 6. Declarator
 * ------------------------------------------------------------------------- */

init_declarator_list:
    init_declarator
        { $$.push_back($1); }
|   init_declarator_list ',' init_declarator
        { $$ = $1; $$.push_back($3); }
;

init_declarator:
    direct_declarator initializer_opt
        {
            $$.declarator = $1;
            $$.initializer = $2;
        }
|   ptr_operator_list direct_declarator initializer_opt
        {
            $$.declarator = $2;
            $$.declarator->ptrSpec = $1;
            $$.initializer = $3;
        }
;
    
declarator:
    direct_declarator
        { $$ = $1; }
|   ptr_operator_list direct_declarator
        { $$ = $2; $$->ptrSpec = $1; }
;

direct_declarator:
    declarator_id
        { $$ = $1; }
|   direct_declarator '(' parameter_declaration_list ')' cv_qualifier_opt
        {
            auto e = MkNode<FunctionDeclarator>();
            e->retType = $1;
            e->params = $3;
            e->isFuncConst = $5;
            $$ = std::move(e);
        }
|   direct_declarator '[' constant_expression_opt ']' 
        {
            auto e = MkNode<ArrayDeclarator>();
            e->elemType = $1;
            e->size = $3;
            $$ = std::move(e);
        }
|   '(' declarator ')'
        { 
            auto e = MkNode<NestedDeclarator>(); 
            e->decl = $2; 
            $$ = std::move(e); 
        }
;

ptr_operator_list:
    ptr_operator
        { $$ = MkNode<PtrSpecifier>(); $$->ptrList.push_back($1); }
|   ptr_operator_list ptr_operator
        { $$ = $1; $$->ptrList.push_back($2); }
;

ptr_operator:
    '*' cv_qualifier_opt
        {
            $$.ptrType = PtrSpecifier::PTR; 
            $$.isPtrConst = $2;
        }
|   '&'
        { $$.ptrType = PtrSpecifier::REF; }
|   nested_name_specifier '*' cv_qualifier_opt
        {
            $$.ptrType = PtrSpecifier::CLASSPTR; 
            $$.isPtrConst = $3;
            $$.classNameSpec = $1;
            pc.PopQueryScopes();
        }
;
    
cv_qualifier:
    CONST                       { $$ = CVQualifier::CONST; }
;

declarator_id:
    id_expression
        { $$ = MkNode<IdDeclarator>(); $$->id = $1; }
;

type_id:
    type_specifier_seq abstract_declarator_opt
        {
            $$ = MkNode<TypeId>();
            $$->typeSpec = $1;
            $$->abstractDecl = $2;
        }
;

type_specifier_seq:
    type_specifier
        { $$ = $1; }
|   type_specifier_seq type_specifier
        {
            $$ = $1;
            auto ss = Combine(std::move($$), $2, $$);
            if (ss)
                throw syntax_error(@$, ss.moveMsg());
        }
;
    
abstract_declarator:
    direct_abstract_declarator
        { $$ = $1; }
|   ptr_operator_list
        { $$ = MkNode<Declarator>(); $$->ptrSpec = $1; }
|   ptr_operator_list direct_abstract_declarator
        { $$ = $2; $$->ptrSpec = $1; }
;

direct_abstract_declarator:
    '(' parameter_declaration_list ')' cv_qualifier_opt
        {
            auto e = MkNode<FunctionDeclarator>();
            e->params = $2;
            e->isFuncConst = $4;
            $$ = std::move(e);
        }
|   '[' constant_expression_opt ']'
        {
            auto e = MkNode<ArrayDeclarator>();
            e->size = $2;
            $$ = std::move(e);
        }
|   direct_abstract_declarator '(' parameter_declaration_list ')' cv_qualifier_opt
        {
            auto e = MkNode<FunctionDeclarator>();
            e->retType = $1;
            e->params = $3;
            e->isFuncConst = $5;
            $$ = std::move(e);
        }
|   direct_abstract_declarator '[' constant_expression_opt ']' 
        {
            auto e = MkNode<ArrayDeclarator>();
            e->elemType = $1;
            e->size = $3;
            $$ = std::move(e);
        }
|   '(' abstract_declarator ')'
        { 
            auto e = MkNode<NestedDeclarator>(); 
            e->decl = $2; 
            $$ = std::move(e);
        }
;

parameter_declaration_list:
        {}
|   parameter_declaration
        { $$.push_back($1); }
|   parameter_declaration_list ',' parameter_declaration
        { $$ = $1; $$.push_back($3); }
;

parameter_declaration:
    decl_specifier_seq declarator assignment_expression_opt
        {
            $$ = MkNode<ParameterDeclaration>();
            $$->declSpec = $1;
            $$->decl = $2;
            $$->defaultExpr = $3;
        }
|   decl_specifier_seq abstract_declarator_opt assignment_expression_opt
        {
            $$ = MkNode<ParameterDeclaration>();
            $$->declSpec = $1;
            $$->decl = $2;
            $$->defaultExpr = $3;
        }
;
    
function_definition:
    declarator function_body
        {
            $$ = MkNode<FunctionDefinition>();
            $$->declarator = $1;
            $$->funcBody = $2;
        }
|   decl_specifier_seq '(' parameter_declaration_list ')' ctor_initializer_opt function_body
        {
            auto decl = $1;
            if (typeid(*decl->typeSpec) != typeid(ElaboratedTypeSpecifier))
                throw syntax_error(@2, "expect member name or ';' here");

            auto edecl = static_cast<ElaboratedTypeSpecifier*>(decl->typeSpec.get());
            if (edecl->typeClass != ElaboratedTypeSpecifier::CLASSNAME
                || edecl->typeName != pc.CurLocalScopeName())
                throw syntax_error(@1, "expect member name or ';' here");

            auto i = MkNode<IdDeclarator>();
            i->id = MkNode<IdExpression>();
            i->id->identifier = edecl->typeName;
            i->id->stype = IdExpression::CONSTRUCTOR;

            auto e = MkNode<FunctionDeclarator>();
            e->retType = std::move(i);
            e->params = $3;

            $$ = MkNode<FunctionDefinition>();
            $$->declarator = std::move(e);
            $$->ctorInitList = $5;
            $$->funcBody = $6;
        }
|   decl_specifier_seq declarator function_body
        {
            $$ = MkNode<FunctionDefinition>();
            $$->declSpec = $1;
            $$->declarator = $2;
            $$->funcBody = $3;
        }
;

function_body:
    compound_statement          { $$ = $1; }
;

initializer:
    '=' initializer_clause      { $$ = $2; }
|   '(' expression_list ')'     
        { 
            auto e = MkNode<ParenthesisInitializer>(); 
            e->exprList = $2; 
            $$ = std::move(e);
        }
;

initializer_clause:
    assignment_expression
        { 
            auto e = MkNode<AssignmentInitializer>(); 
            e->expr = $1; 
            $$ = std::move(e);
        }
|   '{' initializer_list COMMA_opt '}'
        { $$ = $2; }
|   '{' '}'
        { $$ = MkNode<ListInitializer>(); }
;

initializer_list:
    initializer_clause
        { $$ = MkNode<ListInitializer>(); $$->initList.push_back($1); }
|   initializer_list ',' initializer_clause
        { $$ = $1; $$->initList.push_back($3); }
;

/* ------------------------------------------------------------------------- *
 * 7. Class
 * ------------------------------------------------------------------------- */

class_specifier:
    class_head '{' 
        { pc.EnterLastAddedName(); } 
    member_specification '}'
        { 
            $$ = $1;
            $$->members = $4;
            $$->MoveDefaultMember();
            $$->members->Reverse();
            pc.LeaveScope();
        }
;

class_head:
    class_key base_clause_opt
        {
            $$ = MkNode<ClassSpecifier>();
            $$->key = $1;
            $$->baseSpec = $2;
        }
|   class_key identifier base_clause_opt
        {
            $$ = MkNode<ClassSpecifier>();
            $$->key = $1;
            $$->identifier = $2;
            $$->baseSpec = $3;

            if (!pc.AddName($$->identifier, ParseContext::CLASS))
                throw syntax_error(@3, "redeclear of identifier " + $$->identifier);
        }
|   class_key nested_name_specifier class_name base_clause_opt
        {
            $$ = MkNode<ClassSpecifier>();
            $$->key = $1;
            $$->nameSpec = $2;
            $$->identifier = $3;
            $$->baseSpec = $4;
            pc.PopQueryScopes();
        }
;

class_key:
    CLASS                       { $$ = ClassSpecifier::CLASS; }
|   STRUCT                      { $$ = ClassSpecifier::STRUCT; }
;

base_clause_opt:                { $$ = nullptr; }
|   base_clause                 { $$ = $1; }
;

forward_class_specifier:
    class_key identifier
        {
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::CLASSNAME;
            $$->typeName = $2;

            if (!pc.AddName($$->typeName, ParseContext::CLASS))
                throw syntax_error(@2, "redeclear of classname " + $$->typeName);
        }
|   class_key class_name
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();
            $$->typeClass = ElaboratedTypeSpecifier::CLASSNAME;
            $$->typeName = $2;
        }
|   class_key nested_name_specifier class_name
        { 
            $$ = MkNode<ElaboratedTypeSpecifier>();  
            $$->typeClass = ElaboratedTypeSpecifier::CLASSNAME;
            $$->typeName = $3;
            $$->nameSpec = $2;
            pc.PopQueryScopes();
        }
;
    
member_specification:
        { $$ = MkNode<MemberList>(); }
|   member_declaration member_specification
        { $$ = $2; $$->defaultMember.push_back($1); }
|   access_specifier ':' member_specification
        { $$ = $3; $$->MoveDefaultTo($1); }
|   error member_specification
        { $$ = $2; pc.PopQueryScopes(); }
;

member_declaration:
    decl_specifier_seq member_declarator_list ';'
        {
            auto e = MkNode<MemberDefinition>();
            e->declSpec = $1;
            e->decls = $2;
            $$ = std::move(e);
        }
|   member_declarator ';'
        {
            auto e = MkNode<MemberDefinition>();
            e->decls.push_back($1);
            $$ = std::move(e);
        }
|   decl_specifier_seq '(' parameter_declaration_list ')' ';'
        {
            auto decl = $1;
            if (typeid(*decl->typeSpec) != typeid(ElaboratedTypeSpecifier))
                throw syntax_error(@1, "expect member name or ';' here");

            auto edecl = static_cast<ElaboratedTypeSpecifier*>(decl->typeSpec.get());
            if (edecl->typeClass != ElaboratedTypeSpecifier::CLASSNAME
                || edecl->typeName != pc.CurLocalScopeName())
                throw syntax_error(@1, "expect member name or ';' here");

            auto i = MkNode<IdDeclarator>();
            i->id = MkNode<IdExpression>();
            i->id->identifier = edecl->typeName;
            i->id->stype = IdExpression::CONSTRUCTOR;

            auto f = MkNode<FunctionDeclarator>();
            f->retType = std::move(i);
            f->params = $3;

            auto d = MkNode<MemberDeclarator>();
            d->decl = std::move(f);

            auto e = MkNode<MemberDefinition>();
            e->decls.push_back(std::move(d));

            $$ = std::move(e);
        }
|   function_definition COMMA_opt
        { 
            auto e = MkNode<MemberFunction>(); 
            e->func = $1; 
            $$ = std::move(e);
        }
;

member_declarator_list:
        {}
|   member_declarator
        { $$.push_back($1); }
|   member_declarator_list ',' member_declarator
        { $$ = $1; $$.push_back($3); }
;

member_declarator:
    declarator pure_specifier_opt
        {
            $$ = MkNode<MemberDeclarator>();
            $$->decl = $1;
            $$->isPure = $2;
        }
|   declarator constant_initializer
        {
            $$ = MkNode<MemberDeclarator>();
            $$->decl = $1;
            $$->constInit = $2;
        }
;
    
pure_specifier:
    '=' '0'
;

constant_initializer:
    '=' constant_expression     { $$ = $2; }
;

/* ------------------------------------------------------------------------- *
 * 8. Derived class
 * ------------------------------------------------------------------------- */

base_clause:
    ':' 
        { pc.PopQueryScopes(); }
    base_specifier          
        { $$ = $3; }
;

base_specifier:
    class_name
        {
            $$ = MkNode<BaseSpecifier>();
            $$->access = Access::DEFAULT;
            $$->className = $1;
        }
|   nested_name_specifier class_name
        {
            $$ = MkNode<BaseSpecifier>();
            $$->access = Access::DEFAULT;
            $$->nameSpec = $1;
            $$->className = $2;
            pc.PopQueryScopes();
        }
|   access_specifier class_name
        {
            $$ = MkNode<BaseSpecifier>();
            $$->access = $1;
            $$->className = $2;
        }
|   access_specifier nested_name_specifier class_name
        {
            $$ = MkNode<BaseSpecifier>();
            $$->access = $1;
            $$->nameSpec = $2;
            $$->className = $3;
            pc.PopQueryScopes();
        }
;

access_specifier:
    PRIVATE                     { $$ = Access::PRIVATE; }
|   PROTECTED                   { $$ = Access::PROTECTED; }
|   PUBLIC                      { $$ = Access::PUBLIC; }
;

/* ------------------------------------------------------------------------- *
 * 9. Special member function
 * ------------------------------------------------------------------------- */

conversion_function_id:
    OPERATOR conversion_type_id { $$ = $2; }
;

conversion_type_id:
    type_specifier_seq conversion_declarator_opt
        {
            $$ = MkNode<ConversionFunctionId>();
            $$->typeSpec = $1;
            $$->ptrSpec = $2;
        }
;

conversion_declarator:
    ptr_operator_list           { $$ = $1; }
;

ctor_initializer:
    ':' mem_initializer_list    { $$ = $2; }
;

mem_initializer_list:
    mem_initializer
        { $$.push_back($1); }
|   mem_initializer_list ',' mem_initializer
        { $$ = $1; $$.push_back($3); }
;

mem_initializer:
    mem_initializer_id '(' expression_list_opt ')'
        { $$ = $1; $$->exprList = $3; }
;

mem_initializer_id:
    class_name
        {
            $$ = MkNode<CtorMemberInitializer>();
            $$->identifier = $1;
            $$->isBaseCtor = true;
        }
|   nested_name_specifier class_name
        {
            $$ = MkNode<CtorMemberInitializer>();
            $$->nameSpec = $1;
            $$->identifier = $2;
            $$->isBaseCtor = true;
            pc.PopQueryScopes();
        }
|   identifier
        {
            $$ = MkNode<CtorMemberInitializer>();
            $$->identifier = $1;
            $$->isBaseCtor = false;
        }
;

/* ------------------------------------------------------------------------- *
 * 10. Operator overloading
 * ------------------------------------------------------------------------- */

operator_function_id:
    OPERATOR operator
        { $$ = MkNode<OperatorFunctionId>(); $$->overloadOp = $2; }
;

operator:
    '+'                         { $$ = Operator::ADD; }
|   '-'                         { $$ = Operator::SUB; }
|   '*'                         { $$ = Operator::MUL; }
|   '/'                         { $$ = Operator::DIV; }
|   '%'                         { $$ = Operator::MOD; }
|   '^'                         { $$ = Operator::XOR; }
|   '&'                         { $$ = Operator::AND; }
|   '|'                         { $$ = Operator::OR; }
|   '~'                         { $$ = Operator::NOT; }
|   '!'                         { $$ = Operator::LOGINOT; }
|   '='                         { $$ = Operator::ASSIGN; }
|   '<'                         { $$ = Operator::LT; }
|   '>'                         { $$ = Operator::GT; }
|   "+="                        { $$ = Operator::SELFADD; }
|   "-="                        { $$ = Operator::SELFSUB; }
|   "*="                        { $$ = Operator::SELFMUL; }
|   "/="                        { $$ = Operator::SELFDIV; }
|   "%="                        { $$ = Operator::SELFMOD; }
|   "^="                        { $$ = Operator::SELFXOR; }
|   "&="                        { $$ = Operator::SELFAND; }
|   "|="                        { $$ = Operator::SELFOR; }
|   "<<"                        { $$ = Operator::SHL; }
|   ">>"                        { $$ = Operator::SHR; }
|   "<<="                       { $$ = Operator::SELFSHL; }
|   ">>="                       { $$ = Operator::SELFSHR; }
|   "=="                        { $$ = Operator::EQ; }
|   "!="                        { $$ = Operator::NE; }
|   "<="                        { $$ = Operator::LE; }
|   ">="                        { $$ = Operator::GE; }
|   "&&"                        { $$ = Operator::LOGIAND; }
|   "||"                        { $$ = Operator::LOGIOR; }
|   "++"                        { $$ = Operator::SELFINC; }
|   "--"                        { $$ = Operator::SELFDEC; }
|   ','                         { $$ = Operator::COMMA; }
|   "->*"                       { $$ = Operator::ARROWSTAR; }
|   "->"                        { $$ = Operator::ARROW; }
|   '(' ')'                     { $$ = Operator::CALL; }
|   '[' ']'                     { $$ = Operator::SUBSCRIPT; }
;

/* ------------------------------------------------------------------------- *
 * A. Optional epsilon definition
 * ------------------------------------------------------------------------- */

COMMA_opt: | ';';

expression_list_opt:            { $$ = nullptr; }
|   expression_list             { $$ = $1; }
;

new_placememt_opt:              { $$ = nullptr; }
|   new_placememt               { $$ = $1; }
;

new_initializer_opt:            { $$ = nullptr; }
|   new_initializer             { $$ = $1; }
;

expression_opt:                 { $$ = nullptr; }
|   expression                  { $$ = $1; }
;

condition_opt:                  { $$ = nullptr; }
|   condition                   { $$ = $1; }
;

identifier_opt:                 { $$ = ""; }
|   identifier                  { $$ = $1; }
;

initializer_opt:                { $$ = nullptr; }
|   initializer                 { $$ = $1; }
;

cv_qualifier_opt:               { $$ = false; }
|   cv_qualifier                { $$ = true; }
;

abstract_declarator_opt:        { $$ = nullptr; }
|   abstract_declarator         { $$ = $1; }
;

constant_expression_opt:        { $$ = nullptr; }
|   constant_expression         { $$ = $1; }
;

assignment_expression_opt:      { $$ = nullptr; }
|   '=' assignment_expression   { $$ = $2; }
;

pure_specifier_opt:             { $$ = false; }
|   pure_specifier              { $$ = true; }
;

conversion_declarator_opt:      { $$ = nullptr; }
|   conversion_declarator       { $$ = $1; }
;

ctor_initializer_opt:           {}
|   ctor_initializer            { $$ = $1; }
;

%%

namespace yy {

void parser::error(const location_type& l, const std::string& msg) {
    std::cerr << msg << " at location " << l << '\n';
    errcnt++;
}

}