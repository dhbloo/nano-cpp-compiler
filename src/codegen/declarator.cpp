#include "../ast/node.h"
#include "context.h"

#include <cassert>

namespace ast {

void PtrSpecifier::Codegen(CodegenContext &context) const
{
    context.ptrDescList.clear();

    bool hasRefenerce = false;
    for (const auto &p : ptrList) {
        ClassDescriptor *classDesc = nullptr;

        if (hasRefenerce) {
            if (p.ptrType == PtrType::REF)
                throw SemanticError("reference to reference is forbidden", srcLocation);
            else
                throw SemanticError("pointer to reference is forbidden", srcLocation);
        }

        if (p.ptrType == PtrType::CLASSPTR) {
            p.classNameSpec->Codegen(context);
            classDesc              = context.qualifiedScope->GetCurrentClass();
            context.qualifiedScope = nullptr;
            assert(classDesc);
            assert(classDesc->memberTable);
        }
        else if (p.ptrType == PtrType::REF) {
            hasRefenerce = true;
        }

        context.ptrDescList.push_back(Type::PtrDescriptor {p.ptrType, p.cv, classDesc});
    }
}

void Declarator::Codegen(CodegenContext &context) const
{
    if (innerDecl)
        innerDecl->Codegen(context);

    if (ptrSpec) {
        ptrSpec->Codegen(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    // Abstract declarator has no symbol
    context.symbolSet = {};
}

void FunctionDeclarator::Codegen(CodegenContext &context) const
{
    if (innerDecl)
        innerDecl->Codegen(context);

    if (ptrSpec) {
        ptrSpec->Codegen(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    // Check function return type (cannot be array or function)
    if (context.type.IsArray())
        throw SemanticError("function cannot return array type '" + context.type.Name()
                                + "'",
                            srcLocation);

    if (context.type.IsSimple(TypeKind::FUNCTION))
        throw SemanticError("function cannot return function type '" + context.type.Name()
                                + "'",
                            srcLocation);

    // Current context type is return type
    auto funcDesc = std::make_shared<FunctionDescriptor>(context.type);
    funcDesc->funcScope =
        std::make_shared<SymbolTable>(context.symtab, nullptr, funcDesc.get());
    if (context.decl.isFriend)
        funcDesc->friendClass = context.symtab->GetCurrentClass();

    // function parameters
    CodegenContext newContext(context);
    newContext.symtab = funcDesc->funcScope.get();

    for (const auto &p : params) {
        newContext.decl = {DeclState::PARAMDECL};
        p->Codegen(newContext);
    }

    context.type = {funcDesc, funcCV};
}

void ArrayDeclarator::Codegen(CodegenContext &context) const
{
    if (innerDecl)
        innerDecl->Codegen(context);

    size_t arraySize = 0;

    if (size) {
        Type arrayType     = context.type;
        auto lastDecl      = context.decl;
        context.decl.state = DeclState::NODECL;

        size->Codegen(context);
        if (!context.expr.isConstant)
            throw SemanticError("array size is not an integral constant expression",
                                srcLocation);

        if (!context.type.IsConvertibleTo(FundType::INT, &context.expr.constant))
            throw SemanticError("'" + context.type.Name()
                                    + "' is not convertible to integral",
                                srcLocation);

        if (context.expr.constant.intVal <= 0)
            throw SemanticError("array declared with non positive size", srcLocation);
        else
            arraySize = (size_t)context.expr.constant.intVal;

        context.decl = lastDecl;
        context.type = arrayType;
    }

    Type::ArrayDescriptor arrayDesc {arraySize};

    if (ptrSpec) {
        ptrSpec->Codegen(context);
        arrayDesc.ptrDescList = std::move(context.ptrDescList);
    }

    if (!arrayDesc.ptrDescList.empty()
        && arrayDesc.ptrDescList.back().ptrType == PtrType::REF)
        throw SemanticError("array declared with reference to type '"
                                + context.type.Name() + "'",
                            srcLocation);

    if ((context.decl.state != DeclState::PARAMDECL || context.decl.mustComplete)
        && !context.type.IsComplete() && arrayDesc.ptrDescList.empty())
        throw SemanticError("array declared with incomplete element type '"
                                + context.type.Name() + "'",
                            srcLocation);

    if (context.type.IsSimple(TypeKind::FUNCTION) && arrayDesc.ptrDescList.empty()) {
        throw SemanticError("array declared as functions of type '" + context.type.Name()
                                + "'",
                            srcLocation);
    }

    // Array with unknown bound decay to pointer
    if (arrayDesc.size == 0) {
        context.type.ptrDescList = arrayDesc.ptrDescList;
        context.type.AddPtrDesc(Type::PtrDescriptor {PtrType::PTR});
    }
    else
        context.type.arrayDescList.push_back(std::move(arrayDesc));
}

void IdDeclarator::Codegen(CodegenContext &context) const
{
    SymbolTable *insymtab = context.symtab;
    if (context.decl.isFriend) {
        insymtab = insymtab->GetParent();
        context.decl.symbolAccessAttr =
            Symbol::Attribute(context.decl.symbolAccessAttr & ~Symbol::ACCESSMASK);
    }

    if (innerDecl)
        innerDecl->Codegen(context);

    if (ptrSpec) {
        ptrSpec->Codegen(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    if ((context.decl.state != DeclState::PARAMDECL || context.decl.mustComplete)
        && !context.type.IsComplete())
        throw SemanticError("variable has incomplete type '" + context.type.Name() + "'",
                            srcLocation);

    // Parameter type must be decayed
    if (context.decl.state == DeclState::PARAMDECL)
        context.type = context.type.Decay();

    id->Codegen(context);

    if (context.decl.isTypedef || context.symbolSet)
        return;

    // Non static member function has a hidden parameter "this"
    if (context.type.IsSimple(TypeKind::FUNCTION) && context.newSymbol.IsMember()
        && context.newSymbol.Attr() != Symbol::STATIC) {
        auto funcDesc = context.type.Function();
        auto classDesc =
            context.symtab->QueryClass(context.symtab->GetCurrentClass()->className);
        assert(classDesc);

        Type thisType {classDesc};
        thisType.cv = context.type.cv;
        thisType.AddPtrDesc(Type::PtrDescriptor {PtrType::PTR, CVQualifier::CONST});

        SymbolSet thisSymbol = funcDesc->funcScope->AddSymbol(Symbol {"", thisType});
        funcDesc->paramList.insert(funcDesc->paramList.begin(), {(Symbol *)thisSymbol});
    }

    // Record original symbol's attribute
    auto originAttr = context.newSymbol.Attr();

    context.symbolSet = insymtab->AddSymbol(context.newSymbol);
    if (!context.symbolSet) {
        throw SemanticError("redefinition of '" + context.newSymbol.id + "'",
                            srcLocation);
    }
    else if (context.type.IsSimple(TypeKind::FUNCTION)) {
        // Due to function overloading, function type might change
        // Here symbolSet should contain exactly one symbol
        // First, check if the two funtion's return types are the same
        if (!(*context.type.Function() == *context.symbolSet->type.Function()))
            throw SemanticError(
                "functions that differ only in their return type cannot be overloaded",
                srcLocation);

        context.type = context.symbolSet->type;

        // Check if function symbol's attribute changed, which means
        // overrided function from base class using a different attribute
        if (originAttr != Symbol::NORMAL && context.symbolSet->Attr() != originAttr)
            throw SemanticError("function '" + context.newSymbol.id
                                    + "' overrides a virtual function in base class",
                                srcLocation);

        // Set definition symbol in the function descriptor
        // Each function descriptor links to (exactly) one symbol in a symbol table
        auto funcDesc       = context.type.Function();
        funcDesc->defSymbol = context.symbolSet;

        // Create LLVM function and record symbol value
        if (!funcDesc->defSymbol->value) {
            auto linkage = llvm::Function::ExternalLinkage;
            if (!context.symbolSet.Scope()->GetParent() && !funcDesc->IsMember()
                && funcDesc->defSymbol->Attr() == Symbol::STATIC)
                linkage = llvm::Function::InternalLinkage;

            auto function = llvm::Function::Create(
                llvm::cast<llvm::FunctionType>(context.cgHelper.MakeType(context.type)),
                linkage,
                funcDesc->defSymbol->id,
                context.module);

            assert(function->arg_size() == funcDesc->paramList.size());
            auto param = funcDesc->paramList.begin();
            auto arg   = function->arg_begin();
            for (; arg != function->arg_end(); arg++, param++) {
                if (!param->symbol->id.empty())
                    arg->setName(param->symbol->id);
                param->symbol->value = arg;
            }

            funcDesc->defSymbol->value = function;
        }
    }
}

void TypeId::Codegen(CodegenContext &context) const
{
    auto lastDecl = context.decl;
    context.decl  = {DeclState::MINDECL};

    typeSpec->Codegen(context);

    if (abstractDecl)
        abstractDecl->Codegen(context);

    context.decl = lastDecl;
}

void ParameterDeclaration::Codegen(CodegenContext &context) const
{
    declSpec->Codegen(context);

    Symbol *paramSymbol = nullptr;
    if (decl) {
        context.symbolSet = {};
        decl->Codegen(context);
        paramSymbol = context.symbolSet;
    }

    // Unnamed parameter are added directly without check previous symbol
    if (!paramSymbol) {
        context.type      = context.type.Decay();
        context.newSymbol = {"", context.type};
        paramSymbol       = context.symtab->AddSymbol(context.newSymbol);
    }

    if (defaultExpr) {
        context.decl.state = DeclState::NODECL;
        Type paramType     = context.type;

        defaultExpr->Codegen(context);

        if (!context.type.IsConvertibleTo(paramType, context.expr.constOrNull()))
            throw SemanticError("cannot initialize '" + paramType.Name() + "' with '"
                                    + context.type.Name() + "'",
                                srcLocation);
    }

    // Add param symbol to parameter list
    context.symtab->GetCurrentFunction()->paramList.push_back(
        FunctionDescriptor::Param {paramSymbol, defaultExpr != nullptr});
}

void FunctionDefinition::Codegen(CodegenContext &context) const
{
    if (context.decl.memberFirstPass || !context.secondPassContexts) {
        if (declSpec)
            declSpec->Codegen(context);
        else {
            // constructor/destructor/conversion type
            context.type = {FundType::VOID};
        }

        // Do not allow incomplete type in parameter declaration
        context.decl.mustComplete = true;
        declarator->Codegen(context);

        if (!context.type.IsSimple(TypeKind::FUNCTION))
            throw SemanticError("function definition is not a function", srcLocation);

        // Get function descriptor
        if (context.type.Function()->hasBody)
            throw SemanticError("redefinition of function '" + context.symbolSet->id
                                    + "'",
                                srcLocation);

        // On first pass of member function definition, do not enter its function body
        if (context.decl.memberFirstPass) {
            context.secondPassContexts->push_back(context);
            return;
        }
    }

    CodegenContext *contextPtr = &context;
    if (context.secondPassContexts)
        contextPtr = &context.secondPassContexts->front();

    auto funcDesc = contextPtr->type.Function();
    auto function = llvm::cast<llvm::Function>(funcDesc->defSymbol->value);
    auto funcBB   = llvm::BasicBlock::Create(context.llvmContext, "entry", function);

    // Enter function scope
    CodegenContext newContext(*contextPtr);
    newContext.decl.state = DeclState::NODECL;
    newContext.symtab     = funcDesc->funcScope.get();

    llvm::IRBuilder<> newIRBuilder(newContext.llvmContext);
    newIRBuilder.SetInsertPoint(funcBB);
    newContext.IRBuilder = &newIRBuilder;

    auto param = funcDesc->paramList.begin();
    auto arg   = function->arg_begin();
    for (; arg != function->arg_end(); arg++, param++) {
        // Anonymous argument are not visiable to function body, so
        // no need to translate to a local variable.
        if (param->symbol->id.empty())
            continue;

        auto argVar =
            newIRBuilder.CreateAlloca(context.cgHelper.MakeType(param->symbol->type),
                                      nullptr,
                                      param->symbol->id);
        argVar->setAlignment(llvm::Align(param->symbol->type.Alignment()));
        newIRBuilder.CreateAlignedStore(param->symbol->value,
                                        argVar,
                                        llvm::Align(param->symbol->type.Alignment()));
        param->symbol->value = argVar;
    }

    for (const auto &i : ctorInitList) {
        i->Codegen(newContext);
    }

    newContext.stmt = {true};
    funcBody->Codegen(newContext);

    // Leave function scope
    funcDesc->hasBody = true;
    if (newContext.printLocalTable)
        funcDesc->funcScope->Print(newContext.outputStream);
}

void AssignmentInitializer::Codegen(CodegenContext &context) const
{
    SymbolSet varSymbol = context.symbolSet;
    assert(varSymbol.size() == 1);

    expr->Codegen(context);

    // if (!varSymbol->type.IsRef() && !varSymbol->type.IsArray())
    //    context.type = context.type.Decay();

    if (!context.type.IsConvertibleTo(varSymbol->type, context.expr.constOrNull()))
        throw SemanticError("cannot initialize '" + varSymbol->type.Name() + "' with '"
                                + context.type.Name() + "'",
                            srcLocation);

    context.cgHelper.GenAssignInit(*context.IRBuilder,
                                   varSymbol,
                                   context.type,
                                   context.expr);
}

void ListInitializer::Codegen(CodegenContext &context) const
{
    SymbolSet varSymbol = context.symbolSet;
    assert(varSymbol.size() == 1);

    if (varSymbol->type.IsArray()) {
        if (initList.size() > varSymbol->type.ArraySize())
            throw SemanticError("excess elements in array initializer", srcLocation);

        Type elementType = varSymbol->type.ElementType();

        for (size_t i = 0; i < initList.size(); i++) {
            context.type = elementType;
            initList[i]->Codegen(context);
        }
    }
    else if (varSymbol->type.IsSimple(TypeKind::CLASS)) {
        throw SemanticError("unimplemented", srcLocation);
    }
    else {
        if (initList.size() > 1) {
            throw SemanticError("excess elements in scalar initializer", srcLocation);
        }
        else if (initList.empty()) {
            context.cgHelper.GenZeroInit(*context.IRBuilder, varSymbol);
        }
        else {
            initList.front()->Codegen(context);

            if (!context.type.IsConvertibleTo(varSymbol->type,
                                              context.expr.constOrNull()))
                throw SemanticError("cannot initialize '" + varSymbol->type.Name()
                                        + "' with '" + context.type.Name() + "'",
                                    srcLocation);

            context.cgHelper.GenAssignInit(*context.IRBuilder,
                                           varSymbol,
                                           context.type,
                                           context.expr);
        }
    }
}

void ParenthesisInitializer::Codegen(CodegenContext &context) const
{
    assert(context.symbolSet.size() == 1);
    // TODO: find constructor function for class type
    if (context.type.IsSimple(TypeKind::CLASS))
        throw SemanticError("unimp", srcLocation);
    exprList->Codegen(context);
}

}  // namespace ast