#include <core.hpp>
#include <format>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/lexer/token.hpp>
#include <mnstl/fold_result.hxx>

namespace Manganese::ast {

std::string_view primitiveTypeToString(PrimitiveType_t prim) {
    constexpr static std::array<std::string_view, 17> primitiveNames
        = {"not primitive", int8_str,   int16_str,   int32_str,   int64_str,   int128_str, uint8_str,  uint16_str,
           uint32_str,      uint64_str, uint128_str, float32_str, float64_str, char_str,   string_str, bool_str};
    const auto index = static_cast<std::size_t>(prim);
    if (index >= primitiveNames.size()) { ASSERT_UNREACHABLE("Invalid primitive type"); }
    return primitiveNames[index];
}

mnstl::fold_result_t AlignofExpression::fold() const NOEXCEPT_IF_RELEASE { return mnstl::fold_result_t{}; }

mnstl::fold_result_t BinaryExpression::fold() const NOEXCEPT_IF_RELEASE {
    using enum lexer::TokenType;
    const mnstl::fold_result_t leftResult = left->fold();
    const mnstl::fold_result_t rightResult = right->fold();

    if (!leftResult.has_value() || !rightResult.has_value()) { return mnstl::fold_result_t{}; }

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
        default:
            ASSERT_UNREACHABLE(std::format("Unknown binary operator {}", lexer::tokenTypeToString(op)));
    }
    return mnstl::fold_result_t{};
};

mnstl::fold_result_t PrefixExpression::fold() const NOEXCEPT_IF_RELEASE {
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
        case Not: return mnstl::fold_result_t{};
        default: ASSERT_UNREACHABLE(std::format("Unknown prefix operator {}", lexer::tokenTypeToString(op)));
    }
};

mnstl::fold_result_t PostfixExpression::fold() const NOEXCEPT_IF_RELEASE {
    mnstl::fold_result_t result = left->fold();
    if (!result.has_value()) { return mnstl::fold_result_t{}; }

    using enum lexer::TokenType;
    switch (op) {
        case Inc:
        case Dec:
        default: ASSERT_UNREACHABLE(std::format("Unknown postfix operator {}", lexer::tokenTypeToString(op)));
    }
};

mnstl::fold_result_t SizeofExpression::fold() const NOEXCEPT_IF_RELEASE { return mnstl::fold_result_t{}; }

}  // namespace Manganese::ast