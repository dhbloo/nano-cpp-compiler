#include "../core/semantic.h"
#include "node.h"

static const Type IntType {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::INT, nullptr};
static const Type BoolType {TypeClass::FUNDTYPE,
                            CVQualifier::NONE,
                            {},
                            {},
                            FundType::BOOL,
                            nullptr};

namespace ast {

void CaseStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Case标签:\n";
    os << indent + 1 << "Case常量:\n";
    constant->Print(os, indent + 2);
    os << indent + 1 << "语句:\n";
    stmt->Print(os, indent + 2);
}

void DefaultStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "默认标签:\n";
    stmt->Print(os, indent + 1);
}

void ExpressionStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "表达式语句:" << (expr ? "\n" : " (空)\n");
    if (expr) {
        expr->Print(os, indent + 1);
    }
}

void CompoundStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "复合语句:\n";

    for (std::size_t i = 0; i < stmts.size(); i++) {
        os << indent + 1 << "语句[" << i << "]:\n";
        stmts[i]->Print(os, indent + 2);
    }
}

void IfStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "If语句:\n";
    os << indent + 1 << "条件:\n";
    condition->Print(os, indent + 2);
    os << indent + 1 << "True语句:\n";
    trueStmt->Print(os, indent + 2);
    if (falseStmt) {
        os << indent + 1 << "False语句:\n";
        falseStmt->Print(os, indent + 2);
    }
}

void SwitchStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Switch语句:\n";
    os << indent + 1 << "条件:\n";
    condition->Print(os, indent + 2);
    os << indent + 1 << "语句:\n";
    stmt->Print(os, indent + 2);
}

void WhileStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "While语句:\n";
    os << indent + 1 << "条件:\n";
    condition->Print(os, indent + 2);
    os << indent + 1 << "语句:\n";
    stmt->Print(os, indent + 2);
}

void DoStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Do-While语句:\n";
    os << indent + 1 << "条件:\n";
    condition->Print(os, indent + 2);
    os << indent + 1 << "语句:\n";
    stmt->Print(os, indent + 2);
}

void ForStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "For语句:\n";

    if (initType == EXPR) {
        os << indent + 1 << "表达式初始化:\n";
        exprInit->Print(os, indent + 2);
    }
    else {
        os << indent + 1 << "声明初始化:\n";
        declInit->Print(os, indent + 2);
    }

    if (condition) {
        os << indent + 1 << "条件:\n";
        condition->Print(os, indent + 2);
    }

    if (iterExpr) {
        os << indent + 1 << "迭代表达式:\n";
        iterExpr->Print(os, indent + 2);
    }

    os << indent + 1 << "语句:\n";
    stmt->Print(os, indent + 2);
}

void JumpStatement::Print(std::ostream &os, Indent indent) const
{
    if (type == BREAK) {
        os << indent << "Break语句\n";
        return;
    }
    else if (type == CONTINUE) {
        os << indent << "Continue语句\n";
        return;
    }

    os << indent << "返回语句:" << (retExpr ? "\n" : " (无返回值)\n");
    if (retExpr) {
        retExpr->Print(os, indent + 1);
    }
}

void DeclerationStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "声明语句:\n";
    decl->Print(os, indent + 1);
}

void CaseStatement::Analysis(SemanticContext &context) const
{
    if (!context.stmt.isInSwitch)
        throw new SemanticError("case statement is not in switch statement", srcLocation);

    constant->Analysis(context);
    if (!context.expr.isConstant)
        throw new SemanticError("case expression is not an integral constant expression",
                                srcLocation);

    if (!context.type.IsConvertibleTo(IntType))
        throw new SemanticError(context.type.Name() + " is not convertible to integral",
                                srcLocation);

    stmt->Analysis(context);
}

void DefaultStatement::Analysis(SemanticContext &context) const
{
    if (!context.stmt.isInSwitch)
        throw new SemanticError("default statement is not in switch statement", srcLocation);

    stmt->Analysis(context);
}

void ExpressionStatement::Analysis(SemanticContext &context) const
{
    expr->Analysis(context);
}

void CompoundStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt = context.stmt;

    for (const auto &stmt : stmts) {
        try {
            context.stmt.isInSwitch = false;
            stmt->Analysis(context);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
    }

    context.stmt = lastStmt;
}

void IfStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt           = context.stmt;
    context.stmt.isInSwitch = false;

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(BoolType))
        throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);

    trueStmt->Analysis(context);

    if (falseStmt)
        falseStmt->Analysis(context);

    context.stmt = lastStmt;
}

void SwitchStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt           = context.stmt;
    context.stmt.isInSwitch = true;

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(IntType))
        throw SemanticError(context.type.Name() + " is not convertible to integral", srcLocation);

    stmt->Analysis(context);

    context.stmt = lastStmt;
}

void WhileStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt           = context.stmt;
    context.stmt.isInSwitch = false;
    context.stmt.isInLoop   = true;

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(BoolType))
        throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);

    stmt->Analysis(context);

    context.stmt = lastStmt;
}

void DoStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt           = context.stmt;
    context.stmt.isInSwitch = false;
    context.stmt.isInLoop   = true;

    stmt->Analysis(context);

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(BoolType))
        throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);

    context.stmt = lastStmt;
}

void ForStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt           = context.stmt;
    context.stmt.isInSwitch = false;
    context.stmt.isInLoop   = true;

    if (initType == EXPR) {
        exprInit->Analysis(context);
    }
    else {
        declInit->Analysis(context);
    }

    if (condition) {
        condition->Analysis(context);
        if (!context.type.IsConvertibleTo(BoolType))
            throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);
    }

    if (iterExpr)
        iterExpr->Analysis(context);

    stmt->Analysis(context);

    context.stmt = lastStmt;
}

void JumpStatement::Analysis(SemanticContext &context) const
{
    switch (type) {
    case BREAK:
        if (!context.stmt.isInLoop && !context.stmt.isInSwitch)
            throw SemanticError("break statement not in loop or switch statement", srcLocation);
        break;
    case CONTINUE:
        if (!context.stmt.isInLoop)
            throw SemanticError("continue statement not in loop", srcLocation);
        break;
    default:
        if (!context.stmt.isInFunction)
            throw SemanticError("return statement not in function", srcLocation);
        break;
    }
}

void DeclerationStatement::Analysis(SemanticContext &context) const {}

}  // namespace ast