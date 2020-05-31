#include "../core/semantic.h"
#include "node.h"

namespace ast {

void AssignmentExpression::Print(std::ostream &os, Indent indent) const
{
    const char *ASSIGNOP_NAME[] = {"ASSIGN",
                                   "SELFMUL",
                                   "SELFDIV",
                                   "SELFMOD",
                                   "SELFADD",
                                   "SELFSUB",
                                   "SELFSHR",
                                   "SELFSHL",
                                   "SELFAND",
                                   "SELFXOR",
                                   "SELFOR"};

    os << indent << "赋值表达式: " << ASSIGNOP_NAME[(int)op] << "\n";
    os << indent + 1 << "左表达式:\n";
    left->Print(os, indent + 2);
    os << indent + 1 << "右表达式:\n";
    right->Print(os, indent + 2);
}

void ConditionalExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "条件表达式:\n";
    os << indent + 1 << "条件:\n";
    condition->Print(os, indent + 2);
    os << indent + 1 << "真表达式:\n";
    trueExpr->Print(os, indent + 2);
    os << indent + 1 << "假表达式:\n";
    falseExpr->Print(os, indent + 2);
}

void BinaryExpression::Print(std::ostream &os, Indent indent) const
{
    const char *BINARYOP_NAME[] = {"SUBSCRIPT", "DOT", "ARROW", "DOTSTAR", "ARROWSTAR", "MUL",
                                   "DIV",       "MOD", "ADD",   "SUB",     "SHL",       "SHR",
                                   "GT",        "LT",  "LE",    "GE",      "EQ",        "NE",
                                   "AND",       "XOR", "OR",    "LOGIAND", "LOGIOR",    "COMMA"};

    os << indent << "二元运算表达式: " << BINARYOP_NAME[(int)op] << "\n";
    os << indent + 1 << "左表达式:\n";
    left->Print(os, indent + 2);
    os << indent + 1 << "右表达式:\n";
    right->Print(os, indent + 2);
}

void CastExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型转换表达式:\n";
    os << indent + 1 << "目标类型:\n";
    typeId->Print(os, indent + 2);
    os << indent + 1 << "表达式:\n";
    expr->Print(os, indent + 2);
}

void UnaryExpression::Print(std::ostream &os, Indent indent) const
{
    const char *UNARYOP_NAME[] = {"UNREF",
                                  "ADDRESSOF",
                                  "POSI",
                                  "NEG",
                                  "NOT",
                                  "LOGINOT",
                                  "PREINC",
                                  "PREDEC",
                                  "POSTINC",
                                  "POSTDEC",
                                  "SIZEOF"};

    os << indent << "一元运算表达式: " << UNARYOP_NAME[(int)op] << "\n";
    expr->Print(os, indent + 1);
}

void CallExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "函数调用表达式:\n";
    os << indent + 1 << "函数:\n";
    funcExpr->Print(os, indent + 2);
    if (params) {
        os << indent + 1 << "调用参数:\n";
        params->Print(os, indent + 2);
    }
}

void ConstructExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "类型构造表达式:\n";
    os << indent + 1 << "类型:\n";
    type->Print(os, indent + 2);
    if (params) {
        os << indent + 1 << "构造参数:\n";
        params->Print(os, indent + 2);
    }
}

void SizeofExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Sizeof表达式:\n";
    typeId->Print(os, indent + 1);
}

void NewExpression::Print(std::ostream &os, Indent indent) const
{
    if (placement) {
        os << indent << "布置new:\n";
        placement->Print(os, indent + 1);
    }
}

void PlainNew::Print(std::ostream &os, Indent indent) const
{
    os << indent << "New表达式(type 1):\n";
    NewExpression::Print(os, indent + 1);
    typeId->Print(os, indent + 1);
}

void InitializableNew::Print(std::ostream &os, Indent indent) const
{
    os << indent << "New表达式(type 2):\n";
    NewExpression::Print(os, indent + 1);
    os << indent + 1 << "类型:\n";
    typeSpec->Print(os, indent + 2);

    if (ptrSpec) {
        ptrSpec->Print(os, indent + 1);
    }

    for (std::size_t i = 0; i < arraySizes.size(); i++) {
        os << indent + 1 << "数组大小[" << i << "]:\n";
        arraySizes[i]->Print(os, indent + 2);
    }

    if (initializer) {
        os << indent + 1 << "初始化:\n";
        initializer->Print(os, indent + 2);
    }
}

void DeleteExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Delete表达式: " << (isArray ? "(数组)\n" : "\n");
    expr->Print(os, indent + 1);
}

void IdExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Id表达式: " << identifier;

    switch (stype) {
    case DESTRUCTOR: os << " (析构函数)\n"; break;
    case CONSTRUCTOR: os << " (构造函数)\n"; break;
    default: os << '\n'; break;
    }

    if (nameSpec)
        nameSpec->Print(os, indent + 1);
}

void ThisExpression::Print(std::ostream &os, Indent indent) const
{
    os << indent << "This表达式\n";
}

void IntLiteral::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Int字面量: " << value << '\n';
}

void FloatLiteral::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Float字面量: " << value << '\n';
}

void CharLiteral::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Char字面量: '" << value << "'\n";
}

void StringLiteral::Print(std::ostream &os, Indent indent) const
{
    os << indent << "String字面量: \"" << value << "\"\n";
}

void BoolLiteral::Print(std::ostream &os, Indent indent) const
{
    os << indent << "Bool字面量: " << (value ? "true" : "false") << '\n';
}

void ExpressionList::Print(std::ostream &os, Indent indent) const
{
    os << indent << "表达式列表:\n";
    for (std::size_t i = 0; i < exprList.size(); i++) {
        os << indent + 1 << "表达式[" << i << "]:\n";
        exprList[i]->Print(os, indent + 2);
    }
}

void OperatorFunctionId::Print(std::ostream &os, Indent indent) const
{
    const char *OP_NAME[] = {
        "ADD",     "SUB",     "MUL",       "DIV",     "MOD",    "XOR",      "AND",     "OR",
        "NOT",     "LOGINOT", "ASSIGN",    "LT",      "GT",     "SELFADD",  "SELFSUB", "SELFMUL",
        "SELFDIV", "SELFMOD", "SELFXOR",   "SELFAND", "SELFOR", "SHL",      "SHR",     "SELFSHL",
        "SELFSHR", "EQ",      "NE",        "LE",      "GE",     "LOGIAND",  "LOGIOR",  "SELFINC",
        "SELFDEC", "COMMA",   "ARROWSTAR", "ARROW",   "CALL",   "SUBSCRIPT"};

    os << indent << "运算符函数Id: " << OP_NAME[(int)overloadOp] << "\n";
}

void AssignmentExpression::Analysis(SemanticContext &context) const
{
    left->Analysis(context);
    if (!context.expr.isAssignable)
        throw SemanticError("left of expression is not assignable", srcLocation);

    right->Analysis(context);

    context.expr.isAssignable = true;
}

void ConditionalExpression::Analysis(SemanticContext &context) const
{
    condition->Analysis(context);

    trueExpr->Analysis(context);
    bool trueAssignable = context.expr.isAssignable;

    falseExpr->Analysis(context);
    bool falseAssignable = context.expr.isAssignable;

    context.expr.isAssignable = trueAssignable && falseAssignable;
}

void BinaryExpression::Analysis(SemanticContext &context) const
{
    left->Analysis(context);
    right->Analysis(context);

    switch (op) {
    case BinaryOp::SUBSCRIPT:
    case BinaryOp::DOT:
    case BinaryOp::DOTSTAR:
    case BinaryOp::ARROW:
    case BinaryOp::ARROWSTAR: context.expr.isAssignable = true; break;
    default: context.expr.isAssignable = false; break;
    }
}

void CastExpression::Analysis(SemanticContext &context) const
{
    typeId->Analysis(context);
    expr->Analysis(context);
}

void UnaryExpression::Analysis(SemanticContext &context) const
{
    expr->Analysis(context);

    switch (op) {
    case UnaryOp::UNREF:
    case UnaryOp::PREINC:
    case UnaryOp::PREDEC: context.expr.isAssignable = true; break;
    default: context.expr.isAssignable = false; break;
    }
}

void CallExpression::Analysis(SemanticContext &context) const
{
    funcExpr->Analysis(context);
    if (params)
        params->Analysis(context);

    context.expr.isAssignable = true;
}

void ConstructExpression::Analysis(SemanticContext &context) const
{
    type->Analysis(context);
    if (params)
        params->Analysis(context);

    context.expr.isAssignable = true;
}

void SizeofExpression::Analysis(SemanticContext &context) const
{
    typeId->Analysis(context);

    context.expr.isAssignable = false;
}

void PlainNew::Analysis(SemanticContext &context) const
{
    if (placement)
        placement->Analysis(context);

    typeId->Analysis(context);

    context.expr.isAssignable = false;
}

void InitializableNew::Analysis(SemanticContext &context) const
{
    if (placement)
        placement->Analysis(context);

    typeSpec->Analysis(context);

    if (ptrSpec) {
        ptrSpec->Analysis(context);
        context.type.ptrDescList = std::move(context.ptrDescList);
    }

    for (const auto &s : arraySizes) {
        s->Analysis(context);
    }

    if (initializer)
        initializer->Analysis(context);

    context.expr.isAssignable = false;
}

void DeleteExpression::Analysis(SemanticContext &context) const
{
    expr->Analysis(context);
    // TODO: type check

    context.expr.isAssignable = false;
}

void IdExpression::Analysis(SemanticContext &context) const
{
    SymbolTable *symtab    = context.symtab;
    bool         qualified = false;

    if (nameSpec) {
        if (context.decl.state != DeclState::NODECL)
            throw SemanticError("declaration of " + identifier + "outside its scope", srcLocation);

        nameSpec->Analysis(context);
        symtab    = context.specifiedScope;
        qualified = true;
    }

    // Get composed identifier
    std::string composedId = ComposedId(context);

    if (context.decl.state != DeclState::NODECL) {
        if (context.decl.isTypedef) {
            // Inject typename name into symbol table
            if (!symtab->AddTypedef(composedId, context.type))
                throw SemanticError("redeclaration of '" + composedId + "'", srcLocation);

            context.symbolSet = nullptr;
        }
        else {
            // Record new symbol
            context.newSymbol = {composedId, context.type, {}};
            context.symbolSet = &context.newSymbol;
        }
    }
    else {
        context.symbolSet = symtab->QuerySymbol(composedId, qualified);
        if (!context.symbolSet) {
            switch (stype) {
            case DESTRUCTOR:
                // TODO: gen default destructor
                context.newSymbol = {composedId, context.type, {}};
                break;
            case CONSTRUCTOR:
                // TODO: gen default constructor
                context.newSymbol = {composedId, context.type, {}};
                break;
            default:
                throw SemanticError("use of undeclared identifier '" + composedId + "'",
                                    srcLocation);
                break;
            }
            context.symbolSet = &context.newSymbol;
        }
    }

    context.expr.isAssignable = stype == NO;
}

std::string IdExpression::ComposedId(SemanticContext &context) const
{
    switch (stype) {
    case DESTRUCTOR: return "~" + identifier + "()";
    case CONSTRUCTOR: return identifier + "()";
    default: return identifier;
    }
}

void ThisExpression::Analysis(SemanticContext &context) const
{
    context.expr.isAssignable = false;
}

void IntLiteral::Analysis(SemanticContext &context) const
{
    context.type                 = Type::IntType;
    context.expr.constant.intVal = value;
    context.expr.isConstant      = true;
    context.expr.isAssignable    = false;
}

void FloatLiteral::Analysis(SemanticContext &context) const
{
    context.type                   = Type::FloatType;
    context.expr.constant.floatVal = value;
    context.expr.isConstant        = true;
    context.expr.isAssignable      = false;
}

void CharLiteral::Analysis(SemanticContext &context) const
{
    context.type                  = Type::CharType;
    context.expr.constant.charVal = value;
    context.expr.isConstant       = true;
    context.expr.isAssignable     = false;
}

void StringLiteral::Analysis(SemanticContext &context) const
{
    context.type = Type::StringTypeProto;
    context.type.arrayDescList.push_back(Type::ArrayDescriptor {value.length() + 1, {}});

    context.stringTable.push_back(value);
    context.expr.constant.strIdx = context.stringTable.size() - 1;
    context.expr.isConstant      = true;
    context.expr.isAssignable    = false;
}

void BoolLiteral::Analysis(SemanticContext &context) const
{
    context.type                  = Type::BoolType;
    context.expr.constant.boolVal = value;
    context.expr.isConstant       = true;
    context.expr.isAssignable     = false;
}

void ExpressionList::Analysis(SemanticContext &context) const {}

}  // namespace ast