#pragma once

#include "../core/operator.h"
#include "../core/type.h"

#include <cstdint>
#include <memory>
#include <ostream>
#include <vector>

namespace ast {

struct Indent;
struct Node;

// Basic
struct TranslationUnit;
struct NameSpecifier;

// Expression

struct Expression;

struct IntLiteral;
struct FloatLiteral;
struct CharLiteral;
struct StringLiteral;
struct BoolLiteral;

struct LiteralExpression;
struct IdExpression;
struct ThisExpression;

struct UnaryExpression;
struct NewExpression;
struct DeleteExpression;
struct CastExpression;
struct BinaryExpression;
struct ConditionalExpression;
struct AssignmentExpression;

// Statement

struct Statement;
struct CaseStatement;
struct DefaultStatement;
struct ExpressionStatement;
struct CompoundStatement;
struct Condition;
struct IfStatement;
struct SwitchStatement;
struct IterationStatement;
struct WhileStatement;
struct DoStatement;
struct ForStatement;
struct JumpStatement;
struct DeclarationStatement;

// Declaration

struct Declaration;
struct BlockDeclaration;
struct DeclSpecifier;
struct EnumSpecifier;

// Declarator

struct InitDeclarator;
struct Declarator;
struct DirectDeclarator;
struct PtrOperator;

struct FunctionDefinition;

// Class

// Derived Class

// template alias helper
template <typename T>
using Ptr = std::unique_ptr<T>;

template <typename T>
using PtrVec = std::vector<Ptr<T>>;

}  // namespace ast

namespace ast {

struct Indent
{
    Indent(int l = 0) : level(l) {}

    Indent operator+(int i) const { return Indent {level + i}; }

    friend inline std::ostream &operator<<(std::ostream &os, Indent ind)
    {
        for (int i = 0; i < ind.level; i++)
            os << "    ";
    }

private:
    int level;
};

struct Node
{
    virtual void Print(std::ostream &os, Indent indent) const = 0;
};

/* ------------------------------------------------------------------------- *
 * 1. Basic concept
 * ------------------------------------------------------------------------- */

struct TranslationUnit : Node
{
    PtrVec<Declaration> decls;

    void Print(std::ostream &os, Indent indent) const override;
};

struct NameSpecifier : Node
{
    std::vector<std::string> path;
    bool                     global;

    void Print(std::ostream &os, Indent indent) const override;
};

/* ------------------------------------------------------------------------- *
 * 2. Expression
 * ------------------------------------------------------------------------- */

struct Expression : Node
{
    enum ValType { LVALUE, RVALUE };

    virtual ValType ValueType() const { return RVALUE; };
};

struct AssignmentExpression : Expression
{
    AssignOp        op;
    Ptr<Expression> left, right;

    ValType ValueType() const override { return LVALUE; }
};

struct ConditionalExpression : Expression
{
    Ptr<Expression> condition, trueExpr, falseExpr;
};

struct BinaryExpression : Expression
{
    BinaryOp        op;
    Ptr<Expression> left, right;

    void Print(std::ostream &os, Indent indent) const override;
};

struct CastExpression : Expression
{
    // TODO: type-id
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
};

struct UnaryExpression : Expression
{
    UnaryOp         op;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
};

struct NewExpression : Expression
{
    // TODO: type-id
    Ptr<Expression>    placement;
    PtrVec<Expression> initializer;

    void Print(std::ostream &os, Indent indent) const override;
};

struct DeleteExpression : Expression
{
    bool            isArray;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
};

struct IdExpression : Expression
{
    enum IdType { SIMPLE, OPERATOR_FUNC, CONVERSION, DESTRUCTOR };

    std::string        identifier;
    IdType             type;
    Ptr<NameSpecifier> nameSpec;
};

struct ThisExpression : Expression
{};

struct LiteralExpression : Expression
{
    ValType ValueType() const override { return LVALUE; }
};

struct IntLiteral : LiteralExpression
{
    std::intmax_t value;

    void Print(std::ostream &os, Indent indent) const override;
};

struct FloatLiteral : LiteralExpression
{
    double value;

    void Print(std::ostream &os, Indent indent) const override;
};

struct CharLiteral : LiteralExpression
{
    char value;

    void Print(std::ostream &os, Indent indent) const override;
};

struct StringLiteral : LiteralExpression
{
    std::string value;

    void Print(std::ostream &os, Indent indent) const override;
};

struct BoolLiteral : LiteralExpression
{
    bool value;

    void Print(std::ostream &os, Indent indent) const override;
};

/* ------------------------------------------------------------------------- *
 * 3. Statement
 * ------------------------------------------------------------------------- */

struct Statement : Node
{};

struct CaseStatement : Statement
{
    Ptr<Expression> constant;
    Ptr<Statement>  stmt;
};

struct DefaultStatement : Statement
{
    Ptr<Statement> stmt;
};

struct ExpressionStatement : Statement
{
    Ptr<Expression> expr;
};

struct CompoundStatement : Statement
{
    PtrVec<Statement> stmts;
};

struct Condition : Node
{
    bool            hasDecl;
    Ptr<Expression> expr;
    // TODO: decl
};

struct IfStatement : Statement
{
    Ptr<Condition> condition;
    Ptr<Statement> trueStmt, falseStmt;
};

struct SwitchStatement : Statement
{
    Ptr<Condition> condition;
    Ptr<Statement> stmt;
};

struct WhileStatement : Statement
{
    Ptr<Condition> condition;
    Ptr<Statement> stmt;
};

struct DoStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;
};

struct ForStatement : Statement
{
    // TODO: for-init
    Ptr<Condition>  condition;
    Ptr<Expression> iterExpr;
    Ptr<Statement>  stmt;
};

struct JumpStatement : Statement
{
    enum JType { BREAK, CONTINUE, RETURN };

    JType           type;
    Ptr<Expression> retExpr;
};

struct DeclerationStatement : Statement
{
    Ptr<BlockDeclaration> decl;
};

/* ------------------------------------------------------------------------- *
 * 4. Declaration
 * ------------------------------------------------------------------------- */

struct Declaration : Node
{};

struct BlockDeclaration : Declaration
{
    Ptr<DeclSpecifier>     declSpec;
    PtrVec<InitDeclarator> initDeclList;
};

struct DeclSpecifier : Node
{
    enum TypeClass { FUNDTYPE, CLASSTYPE, ENUMTYPE, TYPEDEFTYPE };

    TypeClass typeClass;
    FundType  fundType;
    bool      isConst, isFriend, isVirtual, isTypedef;

    Ptr<NameSpecifier> nameSpec;
    std::string        typeName;

    Ptr<Node> typeSpec;
};

struct EnumSpecifier
{
    using Enumerator = std::pair<std::string, Ptr<Expression>>;

    std::string             identifier;
    std::vector<Enumerator> enumList;
};

/* ------------------------------------------------------------------------- *
 * 5. Declarator
 * ------------------------------------------------------------------------- */

struct InitDeclarator : Node
{};



struct FunctionDefinition : Declaration
{};

/* ------------------------------------------------------------------------- *
 * 6. Class
 * ------------------------------------------------------------------------- */

struct Class

/* ------------------------------------------------------------------------- *
 * 7. Derived class
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * 8. Special member function
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * 9. Overloading
 * ------------------------------------------------------------------------- */

}  // namespace ast