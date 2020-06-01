#include "../ast/node.h"
#include "../core/semantic.h"

#include <cassert>

namespace ast {

FundType SimpleTypeSpecifier::GetFundType() const
{
    constexpr int Mask = 255 ^ 8;
    FundType      ft;

    switch ((int)fundTypePart & Mask) {
    case (int)FundTypePart::VOID &  Mask: return FundType::VOID;
    case (int)FundTypePart::BOOL &  Mask: return FundType::BOOL;
    case (int)FundTypePart::FLOAT & Mask: return FundType::FLOAT;
    case (int)FundTypePart::DOUBLE &Mask: return FundType::DOUBLE;
    case (int)FundTypePart::SHORT & Mask: ft = FundType::SHORT; break;
    case (int)FundTypePart::LONG &  Mask: ft = FundType::LONG; break;
    case (int)FundTypePart::CHAR &  Mask: ft = FundType::CHAR; break;
    default: ft = FundType::INT; break;
    }

    if ((int)fundTypePart & (int)FundTypePart::UNSIGNED)
        return FundType((int)ft + 1);
    else
        return ft;
}

void BlockDeclaration::Analysis(SemanticContext &context) const
{
    auto lastDecl = context.decl;

    declSpec->Analysis(context);
    auto savedDecl = context.decl;
    Type decayType = context.type;

    for (const auto &d : initDeclList) {
        d.declarator->Analysis(context);

        context.decl.state = DeclState::NODECL;

        if (d.initializer)
            d.initializer->Analysis(context);

        context.decl = savedDecl;
        context.type = decayType;
    }

    context.decl = lastDecl;
}

void DeclSpecifier::Analysis(SemanticContext &context) const
{
    if (typeSpec)
        typeSpec->Analysis(context);
    else
        context.type =
            Type {TypeClass::FUNDTYPE, CVQualifier::NONE, {}, {}, FundType::VOID, nullptr};

    switch (declAttr) {
    case STATIC: context.decl.symAttr = Symbol::STATIC;
    case VIRTUAL: context.decl.symAttr = Symbol::VIRTUAL;
    default: context.decl.symAttr = Symbol::NORMAL;
    }

    context.decl.isFriend  = declAttr == FRIEND;
    context.decl.isTypedef = declAttr == TYPEDEF;
}

void SimpleTypeSpecifier::Analysis(SemanticContext &context) const
{
    context.type = Type {TypeClass::FUNDTYPE, cv, {}, {}, GetFundType(), nullptr};
}

void ElaboratedTypeSpecifier::Analysis(SemanticContext &context) const
{
    SymbolTable *symtab    = context.symtab;
    bool         qualified = false;

    if (nameSpec) {
        nameSpec->Analysis(context);
        symtab    = context.specifiedScope;
        qualified = true;
    }

    switch (typeClass) {
    case CLASSNAME: {
        auto classDesc = symtab->QueryClass(typeName, qualified);
        if (!classDesc) {
            if (qualified)
                throw SemanticError("no class named '" + typeName + "' in '" + symtab->ScopeName()
                                        + "'",
                                    srcLocation);

            // Forward declarator of CLASS
            classDesc            = std::make_shared<ClassDescriptor>();
            classDesc->className = typeName;
            assert(classDesc->memberTable == nullptr);

            // Inject class descriptor into symbol table
            symtab->AddClass(classDesc);
        }
        context.type = Type {TypeClass::CLASS, cv, {}, {}, FundType::VOID, classDesc};
        break;
    }
    case ENUMNAME: {
        auto enumDesc = symtab->QueryEnum(typeName, qualified);
        if (!enumDesc) {
            if (qualified)
                throw SemanticError("no enum named '" + typeName + "' in '" + symtab->ScopeName()
                                        + "'",
                                    srcLocation);
            else
                throw SemanticError("forward declaration of enum is forbidden", srcLocation);
        }

        context.type = Type {TypeClass::ENUM, cv, {}, {}, FundType::VOID, enumDesc};
        break;
    }
    default: {
        auto pType = symtab->QueryTypedef(typeName, qualified);
        if (!pType)
            throw SemanticError("unknown typedef name", srcLocation);

        context.type    = *pType;
        context.type.cv = cv;
        break;
    }
    }
}

void ClassTypeSpecifier::Analysis(SemanticContext &context) const
{
    if (context.decl.state != DeclState::FULLDECL && context.decl.state != DeclState::LOCALDECL)
        throw SemanticError("cannot define class type here", srcLocation);

    classType->Analysis(context);
    context.type.cv = cv;
}

void EnumTypeSpecifier::Analysis(SemanticContext &context) const
{
    if (context.decl.state != DeclState::FULLDECL && context.decl.state != DeclState::LOCALDECL)
        throw SemanticError("cannot define enum type here", srcLocation);

    enumType->Analysis(context);
    context.type.cv = cv;
}

void EnumSpecifier::Analysis(SemanticContext &context) const
{
    auto enumDesc      = std::make_shared<EnumDescriptor>();
    enumDesc->enumName = identifier;

    Type enumType {TypeClass::ENUM, CVQualifier::NONE, {}, {}, FundType::VOID, enumDesc};
    int  curConstant = 0;

    for (const auto &e : enumList) {
        Symbol symbol {e.first, enumType, Symbol::CONSTANT};

        if (e.second) {
            e.second->Analysis(context);
            if (!context.expr.isConstant)
                throw SemanticError("enum expression is not integral constant", srcLocation);

            if (!context.type.IsConvertibleTo(Type::IntType))
                throw SemanticError(context.type.Name() + " is not convertible to integral",
                                    srcLocation);

            symbol.constant = context.type.ConvertConstant(context.expr.constant, Type::IntType);
            // TODO: check constant unique
            curConstant = symbol.constant.intVal + 1;
        }
        else {
            symbol.constant.intVal = curConstant;
            curConstant++;
        }

        if (!context.symtab->AddSymbol(std::move(symbol)))
            throw SemanticError("redefinition of '" + e.first + "'", srcLocation);
    }

    // Inject enum name into symbol table
    context.symtab->AddEnum(enumDesc);
    context.type = enumType;
}

}  // namespace ast