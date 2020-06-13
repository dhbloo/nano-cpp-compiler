#pragma once

#include "../core/operator.h"
#include "../core/typeEnum.h"
#include "../parser/yylocation.h"

#include <cstdint>
#include <memory>
#include <ostream>
#include <typeinfo>
#include <vector>

struct CodegenContext;

namespace ast {

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

struct PtrSpecifier;
struct Declarator;
struct FunctionDeclarator;
struct ArrayDeclarator;
struct IdDeclarator;
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
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename V>
inline bool Is(V &&v)
{
    return typeid(v) == typeid(T);
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

                operator bool() const { return error; }
    std::string moveMsg() { return std::move(msg); }

private:
    bool        error;
    std::string msg;
};

/* ------------------------------------------------------------------------- *
 * 0. Node base
 * ------------------------------------------------------------------------- */

struct Node
{
    yy::location srcLocation;
    virtual void Print(std::ostream &os, Indent indent) const = 0;
    virtual void Codegen(CodegenContext &context) const       = 0;
};

/* ------------------------------------------------------------------------- *
 * 1. Basic concept
 * ------------------------------------------------------------------------- */

struct TranslationUnit : Node
{
    PtrVec<Declaration> decls;

    void Print(std::ostream &os, Indent indent = {}) const override;
    void Codegen(CodegenContext &context) const override;
};

struct NameSpecifier : Node
{
    std::vector<std::string> path;
    bool                     isGlobal;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

/* ------------------------------------------------------------------------- *
 * 2. Expression
 * ------------------------------------------------------------------------- */

struct Expression : Node
{};

struct AssignmentExpression : Expression
{
    AssignOp        op;
    Ptr<Expression> left, right;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ConditionalExpression : Expression
{
    Ptr<Expression> condition, trueExpr, falseExpr;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct BinaryExpression : Expression
{
    BinaryOp        op;
    Ptr<Expression> left, right;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct CastExpression : Expression
{
    Ptr<TypeId>     typeId;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct UnaryExpression : Expression
{
    UnaryOp         op;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct CallExpression : Expression
{
    Ptr<Expression>     funcExpr;
    Ptr<ExpressionList> params;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ConstructExpression : Expression
{
    Ptr<ElaboratedTypeSpecifier> type;
    Ptr<ExpressionList>          params;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct SizeofExpression : Expression
{
    Ptr<TypeId> typeId;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
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
    void Codegen(CodegenContext &context) const override;
};

struct InitializableNew : NewExpression
{
    Ptr<TypeSpecifier>  typeSpec;
    Ptr<PtrSpecifier>   ptrSpec;  // opt
    PtrVec<Expression>  arraySizes;
    Ptr<ExpressionList> initializer;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct DeleteExpression : Expression
{
    bool            isArray;
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct IdExpression : Expression
{
    enum SpecialType { NO, DESTRUCTOR, CONSTRUCTOR };

    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        identifier;
    SpecialType        stype;

    void                Print(std::ostream &os, Indent indent) const override;
    void                Codegen(CodegenContext &context) const override;
    virtual std::string ComposedId(CodegenContext &context) const;
};

struct ThisExpression : Expression
{
    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct LiteralExpression : Expression
{};

struct IntLiteral : LiteralExpression
{
    intmax_t value;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct FloatLiteral : LiteralExpression
{
    double value;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct CharLiteral : LiteralExpression
{
    char value;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct StringLiteral : LiteralExpression
{
    std::string value;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct BoolLiteral : LiteralExpression
{
    bool value;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ExpressionList : Node
{
    PtrVec<Expression> exprList;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
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
    void Codegen(CodegenContext &context) const override;
};

struct DefaultStatement : Statement
{
    Ptr<Statement> stmt;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ExpressionStatement : Statement
{
    Ptr<Expression> expr;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct CompoundStatement : Statement
{
    PtrVec<Statement> stmts;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
    int  CountCase() const;
};

struct IfStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  trueStmt;
    Ptr<Statement>  falseStmt;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct SwitchStatement : Statement
{
    Ptr<Expression>        condition;
    Ptr<CompoundStatement> stmt;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct WhileStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct DoStatement : Statement
{
    Ptr<Expression> condition;
    Ptr<Statement>  stmt;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
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
    void Codegen(CodegenContext &context) const override;
};

struct JumpStatement : Statement
{
    enum JType { BREAK, CONTINUE, RETURN };

    JType           type;
    Ptr<Expression> retExpr;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct DeclerationStatement : Statement
{
    Ptr<BlockDeclaration> decl;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
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
    void Codegen(CodegenContext &context) const override;
};

struct DeclSpecifier : Node
{
    enum DeclAttribute { NONE, STATIC, FRIEND, VIRTUAL, TYPEDEF };

    DeclAttribute      declAttr;
    Ptr<TypeSpecifier> typeSpec;  // opt for constructor/destructor/conversion

    friend SyntaxStatus
         Combine(Ptr<DeclSpecifier> n1, Ptr<DeclSpecifier> n2, Ptr<DeclSpecifier> &out);
    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct TypeSpecifier : Node
{
    CVQualifier cv;

    friend SyntaxStatus
         Combine(Ptr<TypeSpecifier> n1, Ptr<TypeSpecifier> n2, Ptr<TypeSpecifier> &out);
    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct SimpleTypeSpecifier : TypeSpecifier
{
    FundTypePart fundTypePart;

    FundType            GetFundType() const;
    friend SyntaxStatus Combine(Ptr<SimpleTypeSpecifier>  n1,
                                Ptr<SimpleTypeSpecifier>  n2,
                                Ptr<SimpleTypeSpecifier> &out);
    void                Print(std::ostream &os, Indent indent) const override;
    void                Codegen(CodegenContext &context) const override;
};

struct ElaboratedTypeSpecifier : TypeSpecifier
{
    enum ElaborateTypeClass { CLASSNAME, ENUMNAME, TYPEDEFNAME };
    ElaborateTypeClass typeKind;
    Ptr<NameSpecifier> nameSpec;  // opt
    std::string        typeName;

    bool operator==(const ElaboratedTypeSpecifier &other);
    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ClassTypeSpecifier : TypeSpecifier
{
    Ptr<ClassSpecifier> classType;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct EnumTypeSpecifier : TypeSpecifier
{
    Ptr<EnumSpecifier> enumType;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct EnumSpecifier : Node
{
    using Enumerator = std::pair<std::string, Ptr<Expression>>;

    std::string             identifier;  // opt
    std::vector<Enumerator> enumList;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

/* ------------------------------------------------------------------------- *
 * 5. Declarator
 * ------------------------------------------------------------------------- */

struct PtrSpecifier : Node
{
    struct PtrOp
    {
        PtrType            ptrType;
        CVQualifier        cv;
        Ptr<NameSpecifier> classNameSpec;
    };

    std::vector<PtrOp> ptrList;

    friend Ptr<PtrSpecifier> Merge(Ptr<PtrSpecifier> p1, Ptr<PtrSpecifier> p2);
    void                     Print(std::ostream &os, Indent indent) const override;
    void                     Codegen(CodegenContext &context) const override;
};

struct Declarator : Node
{
    Ptr<PtrSpecifier> ptrSpec;    // opt
    Ptr<Declarator>   innerDecl;  // opt

    void         Append(Ptr<Declarator> decl);
    void         MergePtrSpec(Ptr<PtrSpecifier> ptrSpec);
    virtual bool IsFunctionDecl() const;
    virtual bool IsTypeConversionDecl() const;
    void         Print(std::ostream &os, Indent indent) const override;
    void         Codegen(CodegenContext &context) const override;
};

struct FunctionDeclarator : Declarator
{
    PtrVec<ParameterDeclaration> params;
    CVQualifier                  funcCV;

    bool IsFunctionDecl() const;
    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ArrayDeclarator : Declarator
{
    Ptr<Expression> size;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct IdDeclarator : Declarator
{
    Ptr<IdExpression> id;

    bool IsFunctionDecl() const;
    bool IsTypeConversionDecl() const;
    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct TypeId : Node
{
    Ptr<TypeSpecifier> typeSpec;
    Ptr<Declarator>    abstractDecl;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ParameterDeclaration : Node
{
    Ptr<DeclSpecifier> declSpec;
    Ptr<Declarator>    decl;         // opt when abstract
    Ptr<Expression>    defaultExpr;  // opt

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct FunctionDefinition : Declaration
{
    Ptr<DeclSpecifier>            declSpec;  // opt for constructor/destructor/conversion
    Ptr<Declarator>               declarator;
    PtrVec<CtorMemberInitializer> ctorInitList;  // opt
    Ptr<CompoundStatement>        funcBody;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct Initializer : Node
{};

struct ClauseInitializer : Initializer
{};

struct AssignmentInitializer : ClauseInitializer
{
    Ptr<Expression> expr;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ListInitializer : ClauseInitializer
{
    PtrVec<ClauseInitializer> initList;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct ParenthesisInitializer : Initializer
{
    Ptr<ExpressionList> exprList;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
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

    Ptr<MemberList> memberList;

    void MoveDefaultMember();
    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct MemberList : Node
{
    PtrVec<MemberDeclaration> members, defaultMembers;

    size_t MemberCount() const;
    void   Reverse();
    void   MoveDefaultTo(Access access);
    void   Print(std::ostream &os, Indent indent) const override;
    void   Codegen(CodegenContext &context) const override;
};

struct MemberDeclaration : Node
{
    Access access;
    void   Codegen(CodegenContext &context) const override;
};

struct MemberDefinition : MemberDeclaration
{
    Ptr<DeclSpecifier>       declSpec;  // opt
    PtrVec<MemberDeclarator> decls;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct MemberDeclarator : Node
{
    Ptr<Declarator> decl;
    Ptr<Expression> constInit;  // opt
    bool            isPure;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

struct MemberFunction : MemberDeclaration
{
    Ptr<FunctionDefinition> func;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
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
    void Codegen(CodegenContext &context) const override;
};

/* ------------------------------------------------------------------------- *
 * 8. Special member function
 * ------------------------------------------------------------------------- */

struct ConversionFunctionId : IdExpression
{
    Ptr<TypeSpecifier> typeSpec;
    Ptr<PtrSpecifier>  ptrSpec;  // opt

    void        Print(std::ostream &os, Indent indent) const override;
    std::string ComposedId(CodegenContext &context) const override;
    void        Codegen(CodegenContext &context) const override;
};

struct CtorMemberInitializer : Node
{
    Ptr<NameSpecifier>  nameSpec;  // opt
    std::string         identifier;
    Ptr<ExpressionList> exprList;  // opt
    bool                isBaseCtor;

    void Print(std::ostream &os, Indent indent) const override;
    void Codegen(CodegenContext &context) const override;
};

/* ------------------------------------------------------------------------- *
 * 9. Operator overloading
 * ------------------------------------------------------------------------- */

struct OperatorFunctionId : IdExpression
{
    Operator overloadOp;

    void        Print(std::ostream &os, Indent indent) const override;
    std::string ComposedId(CodegenContext &context) const override;
};

}  // namespace ast