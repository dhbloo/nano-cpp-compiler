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
struct ConstructExpression;
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
struct MemberDefinition;
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
            os << "  ";
        return os;
    }

private:
    int level;
};

struct SyntaxStatus
{
    SyntaxStatus() : error(false) {}
    template <typename T>
    SyntaxStatus(T msg) : error(true)
                        , msg(std::move(msg))
    {}
    // SyntaxStatus(std::string msg) : error(true), msg(std::move(msg)) {}

                operator bool() const { return error; }
    std::string moveMsg() { return std::move(msg); }

private:
    bool        error;
    std::string msg;
};

struct Node
{
    virtual void Print(std::ostream &os, Indent indent) const /* = 0*/ {}
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

    void Print(std::ostream &os, Indent indent) const override;
};

struct ConditionalExpression : Expression
{
    Ptr<Expression> condition, trueExpr, falseExpr;

    void Print(std::ostream &os, Indent indent) const override;
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

    void Print(std::ostream &os, Indent indent) const override;
};

struct ConstructExpression : Expression
{
    Ptr<ElaboratedTypeSpecifier> type;
    Ptr<ExpressionList>          params;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct SizeofExpression : Expression
{
    Ptr<TypeId> typeId;

    void Print(std::ostream &os, Indent indent) const override;
};

struct NewExpression : Expression
{
    Ptr<ExpressionList> placement;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct PlainNew : NewExpression
{
    Ptr<TypeId> typeId;

    void Print(std::ostream &os, Indent indent) const override;
};

struct InitializableNew : NewExpression
{
    Ptr<TypeSpecifier>  typeSpec;
    Ptr<PtrSpecifier>   ptrSpec;  // opt
    PtrVec<Expression>  arraySizes;
    Ptr<ExpressionList> initializer;  // opt

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
    enum SpecialType { NO, DESTRUCTOR, CONSTRUCTOR };

    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        identifier;
    SpecialType        stype;

    void Print(std::ostream &os, Indent indent) const override;
};

struct ThisExpression : Expression
{
    void Print(std::ostream &os, Indent indent) const override;
};

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

    void Print(std::ostream &os, Indent indent) const override;
};

struct DefaultStatement : Statement
{
    Ptr<Statement> stmt;

    void Print(std::ostream &os, Indent indent) const override;
};

struct ExpressionStatement : Statement
{
    Ptr<Expression> expr;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct CompoundStatement : Statement
{
    PtrVec<Statement> stmts;

    void Print(std::ostream &os, Indent indent) const override;
};

struct IfStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  trueStmt;
    Ptr<Statement>  falseStmt;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct SwitchStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;

    void Print(std::ostream &os, Indent indent) const override;
};

struct WhileStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;

    void Print(std::ostream &os, Indent indent) const override;
};

struct DoStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;

    void Print(std::ostream &os, Indent indent) const override;
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

    void Print(std::ostream &os, Indent indent) const override;
};

struct JumpStatement : Statement
{
    enum JType { BREAK, CONTINUE, RETURN };

    JType           type;
    Ptr<Expression> retExpr;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct DeclerationStatement : Statement
{
    Ptr<BlockDeclaration> decl;

    void Print(std::ostream &os, Indent indent) const override;
};

/* ------------------------------------------------------------------------- *
 * 4. Declaration
 * ------------------------------------------------------------------------- */

struct Declaration : Node
{};

struct BlockDeclaration : Declaration
{
    Ptr<DeclSpecifier> declSpec;

    struct InitDecl
    {
        Ptr<Declarator>  declarator;
        Ptr<Initializer> initializer;  // opt
    };
    std::vector<InitDecl> initDeclList;

    void Print(std::ostream &os, Indent indent) const override;
};

struct DeclSpecifier : Node
{
    bool               isStatic, isFriend, isVirtual, isTypedef;
    Ptr<TypeSpecifier> typeSpec;  // opt for constructor/destructor/conversion

    friend SyntaxStatus
         Combine(Ptr<DeclSpecifier> n1, Ptr<DeclSpecifier> n2, Ptr<DeclSpecifier> &out);
    void Print(std::ostream &os, Indent indent) const override;
};

struct TypeSpecifier : Node
{
    CVQualifier cv;

    friend SyntaxStatus
         Combine(Ptr<TypeSpecifier> n1, Ptr<TypeSpecifier> n2, Ptr<TypeSpecifier> &out);
    void Print(std::ostream &os, Indent indent) const override;
};

struct SimpleTypeSpecifier : TypeSpecifier
{
    FundTypePart fundTypePart;

    friend SyntaxStatus Combine(Ptr<SimpleTypeSpecifier>  n1,
                                Ptr<SimpleTypeSpecifier>  n2,
                                Ptr<SimpleTypeSpecifier> &out);
    void                Print(std::ostream &os, Indent indent) const override;
};

struct ElaboratedTypeSpecifier : TypeSpecifier
{
    enum TypeClass { CLASSNAME, ENUMNAME, TYPEDEFNAME };
    TypeClass          typeClass;
    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        typeName;

    bool operator==(const ElaboratedTypeSpecifier &other);
    void Print(std::ostream &os, Indent indent) const override;
};

struct ClassTypeSpecifier : TypeSpecifier
{
    Ptr<ClassSpecifier> classType;

    void Print(std::ostream &os, Indent indent) const override;
};

struct EnumTypeSpecifier : TypeSpecifier
{
    Ptr<EnumSpecifier> enumType;

    void Print(std::ostream &os, Indent indent) const override;
};

struct EnumSpecifier : Node
{
    using Enumerator = std::pair<std::string, Ptr<Expression>>;

    std::string             identifier;  // opt
    std::vector<Enumerator> enumList;

    void Print(std::ostream &os, Indent indent) const override;
};

/* ------------------------------------------------------------------------- *
 * 5. Declarator
 * ------------------------------------------------------------------------- */

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

    void Print(std::ostream &os, Indent indent) const override;
};

struct Declarator : Node
{
    Ptr<PtrSpecifier> ptrSpec;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct FunctionDeclarator : Declarator
{
    Ptr<Declarator>              retType;  // opt when abstract
    PtrVec<ParameterDeclaration> params;
    bool                         isFuncConst;

    void Print(std::ostream &os, Indent indent) const override;
};

struct ArrayDeclarator : Declarator
{
    Ptr<Declarator> elemType;  // opt when abstract
    Ptr<Expression> size;      // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct IdDeclarator : Declarator
{
    Ptr<IdExpression> id;

    void Print(std::ostream &os, Indent indent) const override;
};

struct NestedDeclarator : Declarator
{
    Ptr<Declarator> decl;

    void Print(std::ostream &os, Indent indent) const override;
};

struct TypeId : Node
{
    Ptr<TypeSpecifier> typeSpec;
    Ptr<Declarator>    abstractDecl;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct ParameterDeclaration : Node
{
    Ptr<DeclSpecifier> declSpec;
    Ptr<Declarator>    decl;         // opt when abstract
    Ptr<Expression>    defaultExpr;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct FunctionDefinition : Declaration
{
    Ptr<DeclSpecifier>            declSpec;  // opt for constructor/destructor/conversion
    Ptr<Declarator>               declarator;
    PtrVec<CtorMemberInitializer> ctorInitList;  // opt
    Ptr<CompoundStatement>        funcBody;

    void Print(std::ostream &os, Indent indent) const override;
};

struct Initializer : Node
{};

struct ClauseInitializer : Initializer
{};

struct AssignmentInitializer : ClauseInitializer
{
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
};

struct ListInitializer : ClauseInitializer
{
    PtrVec<ClauseInitializer> initList;

    void Print(std::ostream &os, Indent indent) const override;
};

struct ParenthesisInitializer : Initializer
{
    Ptr<ExpressionList> exprList;

    void Print(std::ostream &os, Indent indent) const override;
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
    void Print(std::ostream &os, Indent indent) const override;
};

struct MemberList : Node
{
    PtrVec<MemberDeclaration> publicMember, protectedMember, privateMember, defaultMember;

    std::size_t MemberCount() const;
    void        Reverse();
    void        MoveDefaultTo(Access access);
    void        Print(std::ostream &os, Indent indent) const override;
};

struct MemberDeclaration : Node
{};

struct MemberDefinition : MemberDeclaration
{
    Ptr<DeclSpecifier>       declSpec;  // opt
    PtrVec<MemberDeclarator> decls;

    void Print(std::ostream &os, Indent indent) const override;
};

struct MemberDeclarator : Node
{
    Ptr<Declarator> decl;
    Ptr<Expression> constInit;  // opt
    bool            isPure;
};

struct MemberFunction : MemberDeclaration
{
    Ptr<FunctionDefinition> func;

    void Print(std::ostream &os, Indent indent) const override;
};

/* ------------------------------------------------------------------------- *
 * 7. Derived class
 * ------------------------------------------------------------------------- */

struct BaseSpecifier : Node
{
    Access             access;
    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        className;

    void Print(std::ostream &os, Indent indent) const override;
};

/* ------------------------------------------------------------------------- *
 * 8. Special member function
 * ------------------------------------------------------------------------- */

struct ConversionFunctionId : IdExpression
{
    Ptr<TypeSpecifier> typeSpec;
    Ptr<PtrSpecifier>  ptrSpec;  // opt

    void Print(std::ostream &os, Indent indent) const override;
};

struct CtorMemberInitializer : Node
{
    Ptr<NameSpecifier>  nameSpec;  // opt
    std::string         identifier;
    Ptr<ExpressionList> exprList;  // opt
    bool                isBaseCtor;

    void Print(std::ostream &os, Indent indent) const override;
};

/* ------------------------------------------------------------------------- *
 * 9. Operator overloading
 * ------------------------------------------------------------------------- */

struct OperatorFunctionId : IdExpression
{
    Operator overloadOp;
    bool     isGlobal;

    void Print(std::ostream &os, Indent indent) const override;
};

}  // namespace ast