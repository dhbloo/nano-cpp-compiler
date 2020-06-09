#include "../ast/node.h"
#include "../core/semantic.h"

#include <cassert>

namespace ast {

void CaseStatement::Analysis(SemanticContext &context) const
{
    if (!context.stmt.isSwitchLevel)
        throw SemanticError("case statement is not in switch statement", srcLocation);

    constant->Analysis(context);
    if (!context.expr.isConstant)
        throw SemanticError("case expression is not an integral constant expression",
                            srcLocation);

    if (!context.type.IsConvertibleTo(FundType::INT, &context.expr.constant))
        throw SemanticError("'" + context.type.Name()
                                + "' is not convertible to integral",
                            srcLocation);

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
    expr->Analysis(context);
}

void CompoundStatement::Analysis(SemanticContext &context) const
{
    SymbolTable     localSymtab(context.symtab);
    SemanticContext newContext(context);

    // Enter new local scope
    if (!newContext.stmt.keepScope)
        newContext.symtab = &localSymtab;

    // Restore point
    for (const auto &stmt : stmts) {
        // Restore previous stmt (when failure)
        newContext.decl.state     = DeclState::NODECL;
        newContext.stmt           = context.stmt;
        newContext.stmt.keepScope = false;
        try {
            stmt->Analysis(newContext);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }
    }

    // Leave local scope
    if (!context.stmt.keepScope) {
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
    if (!context.type.IsConvertibleTo(FundType::BOOL))
        throw SemanticError(context.type.Name() + " is not convertible to bool",
                            srcLocation);

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
    if (!context.type.IsConvertibleTo(FundType::INT))
        throw SemanticError(context.type.Name() + " is not convertible to integral",
                            srcLocation);

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
    if (!context.type.IsConvertibleTo(FundType::BOOL))
        throw SemanticError(context.type.Name() + " is not convertible to bool",
                            srcLocation);

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
    if (!context.type.IsConvertibleTo(FundType::BOOL))
        throw SemanticError(context.type.Name() + " is not convertible to bool",
                            srcLocation);

    stmt->Analysis(context);

    context.stmt = lastStmt;
}

void ForStatement::Analysis(SemanticContext &context) const
{
    SymbolTable     localSymtab(context.symtab);
    SemanticContext newContext(context);

    newContext.symtab             = &localSymtab;
    newContext.stmt.keepScope     = true;
    newContext.stmt.isSwitchLevel = false;
    newContext.stmt.isInLoop      = true;

    if (initType == EXPR)
        exprInit->Analysis(newContext);
    else {
        newContext.decl = {DeclState::MINDECL};
        declInit->Analysis(newContext);
        newContext.decl.state = DeclState::NODECL;
    }

    if (condition) {
        condition->Analysis(newContext);
        if (!newContext.type.IsConvertibleTo(FundType::BOOL))
            throw SemanticError(newContext.type.Name() + " is not convertible to bool",
                                srcLocation);
    }

    if (iterExpr)
        iterExpr->Analysis(newContext);

    stmt->Analysis(newContext);

    if (context.printAllSymtab)
        localSymtab.Print(context.outputStream);
}

void JumpStatement::Analysis(SemanticContext &context) const
{
    switch (type) {
    case BREAK:
        if (!context.stmt.isInLoop && !context.stmt.isInSwitch)
            throw SemanticError("break statement not in loop or switch statement",
                                srcLocation);
        break;
    case CONTINUE:
        if (!context.stmt.isInLoop)
            throw SemanticError("continue statement not in loop", srcLocation);
        break;
    default:
        Type &retType = context.symtab->GetCurrentFunction()->retType;
        if (retExpr) {
            if (retType.typeClass == TypeClass::FUNDTYPE
                && retType.fundType == FundType::VOID)
                throw SemanticError("void function should not return a value",
                                    srcLocation);

            retExpr->Analysis(context);
            
            if (!context.type.IsConvertibleTo(retType))
                throw SemanticError("connot convert type '" + context.type.Name()
                                        + "' to function return type '" + retType.Name()
                                        + "'",
                                    srcLocation);
        }
        else {
            if (retType.typeClass != TypeClass::FUNDTYPE
                || retType.fundType != FundType::VOID)
                throw SemanticError("non-void function should return a value",
                                    srcLocation);
        }

        break;
    }
}

void DeclerationStatement::Analysis(SemanticContext &context) const
{
    context.decl = {DeclState::LOCALDECL};
    decl->Analysis(context);
}

}  // namespace ast