#include <concepts>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <string>

#if MN_DEBUG
#define WRAP(str) "(" str ")"
#else
#define WRAP(str) str
#endif  // MN_DEBUG

namespace Manganese {
namespace ast {

// Helpers
std::string blockToString(const Block& block) {
    std::string result = " {\n";
    for (const auto stmt : block) { result += "\t" + stmt->toString() + "\n"; }
    result += "}";
    return result;
}

template <class T>
    requires(std::derived_from<T, ASTNode>)
std::string commaSeparatedList(const std::vector<T*>& values) {
    std::string result;
    for (std::size_t i = 0; i < values.size(); ++i) {
        result += values[i]->toString();
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

std::string genericsToString(const std::vector<Type*> params) {
    if (params.empty()) { return ""; }
    return std::format("@[{}]", commaSeparatedList(params));
}

std::string genericsToString(const std::vector<std::string> params) {
    if (params.empty()) { return ""; }
    return std::format("@[{}]", commaSeparatedList(params));
}

// Statements

std::string AggregateDeclarationStatement::toString() const {
    // std::string result = name + genericsToString(genericTypes);
    std::string result = std::format("{} aggregate {}", visibilityToString(visibility), name);
    if (!genericTypes.empty()) { result += std::format("[{}]", commaSeparatedList(genericTypes)); }
    result += " {\n";
    for (const auto& field : fields) { result += "\t" + field.name + ": " + field.type->toString() + ";\n"; }
    result += "}";
    return result;
}

std::string AliasStatement::toString() const {
    return std::format("alias " WRAP("{}") " as {};", baseType->toString(), alias);
}

std::string BreakStatement::toString() const { return "break;"; }

std::string ContinueStatement::toString() const { return "continue;"; }

std::string EmptyStatement::toString() const { return ""; }

std::string EnumDeclarationStatement::toString() const {
    std::string result = std::format("{} enum {}: {}", visibilityToString(visibility), name, baseType->toString());
    result += " {\n";
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto& value = values[i];
        result += "\t" + value.name;
        if (value.value) { result += std::format(" = {}", value.value->toString()); }
        if (i != values.size() - 1) { result += ","; }
        result += '\n';
    }
    result += "}";
    return result;
}

std::string ExpressionStatement::toString() const { return std::format("{};", expression->toString()); }

std::string ForLoopStatement::toString() const {
    std::string result = "for (";
    if (initializationStep) {
        result += initializationStep->toString() + " ";
    } else {
        result += ";";
    }
    if (stopCondition) {
        result += stopCondition->toString() + "; ";
    } else {
        result += ";";
    }
    if (postExpression) { result += postExpression->toString(); }
    result += ")";
    result += blockToString(body);
    return result;
}

std::string FunctionDeclarationStatement::toString() const {
    std::string result = std::format("{} func {}", visibilityToString(visibility), name);

    if (!genericTypes.empty()) {
        // not using the generics helper here since that adds an '@'
        result += std::format("[{}]", commaSeparatedList(genericTypes));
    }

    result += '(';
    for (size_t i = 0; i < parameters.size(); ++i) {
        const auto& param = parameters[i];
        result += std::format("{}: {}{}", param.name, (param.isMutable ? "mut " : ""), param.type->toString());
        if (i < parameters.size() - 1) { result += ", "; }
    }
    result += ')';
    if (returnType) { result += std::format(" -> {}", returnType->toString()); }
    result += blockToString(body);
    return result;
}

std::string IfStatement::toString() const {
    std::string result = std::format("if ({})", condition->toString()) + blockToString(body);
    for (const auto& elif : elifs) {
        result += std::format(" elif ({})", elif.condition->toString()) + blockToString(elif.body);
    }
    if (!elseBody.empty()) { result += " else" + blockToString(elseBody); }
    return result;
}

std::string NestedBlockStatement::toString() const { return blockToString(block); }

std::string ReturnStatement::toString() const { return std::format("return {};", toStringOr(value, "")); }

std::string SwitchStatement::toString() const {
    std::string result = std::format("switch ({})", variable->toString()) + " {\n";
    for (const auto& _case : cases) {
        result += std::format("\tcase {}:\n", _case.literalValue->toString());
        for (const auto stmt : _case.body) { result += "\t\t" + stmt->toString() + "\n"; }
    }
    if (!defaultBody.empty()) {
        result += "\tdefault:\n";
        for (const auto stmt : defaultBody) { result += "\t\t" + stmt->toString() + "\n"; }
    }
    result += "}";
    return result;
}

std::string VariableDeclarationStatement::toString() const {
    std::string typeStr = std::format("{} {}", visibilityToString(visibility), toStringOr(type, "auto"));

    std::string valueStr = value ? " = " + value->toString() : "";
    return std::format("({} {}: {}{});", isMutable ? "let mut" : "let", name, typeStr, valueStr);
}

std::string WhileLoopStatement::toString() const {
    std::string result;
    const auto whileCond = std::format("while ({})", condition->toString());
    if (isDoWhile) {
        result = "do";
    } else {
        result = whileCond;
    }
    result += blockToString(body);
    if (isDoWhile) { result += " " + whileCond + ";"; }

    return result;
}

// Expressions

std::string AggregateInstantiationExpression::toString() const {
    std::string result = std::format("{}{} ", name, genericsToString(genericTypes)) + "{";
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto& field = fields[i];
        result += std::format("{} = {}", field.name, field.value->toString());
        if (i != fields.size() - 1) { result += ", "; }
    }
    result += "}";

    return result;
}

std::string AggregateLiteralExpression::toString() const {
    std::string result = "{";
    for (size_t i = 0; i < elements.size(); ++i) {
        result += elements[i]->toString();
        if (i < elements.size() - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string AlignofExpression::toString() const { return std::format(WRAP("alignof({})"), type->toString()); }

std::string ArrayLiteralExpression::toString() const {
    std::string result = "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        result += elements[i]->toString();
        if (i < elements.size() - 1) [[likely]] { result += ", "; }
    }
    result += "]";
    return result;
}

std::string AssignmentExpression::toString() const {
    return std::format(WRAP("{} {} {}"), assignee->toString(), lexer::tokenTypeToString(op), value->toString());
}
std::string BinaryExpression::toString() const {
    return std::format(WRAP("{} {} {}"), left->toString(), lexer::tokenTypeToString(op), right->toString());
}

std::string BoolLiteralExpression::toString() const { return value ? "true" : "false"; }

std::string CharLiteralExpression::toString() const { return std::format("'{}'", static_cast<char>(value)); }

std::string FunctionCallExpression::toString() const {
    std::string result = callee->toString() + "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        result += arguments[i]->toString();
        if (i < arguments.size() - 1) [[likely]] { result += ", "; }
    }
    result += ")";
    return result;
}

std::string GenericExpression::toString() const { return identifier->toString() + genericsToString(types); }

std::string IdentifierExpression::toString() const { return value; }

std::string IndexExpression::toString() const { return std::format("{}[{}]", variable->toString(), index->toString()); }

std::string MemberAccessExpression::toString() const { return std::format("{}.{}", object->toString(), property); }

std::string NumberLiteralExpression::toString() const { return value.to_string(true); }

std::string PostfixExpression::toString() const {
    return std::format(WRAP("{}{}"), left->toString(), lexer::tokenTypeToString(op));
}

std::string PrefixExpression::toString() const {
    return std::format(WRAP("{}{}"), lexer::tokenTypeToString(op), right->toString());
}

std::string ScopeResolutionExpression::toString() const { return std::format("{}::{}", scope->toString(), element); }

std::string SizeofExpression::toString() const { return std::format(WRAP("sizeof({})"), type->toString()); }

std::string StringLiteralExpression::toString() const { return std::format("\"{}\"", value); }

std::string TypeCastExpression::toString() const {
    return std::format(WRAP("{} as {}"), originalValue->toString(), targetType->toString());
}

// Types

std::string AggregateType::toString() const {
    std::string result = "aggregate {";
    for (size_t i = 0; i < fieldTypes.size(); ++i) {
        result += fieldTypes[i]->toString();
        if (i < fieldTypes.size() - 1) [[likely]] { result += ", "; }
    }
    result += "}";
    return result;
}

std::string ArrayType::toString() const {
    return std::format("{}[{}]", elementType->toString(), toStringOr(lengthExpression, ""));
}

std::string FunctionType::toString() const {
    std::string result = "func(";
    for (std::size_t i = 0; i < parameterTypes.size(); ++i) {
        const auto& param = parameterTypes[i];
        result += std::format("{}{}", (param.isMutable ? "mut " : ""), param.type->toString());
        if (i != parameterTypes.size() - 1) {result += ", ";}
    }
    result += ")";
    if (returnType) { result += std::format(" -> {}", returnType->toString()); }
    return result;
}

std::string GenericType::toString() const { return baseType->toString() + genericsToString(typeParameters); }

std::string PointerType::toString() const {
    return std::format("ptr {}{}", (isMutable ? "mut " : ""), baseType->toString());
}

std::string SymbolType::toString() const { return name; }

std::string TypeofType::toString() const { return std::format("typeof({})", expression->toString()); }

}  // namespace ast

}  // namespace Manganese