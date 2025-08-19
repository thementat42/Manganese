#include <frontend/semantic/semantic_analyzer.hpp>
#include <frontend/ast.hpp>

namespace Manganese {
namespace semantic {
using ast::toStringOr;
void SemanticAnalyzer::visit(ast::ArrayLiteralExpression* expression) {
    // Check that each element is the same type
    // Assume that the first element's type is the array's type
    if (expression->elements.empty()) {
        logging::logInternal("Array literal is empty, assuming type 'int'", logging::LogLevel::Warning);
        expression->setType(std::make_shared<ast::ArrayType>(std::make_shared<ast::SymbolType>("int32"),
                                                             std::make_unique<ast::NumberLiteralExpression>(0)));
        return;
    }

    ast::TypeSPtr_t elementType;

    for (size_t i = 0; i < expression->elements.size(); ++i) {
        ast::Expression* element = expression->elements[i].get();
        visit(element);
        if (!element->getType()) {
            logError("Could not deduce type of {}, assuming 'int32'", element, toStringOr(element));
            element->setType(std::make_shared<ast::SymbolType>("int32"));
        }
        if (i == 0) {
            elementType = element->getTypePtr();
        } else if (!areTypesCompatible(element->getType(), elementType.get())) {
            logError("Element {} has type {}, expected {}", element, toStringOr(element), toStringOr(elementType),
                     toStringOr(elementType));
        }
    }

    expression->elementType = elementType;
    expression->lengthExpression = std::make_unique<ast::NumberLiteralExpression>(expression->elements.size());
    expression->setType(std::make_shared<ast::ArrayType>(
        elementType, std::make_unique<ast::NumberLiteralExpression>(expression->elements.size())));
}

void SemanticAnalyzer::visit(ast::BoolLiteralExpression* expression) {
    expression->setType(std::make_shared<ast::SymbolType>("bool"));
}

void SemanticAnalyzer::visit(ast::CharLiteralExpression* expression) {
    expression->setType(std::make_shared<ast::SymbolType>("char"));
}

void SemanticAnalyzer::visit(ast::NumberLiteralExpression* expression) {
    auto visitor = [](auto&& arg) -> ast::TypeSPtr_t {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int8_t>) {
            return std::make_shared<ast::SymbolType>(int8_str);
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return std::make_shared<ast::SymbolType>(int16_str);
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return std::make_shared<ast::SymbolType>(int32_str);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return std::make_shared<ast::SymbolType>(int64_str);
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return std::make_shared<ast::SymbolType>(uint8_str);
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return std::make_shared<ast::SymbolType>(uint16_str);
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return std::make_shared<ast::SymbolType>(uint32_str);
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return std::make_shared<ast::SymbolType>(uint64_str);
        } else if constexpr (std::is_same_v<T, float32_t>) {
            return std::make_shared<ast::SymbolType>(float32_str);
        } else if constexpr (std::is_same_v<T, float64_t>) {
            return std::make_shared<ast::SymbolType>(float64_str);
        } else {
            ASSERT_UNREACHABLE(std::format("Unsupported number literal type: {}", typeid(T).name()));
            return nullptr;
        }
    };
    auto type = std::visit(visitor, expression->value);
    if (!type) [[unlikely]] {
        logError("Failed to determine type for number literal expression: {}", expression, toStringOr(expression));
        type = std::make_shared<ast::SymbolType>("unknown");
    }
    expression->setType(type);
}

void SemanticAnalyzer::visit(ast::StringLiteralExpression* expression) {
    expression->setType(std::make_shared<ast::SymbolType>("string"));
}

}  // namespace semantic

}  // namespace Manganese