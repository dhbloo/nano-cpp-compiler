#pragma once

#include "../core/operator.h"
#include "../core/type.h"

#include <cstdint>
#include <memory>
#include <ostream>
#include <vector>

namespace ast {

struct Indent;
struct SyntaxStatus;
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
struct SizeofExpression;

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
struct TypeSpecifier;
struct EnumSpecifier;

// Declarator

struct InitDeclarator;
struct PtrSpecifier;
struct AbstractDeclarator;
struct Declarator;
struct TypeId;
struct ParameterDeclaration;
struct FunctionDefinition;
struct Initializer;
struct InitializerClause;

// Class

struct ClassSpecifier;
struct MemberDeclaration;
struct MemberVariable;
struct MemberDeclarator;
struct MemberFunction;
struct MemberIdentifier;

// Derived class

struct BaseSpecifier;

// Special member function

struct ConversionFunctionId;
struct CtorMemberInitializer;

// Operator overloading

struct OperatorFunctionId;

// template alias helper
template <typename T>
using Ptr = std::unique_ptr<T>;

template <typename T>
using PtrVec = std::vector<Ptr<T>>;

template <typename T, typename... Args>
inline Ptr<T> MkNode(Args &&... args)
{
    return std::move(std::unique_ptr<T> {new T {std::forward<Args>(args)...}});
}

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
        return os;
    }

private:
    int level;
};

struct SyntaxStatus
{
    SyntaxStatus() : error(false) {}
    SyntaxStatus(std::string msg) : error(true), msg(std::move(msg)) {}

                operator bool() const { return error; }
    std::string moveMsg() { return std::move(msg); }

private:
    bool        error;
    std::string msg;
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

    void Print(std::ostream &os, Indent indent = {}) const override;
};

struct NameSpecifier : Node
{
    std::vector<std::string> path;
    bool                     isGlobal;

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

struct SizeofExpression : Expression
{
    Ptr<TypeId> typeId;
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
    Ptr<NameSpecifier> nameSpec;
    std::string        identifier;
    bool               isDestructor;
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
    bool               isFriend, isVirtual, isTypedef;
    Ptr<TypeSpecifier> typeSpec;

    SyntaxStatus combine(Ptr<DeclSpecifier> other);
};

struct TypeSpecifier : Node
{
    CVQualifier cv;
};

struct SimpleTypeSpecifier : TypeSpecifier
{
    FundTypePart fundTypePart;
};

struct ElaboratedTypeSpecifier : TypeSpecifier
{
    enum TypeClass { CLASSNAME, ENUMNAME, TYPEDEFNAME };
    TypeClass          typeClass;
    Ptr<NameSpecifier> nameSpec;
    std::string        typeName;
};

struct ClassTypeSpecifier : TypeSpecifier
{
    Ptr<ClassSpecifier> classType;
};

struct EnumTypeSpecifier : TypeSpecifier
{
    Ptr<EnumSpecifier> enumType;
};

struct EnumSpecifier : Node
{
    using Enumerator = std::pair<std::string, Ptr<Expression>>;

    std::string             identifier;
    std::vector<Enumerator> enumList;
};

/* ------------------------------------------------------------------------- *
 * 5. Declarator
 * ------------------------------------------------------------------------- */

struct InitDeclarator : Node
{
    Ptr<Declarator>  declarator;
    Ptr<Initializer> initializer;
};

struct PtrSpecifier : Node
{
    enum PtrType { NO, PTR, REF, CLASSPTR };

    PtrType            ptrType;
    bool               isPtrConst;
    Ptr<NameSpecifier> classNameSpec;
};

struct AbstractDeclarator : Node
{
    Ptr<PtrSpecifier> ptrSpec;

    // function
    PtrVec<ParameterDeclaration> funcParamList;
    bool                         isFuncConst;

    // array
    Ptr<Expression>         arraySize;
    Ptr<AbstractDeclarator> arrayDecl;
};

struct Declarator : AbstractDeclarator
{
    enum IdType { IDEXPR, TYPENAME };

    IdType            idType;
    Ptr<IdExpression> declId;

    Ptr<NameSpecifier> typeNameSpec;
    std::string        typeName;
};

struct TypeId : Node
{
    Ptr<TypeSpecifier>      typeSpec;
    Ptr<AbstractDeclarator> abstractDecl;
};

struct ParameterDeclaration : Node
{
    Ptr<DeclSpecifier>        declSpec;
    Ptr<AbstractDeclarator>   decl;
    Ptr<AssignmentExpression> assignExpr;
};

struct FunctionDefinition : Declaration
{
    Ptr<DeclSpecifier>            declSpec;
    Ptr<Declarator>               declarator;
    PtrVec<CtorMemberInitializer> ctorInitList;
    Ptr<CompoundStatement>        funcBody;
};

struct Initializer : Node
{
    // type 1 init
    Ptr<InitializerClause> init;

    // type 2 init
    PtrVec<Expression> exprList;
};

struct InitializerClause : Node
{
    Ptr<AssignmentExpression> assignExpr;
    PtrVec<InitializerClause> initList;
};

/* ------------------------------------------------------------------------- *
 * 6. Class
 * ------------------------------------------------------------------------- */

struct ClassSpecifier : Node
{
    enum Key { CLASS, STRUCT };

    Key                key;
    Ptr<NameSpecifier> nameSpec;
    std::string        identifier;
    Ptr<BaseSpecifier> baseSpec;

    PtrVec<MemberDeclaration> publicMember, protectedMember, privateMember;
};

struct MemberDeclaration : Node
{};

struct MemberVariable : MemberDeclaration
{
    Ptr<DeclSpecifier> declSpec;
};

struct MemberDeclarator : Node
{
    Ptr<Declaration> decl;
    Ptr<Expression>  constInit;
    bool             isPure;
};

struct MemberFunction : MemberDeclaration
{
    Ptr<FunctionDefinition> func;
};

struct MemberIdentifier : MemberDeclaration
{
    Ptr<NameSpecifier> nameSpec;
    std::string        identifier;
};

/* ------------------------------------------------------------------------- *
 * 7. Derived class
 * ------------------------------------------------------------------------- */

struct BaseSpecifier : Node
{
    enum AccessSpec { DEFAULT, PRIVATE, PROTECTED, PUBLIC };

    AccessSpec         accessSpec;
    Ptr<NameSpecifier> nameSpec;
    std::string        className;
};

/* ------------------------------------------------------------------------- *
 * 8. Special member function
 * ------------------------------------------------------------------------- */

struct ConversionFunctionId : IdExpression
{
    Ptr<TypeSpecifier> typeSpec;
    Ptr<PtrSpecifier>  ptrSpec;
};

struct CtorMemberInitializer : Node
{
    Ptr<NameSpecifier> nameSpec;
    std::string        identifier;
    PtrVec<Expression> exprList;
};

/* ------------------------------------------------------------------------- *
 * 9. Operator overloading
 * ------------------------------------------------------------------------- */

struct OperatorFunctionId : IdExpression
{
    OverloadOperator overloadOp;
    bool             isGlobal;
};

}  // namespace ast