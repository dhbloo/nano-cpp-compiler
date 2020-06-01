#include "../core/semantic.h"
#include "../ast/node.h"

#include <cassert>

namespace ast {

void CaseStatement::Analysis(SemanticContext &context) const
{
    if (!context.stmt.isSwitchLevel)
        throw SemanticError("case statement is not in switch statement", srcLocation);

    constant->Analysis(context);
    if (!context.expr.isConstant)
        throw SemanticError("case expression is not an integral constant expression", srcLocation);

    if (!context.type.IsConvertibleTo(Type::IntType))
        throw SemanticError(context.type.Name() + " is not convertible to integral", srcLocation);

    stmt->Analysis(context);
}

void DefaultStatement::Analysis(SemanticContext &context) const
{
    if (!context.stmt.isSwitchLevel)
        throw SemanticError("default statement is not in switch statement", srcLocation);

    stmt->Analysis(context);
}

void ExpressionStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt          = context.stmt;
    context.stmt.keepScope = false;

    expr->Analysis(context);

    context.stmt = lastStmt;
}

void CompoundStatement::Analysis(SemanticContext &context) const
{
    SymbolTable localSymtab(context.symtab);
    auto        lastStmt = context.stmt;

    // Enter new local scope
    if (!context.stmt.keepScope)
        context.symtab = &localSymtab;

    for (const auto &stmt : stmts) {
        context.stmt.keepScope = false;
        try {
            stmt->Analysis(context);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
        // Restore previous stmt (when failure)
        context.stmt = lastStmt;
    }

    // Leave local scope
    if (!context.stmt.keepScope) {
        context.symtab = context.symtab->GetParent();

        if (context.printAllSymtab)
            localSymtab.Print(context.outputStream);
    }
}

void IfStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isSwitchLevel = false;

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(Type::BoolType))
        throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);

    trueStmt->Analysis(context);

    if (falseStmt)
        falseStmt->Analysis(context);

    context.stmt = lastStmt;
}

void SwitchStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isInSwitch    = true;
    context.stmt.isSwitchLevel = true;

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(Type::IntType))
        throw SemanticError(context.type.Name() + " is not convertible to integral", srcLocation);

    stmt->Analysis(context);

    context.stmt = lastStmt;
}

void WhileStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isSwitchLevel = false;
    context.stmt.isInLoop      = true;

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(Type::BoolType))
        throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);

    stmt->Analysis(context);

    context.stmt = lastStmt;
}

void DoStatement::Analysis(SemanticContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isSwitchLevel = false;
    context.stmt.isInLoop      = true;

    condition->Analysis(context);
    if (!context.type.IsConvertibleTo(Type::BoolType))
        throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);

    stmt->Analysis(context);

    context.stmt = lastStmt;
}

void ForStatement::Analysis(SemanticContext &context) const
{
    SymbolTable localSymtab(context.symtab);
    context.symtab = &localSymtab;

    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = true;
    context.stmt.isSwitchLevel = false;
    context.stmt.isInLoop      = true;

    if (initType == EXPR)
        exprInit->Analysis(context);
    else {
        auto lastDecl      = context.decl;
        context.decl.state = DeclState::MINDECL;

        declInit->Analysis(context);

        context.decl = lastDecl;
    }

    if (condition) {
        condition->Analysis(context);
        if (!context.type.IsConvertibleTo(Type::BoolType))
            throw SemanticError(context.type.Name() + " is not convertible to bool", srcLocation);
    }

    if (iterExpr)
        iterExpr->Analysis(context);

    stmt->Analysis(context);

    context.stmt   = lastStmt;
    context.symtab = context.symtab->GetParent();

    if (context.printAllSymtab)
        localSymtab.Print(context.outputStream);
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
        Type &retType = context.symtab->GetFunction()->retType;
        if (retExpr) {
            if (retType.typeClass == TypeClass::FUNDTYPE && retType.fundType == FundType::VOID)
                throw SemanticError("void function should not return a value", srcLocation);

            retExpr->Analysis(context);
            // TODO: check return type match func type
            if (!context.type.IsConvertibleTo(retType))
                throw SemanticError("connot convert type '" + context.type.Name()
                                        + "' to function return type '" + retType.Name() + "'",
                                    srcLocation);
        }
        else {
            if (retType.typeClass != TypeClass::FUNDTYPE || retType.fundType != FundType::VOID)
                throw SemanticError("non-void function should return a value", srcLocation);
        }

        break;
    }
}

void DeclerationStatement::Analysis(SemanticContext &context) const
{
    auto lastDecl = context.decl;
    context.decl  = {DeclState::LOCALDECL, false, false, {}};

    decl->Analysis(context);

    context.decl = lastDecl;
}

}  // namespace ast