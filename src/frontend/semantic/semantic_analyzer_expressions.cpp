/**
 * @file semantic_analyzer_expressions.cpp
 * @brief Responsible for performing semantic analysis on all the AST expression nodes
 */

#include <frontend/semantic/semantic_analyzer.h>
#include <global_macros.h>

#include <format>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace Manganese {
namespace semantic {

void SemanticAnalyzer::checkExpression(ast::Expression* expression) noexcept_if_release {
    switch (expression->kind()) {
        case ast::ExpressionKind::ArrayLiteralExpression:
            checkArrayLiteralExpression(static_cast<ast::ArrayLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::AssignmentExpression:
            checkAssignmentExpression(static_cast<ast::AssignmentExpression*>(expression));
            break;
        case ast::ExpressionKind::BinaryExpression:
            checkBinaryExpression(static_cast<ast::BinaryExpression*>(expression));
            break;
        case ast::ExpressionKind::BoolLiteralExpression:
            checkBoolLiteralExpression(static_cast<ast::BoolLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::BundleInstantiationExpression:
            checkBundleInstantiationExpression(static_cast<ast::BundleInstantiationExpression*>(expression));
            break;
        case ast::ExpressionKind::CharLiteralExpression:
            checkCharLiteralExpression(static_cast<ast::CharLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::FunctionCallExpression:
            checkFunctionCallExpression(static_cast<ast::FunctionCallExpression*>(expression));
            break;
        case ast::ExpressionKind::GenericExpression:
            checkGenericExpression(static_cast<ast::GenericExpression*>(expression));
            break;
        case ast::ExpressionKind::IdentifierExpression:
            checkIdentifierExpression(static_cast<ast::IdentifierExpression*>(expression));
            break;
        case ast::ExpressionKind::IndexExpression:
            checkIndexExpression(static_cast<ast::IndexExpression*>(expression));
            break;
        case ast::ExpressionKind::MemberAccessExpression:
            checkMemberAccessExpression(static_cast<ast::MemberAccessExpression*>(expression));
            break;
        case ast::ExpressionKind::NumberLiteralExpression:
            checkNumberLiteralExpression(static_cast<ast::NumberLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::PostfixExpression:
            checkPostfixExpression(static_cast<ast::PostfixExpression*>(expression));
            break;
        case ast::ExpressionKind::PrefixExpression:
            checkPrefixExpression(static_cast<ast::PrefixExpression*>(expression));
            break;
        case ast::ExpressionKind::ScopeResolutionExpression:
            checkScopeResolutionExpression(static_cast<ast::ScopeResolutionExpression*>(expression));
            break;
        case ast::ExpressionKind::StringLiteralExpression:
            checkStringLiteralExpression(static_cast<ast::StringLiteralExpression*>(expression));
            break;
        case ast::ExpressionKind::TypeCastExpression:
            checkTypeCastExpression(static_cast<ast::TypeCastExpression*>(expression));
            break;
        default:
            using std::format;
            ASSERT_UNREACHABLE(
                format("No semantic analysis method for expression type {}",
                       static_cast<int>(expression->kind())));
    }
}

// ===== Specific Expression Checks =====
void SemanticAnalyzer::checkArrayLiteralExpression(ast::ArrayLiteralExpression* expression) {
    // Check that each element is the same type
    // Assume that the first element's type is the array's type
    if (expression->elements.empty()) {
        logging::logInternal("Array literal is empty, assuming type 'int'", logging::LogLevel::Warning);
        expression->setType(std::make_unique<ast::ArrayType>(std::make_unique<ast::SymbolType>("int32"), std::make_unique<ast::NumberLiteralExpression>(0)));
        return;
    }
    // Assign the type for each element (expression)
    auto firstElement = expression->elements[0].get();
    checkExpression(firstElement);
    if (!firstElement->getType()) {
        logError("Could not deduce type of {}, assuming 'int32'", expression, expression->elements[0]->toString());
        firstElement->setType(std::make_unique<ast::SymbolType>("int32"));
    }
    for (const auto& element : expression->elements) {
        checkExpression(element.get());
        if (!element->getType()) {
            logError("Could not deduce type of {}, assuming 'int32'", element.get(), element->toString());
            element->setType(std::make_unique<ast::SymbolType>("int32"));
        } else if (!areTypesCompatible(element->getType(), firstElement->getType())) {
            logError("Element {} has type {}, expected {}", element.get(), element->toString(), element->getType()->toString(), firstElement->getType()->toString());
        }
    }
    expression->elementType = firstElement->getTypePtr();
    expression->lengthExpression = std::make_unique<ast::NumberLiteralExpression>(expression->elements.size());
}
void SemanticAnalyzer::checkAssignmentExpression(ast::AssignmentExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkBinaryExpression(ast::BinaryExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkBoolLiteralExpression(ast::BoolLiteralExpression* expression) {
    expression->setType(std::make_shared<ast::SymbolType>("bool"));
}
void SemanticAnalyzer::checkBundleInstantiationExpression(ast::BundleInstantiationExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkCharLiteralExpression(ast::CharLiteralExpression* expression) {
    expression->setType(std::make_shared<ast::SymbolType>("char"));
}
void SemanticAnalyzer::checkFunctionCallExpression(ast::FunctionCallExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkGenericExpression(ast::GenericExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkIdentifierExpression(ast::IdentifierExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkIndexExpression(ast::IndexExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkMemberAccessExpression(ast::MemberAccessExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkNumberLiteralExpression(ast::NumberLiteralExpression* expression) {
    auto visitor = [](auto&& arg) -> ast::TypeSPtr_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int8_t>) {
            return std::make_shared<ast::SymbolType>("int8");
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return std::make_shared<ast::SymbolType>("int16");
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return std::make_shared<ast::SymbolType>("int32");
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::make_shared<ast::SymbolType>("int64");
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return std::make_shared<ast::SymbolType>("uint8");
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return std::make_shared<ast::SymbolType>("uint16");
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return std::make_shared<ast::SymbolType>("uint32");
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return std::make_shared<ast::SymbolType>("uint64");
        } else if constexpr (std::is_same_v<T, float>) {
            return std::make_shared<ast::SymbolType>("float32");
        } else if constexpr (std::is_same_v<T, double>) {
            return std::make_shared<ast::SymbolType>("float64");
        } else {
            ASSERT_UNREACHABLE(
                std::format("Unsupported number literal type: {}", typeid(T).name()));
            return nullptr;
        }
    };
    auto type = std::visit(visitor, expression->value);
    if (!type) [[unlikely]] {
        logError("Failed to determine type for number literal expression: {}", expression, expression->toString());
        type = std::make_shared<ast::SymbolType>("unknown");
    }
    expression->setType(type);
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
void SemanticAnalyzer::checkScopeResolutionExpression(ast::ScopeResolutionExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}
void SemanticAnalyzer::checkStringLiteralExpression(ast::StringLiteralExpression* expression) {
    expression->setType(std::make_shared<ast::SymbolType>("string"));
}
void SemanticAnalyzer::checkTypeCastExpression(ast::TypeCastExpression* expression) {
    DISCARD(expression);
    PRINT_LOCATION;
    throw std::runtime_error("Not implemented");
}

}  // namespace semantic
}  // namespace Manganese