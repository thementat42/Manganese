/**
 * @file operators.h
 * @brief This file contains the definition of operators functionality for the Manganese compiler.
 */

#ifndef OPERATORS_H
#define OPERATORS_H

#include <optional>
#include <string>
#include <unordered_map>

#include "../../global_macros.h"

/**
 * @brief Defines all supported operator types in Manganese
 *
 * This enum represents all operators recognized by the compiler,
 * categorized by their functional purpose (arithmetic, comparison, etc.).
 * Each operator is documented with its corresponding symbol.
 *
 * Used by the lexer for token classification and by the parser
 * for expression handling and precedence rules.
 */
namespace manganese {
namespace core {
enum class OperatorType : uint8_t {
    //~ Arithmetic Operators
    Plus,      // `+`
    Minus,     // `-`
    Mul,       // `*`
    Div,       // `/`
    FloorDiv,  // `//`
    Mod,       // `%`
    Exp,       // `**`
    Inc,       // `++`
    Dec,       // `--`

    //~ Arithmetic Assignment Operators
    // Overrides the value of the variable in place (e.g. x += 2 is the same as x = x + 2)
    PlusAssign,      // `+=`
    MinusAssign,     // `-=`
    MulAssign,       // `*=`
    DivAssign,       // `/=`
    FloorDivAssign,  // `//=`
    ModAssign,       // `%=`
    ExpAssign,       // `**=`

    //~ Comparison Operators
    GreaterThan,         // `>`
    GreaterThanOrEqual,  // `>=`
    LessThan,            // `<`
    LessThanOrEqual,     // `<=`,
    Equal,               // `==`
    NotEqual,            // `!=`

    //~ Boolean Operators
    And,  // `&&`
    Or,   // `||`
    Not,  // `!`

    //~ Bitwise Operators
    BitAnd,     // `&`
    BitOr,      // `|`
    BitNot,     // `~`
    BitXor,     // `^`
    BitLShift,  // `<<`
    BitRShift,  // `>>`

    //~ Bitwise Assignment Operators
    // Overrides the value of the variable in place (e.g. x &= y is the same as x = x & y)
    BitAndAssign,     // `&=`
    BitOrAssign,      // `|=`
    BitNotAssign,     // `~=`
    BitXorAssign,     // `^=`
    BitLShiftAssign,  // `<<=`
    BitRShiftAssign,  // `>>=`

    //~ Pointer Operators
    AddressOf,    // `?`
    Dereference,  // `@`

    //~ Access Operators
    MemberAccess,     // `.`
    ScopeResolution,  // `::`

    //~ Misc
    Assignment,  // `=`
    Arrow,       // `->`
    Ellipsis,    // `...`
};

enum class OperatorBindingPower : uint8_t {
    Default = 0,
    Comma = 1,
    Assignment = 2,
    Logical = 3,
    Relational = 4,
    Additive = 5,
    Multiplicative = 6,
    Unary = 7,
    Call = 8,
    Member = 9,
    Primary = 10,
};

// TODO: Add debug check that these are in the right order

/**
 * @brief Maps string representations of operators to their corresponding OperatorType enum values.
 */
extern std::unordered_map<std::string, const OperatorType> operatorMap;

/**
 * @brief Convert a string to the corresponding OperatorType enum
 * @param op The string to convert
 * @return std::optional<OperatorType> The corresponding OperatorType, or std::nullopt if not found
 */
std::optional<OperatorType> operatorFromString(const std::string& op);
/**
 * @brief Convert OperatorType enum to string representation
 * @param op The OperatorType to convert
 * @return String representation of the OperatorType
 */
std::optional<std::string> operatorToString(std::optional<OperatorType> op);

}  // namespace core
}  // namespace manganese

// Hash specialization for OperatorType, for lookups in the parser
namespace std {  //! Ok since extending std
template <>
struct hash<manganese::core::OperatorType> {
    size_t operator()(const manganese::core::OperatorType& op) const noexcept {
        return static_cast<size_t>(op);
    }
};
}  // namespace std

#endif  // OPERATORS_H