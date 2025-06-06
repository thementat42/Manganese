/**
 * @file keywords.h
 * @brief This file contains the definition of keywords functionality for the Manganese compiler.
 */

#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <optional>
#include <string>
#include <unordered_map>

#include "../../global_macros.h"

/**
 * @brief Defines all reserved keywords in Manganese
 *
 * This enum represents all keywords recognized by the compiler,
 * categorized by their functional purpose (types, control flow, etc.).
 * Each keyword is documented with a brief description of its usage.
 *
 * Used by the lexer for token classification and by the parser
 * for syntax validation and semantic analysis. These keywords cannot
 * be used as identifiers in Manganese programs.
 */
namespace manganese {
namespace core {


}  // namespace core
}  // namespace manganese

#endif  // KEYWORDS_H