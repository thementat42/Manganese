#include <core.hpp>
#include <format>
#include <frontend/ast/ast_expressions.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/semantic/type_context.hpp>
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

mnstl::fold_result_t AlignofExpression::fold(const TargetInfo& target) const NOEXCEPT_IF_RELEASE {
    if (!type->semanticType) { return mnstl::fold_result_t{}; }
    return mnstl::fold_result_t{mnstl::number_t{type->semanticType->alignment(target)}};
}

mnstl::fold_result_t BinaryExpression::fold(const TargetInfo& target) const NOEXCEPT_IF_RELEASE {
    using enum lexer::TokenType;
    const mnstl::fold_result_t leftResult = left->fold(target);
    const mnstl::fold_result_t rightResult = right->fold(target);

    if (!leftResult.has_value() || !rightResult.has_value()) { return mnstl::fold_result_t{}; }

    switch (op) {
        case Plus:
        case Minus:
        case Mul:
        case Div:
        case FloorDiv:
        case Mod:
        case BitAnd:
        case BitOr:
        case BitXor:
        case BitLShift:
        case BitRShift: {
            if (!leftResult.is_number() || !rightResult.is_number()) { return mnstl::fold_result_t{}; }
            auto l = leftResult.number_unchecked();
            auto r = rightResult.number_unchecked();
            if ((op == Div || op == FloorDiv || op == Mod) && r == 0) { return mnstl::fold_result_t{}; }
            if (op == Plus) { return mnstl::fold_result_t{l + r}; }
            if (op == Minus) { return mnstl::fold_result_t{l - r}; }
            if (op == Mul) { return mnstl::fold_result_t{l * r}; }
            if (op == Div) { return mnstl::fold_result_t{l.true_div(r)}; }
            if (op == FloorDiv) { return mnstl::fold_result_t{l.floor_div(r)}; }
            if (op == Mod) { return mnstl::fold_result_t{l % r}; }
            if (op == BitAnd) { return mnstl::fold_result_t{l & r}; }
            if (op == BitOr) { return mnstl::fold_result_t{l | r}; }
            if (op == BitXor) { return mnstl::fold_result_t{l ^ r}; }
            if (op == BitLShift) { return mnstl::fold_result_t{l << r}; }
            if (op == BitRShift) { return mnstl::fold_result_t{l >> r}; }
            break;
        }
        case GreaterThan:
        case GreaterThanOrEqual:
        case LessThan:
        case LessThanOrEqual: {
            if (!leftResult.is_number() || !rightResult.is_number()) { return mnstl::fold_result_t{}; }
            auto l = leftResult.number_unchecked();
            auto r = rightResult.number_unchecked();
            if (op == GreaterThan) { return mnstl::fold_result_t{l > r}; }
            if (op == GreaterThanOrEqual) { return mnstl::fold_result_t{l >= r}; }
            if (op == LessThan) { return mnstl::fold_result_t{l < r}; }
            if (op == LessThanOrEqual) { return mnstl::fold_result_t{l <= r}; }
            break;
        }
        case Equal:
        case NotEqual: {
            if (leftResult.held_type() != rightResult.held_type()) { return mnstl::fold_result_t{op == NotEqual}; }
            bool eq = false;
            if (leftResult.is_number()) {
                eq = (leftResult.number_unchecked() == rightResult.number_unchecked());
            } else if (leftResult.is_bool()) {
                eq = (leftResult.boolean_unchecked() == rightResult.boolean_unchecked());
            } else if (leftResult.is_char()) {
                eq = (leftResult.character_unchecked() == rightResult.character_unchecked());
            } else if (leftResult.is_string()) {
                eq = (leftResult.string_unchecked() == rightResult.string_unchecked());
            } else {
                return mnstl::fold_result_t{};
            }
            return mnstl::fold_result_t{op == Equal ? eq : !eq};
        }
        case And:
        case Or: {
            if (!leftResult.is_bool() || !rightResult.is_bool()) { return mnstl::fold_result_t{}; }
            bool l = leftResult.boolean_unchecked();
            bool r = rightResult.boolean_unchecked();
            if (op == And) { return mnstl::fold_result_t{l && r}; }
            if (op == Or) { return mnstl::fold_result_t{l || r}; }
            break;
        }
        case Not: return mnstl::fold_result_t{};
        default: ASSERT_UNREACHABLE(std::format("Unknown binary operator {}", lexer::tokenTypeToString(op)));
    }
    return mnstl::fold_result_t{};
}

mnstl::fold_result_t PrefixExpression::fold(const TargetInfo& target) const NOEXCEPT_IF_RELEASE {
    using enum lexer::TokenType;

    switch (op) {
        case AddressOf:
        case Dereference:
        case Inc:
        case Dec: return mnstl::fold_result_t{};
        default: {
            mnstl::fold_result_t result = right->fold(target);
            if (!result.has_value()) { return mnstl::fold_result_t{}; }

            switch (op) {
                case UnaryPlus:
                case UnaryMinus:
                case BitNot: {
                    if (!result.is_number()) { return mnstl::fold_result_t{}; }
                    auto val = result.number_unchecked();
                    if (op == UnaryPlus) { return mnstl::fold_result_t{val}; }
                    if (op == UnaryMinus) { return mnstl::fold_result_t{-val}; }
                    if (op == BitNot) { return mnstl::fold_result_t{~val}; }
                    break;
                }
                case Not: {
                    if (!result.is_bool()) { return mnstl::fold_result_t{}; }
                    return mnstl::fold_result_t{!result.boolean_unchecked()};
                }
                default: ASSERT_UNREACHABLE(std::format("Unknown prefix operator {}", lexer::tokenTypeToString(op)));
            }
        }
    }
    return mnstl::fold_result_t{};
}

mnstl::fold_result_t PostfixExpression::fold(const TargetInfo& target) const NOEXCEPT_IF_RELEASE {
    using enum lexer::TokenType;
    const mnstl::fold_result_t result = left->fold(target);
    if (!result.has_value()) { return mnstl::fold_result_t{}; }

    switch (op) {
        case Inc:
        case Dec: return mnstl::fold_result_t{};
        default: ASSERT_UNREACHABLE(std::format("Unknown postfix operator {}", lexer::tokenTypeToString(op)));
    }
}

mnstl::fold_result_t SizeofExpression::fold(const TargetInfo& target) const NOEXCEPT_IF_RELEASE {
    if (!type->semanticType) { return mnstl::fold_result_t{}; }
    return mnstl::fold_result_t{mnstl::number_t{type->semanticType->size(target)}};
}

}  // namespace Manganese::ast