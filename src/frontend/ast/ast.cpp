#include <core.hpp>
#include <format>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/lexer/token_type.hpp>
#include <mnstl/fold_result.hxx>

namespace Manganese {
namespace ast {

mnstl::fold_result_t BinaryExpression::fold() const noexcept {
    using enum lexer::TokenType;
    mnstl::fold_result_t leftResult = left->fold();
    mnstl::fold_result_t rightResult = right->fold();

    if (!leftResult.has_value() || rightResult.has_value()) { return mnstl::fold_result_t{}; }

    switch (op) {
        case Plus: break;
        case Minus: break;
        case Mul: break;
        case Div: break;
        case FloorDiv: break;
        case Mod: break;
        case GreaterThan: break;
        case GreaterThanOrEqual: break;
        case LessThan: break;
        case LessThanOrEqual: break;
        case Equal: break;
        case NotEqual: break;
        case And: break;
        case Or: break;
        case Not: break;
        case BitAnd: break;
        case BitOr: break;
        case BitXor: break;
        case BitLShift: break;
        case BitRShift: break;
        default: ASSERT_UNREACHABLE(std::format("Unknown binary operator {}", lexer::tokenTypeToString(op)));
    }
    return mnstl::fold_result_t{};
};

mnstl::fold_result_t PrefixExpression::fold() const noexcept {
    mnstl::fold_result_t result = right->fold();
    if (!result.has_value()) { return mnstl::fold_result_t{}; }

    using enum lexer::TokenType;
    switch (op) {
        case AddressOf:
        case Dereference:
        case Inc:
        case Dec:
        case UnaryPlus:
        case UnaryMinus:
        case BitNot:
        default: ASSERT_UNREACHABLE(std::format("Unknown prefix operator {}", lexer::tokenTypeToString(op)));
    }
};

mnstl::fold_result_t PostfixExpression::fold() const noexcept {
    mnstl::fold_result_t result = left->fold();
    if (!result.has_value()) { return mnstl::fold_result_t{}; }

    using enum lexer::TokenType;
    switch (op) {
        case Inc:
        case Dec:
        default: ASSERT_UNREACHABLE(std::format("Unknown postfix operator {}", lexer::tokenTypeToString(op)));
    }
};

}  // namespace ast
}  // namespace Manganese