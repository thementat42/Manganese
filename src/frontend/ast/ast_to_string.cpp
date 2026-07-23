#include <concepts>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/semantic/type_context.hpp>
#include <string>
#include <vector>


#if MN_DEBUG
#define WRAP(str) "(" str ")"
#else
#define WRAP(str) str
#endif  // MN_DEBUG

namespace Manganese {
namespace ast {

static inline std::string getIndent(size_t indent) { return std::string(indent * 4, ' '); }

// Helpers
std::string blockToString(const Block& block, size_t indent) {
    std::string result = "{\n";
    for (const auto stmt : block) { result += stmt->toString(indent + 1) + "\n"; }
    result += getIndent(indent) + "}";
    return result;
}

template <class T>
    requires(std::derived_from<T, ASTNode>)
std::string commaSeparatedList(const std::vector<T*>& values, size_t indent = 0) {
    std::string result;
    for (std::size_t i = 0; i < values.size(); ++i) {
        result += values[i]->toString(indent);
        if (i != values.size() - 1) [[likely]] { result += ", "; }
    }
    return result;
}

template <class T>
    requires(std::same_as<T, std::string>)
std::string commaSeparatedList(const std::vector<T>& values) {
    std::string result;
    for (std::size_t i = 0; i < values.size(); ++i) {
        result += values[i];
        if (i != values.size() - 1) [[likely]] { result += ", "; }
    }
    return result;
}

std::string genericsToString(const std::vector<Type*> params, size_t indent = 0) {
    if (params.empty()) { return ""; }
    return std::format("@[{}]", commaSeparatedList(params, indent));
}

std::string genericsToString(const std::vector<std::string> params) {
    if (params.empty()) { return ""; }
    return std::format("@[{}]", commaSeparatedList(params));
}

// Statements

std::string AggregateDeclarationStatement::toString(size_t indent) const {
    std::string result = getIndent(indent) + std::format("{} aggregate {}", visibilityToString(visibility), name);
    if (!genericTypes.empty()) { result += std::format("[{}]", commaSeparatedList(genericTypes)); }
    result += " {\n";
    for (const auto& field : fields) {
        result += getIndent(indent + 1) + field.name + ": " + field.type->toString(indent + 1) + ";\n";
    }
    result += getIndent(indent) + "}";
    return result;
}

std::string AliasStatement::toString(size_t indent) const {
    return getIndent(indent) + std::format("alias " WRAP("{}") " as {};", baseType->toString(indent), alias);
}

std::string BreakStatement::toString(size_t indent) const { return getIndent(indent) + "break;"; }

std::string ContinueStatement::toString(size_t indent) const { return getIndent(indent) + "continue;"; }

std::string EmptyStatement::toString(size_t) const { return ""; }

std::string EnumDeclarationStatement::toString(size_t indent) const {
    std::string result = getIndent(indent)
        + std::format("{} enum {}: {}", visibilityToString(visibility), name, baseType->toString(indent));
    result += " {\n";
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto& value = values[i];
        result += getIndent(indent + 1) + value.name;
        if (value.value) { result += std::format(" = {}", value.value->toString(indent + 1)); }
        if (i != values.size() - 1) { result += ","; }
        result += '\n';
    }
    result += getIndent(indent) + "}";
    return result;
}

std::string ExpressionStatement::toString(size_t indent) const {
    return getIndent(indent) + std::format("{};", expression->toString(indent));
}

std::string ForLoopStatement::toString(size_t indent) const {
    std::string result = getIndent(indent) + "for (";
    if (initializationStep) {
        // Strip leading indentation from statement parts inside loop clauses if they add it
        std::string init = initializationStep->toString(0);
        result += init + " ";
    } else {
        result += ";";
    }
    if (stopCondition) {
        result += stopCondition->toString(0) + "; ";
    } else {
        result += ";";
    }
    if (postExpression) { result += postExpression->toString(0); }
    result += ") ";
    result += blockToString(body, indent);
    return result;
}

std::string FunctionDeclarationStatement::toString(size_t indent) const {
    std::string result = getIndent(indent) + std::format("{} func {}", visibilityToString(visibility), name);

    if (!genericTypes.empty()) { result += std::format("[{}]", commaSeparatedList(genericTypes)); }

    result += '(';
    for (size_t i = 0; i < parameters.size(); ++i) {
        const auto& param = parameters[i];
        result += std::format("{}: {}{}", param.name, (param.isMutable ? "mut " : ""), param.type->toString(indent));
        if (i < parameters.size() - 1) { result += ", "; }
    }
    result += ')';
    if (returnType) { result += std::format(" -> {}", returnType->toString(indent)); }
    result += ' ';
    result += blockToString(body, indent);
    return result;
}

std::string IfStatement::toString(size_t indent) const {
    std::string result
        = getIndent(indent) + std::format("if ({}) ", condition->toString(indent)) + blockToString(body, indent);
    for (const auto& elif : elifs) {
        result += std::format(" elif ({}) ", elif.condition->toString(indent)) + blockToString(elif.body, indent);
    }
    if (!elseBody.empty()) { result += " else " + blockToString(elseBody, indent); }
    return result;
}

std::string NestedBlockStatement::toString(size_t indent) const {
    return getIndent(indent) + blockToString(block, indent);
}

std::string ReturnStatement::toString(size_t indent) const {
    std::string valStr = value ? value->toString(indent) : "";
    return getIndent(indent) + std::format("return {};", valStr);
}

std::string SwitchStatement::toString(size_t indent) const {
    std::string result = getIndent(indent) + std::format("switch ({})", variable->toString(indent)) + " {\n";
    for (const auto& _case : cases) {
        result += getIndent(indent + 1) + std::format("case {}:\n", _case.literalValue->toString(indent + 1));
        for (const auto stmt : _case.body) { result += stmt->toString(indent + 2) + "\n"; }
    }
    if (!defaultBody.empty()) {
        result += getIndent(indent + 1) + "default:\n";
        for (const auto stmt : defaultBody) { result += stmt->toString(indent + 2) + "\n"; }
    }
    result += getIndent(indent) + "}";
    return result;
}

std::string VariableDeclarationStatement::toString(size_t indent) const {
    std::string typeName;
    if (type) {
        typeName = type->toString();
    } else if (value && value->semanticType) {
        typeName = value->semanticType->toString();
    } else {
        typeName = "auto";
    }
    std::string typeStr = std::format("{} {}", visibilityToString(visibility), typeName);
    std::string valueStr = value ? " = " + value->toString(0) : "";

    return getIndent(indent) + std::format("({} {}: {}{});", isMutable ? "let mut" : "let", name, typeStr, valueStr);
}

std::string WhileLoopStatement::toString(size_t indent) const {
    std::string result = getIndent(indent);
    const auto whileCond = std::format("while ({})", condition->toString(indent));
    if (isDoWhile) {
        result += "do ";
    } else {
        result += whileCond + ' ';
    }
    result += blockToString(body, indent);
    if (isDoWhile) { result += " " + whileCond + ";"; }

    return result;
}

// Expressions (Expressions do not typically prepend self-indentation since they sit inside statements)

std::string AggregateInstantiationExpression::toString(size_t indent) const {
    std::string result = std::format("{}{} ", name, genericsToString(genericTypes, indent)) + "{";
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto& field = fields[i];
        result += std::format("{} = {}", field.name, field.value->toString(indent));
        if (i != fields.size() - 1) { result += ", "; }
    }
    result += "}";
    return result;
}

std::string AggregateLiteralExpression::toString(size_t indent) const {
    std::string result = "{";
    for (size_t i = 0; i < elements.size(); ++i) {
        result += elements[i]->toString(indent);
        if (i < elements.size() - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string AlignofExpression::toString(size_t indent) const {
    return std::format(WRAP("alignof({})"), type->toString(indent));
}

std::string ArrayLiteralExpression::toString(size_t indent) const {
    std::string result = "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        result += elements[i]->toString(indent);
        if (i < elements.size() - 1) [[likely]] { result += ", "; }
    }
    result += "]";
    return result;
}

std::string AssignmentExpression::toString(size_t indent) const {
    return std::format(WRAP("{} {} {}"), assignee->toString(indent), lexer::tokenTypeToString(op),
                       value->toString(indent));
}
std::string BinaryExpression::toString(size_t indent) const {
    return std::format(WRAP("{} {} {}"), left->toString(indent), lexer::tokenTypeToString(op), right->toString(indent));
}

std::string BoolLiteralExpression::toString(size_t) const { return value ? "true" : "false"; }

std::string CharLiteralExpression::toString(size_t) const { return std::format("'{}'", static_cast<char>(value)); }

std::string FunctionCallExpression::toString(size_t indent) const {
    std::string result = callee->toString(indent) + "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        result += arguments[i]->toString(indent);
        if (i < arguments.size() - 1) [[likely]] { result += ", "; }
    }
    result += ")";
    return result;
}

std::string GenericExpression::toString(size_t indent) const {
    return identifier->toString(indent) + genericsToString(types, indent);
}

std::string IdentifierExpression::toString(size_t) const { return value; }

std::string IndexExpression::toString(size_t indent) const {
    return std::format("{}[{}]", variable->toString(indent), index->toString(indent));
}

std::string MemberAccessExpression::toString(size_t indent) const {
    return std::format("{}.{}", object->toString(indent), property);
}

std::string NumberLiteralExpression::toString(size_t) const { return value.to_string(true); }

std::string PostfixExpression::toString(size_t indent) const {
    return std::format(WRAP("{}{}"), left->toString(indent), lexer::tokenTypeToString(op));
}

std::string PrefixExpression::toString(size_t indent) const {
    return std::format(WRAP("{}{}"), lexer::tokenTypeToString(op), right->toString(indent));
}

std::string ScopeResolutionExpression::toString(size_t indent) const {
    return std::format("{}::{}", scope->toString(indent), element);
}

std::string SizeofExpression::toString(size_t indent) const {
    return std::format(WRAP("sizeof({})"), type->toString(indent));
}

std::string StringLiteralExpression::toString(size_t) const { return std::format("\"{}\"", value); }

std::string TypeCastExpression::toString(size_t indent) const {
    return std::format(WRAP("{} as {}"), originalValue->toString(indent), targetType->toString(indent));
}

// Types

std::string AggregateType::toString(size_t indent) const {
    std::string result = "aggregate {";
    for (size_t i = 0; i < fieldTypes.size(); ++i) {
        result += fieldTypes[i]->toString(indent);
        if (i < fieldTypes.size() - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string ArrayType::toString(size_t indent) const {
    std::string lengthStr;
    if (lengthExpression) {
        lengthStr = lengthExpression->toString();
    } else if (semanticType && semanticType->isArray()) {
        lengthStr = std::to_string(static_cast<const semantic::Array*>(semanticType)->length);
    }
    return std::format("{}[{}]", elementType->toString(indent), lengthStr);
}

std::string FunctionType::toString(size_t indent) const {
    std::string result = "func(";
    for (std::size_t i = 0; i < parameterTypes.size(); ++i) {
        const auto& param = parameterTypes[i];
        result += std::format("{}{}", (param.isMutable ? "mut " : ""), param.type->toString(indent));
        if (i != parameterTypes.size() - 1) { result += ", "; }
    }
    result += ")";
    if (returnType) { result += std::format(" -> {}", returnType->toString(indent)); }
    return result;
}

std::string GenericType::toString(size_t indent) const {
    return baseType->toString(indent) + genericsToString(typeParameters, indent);
}

std::string PointerType::toString(size_t indent) const {
    return std::format("ptr {}{}", (isMutable ? "mut " : ""), baseType->toString(indent));
}

std::string SymbolType::toString(size_t) const { return name; }

std::string TypeofType::toString(size_t indent) const {
    return std::format("typeof({})", expression->toString(indent));
}

}  // namespace ast
}  // namespace Manganese