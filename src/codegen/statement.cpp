#include "../ast/node.h"
#include "context.h"

#include <cassert>

namespace ast {

void CaseStatement::Codegen(CodegenContext &context) const
{
    if (!context.stmt.isSwitchLevel)
        throw SemanticError("case statement is not in switch statement", srcLocation);

    constant->Codegen(context);
    if (!context.expr.isConstant)
        throw SemanticError("case expression is not an integral constant expression",
                            srcLocation);

    if (!context.type.IsConvertibleTo(FundType::INT, &context.expr.constant))
        throw SemanticError("'" + context.type.Name()
                                + "' is not convertible to integral",
                            srcLocation);

    auto function    = context.IRBuilder.GetInsertBlock()->getParent();
    auto caseBB      = llvm::BasicBlock::Create(context.llvmContext, "case", function);
    auto constantInt = llvm::cast<llvm::ConstantInt>(
        context.cgHelper.CreateConstant(FundType::INT, context.expr.constant));

    context.stmt.switchInst->addCase(constantInt, caseBB);
    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(caseBB);
    context.IRBuilder.SetInsertPoint(caseBB);

    stmt->Codegen(context);
}

void DefaultStatement::Codegen(CodegenContext &context) const
{
    if (!context.stmt.isSwitchLevel)
        throw SemanticError("default statement is not in switch statement", srcLocation);

    auto defaultBB = context.stmt.switchInst->getDefaultDest();

    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(defaultBB);
    context.IRBuilder.SetInsertPoint(defaultBB);

    stmt->Codegen(context);
}

void ExpressionStatement::Codegen(CodegenContext &context) const
{
    expr->Codegen(context);
}

void CompoundStatement::Codegen(CodegenContext &context) const
{
    SymbolTable    localSymtab(context.symtab);
    CodegenContext newContext(context);

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
            stmt->Codegen(newContext);
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }

        // If the block has ended, and there are still statements unprocessed,
        // create a new block for the following statements. This new block is
        // unreachable and can be further eliminated.
        if (context.IRBuilder.GetInsertBlock()->getTerminator() && stmt != stmts.back()) {
            auto function = context.IRBuilder.GetInsertBlock()->getParent();
            auto nextBB   = llvm::BasicBlock::Create(context.llvmContext, "", function);
            context.IRBuilder.SetInsertPoint(nextBB);

            // Explicitly create an unreachable instruction
            // context.IRBuilder.CreateUnreachable();
        }
    }

    // Leave local scope
    if (!context.stmt.keepScope) {
        if (context.printLocalTable)
            localSymtab.Print(context.outputStream);
    }
}

int CompoundStatement::CountCase() const
{
    int count = 0;
    for (const auto &stmt : stmts) {
        if (Is<CaseStatement>(*stmt))
            count++;
    }
    return count;
}

void IfStatement::Codegen(CodegenContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isSwitchLevel = false;

    condition->Codegen(context);
    if (!context.type.IsConvertibleTo(FundType::BOOL, context.expr.constOrNull()))
        throw SemanticError(context.type.Name() + " is not convertible to bool",
                            srcLocation);

    context.expr =
        context.cgHelper.CreateValue(context.type, FundType::BOOL, context.expr);

    auto function = context.IRBuilder.GetInsertBlock()->getParent();
    auto thenBB   = llvm::BasicBlock::Create(context.llvmContext, "if.then", function);
    auto elseBB   = llvm::BasicBlock::Create(context.llvmContext, "if.else");
    auto mergeBB  = llvm::BasicBlock::Create(context.llvmContext, "if.merge");
    context.IRBuilder.CreateCondBr(context.expr.value, thenBB, elseBB);

    context.IRBuilder.SetInsertPoint(thenBB);
    trueStmt->Codegen(context);
    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(mergeBB);

    function->getBasicBlockList().push_back(elseBB);
    context.IRBuilder.SetInsertPoint(elseBB);
    if (falseStmt) {
        falseStmt->Codegen(context);
    }
    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(mergeBB);

    function->getBasicBlockList().push_back(mergeBB);
    context.IRBuilder.SetInsertPoint(mergeBB);

    context.stmt = lastStmt;
}

void SwitchStatement::Codegen(CodegenContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isInSwitch    = true;
    context.stmt.isSwitchLevel = true;

    auto function  = context.IRBuilder.GetInsertBlock()->getParent();
    auto defaultBB = llvm::BasicBlock::Create(context.llvmContext, "switch.default");
    auto endBB     = llvm::BasicBlock::Create(context.llvmContext, "switch.end");

    condition->Codegen(context);
    if (!context.type.IsConvertibleTo(FundType::INT, context.expr.constOrNull()))
        throw SemanticError(context.type.Name() + " is not convertible to integral",
                            srcLocation);

    context.expr =
        context.cgHelper.CreateValue(context.type, FundType::INT, context.expr);

    context.stmt.switchInst =
        context.IRBuilder.CreateSwitch(context.expr.value, defaultBB, stmt->CountCase());
    context.stmt.breakBB = endBB;

    stmt->Codegen(context);

    function->getBasicBlockList().push_back(defaultBB);
    context.IRBuilder.SetInsertPoint(defaultBB);
    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(endBB);

    function->getBasicBlockList().push_back(endBB);
    context.IRBuilder.SetInsertPoint(endBB);

    context.stmt = lastStmt;
}

void WhileStatement::Codegen(CodegenContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isSwitchLevel = false;
    context.stmt.isInLoop      = true;

    auto function = context.IRBuilder.GetInsertBlock()->getParent();
    auto condBB   = llvm::BasicBlock::Create(context.llvmContext, "while.cond", function);
    auto loopBB   = llvm::BasicBlock::Create(context.llvmContext, "while.loop");
    auto endBB    = llvm::BasicBlock::Create(context.llvmContext, "while.end");
    context.stmt.breakBB    = endBB;
    context.stmt.continueBB = condBB;

    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(condBB);
    context.IRBuilder.SetInsertPoint(condBB);

    condition->Codegen(context);
    if (!context.type.IsConvertibleTo(FundType::BOOL, context.expr.constOrNull()))
        throw SemanticError(context.type.Name() + " is not convertible to bool",
                            srcLocation);

    context.expr =
        context.cgHelper.CreateValue(context.type, FundType::BOOL, context.expr);

    context.IRBuilder.CreateCondBr(context.expr.value, loopBB, endBB);
    function->getBasicBlockList().push_back(loopBB);
    context.IRBuilder.SetInsertPoint(loopBB);

    stmt->Codegen(context);

    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(condBB);
    function->getBasicBlockList().push_back(endBB);
    context.IRBuilder.SetInsertPoint(endBB);

    context.stmt = lastStmt;
}

void DoStatement::Codegen(CodegenContext &context) const
{
    auto lastStmt              = context.stmt;
    context.stmt.keepScope     = false;
    context.stmt.isSwitchLevel = false;
    context.stmt.isInLoop      = true;

    auto function = context.IRBuilder.GetInsertBlock()->getParent();
    auto loopBB   = llvm::BasicBlock::Create(context.llvmContext, "do.loop", function);
    auto condBB   = llvm::BasicBlock::Create(context.llvmContext, "do.cond");
    auto endBB    = llvm::BasicBlock::Create(context.llvmContext, "do.end");
    context.stmt.breakBB    = endBB;
    context.stmt.continueBB = condBB;

    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(loopBB);
    context.IRBuilder.SetInsertPoint(loopBB);

    stmt->Codegen(context);

    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(condBB);
    function->getBasicBlockList().push_back(condBB);
    context.IRBuilder.SetInsertPoint(condBB);

    condition->Codegen(context);
    if (!context.type.IsConvertibleTo(FundType::BOOL, context.expr.constOrNull()))
        throw SemanticError(context.type.Name() + " is not convertible to bool",
                            srcLocation);

    context.expr =
        context.cgHelper.CreateValue(context.type, FundType::BOOL, context.expr.value);

    context.IRBuilder.CreateCondBr(context.expr.value, loopBB, endBB);
    function->getBasicBlockList().push_back(endBB);
    context.IRBuilder.SetInsertPoint(endBB);

    context.stmt = lastStmt;
}

void ForStatement::Codegen(CodegenContext &context) const
{
    SymbolTable    localSymtab(context.symtab);
    CodegenContext newContext(context);

    newContext.symtab             = &localSymtab;
    newContext.stmt.keepScope     = true;
    newContext.stmt.isSwitchLevel = false;
    newContext.stmt.isInLoop      = true;

    if (initType == EXPR)
        exprInit->Codegen(newContext);
    else {
        newContext.decl = {DeclState::MINDECL};
        declInit->Codegen(newContext);
        newContext.decl.state = DeclState::NODECL;
    }

    auto function = context.IRBuilder.GetInsertBlock()->getParent();
    auto condBB   = llvm::BasicBlock::Create(context.llvmContext, "for.cond", function);
    auto iterBB   = llvm::BasicBlock::Create(context.llvmContext, "for.iter");
    auto loopBB   = llvm::BasicBlock::Create(context.llvmContext, "for.loop");
    auto endBB    = llvm::BasicBlock::Create(context.llvmContext, "for.end");
    newContext.stmt.breakBB    = endBB;
    newContext.stmt.continueBB = iterBB;

    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(condBB);
    context.IRBuilder.SetInsertPoint(condBB);

    if (condition) {
        condition->Codegen(newContext);
        if (!newContext.type.IsConvertibleTo(FundType::BOOL,
                                             newContext.expr.constOrNull()))
            throw SemanticError(newContext.type.Name() + " is not convertible to bool",
                                srcLocation);

        if (newContext.expr.isConstant) {
            newContext.expr =
                context.cgHelper.CreateConstant(FundType::BOOL, newContext.expr.constant);
        }
        else {
            newContext.expr = context.cgHelper.ConvertType(newContext.type,
                                                           FundType::BOOL,
                                                           newContext.expr.value);
        }

        context.IRBuilder.CreateCondBr(newContext.expr.value, loopBB, endBB);
    }
    else {
        context.IRBuilder.CreateBr(loopBB);
    }

    function->getBasicBlockList().push_back(iterBB);
    context.IRBuilder.SetInsertPoint(iterBB);

    if (iterExpr)
        iterExpr->Codegen(newContext);

    context.IRBuilder.CreateBr(condBB);
    function->getBasicBlockList().push_back(loopBB);
    context.IRBuilder.SetInsertPoint(loopBB);

    stmt->Codegen(newContext);

    if (!context.IRBuilder.GetInsertBlock()->getTerminator())
        context.IRBuilder.CreateBr(iterBB);
    function->getBasicBlockList().push_back(endBB);
    context.IRBuilder.SetInsertPoint(endBB);

    if (context.printLocalTable)
        localSymtab.Print(context.outputStream);
}

void JumpStatement::Codegen(CodegenContext &context) const
{
    switch (type) {
    case BREAK:
        if (!context.stmt.isInLoop && !context.stmt.isInSwitch)
            throw SemanticError("break statement not in loop or switch statement",
                                srcLocation);

        if (!context.IRBuilder.GetInsertBlock()->getTerminator())
            context.IRBuilder.CreateBr(context.stmt.breakBB);
        break;
    case CONTINUE:
        if (!context.stmt.isInLoop)
            throw SemanticError("continue statement not in loop", srcLocation);

        if (!context.IRBuilder.GetInsertBlock()->getTerminator())
            context.IRBuilder.CreateBr(context.stmt.continueBB);
        break;
    default:
        Type &retType = context.symtab->GetCurrentFunction()->retType;
        if (retExpr) {
            if (retType.IsSimple(TypeKind::FUNDTYPE)
                && retType.fundType == FundType::VOID)
                throw SemanticError("void function should not return a value",
                                    srcLocation);

            retExpr->Codegen(context);

            if (!context.type.IsConvertibleTo(retType, context.expr.constOrNull()))
                throw SemanticError("connot convert type '" + context.type.Name()
                                        + "' to function return type '" + retType.Name()
                                        + "'",
                                    srcLocation);

            context.expr =
                context.cgHelper.CreateValue(context.type, retType, context.expr);
            context.IRBuilder.CreateRet(context.expr.value);
        }
        else {
            if (!retType.IsSimple(TypeKind::FUNDTYPE)
                || retType.fundType != FundType::VOID)
                throw SemanticError("non-void function should return a value",
                                    srcLocation);

            context.IRBuilder.CreateRetVoid();
        }

        break;
    }
}

void DeclerationStatement::Codegen(CodegenContext &context) const
{
    context.decl = {DeclState::LOCALDECL};
    decl->Codegen(context);
}

}  // namespace ast