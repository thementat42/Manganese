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
