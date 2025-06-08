#include "include/ast.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <variant>

#include "../global_macros.h"

namespace manganese {
namespace ast {

# if DEBUG  // Thse functions are only used in debug mode
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

std::string NumberExpression::toString() const {
#if !DEBUG
    return "";
    ;
#endif
    std::ostringstream oss;

    std::visit([&oss](auto&& arg) {
        oss << arg;
    },
               value);

    return oss.str();
}

void NumberExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "NumberExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "type: " << getNumberTypeName(value) << "\n";
    os << getIndent(indent + 1) << "value: " << toString() << "\n";
    os << getIndent(indent) << "}\n";
}

std::string StringExpression::toString() const {
#if !DEBUG
    return "";
#endif
    return "\"" + value + "\"";
}

void StringExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "StringExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "value: " << toString() << "\n";
    os << getIndent(indent + 1) << "length: " << value.length() << "\n";
    os << getIndent(indent) << "}\n";
}

std::string SymbolExpression::toString() const {
#if !DEBUG
    return "";
#endif
    return value;
}

void SymbolExpression::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "SymbolExpression [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << value << "\n";
    os << getIndent(indent) << "}\n";
}

std::string BinaryExpression::toString() const {
#if !DEBUG
    return "";
#endif
    std::ostringstream oss;
    oss << "(" << left->toString() << " ";
    oss << lexer::tokenTypeToString(op) << " ";
    oss << right->toString() << ")";
    return oss.str();
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

std::string ExpressionStatement::toString() const {
#if !DEBUG
    return "";
#endif
    return expression->toString() + ";";
}

void ExpressionStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "ExpressionStatement [" << getLine() << ":" << getColumn() << "] {\n";
    expression->dump(os, indent + 1);
    os << getIndent(indent) << "}\n";
}

std::string VariableDeclarationStatement::toString() const {
#if !DEBUG
    return "";
#endif
    std::string prefix = isConst ? "const " : "let ";
    return "(" + prefix + name + " = " + value->toString() + ");";
}

void VariableDeclarationStatement::dump(std::ostream& os, int indent) const {
    os << getIndent(indent) << "VariableDeclarationStatement [" << getLine() << ":" << getColumn() << "] {\n";
    os << getIndent(indent + 1) << "name: " << name << "\n";
    os << getIndent(indent + 1) << "isConst: " << (isConst ? "true" : "false") << "\n";
    
    std::string visString;
    switch (visibility) {
        case Visibility::Public:    visString = "Public"; break;
        case Visibility::ReadOnly:  visString = "ReadOnly"; break;
        case Visibility::Private:   visString = "Private"; break;
    }
    os << getIndent(indent + 1) << "visibility: " << visString << "\n";
    
    os << getIndent(indent + 1) << "value: \n";
    value->dump(os, indent + 2);
    os << getIndent(indent) << "}\n";
}

#endif  // DEBUG

}  // namespace ast
}  // namespace manganese