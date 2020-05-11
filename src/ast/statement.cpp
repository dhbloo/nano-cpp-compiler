#include "node.h"

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
    os << indent << "White语句:\n";
    os << indent + 1 << "条件:\n";
    condition->Print(os, indent + 2);
    os << indent + 1 << "语句:\n";
    stmt->Print(os, indent + 2);
}

void DoStatement::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Do-White语句:\n";
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

}  // namespace ast