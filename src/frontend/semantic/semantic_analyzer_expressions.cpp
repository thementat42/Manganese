#include <frontend/ast.h>
#include <frontend/semantic.h>
#include <io/logging.h>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::checkExpression(const ast::Expression* expression) {
    if (!expression) [[unlikely]] {
        logging::logInternal("Null pointer passed to checkExpression", logging::LogLevel::Error);
        return;
    }
    using ast::ExpressionKind;
    switch (expression->kind()) {
        case ExpressionKind::ArrayLiteralExpression:
            checkArrayLiteralExpression(static_cast<const ast::ArrayLiteralExpression*>(expression));
            break;
        case ExpressionKind::AssignmentExpression:
            checkAssignmentExpression(static_cast<const ast::AssignmentExpression*>(expression));
            break;
        case ExpressionKind::BinaryExpression:
            checkBinaryExpression(static_cast<const ast::BinaryExpression*>(expression));
            break;
        case ExpressionKind::BoolLiteralExpression:
            checkBoolLiteralExpression(static_cast<const ast::BoolLiteralExpression*>(expression));
            break;
        case ExpressionKind::BundleInstantiationExpression:
            checkBundleInstantiationExpression(static_cast<const ast::BundleInstantiationExpression*>(expression));
            break;
        case ExpressionKind::CharLiteralExpression:
            checkCharLiteralExpression(static_cast<const ast::CharLiteralExpression*>(expression));
            break;
        case ExpressionKind::FunctionCallExpression:
            checkFunctionCallExpression(static_cast<const ast::FunctionCallExpression*>(expression));
            break;
        case ExpressionKind::GenericExpression:
            checkGenericExpression(static_cast<const ast::GenericExpression*>(expression));
            break;
        case ExpressionKind::IdentifierExpression:
            checkIdentifierExpression(static_cast<const ast::IdentifierExpression*>(expression));
            break;
        case ExpressionKind::IndexExpression:
            checkIndexExpression(static_cast<const ast::IndexExpression*>(expression));
            break;
        case ExpressionKind::MemberAccessExpression:
            checkMemberAccessExpression(static_cast<const ast::MemberAccessExpression*>(expression));
            break;
        case ExpressionKind::NumberLiteralExpression:
            checkNumberLiteralExpression(static_cast<const ast::NumberLiteralExpression*>(expression));
            break;
        case ExpressionKind::PostfixExpression:
            checkPostfixExpression(static_cast<const ast::PostfixExpression*>(expression));
            break;
        case ExpressionKind::PrefixExpression:
            checkPrefixExpression(static_cast<const ast::PrefixExpression*>(expression));
            break;
        case ExpressionKind::ScopeResolutionExpression:
            checkScopeResolutionExpression(static_cast<const ast::ScopeResolutionExpression*>(expression));
            break;
        case ExpressionKind::StringLiteralExpression:
            checkStringLiteralExpression(static_cast<const ast::StringLiteralExpression*>(expression));
            break;
        case ExpressionKind::TypeCastExpression:
            checkTypeCastExpression(static_cast<const ast::TypeCastExpression*>(expression));
            break;
        default:
            ASSERT_UNREACHABLE("Unknown expression kind: " + std::to_string(static_cast<int>(expression->kind())));
    }
}

// ===== Specific Expression Checks =====
void SemanticAnalyzer::checkArrayLiteralExpression(const ast::ArrayLiteralExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkAssignmentExpression(const ast::AssignmentExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkBinaryExpression(const ast::BinaryExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkBoolLiteralExpression(const ast::BoolLiteralExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkBundleInstantiationExpression(const ast::BundleInstantiationExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkCharLiteralExpression(const ast::CharLiteralExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkFunctionCallExpression(const ast::FunctionCallExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkGenericExpression(const ast::GenericExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkIdentifierExpression(const ast::IdentifierExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkIndexExpression(const ast::IndexExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkMemberAccessExpression(const ast::MemberAccessExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkNumberLiteralExpression(const ast::NumberLiteralExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkPostfixExpression(const ast::PostfixExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkPrefixExpression(const ast::PrefixExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkScopeResolutionExpression(const ast::ScopeResolutionExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkStringLiteralExpression(const ast::StringLiteralExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkTypeCastExpression(const ast::TypeCastExpression* expression) {
    DISCARD(expression);  // Avoid unused variable warning, for now
    throw std::runtime_error("Not implemented");
}

}  // namespace semantic

}  // namespace Manganese