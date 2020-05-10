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
struct AssignmentExpression;
struct ConditionalExpression;
struct BinaryExpression;
struct CastExpression;
struct UnaryExpression;
struct CallExpression;
struct SizeofExpression;
struct NewExpression;
struct PlainNew;
struct InitializableNew;
struct DeleteExpression;
struct IdExpression;
struct ThisExpression;

struct LiteralExpression;
struct IntLiteral;
struct FloatLiteral;
struct CharLiteral;
struct StringLiteral;
struct BoolLiteral;

struct ExpressionList;

// Statement

struct Statement;
struct CaseStatement;
struct DefaultStatement;
struct ExpressionStatement;
struct CompoundStatement;
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
struct SimpleTypeSpecifier;
struct ElaboratedTypeSpecifier;
struct ClassTypeSpecifier;
struct EnumTypeSpecifier;
struct EnumSpecifier;

// Declarator

struct InitDeclarator;
struct PtrSpecifier;
struct Declarator;
struct FunctionDeclarator;
struct ArrayDeclarator;
struct IdDeclarator;
struct NestedDeclarator;
struct TypeId;
struct ParameterDeclaration;
struct FunctionDefinition;
struct Initializer;
struct ClauseInitializer;
struct AssignmentInitializer;
struct ListInitializer;
struct ParenthesisInitializer;

// Class

struct ClassSpecifier;
struct MemberList;
struct MemberDeclaration;
struct MemberVariable;
struct MemberDeclarator;
struct MemberFunction;

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
    virtual void Print(std::ostream &os, Indent indent) const /* = 0*/;
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
    Ptr<TypeId>     typeId;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
};

struct UnaryExpression : Expression
{
    UnaryOp         op;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
};

struct CallExpression : Expression
{
    Ptr<Expression>     funcExpr;
    Ptr<ExpressionList> params;  // opt
};

struct SizeofExpression : Expression
{
    Ptr<TypeId> typeId;
};

struct NewExpression : Expression
{
    Ptr<ExpressionList> placement;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct PlainNew : NewExpression
{
    Ptr<TypeId> typeId;
};

struct InitializableNew : NewExpression
{
    Ptr<TypeSpecifier>  typeSpec;
    Ptr<PtrSpecifier>   ptrSpec;  // opt
    PtrVec<Expression>  arraySizes;
    Ptr<ExpressionList> initializer;  // opt
};

struct DeleteExpression : Expression
{
    bool            isArray;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
};

struct IdExpression : Expression
{
    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        identifier;
    bool               isDestructor;
};

struct DestructorExpression : Expression
{
    Ptr<NameSpecifier>           nameSpec;  // opt
    Ptr<ElaboratedTypeSpecifier> typeName;
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

struct ExpressionList : Node
{
    PtrVec<Expression> exprList;
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
    Ptr<Expression> expr;  // opt
};

struct CompoundStatement : Statement
{
    PtrVec<Statement> stmts;
};

struct IfStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  trueStmt, falseStmt;
};

struct SwitchStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;
};

struct WhileStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;
};

struct DoStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;
};

struct ForStatement : Statement
{
    enum InitType { EXPR, DECL };

    // for-init
    InitType                 initType;
    Ptr<ExpressionStatement> exprInit;
    Ptr<BlockDeclaration>    declInit;

    Ptr<Expression> condition;  // opt
    Ptr<Expression> iterExpr;   // opt
    Ptr<Statement>  stmt;
};

struct JumpStatement : Statement
{
    enum JType { BREAK, CONTINUE, RETURN };

    JType           type;
    Ptr<Expression> retExpr;  // opt
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

    SyntaxStatus Combine(Ptr<DeclSpecifier> other);
};

struct TypeSpecifier : Node
{
    CVQualifier cv;

    SyntaxStatus Combine(Ptr<TypeSpecifier> other);
};

struct SimpleTypeSpecifier : TypeSpecifier
{
    FundTypePart fundTypePart;
};

struct ElaboratedTypeSpecifier : TypeSpecifier
{
    enum TypeClass { CLASSNAME, ENUMNAME, TYPEDEFNAME };
    TypeClass          typeClass;
    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        typeName;

    bool operator==(const ElaboratedTypeSpecifier &other);
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

    std::string             identifier;  // opt
    std::vector<Enumerator> enumList;
};

/* ------------------------------------------------------------------------- *
 * 5. Declarator
 * ------------------------------------------------------------------------- */

struct InitDeclarator : Node
{
    Ptr<Declarator>  declarator;
    Ptr<Initializer> initializer;  // opt
};

struct PtrSpecifier : Node
{
    enum PtrType { PTR, REF, CLASSPTR };

    struct PtrOp
    {
        PtrType            ptrType;
        bool               isPtrConst;
        Ptr<NameSpecifier> classNameSpec;
    };

    std::vector<PtrOp> ptrList;
};

struct Declarator : Node
{
    Ptr<PtrSpecifier> ptrSpec;  // opt
};

struct FunctionDeclarator : Declarator
{
    Ptr<Declarator>              retType;  // opt when abstract
    PtrVec<ParameterDeclaration> params;
    bool                         isFuncConst;
};

struct ArrayDeclarator : Declarator
{
    Ptr<Declarator> elemType;  // opt when abstract
    Ptr<Expression> size;
};

struct IdDeclarator : Declarator
{
    Ptr<IdExpression> id;
};

struct NestedDeclarator : Declarator
{
    Ptr<Declarator> decl;
};

struct TypeId : Node
{
    Ptr<TypeSpecifier> typeSpec;
    Ptr<Declarator>    abstractDecl;  // opt
};

struct ParameterDeclaration : Node
{
    Ptr<DeclSpecifier> declSpec;
    Ptr<Declarator>    decl;         // opt when abstract
    Ptr<Expression>    defaultExpr;  // opt
};

struct FunctionDefinition : Declaration
{
    Ptr<DeclSpecifier>            declSpec;  // opt for constructor/destructor/conversion
    Ptr<Declarator>               declarator;
    PtrVec<CtorMemberInitializer> ctorInitList;  // opt
    Ptr<CompoundStatement>        funcBody;
};

struct Initializer : Node
{};

struct ClauseInitializer : Initializer
{};

struct AssignmentInitializer : ClauseInitializer
{
    Ptr<Expression> expr;
};

struct ListInitializer : ClauseInitializer
{
    PtrVec<ClauseInitializer> initList;
};

struct ParenthesisInitializer : Initializer
{
    Ptr<ExpressionList> exprList;
};

/* ------------------------------------------------------------------------- *
 * 6. Class
 * ------------------------------------------------------------------------- */

struct ClassSpecifier : Node
{
    enum Key { CLASS, STRUCT };

    Key                key;
    Ptr<NameSpecifier> nameSpec;    // opt
    std::string        identifier;  // opt
    Ptr<BaseSpecifier> baseSpec;    // opt

    Ptr<MemberList> members;

    void MoveDefaultMember();
};

struct MemberList : Node
{
    PtrVec<MemberDeclaration> publicMember, protectedMember, privateMember, defaultMember;

    void MoveDefaultTo(Access access);
};

struct MemberDeclaration : Node
{};

struct MemberVariable : MemberDeclaration
{
    Ptr<DeclSpecifier>       declSpec;
    PtrVec<MemberDeclarator> decls;
};

struct MemberDeclarator : Node
{
    Ptr<Declarator> decl;
    Ptr<Expression> constInit;
    bool            isPure;
};

struct MemberFunction : MemberDeclaration
{
    Ptr<FunctionDefinition> func;
};

/* ------------------------------------------------------------------------- *
 * 7. Derived class
 * ------------------------------------------------------------------------- */

struct BaseSpecifier : Node
{
    Access             access;
    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        className;
};

/* ------------------------------------------------------------------------- *
 * 8. Special member function
 * ------------------------------------------------------------------------- */

struct ConversionFunctionId : IdExpression
{
    Ptr<TypeSpecifier> typeSpec;
    Ptr<PtrSpecifier>  ptrSpec;  // opt
};

struct CtorMemberInitializer : Node
{
    Ptr<NameSpecifier>  nameSpec;  // opt
    std::string         identifier;
    Ptr<ExpressionList> exprList;  // opt
    bool                isBaseCtor;
};

/* ------------------------------------------------------------------------- *
 * 9. Operator overloading
 * ------------------------------------------------------------------------- */

struct OperatorFunctionId : IdExpression
{
    Operator overloadOp;
    bool     isGlobal;
};

}  // namespace ast