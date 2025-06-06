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

}  // namespace core
}  // namespace manganese

#endif  // OPERATORS_H