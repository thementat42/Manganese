/**
 * @file operators.h
 * @brief This file contains the definition of operators functionality for the Manganese compiler.
 */

#ifndef OPERATORS_H
#define OPERATORS_H

#include <optional>
#include <string>
#include <unordered_map>

#include "../../../global_macros.h"

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
MANG_BEGIN
namespace lexer {
enum class OperatorType : unsigned char {
    //~ Arithmetic Operators
    ADD,        // `+`
    SUB,        // `-`
    MUL,        // `*`
    DIV,        // `/`
    FLOOR_DIV,  // `//`
    MOD,        // `%`
    EXP,        // `**`
    INC,        // `++`
    DEC,        // `--`

    //~ Arithmetic Assignment Operators
    // Overrides the value of the variable in place (e.g. x += 2 is the same as x = x + 2)
    ADD_ASSIGN,        // `+=`
    SUB_ASSIGN,        // `-=`
    MUL_ASSIGN,        // `*=`
    DIV_ASSIGN,        // `/=`
    FLOOR_DIV_ASSIGN,  // `//=`
    MOD_ASSIGN,        // `%=`
    EXP_ASSIGN,        // `**=`

    //~ Comparison Operators
    GT,   // `>`
    GEQ,  // `>=`
    LT,   // `<`
    LEQ,  // `<=`,
    EQ,   // `==`
    NEQ,  // `!=`

    //~ Boolean Operators
    AND,  // `&&`
    OR,   // `||`
    NOT,  // `!`

    //~ Bitwise Operators
    BIT_AND,     // `&`
    BIT_OR,      // `|`
    BIT_NOT,     // `~`
    BIT_XOR,     // `^`
    BIT_LSHIFT,  // `<<`
    BIT_RSHIFT,  // `>>`

    //~ Bitwise Assignment Operators
    // Overrides the value of the variable in place (e.g. x &~= y is the same as x = x &~ y)
    BIT_AND_ASSIGN,     // `&=`
    BIT_OR_ASSIGN,      // `|=`
    BIT_NOT_ASSIGN,     // `~=`
    BIT_XOR_ASSIGN,     // `^=`
    BIT_LSHIFT_ASSIGN,  // `<<=`
    BIT_RSHIFT_ASSIGN,  // `>>=`

    //~ Pointer Operators
    ADDRESS,  // `?`
    DEREF,    // `@`

    //~ Access Operators
    MEMBER,            // `.`
    SCOPE_RESOLUTION,  // `::`

    //~ Misc
    ASSIGNMENT,  // `=`
    ARROW,       // `->`
};
extern std::unordered_map<std::string, const OperatorType> operator_map;

/**
 * @brief Convert a string to OperatorType enum
 * @param op The string to convert
 * @return std::optional<OperatorType> The corresponding OperatorType, or std::nullopt if not found
 */
std::optional<OperatorType> operatorFromString(const std::string& op);
/**
 * @brief Convert OperatorType enum to string representation
 * @param op The OperatorType to convert
 * @return String representation of the OperatorType
 */
std::string operatorToString(OperatorType op);

}  // namespace lexer
MANG_END

#endif // OPERATORS_H