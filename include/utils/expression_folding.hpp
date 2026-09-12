#include <frontend/ast.hpp>
#include <frontend/semantic/type_context.hpp>
#include <mnstl/i128.hxx>
#include <utils/str_to_num.hpp>

#include "mnstl/ext_num_config.hxx"

namespace Manganese::utils {

template <class T, class ErrorLogger>
    requires(std::is_arithmetic_v<T> || mnstl::Numeric<T>)
std::optional<T> computeExpression(const ast::Expression* expression, const utils::TargetInfo& targetInfo,
                                   ErrorLogger&& logger) noexcept {
    using enum lexer::TokenType;
    switch (expression->kind) {
        case ast::ExpressionKind::AlignofExpression: {
            const auto* alignofExpression = static_cast<const ast::AlignofExpression*>(expression);
            return static_cast<T>(alignofExpression->semanticType->alignment(targetInfo));
        }
        case ast::ExpressionKind::BinaryExpression: {
            const auto* binaryExpression = static_cast<const ast::BinaryExpression*>(expression);
            auto leftValue = computeExpression<T>(binaryExpression->left, targetInfo, logger);
            auto rightValue = computeExpression<T>(binaryExpression->right, targetInfo, logger);
            if (!leftValue.has_value() || !rightValue.has_value()) { break; }
            switch (binaryExpression->op) {
                case Plus: return *leftValue + *rightValue;
                case Minus: return *leftValue - *rightValue;
                case Mul: return *leftValue * *rightValue;
                case Div: return static_cast<T>(static_cast<float>(*leftValue) / static_cast<float>(*rightValue));
                case FloorDiv: return *leftValue / *rightValue;
                case Mod: return *leftValue % *rightValue;
                case GreaterThan: return static_cast<T>(*leftValue > *rightValue);
                case GreaterThanOrEqual: return static_cast<T>(*leftValue >= *rightValue);
                case LessThan: return static_cast<T>(*leftValue < *rightValue);
                case LessThanOrEqual: return static_cast<T>(*leftValue <= *rightValue);
                case Equal: return static_cast<T>(*leftValue == *rightValue);
                case NotEqual: return static_cast<T>(*leftValue != *rightValue);
                case And: return static_cast<T>(static_cast<bool>(*leftValue) && static_cast<bool>(*rightValue));
                case Or: return static_cast<T>(static_cast<bool>(*leftValue) || static_cast<bool>(*rightValue));
                case BitAnd: return static_cast<T>(*leftValue & *rightValue);
                case BitOr: return static_cast<T>(*leftValue | *rightValue);
                case BitXor: return static_cast<T>(*leftValue ^ *rightValue);
                case BitLShift: return static_cast<T>(*leftValue << *rightValue);
                case BitRShift: return static_cast<T>(*leftValue >> *rightValue);
                case MemberAccess:
                case ScopeResolution:
                case Assignment: return std::nullopt;  // can't fold these
                default: break;
            }
            ASSERT_UNREACHABLE(std::format("Unknown binary operator '{}' in computeExpression",
                                           lexer::tokenTypeToString(binaryExpression->op)));
        }
        case ast::ExpressionKind::BoolLiteralExpression: {
            const auto* boolLiteralExpression = static_cast<const ast::BoolLiteralExpression*>(expression);
            return static_cast<T>(boolLiteralExpression->value);
        }
        case ast::ExpressionKind::CharLiteralExpression: {
            const auto* charLiteralExpression = static_cast<const ast::CharLiteralExpression*>(expression);
            return static_cast<T>(charLiteralExpression->value);
        }
        case ast::ExpressionKind::NumberLiteralExpression: {
            const auto* num = static_cast<const ast::NumberLiteralExpression*>(expression);
            if constexpr (std::is_same_v<T, bool>) {
                const auto result = utils::stringToNumber<std::int64_t>(num->value, num->isFloat);
                if (!result.exists || result.overflowed) {
                    logger(expression, "Invalid boolean numeric literal '{}'", num->value);
                    break;
                }
                return static_cast<bool>(result.value != 0);
            } else {
                using num_t_helper
                    = std::conditional_t<(sizeof(T) > sizeof(std::int64_t)), mnstl::int128_t, std::int64_t>;
                using num_t = std::conditional_t<std::is_signed_v<T>, num_t_helper, mnstl::mnstl_make_unsigned_t<T>>;
                const utils::string_conversion_result_t result = utils::stringToNumber<num_t>(num->value, num->isFloat);
                if (!result.exists) {
                    logger(expression, "Invalid number literal '{}'", num->value);
                    break;
                }
                if (result.overflowed) {
                    logger(expression, "Number literal '{}' cannot fit in its assigned type", num->value);
                    break;
                }
                return static_cast<T>(result.value);
            }
        }
        case ast::ExpressionKind::PostfixExpression: {
            const auto* postfixExpression = static_cast<const ast::PostfixExpression*>(expression);
            auto operandValue = computeExpression<T>(postfixExpression->left, targetInfo, logger);
            if (!operandValue.has_value()) { break; }
            switch (postfixExpression->op) {
                case UnaryPlus: return static_cast<T>(+*operandValue);
                case UnaryMinus: return static_cast<T>(-*operandValue);
                case Not: return static_cast<T>(!static_cast<bool>(*operandValue));
                case BitNot: {
                    /* cast to int here since ~bool doesn't work (T can be bool)*/
                    return static_cast<T>(~static_cast<int>(*operandValue));
                }
                case Inc:
                case AddressOf:
                case Dereference:
                    // can only increment/decrement lvalues which aren't foldable
                    // addresses are runtime values
                    break;
                default:;
            }
            ASSERT_UNREACHABLE(std::format("Unknown postfix operator '{}' in computeExpression",
                                           lexer::tokenTypeToString(postfixExpression->op)));
        }
        case ast::ExpressionKind::PrefixExpression: {
            // can only increment/decrement lvalues which aren't foldable
            break;
        }
        case ast::ExpressionKind::SizeofExpression: {
            const auto* sizeofExpression = static_cast<const ast::SizeofExpression*>(expression);
            return sizeofExpression->semanticType->size(targetInfo);
        }
        case ast::ExpressionKind::TypeCastExpression: {
            const auto* typeCastExpression = static_cast<const ast::TypeCastExpression*>(expression);
            return computeExpression<T>(typeCastExpression->originalValue, targetInfo, logger);
        }
        default: break;
    }
    return std::nullopt;
}

}  // namespace Manganese::utils