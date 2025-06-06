#include "include/ast.h"
#include <sstream>
#include <variant>

namespace manganese {
namespace ast {

std::string NumberExpression::toString() const {
    std::ostringstream oss;
    
    std::visit([&oss](auto&& arg) {
        oss << arg;
    }, value);
    
    return oss.str();
}

std::string StringExpression::toString() const {
    return "\"" + value + "\"";
}

std::string SymbolExpression::toString() const {
    return value;
}

std::string BinaryExpression::toString() const {
    std::ostringstream oss;
    oss << "(" << left->toString() << " ";
    oss << lexer::tokenTypeToString(op) << " ";
    oss << right->toString() << ")";
    return oss.str();
}

std::string ExpressionStatement::toString() const {
    return expression->toString() + ";";
}

}  // namespace ast
}  // namespace manganese