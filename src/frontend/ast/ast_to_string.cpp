

/**
 * @file ast_to_string.cpp
 * @brief Implements the toString() methods for AST nodes
 *
 * The toString() methods are mainly used for error reporting
 * In the test suite, they are used to ensure the program is parsed correctly
 */
#include <algorithm>
#include <format>
#include <frontend/ast.hpp>
#include <global_macros.hpp>
#include <iomanip>
#include <sstream>
#include <string>
#include <variant>
#include "frontend/ast/ast_base.hpp"

namespace Manganese {
namespace ast {

// ===== Expressions =====
std::string AggregateInstantiationExpression::toString() const {
    std::ostringstream oss;
    oss << name;
    if (!genericTypes.empty()) {
        oss << "@[";
        for (size_t i = 0; i < genericTypes.size(); ++i) {
            oss << toStringOr(genericTypes[i]);
            if (i < genericTypes.size() - 1) [[likely]] { oss << ", "; }
        }
        oss << "]";
    }
    oss << " {";

    bool first = true;
    for (const auto& field : fields) {
        if (!first) { oss << ", "; }
        first = false;
        oss << field.name << " = " << field.value->toString();
    }

    oss << "}";
    return oss.str();
}

std::string AggregateLiteralExpression::toString() const {
    std::ostringstream oss;
    oss << "{";
    for (size_t i = 0; i < elements.size(); ++i) {
        oss << toStringOr(elements[i]);
        if (i < elements.size() - 1) [[likely]] {oss << ", ";}
    }
    oss << "}";
    return oss.str();
}

std::string ArrayLiteralExpression::toString() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        oss << toStringOr(elements[i]);
        if (i < elements.size() - 1) [[likely]] { oss << ", "; }
    }
    oss << "]";
    return oss.str();
}

std::string AssignmentExpression::toString() const {
    return std::format("({} {} {})", assignee->toString(), lexer::tokenTypeToString(op), value->toString());
}

std::string BinaryExpression::toString() const {
    return std::format("({} {} {})", left->toString(), lexer::tokenTypeToString(op), right->toString());
}

std::string BoolLiteralExpression::toString() const { return value ? "true" : "false"; }

std::string CharLiteralExpression::toString() const { return std::format("'{}'", static_cast<char>(value)); }

std::string FunctionCallExpression::toString() const {
    std::ostringstream oss;
    oss << callee->toString() << "(";

    for (size_t i = 0; i < arguments.size(); ++i) {
        oss << toStringOr(arguments[i]);
        if (i < arguments.size() - 1) [[likely]] { oss << ", "; }
    }

    oss << ")";
    return oss.str();
}

std::string GenericExpression::toString() const {
    std::ostringstream oss;
    oss << identifier->toString() << "@[";
    for (size_t i = 0; i < types.size(); ++i) {
        oss << toStringOr(types[i]);
        if (i < types.size() - 1) [[likely]] { oss << ", "; }
    }
    oss << "]";
    return oss.str();
}

std::string IdentifierExpression::toString() const { return value; }

std::string IndexExpression::toString() const { return std::format("{}[{}]", variable->toString(), index->toString()); }

std::string MemberAccessExpression::toString() const { return std::format("{}.{}", object->toString(), property); }

std::string NumberLiteralExpression::toString() const {
    std::ostringstream oss;

    auto visitor = [&oss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
            // we don't want to interpret int8s as chars so force promote it to an int
            oss << +arg;
        }
        // Special handling for floating point types to show decimal point
        else if constexpr (std::is_same_v<T, float32_t> || std::is_same_v<T, float64_t>) {
            std::string s = std::to_string(arg);
            size_t dotPos = s.find('.');
            int precision = 1;
            if (dotPos != std::string::npos) [[likely]] {
                // Count digits after the decimal point
                int count = static_cast<int>(s.size()) - static_cast<int>(dotPos) - 1;
                // Remove trailing zeros
                while (count > 1 && s[s.size() - 1] == '0') {
                    --count;
                    s.pop_back();
                }
                // Remove trailing dot if all zeros were removed
                if (count == 0 && s[s.size() - 1] == '.') { s.pop_back(); }
                precision = std::max(1, count);
            }
            oss << std::fixed << std::setprecision(precision) << arg;
        } else {
            oss << arg;
        }
    };

    std::visit(visitor, value);

    return oss.str();
}

std::string PostfixExpression::toString() const {
    return std::format("({}{})", left->toString(), lexer::tokenTypeToString(op));
}

std::string PrefixExpression::toString() const {
    return std::format("({}{})", lexer::tokenTypeToString(op), right->toString());
}

std::string ScopeResolutionExpression::toString() const { return std::format("{}::{}", scope->toString(), element); }

std::string StringLiteralExpression::toString() const { return std::format("\"{}\"", value); }

std::string TypeCastExpression::toString() const {
    return std::format("({} as {})", originalValue->toString(), targetType->toString());
}

// ===== Statements =====

std::string AliasStatement::toString() const { return std::format("alias ({}) as {};", baseType->toString(), alias); }

std::string BreakStatement::toString() const { return "break;"; }

std::string AggregateDeclarationStatement::toString() const {
    std::ostringstream oss;
    oss << visibilityToString(visibility) << "aggregate " << name;
    if (!genericTypes.empty()) {
        oss << "[";
        for (size_t i = 0; i < genericTypes.size(); ++i) {
            oss << genericTypes[i];
            if (i < genericTypes.size() - 1) { oss << ", "; }
        }
        oss << "]";
    }
    oss << " {\n";
    for (const auto& field : fields) { oss << "\t" << field.name << ": " << field.type->toString() << ";\n"; }
    oss << "}";
    return oss.str();
}

std::string ContinueStatement::toString() const { return "continue;"; }

std::string EmptyStatement::toString() const { return ""; }

std::string EnumDeclarationStatement::toString() const {
    std::ostringstream oss;
    oss << visibilityToString(visibility) << "enum " << name << ": " << baseType->toString() << " {\n";
    for (const auto& value : values) {
        oss << "\t" << value.name;
        if (value.value) { oss << " = " << value.value->toString(); }
        oss << ",\n";
    }
    oss << "}";
    return oss.str();
}

std::string ExpressionStatement::toString() const { return expression->toString() + ";"; }

std::string FunctionDeclarationStatement::toString() const {
    std::ostringstream oss;
    oss << visibilityToString(visibility) << "func " << name;
    if (!genericTypes.empty()) {
        oss << "[";
        for (size_t i = 0; i < genericTypes.size(); ++i) {
            oss << genericTypes[i];
            if (i < genericTypes.size() - 1) { oss << ", "; }
        }
        oss << "]";
    }
    oss << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        const auto& param = parameters[i];
        oss << param.name << ": " << (param.isMutable ? "mut " : "") << param.type->toString();
        if (i < parameters.size() - 1) { oss << ", "; }
    }
    oss << ")";
    if (returnType) { oss << " -> " << returnType->toString(); }
    oss << " {\n";
    if (!body.empty()) {
        for (size_t i = 0; i < body.size(); ++i) { oss << toStringOr(body[i]) << "\n"; }
    }
    oss << "}";
    return oss.str();
}

std::string IfStatement::toString() const {
    std::ostringstream oss;
    oss << "if (" << condition->toString() << ") {\n";
    for (const auto& stmt : body) { oss << "\t" << stmt->toString() << "\n"; }
    oss << "}";
    if (!elifs.empty()) {
        for (const auto& elif : elifs) {
            oss << " elif (" << elif.condition->toString() << ") {\n";
            for (const auto& stmt : elif.body) { oss << "\t" << stmt->toString() << "\n"; }
            oss << "}";
        }
    }
    if (!elseBody.empty()) {
        oss << " else {\n";
        for (const auto& stmt : elseBody) { oss << "\t" << stmt->toString() << "\n"; }
        oss << "}";
    }
    return oss.str();
}

std::string RepeatLoopStatement::toString() const {
    std::ostringstream oss;
    oss << "repeat (" << numIterations->toString() << ") {\n";
    for (const auto& stmt : body) { oss << "\t" << stmt->toString() << "\n"; }
    oss << "}";
    return oss.str();
}

std::string SwitchStatement::toString() const {
    std::ostringstream oss;
    oss << "switch (" << variable->toString() << ") {\n";
    for (const auto& case_ : cases) {
        oss << "\tcase " << case_.literalValue->toString() << ":\n";
        for (const auto& stmt : case_.body) { oss << "\t\t" << stmt->toString() << "\n"; }
    }
    if (!defaultBody.empty()) {
        oss << "\tdefault:\n";
        for (const auto& stmt : defaultBody) { oss << "\t\t" << stmt->toString() << "\n"; }
    }
    oss << "}";
    return oss.str();
}

std::string VariableDeclarationStatement::toString() const {
    // Convert visibility to string
    std::string typeStr = visibilityToString(visibility) + toStringOr(type, "auto");
    std::string valueStr = value ? " = " + value->toString() : "";
    return std::format("({} {}: {}{});", isMutable ? "let mut" : "let", name, typeStr, valueStr);
}

std::string ReturnStatement::toString() const {
    return std::format("return{};", (value ? " " + value->toString() : ""));
}

std::string WhileLoopStatement::toString() const {
    std::ostringstream oss;
    if (isDoWhile) {
        oss << "do {\n";
    } else {
        oss << "while (" << condition->toString() << ") {\n";
    }
    for (const auto& stmt : body) { oss << "\t" << stmt->toString() << "\n"; }
    oss << "}";
    if (isDoWhile) { oss << " while (" << condition->toString() << ");"; }
    return oss.str();
}

// ===== Types =====

std::string AggregateType::toString() const {
    std::ostringstream oss;
    oss << "aggregate {";
    for (size_t i = 0; i < fieldTypes.size(); ++i) {
        oss << toStringOr(fieldTypes[i]);
        if (i < fieldTypes.size() - 1) [[likely]] { oss << ", "; }
    }
    oss << "}";
    return oss.str();
}

std::string ArrayType::toString() const {
    std::ostringstream oss;
    oss << elementType->toString() << "[";
    if (lengthExpression) { oss << lengthExpression->toString(); }
    oss << "]";
    return oss.str();
}

std::string FunctionType::toString() const {
    std::ostringstream oss;
    oss << "func(";
    for (size_t i = 0; i < parameterTypes.size(); ++i) {
        if (parameterTypes[i].isMutable) { oss << "mut "; }
        oss << parameterTypes[i].type->toString();
        if (i < parameterTypes.size() - 1) [[likely]] { oss << ", "; }
    }
    oss << ")";
    oss << " -> " << toStringOr(returnType, "no return");

    return oss.str();
}

std::string GenericType::toString() const {
    std::ostringstream oss;
    oss << baseType->toString() << "@[";
    for (size_t i = 0; i < typeParameters.size(); ++i) {
        oss << toStringOr(typeParameters[i]);
        if (i < typeParameters.size() - 1) [[likely]] { oss << ", "; }
    }
    oss << "]";
    return oss.str();
}

std::string PointerType::toString() const {
    return std::format("ptr {}{}", (isMutable? "mut " : ""), baseType->toString());
}

std::string SymbolType::toString() const { return name; }

}  // namespace ast
}  // namespace Manganese
