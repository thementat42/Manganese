

/**
 * @file ast_to_string.cpp
 * @brief Implements the toString() methods for AST nodes
 *
 * The toString() methods are mainly used for error reporting
 * In the test suite, they are used to ensure the program is parsed correctly
 */
#include <frontend/ast.h>
#include <global_macros.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <variant>
namespace Manganese {
namespace ast {

// ===== Expressions =====
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
    std::string opStr = lexer::tokenTypeToString(op);
    return "(" + assignee->toString() + " " + opStr + " " + value->toString() + ")";
}

std::string BinaryExpression::toString() const {
    std::ostringstream oss;
    oss << "(" << left->toString() << " ";
    oss << lexer::tokenTypeToString(op) << " ";
    oss << right->toString() << ")";
    return oss.str();
}

std::string BoolLiteralExpression::toString() const { return value ? "true" : "false"; }

std::string BundleInstantiationExpression::toString() const {
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

std::string CharLiteralExpression::toString() const {
    std::ostringstream oss;
    oss << "'" << static_cast<char>(value) << "'";
    return oss.str();
}

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

std::string IndexExpression::toString() const { return variable->toString() + "[" + index->toString() + "]"; }

std::string MemberAccessExpression::toString() const { return object->toString() + "." + property; }

std::string NumberLiteralExpression::toString() const {
    std::ostringstream oss;

    auto visitor = [&oss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
            // we don't want to interpret int8s as chars so force promote it to an int
            oss << +arg;
        }
        // Special handling for floating point types to show decimal point
        else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
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
    std::string opStr = lexer::tokenTypeToString(op);
    return "(" + left->toString() + opStr + ")";
}

std::string PrefixExpression::toString() const {
    std::string opStr = lexer::tokenTypeToString(op);
    return "(" + opStr + right->toString() + ")";
}

std::string ScopeResolutionExpression::toString() const { return scope->toString() + "::" + element; }

std::string StringLiteralExpression::toString() const { return "\"" + value + "\""; }

std::string TypeCastExpression::toString() const {
    return "(" + originalValue->toString() + " as " + targetType->toString() + ")";
}

// ===== Statements =====

std::string AliasStatement::toString() const { return "alias (" + baseType->toString() + ") as " + alias + ";"; }

std::string BreakStatement::toString() const { return "break;"; }

std::string BundleDeclarationStatement::toString() const {
    std::ostringstream oss;
    oss << visibilityToString(visibility) << "bundle " << name;
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
        oss << param.name << ": " << (param.isConst ? "const " : "") << param.type->toString();
        if (i < parameters.size() - 1) { oss << ", "; }
    }
    oss << ")";
    if (returnType) { oss << " -> " << returnType->toString(); }
    oss << " {\n";
    if (!body.empty()) {
        for (size_t i = 0; i < body.size(); ++i) { oss << toStringOr(body[i]) << "\n";}
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
    std::string prefix = isConst ? "const " : "let ";
    std::string typeStr = ": " + visibilityToString(visibility) + (type ? type->toString() : "auto");
    std::string valueStr = value ? " = " + value->toString() : "";
    return "(" + prefix + name + typeStr + valueStr + ");";
}

std::string ReturnStatement::toString() const { return "return" + (value ? " " + value->toString() : "") + ";"; }

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

std::string ArrayType::toString() const {
    std::ostringstream oss;
    oss << elementType->toString() << "[";
    if (lengthExpression) { oss << lengthExpression->toString(); }
    oss << "]";
    return oss.str();
}

std::string BundleType::toString() const {
    std::ostringstream oss;
    oss << "bundle {";
    for (size_t i = 0; i < fieldTypes.size(); ++i) {
        oss << toStringOr(fieldTypes[i]);
        if (i < fieldTypes.size() - 1) [[likely]] { oss << ", "; }
    }
    oss << "}";
    return oss.str();
}

std::string FunctionType::toString() const {
    std::ostringstream oss;
    oss << "func(";
    for (size_t i = 0; i < parameterTypes.size(); ++i) {
        if (parameterTypes[i].isConst) { oss << "const "; }
        oss << parameterTypes[i].type->toString();
        if (i < parameterTypes.size() - 1) [[likely]] { oss << ", "; }
    }
    oss << ")";
    oss << " -> " << (returnType ? returnType->toString() : "no return");

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

std::string PointerType::toString() const { return "ptr " + baseType->toString(); }

std::string SymbolType::toString() const { return name; }

}  // namespace ast
}  // namespace Manganese
