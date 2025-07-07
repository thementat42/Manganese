#include <frontend/ast.h>
#include <global_macros.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <variant>
namespace Manganese {
namespace ast {

// Helper function to create indentation
inline static std::string getIndent(int indent) {
    DISABLE_CONVERSION_WARNING
    return std::string(indent * 2, ' ');
    ENABLE_CONVERSION_WARNING
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

std::string NumberLiteralExpression::toString() const {
    std::ostringstream oss;

    auto visitor = [&oss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        // Special handling for floating point types to show decimal point
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
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
                if (count == 0 && s[s.size() - 1] == '.') {
                    s.pop_back();
                }
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

std::string CharLiteralExpression::toString() const {
    std::ostringstream oss;
    oss << "'" << static_cast<char>(value) << "'";
    return oss.str();
}

std::string StringLiteralExpression::toString() const {
    return "\"" + value + "\"";
}

std::string IdentifierExpression::toString() const {
    return value;
}

std::string BoolLiteralExpression::toString() const {
    return value ? "true" : "false";
}

std::string ArrayLiteralExpression::toString() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        oss << elements[i]->toString();
        if (i < elements.size() - 1) [[likely]] {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
}

std::string BinaryExpression::toString() const {
    std::ostringstream oss;
    oss << "(" << left->toString() << " ";
    oss << lexer::tokenTypeToString(op) << " ";
    oss << right->toString() << ")";
    return oss.str();
}

std::string PrefixExpression::toString() const {
    std::string opStr = lexer::tokenTypeToString(op);
    return "(" + opStr + right->toString() + ")";
}

std::string PostfixExpression::toString() const {
    std::string opStr = lexer::tokenTypeToString(op);
    return "(" + left->toString() + opStr + ")";
}

std::string TypeCastExpression::toString() const {
    return "(" + expression->toString() + " as " + type->toString() + ")";
}

std::string AssignmentExpression::toString() const {
    std::string opStr = lexer::tokenTypeToString(op);
    return "(" + assignee->toString() + " " + opStr + " " + value->toString() + ")";
}

std::string FunctionCallExpression::toString() const {
    std::ostringstream oss;
    oss << callee->toString() << "(";

    for (size_t i = 0; i < arguments.size(); ++i) {
        oss << arguments[i]->toString();
        if (i < arguments.size() - 1) [[likely]] {
            oss << ", ";
        }
    }

    oss << ")";
    return oss.str();
}

std::string GenericExpression::toString() const {
    std::ostringstream oss;
    oss << identifier->toString() << "@[";
    for (size_t i = 0; i < types.size(); ++i) {
        oss << types[i]->toString();
        if (i < types.size() - 1) [[likely]] {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
}

std::string ExpressionStatement::toString() const {
    return expression->toString() + ";";
}

std::string SymbolType::toString() const {
    return name;
}
std::string ArrayType::toString() const {
    std::string result = elementType->toString() + "[";
    if (lengthExpression) {
        result += lengthExpression->toString();
    }
    result += "]";
    return result;
}

std::string VariableDeclarationStatement::toString() const {
    // Convert visibility to string
    std::string visStr;
    switch (visibility) {
        case Visibility::Public:
            visStr = "public ";
            break;
        case Visibility::ReadOnly:
            visStr = "readonly ";
            break;
        case Visibility::Private:
            visStr = "private ";
            break;
        default:
            ASSERT_UNREACHABLE("Variable did not have a valid visibility");
    }

    std::string prefix = isConst ? "const " : "let ";
    std::string typeStr = type ? ": " + visStr + type->toString() : "";
    std::string valueStr = value ? " = " + value->toString() : "";
    return "(" + prefix + name + typeStr + valueStr + ");";
}

std::string FunctionDeclarationStatement::toString() const {
    std::ostringstream oss;
    oss << "func " << name;
    if (!genericTypes.empty()) {
        oss << "[";
        for (size_t i = 0; i < genericTypes.size(); ++i) {
            oss << genericTypes[i];
            if (i < genericTypes.size() - 1) {
                oss << ", ";
            }
        }
        oss << "]";
    }
    oss << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        const auto& param = parameters[i];
        oss << param.name << ": " << (param.isConst ? "const " : "") << param.type->toString();
        if (i < parameters.size() - 1) {
            oss << ", ";
        }
    }
    oss << ")";
    if (returnType) {
        oss << " -> " << returnType->toString();
    }
    oss << " {\n";
    if (!body.empty()) {
        for (size_t i = 0; i < body.size(); ++i) {
            oss << body[i]->toString() << "\n";
        }
    }
    oss << "}";
    return oss.str();
}

std::string ReturnStatement::toString() const {
    return "return" + (value ? " " + value->toString() : "") + ";";
}

std::string IfStatement::toString() const {
    std::ostringstream oss;
    oss << "if (" << condition->toString() << ") {\n";
    for (const auto& stmt : body) {
        oss << "\t" << stmt->toString() << "\n";
    }
    oss << "}";
    if (!elifs.empty()) {
        for (const auto& elif : elifs) {
            oss << " elif (" << elif.condition->toString() << ") {\n";
            for (const auto& stmt : elif.body) {
                oss << "\t" << stmt->toString() << "\n";
            }
            oss << "}";
        }
    }
    if (!elseBody.empty()) {
        oss << " else {\n";
        for (const auto& stmt : elseBody) {
            oss << "\t" << stmt->toString() << "\n";
        }
        oss << "}";
    }
    return oss.str();
}

std::string WhileLoopStatement::toString() const {
    std::ostringstream oss;
    if (isDoWhile) {
        oss << "do {\n";
    } else {
        oss << "while (" << condition->toString() << ") {\n";
    }
    for (const auto& stmt : body) {
        oss << "\t" << stmt->toString() << "\n";
    }
    oss << "}";
    if (isDoWhile) {
        oss << " while (" << condition->toString() << ");";
    }
    return oss.str();
}

std::string RepeatLoopStatement::toString() const {
    std::ostringstream oss;
    oss << "repeat (" << numIterations->toString() << ") {\n";
    for (const auto& stmt : body) {
        oss << "\t" << stmt->toString() << "\n";
    }
    oss << "}";
    return oss.str();
}

std::string BundleDeclarationStatement::toString() const {
    std::ostringstream oss;
    oss << "bundle " << name << " {\n";
    for (const auto& field : fields) {
        oss << "\t" << field.name << ": " << (field.isStatic ? "static " : "") << field.type->toString() << ";\n";
    }
    oss << "}";
    return oss.str();
}

std::string BundleInstantiationExpression::toString() const {
    std::ostringstream oss;
    oss << name << " {";

    bool first = true;
    for (const auto& field : fields) {
        if (!first) {
            oss << ", ";
        }
        first = false;
        oss << field.name << " = " << field.value->toString();
    }

    oss << "}";
    return oss.str();
}

std::string MemberAccessExpression::toString() const {
    return object->toString() + "." + property;
}

std::string IndexExpression::toString() const {
    return variable->toString() + "[" + index->toString() + "]";
}

std::string ScopeResolutionExpression::toString() const {
    return scope->toString() + "::" + element;
}

std::string EnumDeclarationStatement::toString() const {
    std::ostringstream oss;
    oss << "enum " << name << ": " << baseType->toString() << " {\n";
    for (const auto& value : values) {
        oss << "\t" << value.name;
        if (value.value) {
            oss << " = " << value.value->toString();
        }
        oss << ",\n";
    }
    oss << "}";
    return oss.str();
}

std::string SwitchStatement::toString() const {
    std::ostringstream oss;
    oss << "switch (" << variable->toString() << ") {\n";
    for (const auto& case_ : cases) {
        oss << "\tcase " << case_.literalValue->toString() << ":\n";
        for (const auto& stmt : case_.body) {
            oss << "\t\t" << stmt->toString() << "\n";
        }
    }
    if (!defaultBody.empty()) {
        oss << "\tdefault:\n";
        for (const auto& stmt : defaultBody) {
            oss << "\t\t" << stmt->toString() << "\n";
        }
    }
    oss << "}";
    return oss.str();
}

#if DEBUG  // Only include dump methods in debug builds
DISABLE_CONVERSION_WARNING
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
        os << getIndent(indent + 3) << "isStatic: " << (field.isStatic ? "true" : "false") << "\n";
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
#endif  // DEBUG

}  // namespace ast
}  // namespace Manganese
