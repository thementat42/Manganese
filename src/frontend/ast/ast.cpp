#include <core.hpp>
#include <format>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/lexer/token.hpp>
#include <mnstl/fold_result.hxx>

namespace Manganese::ast {

std::string_view primitiveTypeToString(PrimitiveType_t prim) {
    switch (prim) {
        case PrimitiveType_t::not_primitive: return "not primitive";
        case PrimitiveType_t::i8: return int8_str;
        case PrimitiveType_t::i16: return int16_str;
        case PrimitiveType_t::i32: return int32_str;
        case PrimitiveType_t::i64: return int64_str;
        case PrimitiveType_t::i128: return int128_str;
        case PrimitiveType_t::u8: return uint8_str;
        case PrimitiveType_t::u16: return uint16_str;
        case PrimitiveType_t::u32: return uint32_str;
        case PrimitiveType_t::u64: return uint64_str;
        case PrimitiveType_t::u128: return uint128_str;
        case PrimitiveType_t::f32: return float32_str;
        case PrimitiveType_t::f64: return float64_str;
        case PrimitiveType_t::character: return char_str;
        case PrimitiveType_t::str: return string_str;
        case PrimitiveType_t::boolean: return bool_str;
        default: ASSERT_UNREACHABLE("Invalid primitive type");
    }
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
        default: ASSERT_UNREACHABLE(std::format("Unknown binary operator {}", lexer::tokenTypeToString(op)));
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