#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_OPERATORS_HPP
#define MANGANESE_INCLUDE_FRONTEND_PARSER_OPERATORS_HPP

namespace Manganese::parser {

/**
 * @brief Enumeration of operator precedence levels (bigger = higher precedence)
 */
enum class Precedence : char {
    Default = 0,
    Arrow = 1,  // Not really needed
    Assignment = 2,  // = and op=
    TypeCast = 2,  // as
    Ternary = 3,
    LogicalOr = 4,  // ||
    LogicalAnd = 5,  // &&
    BitwiseOr = 6,  // |
    BitwiseXor = 7,  // ^
    BitwiseAnd = 8,  // &
    Equality = 9,  // == and !=
    Relational = 10,  // <, >, <=, >=
    BitwiseShift = 11,  // << and >>
    Additive = 12,  // + and -
    Multiplicative = 13,  // *, /, and %
    Exponential = 14,  // ^^
    Unary = 15,  // +, -, !, ~, & (address of), * (dereference), ++, --
    Postfix = 16,  // ++ , --, [], ()
    Member = 17,  // . (member access)
    ScopeResolution = 18,  // ::
    Generic = 18,  // @ (for generics)
    Primary = 19  // (expression), literal, identifier, etc.
};

struct Operator {
    Precedence leftBindingPower, rightBindingPower;

    constexpr static Operator prefix(Precedence _rightBindingPower) noexcept {
        return Operator{.leftBindingPower = Precedence::Unary, .rightBindingPower = _rightBindingPower};
    }

    constexpr static Operator postfix(Precedence _leftBindingPower) noexcept {
        return Operator{.leftBindingPower = _leftBindingPower, .rightBindingPower = Precedence::Postfix};
    }

    constexpr static Operator binary(Precedence bindingPower) noexcept {
        return Operator{.leftBindingPower = bindingPower, .rightBindingPower = bindingPower};
    }

    constexpr static Operator statement() noexcept {
        return Operator{.leftBindingPower = Precedence::Default, .rightBindingPower = Precedence::Default};
    }

    // a helper just to make initialization clearer in the lookup tables
    constexpr static Operator binaryType(Precedence bindingPower) noexcept { return binary(bindingPower); }
    constexpr static Operator type() noexcept {
        return Operator{.leftBindingPower = Precedence::Primary, .rightBindingPower = Precedence::Default};
    }
};
}  // namespace Manganese::parser

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_OPERATORS_HPP
