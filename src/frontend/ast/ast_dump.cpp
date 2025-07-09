/**
 * @file ast_dump.cpp
 * @brief Implements AST node dump methods for debugging purposes in the Manganese frontend.
 *
 * This file contains the implementation of the `dump` methods for various AST node classes
 *
 * The dump methods are only included in debug builds (guarded by `#if DEBUG`).
 * They are useful for visualizing the AST structure.
 * @note This file does nothing in release builds.
 */

#if DEBUG  // Only include dump methods in debug builds
#include <frontend/ast.h>
#include <global_macros.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <variant>

namespace Manganese {
namespace ast {

DISABLE_CONVERSION_WARNING

// Helper function to create indentation
inline static std::string getIndent(int indent) {
    return std::string(indent * 2, ' ');
}

// Helper function to get the type of a number variant
static std::string getNumberTypeName(const number_t& value) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int8_t>)
            return "int8";
        else if constexpr (std::is_same_v<T, uint8_t>)
            return "uint8";
        else if constexpr (std::is_same_v<T, int16_t>)
            return "int16";
        else if constexpr (std::is_same_v<T, uint16_t>)
            return "uint16";
        else if constexpr (std::is_same_v<T, int32_t>)
            return "int32";
        else if constexpr (std::is_same_v<T, uint32_t>)
            return "uint32";
        else if constexpr (std::is_same_v<T, int64_t>)
            return "int64";
        else if constexpr (std::is_same_v<T, uint64_t>)
            return "uint64";
        else if constexpr (std::is_same_v<T, float>)
            return "float";
        else if constexpr (std::is_same_v<T, double>)
            return "double";
        else
            return "unknown";
    },
                      value);
}

void BreakStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "BreakStatement [" << getLine() << ":" << getColumn() << "]\n";
}

void ContinueStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "ContinueStatement [" << getLine() << ":" << getColumn() << "]\n";
}

void NumberLiteralExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "NumberLiteralExpression [" << getLine()
       << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "type: " << getNumberTypeName(value) << "\n";
    os << getIndent(indent + 1) << "value: " << toString() << "\n";
    os << getIndent(indent) << "}\n";
}

void CharLiteralExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "CharLiteralExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "value: '" << static_cast<char>(value) << "'\n";
    os << getIndent(indent + 1) << "code point: " << static_cast<int>(value) << "\n";
    os << getIndent(indent) << "}\n";
}

void StringLiteralExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "StringLiteralExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "value: " << toString() << "\n";
    os << getIndent(indent + 1) << "length: " << value.length() << "\n";
    os << getIndent(indent) << "}\n";
}

void IdentifierExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "IdentifierExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << value << "\n";
    os << getIndent(indent) << "}\n";
}

void BoolLiteralExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "BoolExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "value: " << toString() << "\n";
    os << getIndent(indent) << "}\n";
}

void ArrayLiteralExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "ArrayLiteralExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "elements: [\n";

    for (const auto& element : elements) {
        os << getIndent(indent + 2) << "{\n";
        element->dump(os, indent + 3);
        os << getIndent(indent + 2) << "}\n";
    }

    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void FunctionCallExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "FunctionCallExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "callee: \n";
    callee->dump(os, indent + 2);
    os << getIndent(indent + 1) << "arguments: [\n";

    for (const auto& arg : arguments) {
        os << getIndent(indent + 2) << "{\n";
        arg->dump(os, indent + 3);
        os << getIndent(indent + 2) << "}\n";
    }

    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void GenericExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "GenericExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "identifier: " << identifier->toString() << "\n";
    os << getIndent(indent + 1) << "generic types: [\n";

    for (const auto& type : types) {
        os << getIndent(indent + 2) << type->toString() << "\n";
    }

    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void BinaryExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "BinaryExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << getIndent(indent + 1) << "left: \n";
    left->dump(os, indent + 2);
    os << getIndent(indent + 1) << "right: \n";
    right->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

void PrefixExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "PrefixExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << getIndent(indent + 1) << "operand: \n";
    right->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

void PostfixExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "PostfixExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << getIndent(indent + 1) << "operand: \n";
    left->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

void TypeCastExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "TypeCastExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "expression: \n";
    expression->dump(os, indent + 2);
    os << getIndent(indent + 1) << "target type: \n";
    type->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

void AssignmentExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "AssignmentExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "operator: " << lexer::tokenTypeToString(op) << "\n";
    os << getIndent(indent + 1) << "assignee: \n";
    assignee->dump(os, indent + 2);
    os << getIndent(indent + 1) << "value: \n";
    value->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

void BundleInstantiationExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "BundleInstantiationExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "bundleName: " << name << "\n";
    os << getIndent(indent + 1) << "fields: [\n";

    for (const auto& field : fields) {
        os << getIndent(indent + 2) << "{\n";
        os << getIndent(indent + 3) << "name: " << field.name << "\n";
        os << getIndent(indent + 3) << "value: \n";
        field.value->dump(os, indent + 4);
        os << getIndent(indent + 2) << "}\n";
    }

    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void MemberAccessExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "MemberAccessExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "object: \n";
    object->dump(os, indent + 2);
    os << getIndent(indent + 1) << "property: " << property << "\n";
    os << getIndent(indent) << "}\n";
}

void IndexExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "IndexExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "variable: \n";
    variable->dump(os, indent + 2);
    os << getIndent(indent + 1) << "index: \n";
    index->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

void ScopeResolutionExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "ScopeResolutionExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "scope: \n";
    scope->dump(os, indent + 2);
    os << getIndent(indent + 1) << "element: " << element << "\n";
    os << getIndent(indent) << "}\n";
}

void BundleDeclarationStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "BundleDeclarationStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << name << "\n";
    os << getIndent(indent + 1) << "fields: [\n";

    for (const auto& field : fields) {
        os << getIndent(indent + 2) << "{\n";
        os << getIndent(indent + 3) << "name: " << field.name << "\n";
        os << getIndent(indent + 3) << "type: \n";
        field.type->dump(os, indent + 4);
        os << getIndent(indent + 2) << "}\n";
    }

    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void SymbolType::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "SymbolType [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << name << "\n";
    os << getIndent(indent) << "}\n";
}

void ArrayType::dump(std::ostream& os, int indent) const {
    os << std::string(indent, ' ') << "ArrayType: " << '\n';
    os << std::string(indent + 2, ' ') << "elementType: " << '\n';
    elementType->dump(os, indent + 4);

    if (lengthExpression) {
        os << std::string(indent + 2, ' ') << "length: " << '\n';
        lengthExpression->dump(os, indent + 4);
    }
}

void GenericType::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "GenericType [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "base: ";
    baseType->dump(os, indent + 1);
    os << "\n";
    os << getIndent(indent + 1) << "generic types: [\n";

    for (const auto& type : typeParameters) {
        type->dump(os, indent + 2);
    }

    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void ExpressionStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "ExpressionStatement [" << getLine() << ":" << getColumn() << "] {\n";
    expression->dump(os, indent + 1);
    os << getIndent(indent) << "}\n";
}

void VariableDeclarationStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "VariableDeclarationStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << name << "\n";
    os << getIndent(indent + 1) << "isConst: " << (isConst ? "true" : "false") << "\n";

    std::string visString;
    switch (visibility) {
        case Visibility::Public:
            visString = "Public";
            break;
        case Visibility::ReadOnly:
            visString = "ReadOnly";
            break;
        case Visibility::Private:
            visString = "Private";
            break;
    }
    os << getIndent(indent + 1) << "visibility: " << visString << "\n";

    os << getIndent(indent + 1) << "value: \n";
    if (value) {
        value->dump(os, indent + 2);
    } else {
        os << getIndent(indent + 2) << "null\n";
    }

    if (type) {
        os << getIndent(indent + 1) << "type: \n";
        type->dump(os, indent + 2);
    }

    os << getIndent(indent) << "}\n";
}

void FunctionDeclarationStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "FunctionDeclarationStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << name << "\n";

    if (!genericTypes.empty()) {
        os << getIndent(indent + 1) << "generic types: [";
        for (size_t i = 0; i < genericTypes.size(); ++i) {
            os << genericTypes[i];
            if (i < genericTypes.size() - 1) [[likely]] {
                os << ", ";
            }
        }
        os << "]\n";
    } else {
        os << getIndent(indent + 1) << "generic types: []\n";
    }

    os << getIndent(indent + 1) << "parameters: [\n";
    for (const auto& param : parameters) {
        os << getIndent(indent + 2) << "{\n";
        os << getIndent(indent + 3) << "name: " << param.name << "\n";
        os << getIndent(indent + 3) << "isConst: " << (param.isConst ? "true" : "false") << "\n";
        os << getIndent(indent + 3) << "type: \n";
        param.type->dump(os, indent + 4);
        os << getIndent(indent + 2) << "}\n";
    }
    os << getIndent(indent + 1) << "]\n";

    // Return type
    os << getIndent(indent + 1) << "returnType: ";
    if (returnType) {
        os << "\n";
        returnType->dump(os, indent + 2);
    } else {
        os << "void\n";
    }

    // Body
    os << getIndent(indent + 1) << "body: [\n";
    for (const auto& stmt : body) {
        stmt->dump(os, indent + 2);
    }
    os << getIndent(indent + 1) << "]\n";

    os << getIndent(indent) << "}\n";
}

void ReturnStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "ReturnStatement [" << getLine() << ":" << getColumn() << "] {\n";
    if (value) {
        os << getIndent(indent + 1) << "value: \n";
        value->dump(os, indent + 2);
    } else {
        os << getIndent(indent + 1) << "value: null\n";
    }
    os << getIndent(indent) << "}\n";
}

void IfStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "IfStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "condition: \n";
    condition->dump(os, indent + 2);
    os << getIndent(indent + 1) << "body: [\n";
    for (const auto& stmt : body) {
        stmt->dump(os, indent + 2);
    }
    os << getIndent(indent + 1) << "]\n";
    if (!elifs.empty()) {
        os << getIndent(indent + 1) << "elif clauses: [\n";
        for (const auto& elif : elifs) {
            os << getIndent(indent + 2) << "{\n";
            os << getIndent(indent + 3) << "condition: \n";
            elif.condition->dump(os, indent + 4);
            os << getIndent(indent + 3) << "body: [\n";
            for (const auto& stmt : elif.body) {
                stmt->dump(os, indent + 4);
            }
            os << getIndent(indent + 3) << "]\n";
            os << getIndent(indent + 2) << "}\n";
        }
        os << getIndent(indent + 1) << "]\n";
    }
    if (!elseBody.empty()) {
        os << getIndent(indent + 1) << "else body: [\n";
        for (const auto& stmt : elseBody) {
            stmt->dump(os, indent + 2);
        }
        os << getIndent(indent + 1) << "]\n";
    }
    os << getIndent(indent) << "}\n";
}

void WhileLoopStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "WhileLoopStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "body: [\n";
    for (const auto& stmt : body) {
        stmt->dump(os, indent + 2);
    }
    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent + 1) << "condition: \n";
    condition->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

void RepeatLoopStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "RepeatLoopStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "numIterations: \n";
    numIterations->dump(os, indent + 2);
    os << getIndent(indent + 1) << "body: [\n";
    for (const auto& stmt : body) {
        stmt->dump(os, indent + 2);
    }
    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void EnumDeclarationStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "EnumDeclarationStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << name << "\n";
    os << getIndent(indent + 1) << "values: [\n";

    for (const auto& value : values) {
        os << getIndent(indent + 2) << "{\n";
        os << getIndent(indent + 3) << "name: " << value.name << "\n";
        os << getIndent(indent + 3) << "value: " << value.value << "\n";
        os << getIndent(indent + 2) << "}\n";
    }

    os << getIndent(indent + 1) << "]\n";
    os << getIndent(indent) << "}\n";
}

void SwitchStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "SwitchStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "variable: \n";
    variable->dump(os, indent + 2);
    os << getIndent(indent + 1) << "cases: [\n";

    for (const auto& case_ : cases) {
        os << getIndent(indent + 2) << "{\n";
        os << getIndent(indent + 3) << "literalValue: \n";
        case_.literalValue->dump(os, indent + 4);
        os << getIndent(indent + 3) << "body: [\n";
        for (const auto& stmt : case_.body) {
            stmt->dump(os, indent + 4);
        }
        os << getIndent(indent + 3) << "]\n";
        os << getIndent(indent + 2) << "}\n";
    }

    if (!defaultBody.empty()) {
        os << getIndent(indent + 1) << "default body: [\n";
        for (const auto& stmt : defaultBody) {
            stmt->dump(os, indent + 2);
        }
        os << getIndent(indent + 1) << "]\n";
    }

    os << getIndent(indent) << "}\n";
}

ENABLE_CONVERSION_WARNING
}  // namespace ast
}  // namespace Manganese
#endif  // DEBUG