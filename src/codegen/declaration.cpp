#include "../ast/node.h"
#include "context.h"

#include <cassert>

namespace ast {

FundType SimpleTypeSpecifier::GetFundType() const
{
    constexpr int Mask = 255 ^ 8;
    FundType      ft;

    switch ((int)fundTypePart & Mask) {
    case (int)FundTypePart::VOID &Mask:
        return FundType::VOID;
    case (int)FundTypePart::BOOL &Mask:
        return FundType::BOOL;
    case (int)FundTypePart::FLOAT &Mask:
        return FundType::FLOAT;
    case (int)FundTypePart::DOUBLE &Mask:
        return FundType::DOUBLE;
    case (int)FundTypePart::SHORT &Mask:
        ft = FundType::SHORT;
        break;
    case (int)FundTypePart::LONG &Mask:
        ft = FundType::LONG;
        break;
    case (int)FundTypePart::CHAR &Mask:
        ft = FundType::CHAR;
        break;
    default:
        ft = FundType::INT;
        break;
    }

    if ((int)fundTypePart & (int)FundTypePart::UNSIGNED)
        return FundType((int)ft + 1);
    else
        return ft;
}

void BlockDeclaration::Codegen(CodegenContext &context) const
{
    if (context.decl.isFriend && !initDeclList.empty())
        throw SemanticError("friends can only be classes or functions", srcLocation);

    declSpec->Codegen(context);

    auto savedDecl = context.decl;
    Type decayType = context.type;

    // Restore point
    for (const auto &d : initDeclList) {
        try {
            d.declarator->Codegen(context);

            if (context.decl.isTypedef)
                goto next_declarator;

            SymbolSet             varSymbol = context.symbolSet;
            llvm::GlobalVariable *globalVar = nullptr;
            assert(varSymbol);

            if (context.symtab->GetParent()) {
                // Local variables
                auto localVar = context.IRBuilder.CreateAlloca(
                    context.cgHelper.MakeType(context.type),
                    nullptr,
                    varSymbol->id);
                localVar->setAlignment(llvm::Align(context.type.Alignment()));
                varSymbol->value = localVar;
            }
            else if (!varSymbol->type.IsSimple(TypeKind::FUNCTION)) {
                // Global variables
                varSymbol->value = context.module.getOrInsertGlobal(
                    varSymbol->id,
                    context.cgHelper.MakeType(context.type));

                globalVar = context.module.getGlobalVariable(varSymbol->id);
                globalVar->setConstant(context.type.IsConstInit());
                globalVar->setAlignment(llvm::Align(context.type.Alignment()));
                globalVar->setDSOLocal(true);
                if (varSymbol->Attr() == Symbol::STATIC)
                    globalVar->setLinkage(llvm::GlobalValue::InternalLinkage);
            }
            else {
                // Function forward declaration
                goto next_declarator;
            }

            if (d.initializer) {
                context.decl.state = DeclState::NODECL;
                d.initializer->Codegen(context);
            }
            else if (context.type.IsConstInit()) {
                // Const type must have a initializer
                throw SemanticError("default initialization of an object of const type '"
                                        + context.type.Name() + "'",
                                    srcLocation);
            }
            else if (globalVar) {
                // Global variable are zero initialized by default
                if (varSymbol->type.IsSimple(TypeKind::CLASS)) {
                    // TODO: find default constructor
                    context.cgHelper.GenZeroInit(varSymbol);
                }
                else {
                    context.cgHelper.GenZeroInit(varSymbol);
                }
            }
        }
        catch (SemanticError error) {
            context.errorStream << error;
            context.errCnt++;
        }

    next_declarator:
        context.decl = savedDecl;
        context.type = decayType;
    }
}

void DeclSpecifier::Codegen(CodegenContext &context) const
{
    if (typeSpec)
        typeSpec->Codegen(context);
    else
        context.type = {FundType::VOID};

    auto &attr = context.decl.symbolAccessAttr;
    switch (declAttr) {
    case STATIC:
        attr = Symbol::Attribute(attr | Symbol::STATIC);
        break;
    case VIRTUAL:
        attr = Symbol::Attribute(attr | Symbol::VIRTUAL);
        break;
    case FRIEND:
        if (!context.symtab->GetCurrentClass())
            throw SemanticError("'friend' used outside of class", srcLocation);

        context.decl.isFriend = true;
        break;
    case TYPEDEF:
        context.decl.isTypedef = true;
        break;
    default:
        break;
    }
}

void TypeSpecifier::Codegen(CodegenContext &context) const
{
    throw SemanticError("C++ requires a type specifier for all declarations",
                        srcLocation);
}

void SimpleTypeSpecifier::Codegen(CodegenContext &context) const
{
    if (context.decl.isFriend)
        throw SemanticError("friends can only be classes or functions", srcLocation);

    context.type = {GetFundType(), cv};
}

void ElaboratedTypeSpecifier::Codegen(CodegenContext &context) const
{
    SymbolTable *symtab    = nullptr;
    bool         qualified = false;

    if (nameSpec) {
        nameSpec->Codegen(context);
        std::swap(symtab, context.qualifiedScope);
        qualified = true;
    }
    else {
        symtab = context.symtab;
    }

    switch (typeKind) {
    case CLASSNAME: {
        auto classDesc = symtab->QueryClass(typeName, qualified);
        if (!classDesc) {
            if (qualified)
                throw SemanticError("no class named '" + typeName + "' in '"
                                        + symtab->ScopeName() + "'",
                                    srcLocation);

            // Forward declarator of CLASS
            classDesc            = std::make_shared<ClassDescriptor>();
            classDesc->className = typeName;
            assert(classDesc->memberTable == nullptr);

            // Inject class descriptor into symbol table
            symtab->AddClass(classDesc);
        }

        if (context.decl.isFriend)
            classDesc->friendClassTo.push_back(context.symtab->GetCurrentClass());

        context.type = {classDesc, cv};
        break;
    }
    case ENUMNAME: {
        if (context.decl.isFriend)
            throw SemanticError("friends can only be classes or functions", srcLocation);

        auto enumDesc = symtab->QueryEnum(typeName, qualified);
        if (!enumDesc) {
            if (qualified)
                throw SemanticError("no enum named '" + typeName + "' in '"
                                        + symtab->ScopeName() + "'",
                                    srcLocation);
            else
                throw SemanticError("forward declaration of enum is forbidden",
                                    srcLocation);
        }

        context.type = {enumDesc, cv};
        break;
    }
    default:
        if (context.decl.isFriend)
            throw SemanticError("friends can only be classes or functions", srcLocation);

        auto pType = symtab->QueryTypedef(typeName, qualified);
        if (!pType)  // might be not useful
            throw SemanticError("unknown typedef name", srcLocation);

        context.type    = *pType;
        context.type.cv = cv;
        break;
    }
}

void ClassTypeSpecifier::Codegen(CodegenContext &context) const
{
    if (context.decl.state != DeclState::FULLDECL
        && context.decl.state != DeclState::LOCALDECL)
        throw SemanticError("cannot define class type here", srcLocation);

    if (context.decl.isFriend)
        throw SemanticError("cannot define a type in a friend declaration", srcLocation);

    classType->Codegen(context);
    context.type.cv = cv;
}

void EnumTypeSpecifier::Codegen(CodegenContext &context) const
{
    if (context.decl.state != DeclState::FULLDECL
        && context.decl.state != DeclState::LOCALDECL)
        throw SemanticError("cannot define enum type here", srcLocation);

    if (context.decl.isFriend)
        throw SemanticError("cannot define a type in a friend declaration", srcLocation);

    enumType->Codegen(context);
    context.type.cv = cv;
}

void EnumSpecifier::Codegen(CodegenContext &context) const
{
    auto enumDesc      = std::make_shared<EnumDescriptor>();
    enumDesc->enumName = identifier;
    if (enumDesc->enumName.empty()) {
        enumDesc->enumName = "<anonymous enum>";
    }

    Type enumType {enumDesc};
    int  curConstant = 0;

    for (const auto &e : enumList) {
        Symbol symbol {e.first, enumType, Symbol::CONSTANT};

        if (e.second) {
            auto lastDecl      = context.decl;
            context.decl.state = DeclState::NODECL;

            try {
                e.second->Codegen(context);
                context.decl = lastDecl;
            }
            catch (SemanticError error) {
                context.decl = lastDecl;
                context.errorStream << error;
                context.errCnt++;
                continue;
            }

            if (!context.expr.isConstant)
                throw SemanticError("enum expression is not integral constant",
                                    srcLocation);

            if (!context.type.IsConvertibleTo(FundType::INT, &context.expr.constant))
                throw SemanticError(context.type.Name()
                                        + " is not convertible to integral",
                                    srcLocation);

            // TODO: check constant unique
            symbol.intConstant = context.expr.constant.intVal;
            curConstant        = symbol.intConstant + 1;
        }
        else {
            symbol.intConstant = curConstant;
            curConstant++;
        }

        if (!context.symtab->AddSymbol(std::move(symbol)))
            throw SemanticError("redefinition of '" + e.first + "'", srcLocation);
    }

    // Inject enum name into symbol table
    if (!identifier.empty())
        context.symtab->AddEnum(enumDesc);
    context.type = enumType;
}

}  // namespace ast