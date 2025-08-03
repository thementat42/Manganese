/**
 * @file operators.h
 * @brief Defines operator precedence levels and operator binding powers for the Manganese parser.
 *
 * This header provides Operator struct, which encapsulates the left and right binding powers for operators.
 * There are utility static methods to construct prefix, postfix, binary, and right-associative operators.
 *
 * @note Higher values in @ref Precedence indicate higher precedence.
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_OPERATORS_H
#define MANGANESE_INCLUDE_FRONTEND_PARSER_OPERATORS_H

#include <frontend/lexer/token.h>
#include <global_macros.h>

namespace Manganese {
namespace parser {

/**
 * @brief Enumeration of operator precedence levels (bigger = higher precedence)
 */
enum class Precedence : uint8_t {
    Default = 0,
    Arrow = 1,  // Not really needed
    Assignment = 2,  // = and op=
    TypeCast = 2,  // as
    LogicalOr = 3,  // ||
    LogicalAnd = 4,  // &&
    BitwiseOr = 5,  // |
    BitwiseXor = 6,  // ^
    BitwiseAnd = 7,  // &
    Equality = 8,  // == and !=
    Relational = 9,  // <, >, <=, >=
    BitwiseShift = 10,  // << and >>
    Additive = 11,  // + and -
    Multiplicative = 12,  // *, /, and %
    Exponential = 13,  // ^^
    Unary = 14,  // +, -, !, ~, & (address of), * (dereference), ++, --
    Postfix = 15,  // ++ , --, [], ()
    Member = 16,  // . (member access)
    ScopeResolution = 17,  // ::
    Generic = 17,  // @ (for generics)
    Primary = 18  // (expression), literal, identifier, etc.
};

struct Operator {
    Precedence leftBindingPower, rightBindingPower;

    static Operator prefix(Precedence rightBindingPower_ = Precedence::Default) {
        return Operator{.leftBindingPower = Precedence::Unary, .rightBindingPower = rightBindingPower_};
    }

    static Operator postfix(Precedence leftBindingPower_ = Precedence::Default) {
        return Operator{.leftBindingPower = leftBindingPower_, .rightBindingPower = Precedence::Postfix};
    }

    static Operator binary(Precedence bindingPower) {
        return Operator{.leftBindingPower = bindingPower, .rightBindingPower = bindingPower};
    }

    static Operator rightAssociative(Precedence bindingPower) {
        auto rightValue = static_cast<std::underlying_type<Precedence>::type>(bindingPower) - 1;
        return Operator{.leftBindingPower = bindingPower, .rightBindingPower = static_cast<Precedence>(rightValue)};
    }
};
}  // namespace parser
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_OPERATORS_H
