#include <frontend/semantic/semantic_analyzer.h>

namespace Manganese {

namespace semantic {

void SemanticAnalyzer::checkBinaryExpression(ast::BinaryExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

void SemanticAnalyzer::checkPostfixExpression(ast::PostfixExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkPrefixExpression(ast::PrefixExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

void SemanticAnalyzer::checkTypeCastExpression(ast::TypeCastExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

}  // namespace semantic

}  // namespace Manganese