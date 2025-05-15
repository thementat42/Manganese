/**
 * @file keywords.h
 * @brief This file contains the definition of keywords functionality for the Manganese compiler.
 */

#ifndef KEYWORDS_H
#define KEYWORDS_H
#include <optional>
#include <string>
#include <unordered_map>

#include "../../../global_macros.h"

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
MANG_BEGIN
namespace lexer {
enum class KeywordType : unsigned char {

    //~ Type Qualifiers
    CONST,     // constant variable
    UNSIGNED,  // positive values only
    PTR,       // pointer variables

    //~ Access Levels
    PRIVATE,   // not accessible outside module
    READONLY,  // readable, but can't be modified, outside the module
    PUBLIC,    // can be read and modified outside the module

    //~ Primitive Types
    INT8,     // 8 bit int
    INT16,    // 16 bit int
    INT32,    // 32 bit int
    INT64,    // 64 bit int
    FLOAT,    // floating point
    FLOAT32,  // 32-bit float (single precision)
    FLOAT64,  // 64-bit float (double precision)
    CHAR,     // single character
    BOOL,     // true or false
    VARIANT,  // multiple types
    TRUE,     // boolean true
    FALSE,    // boolean false

    //~ Data Structures
    STRING,  // string of characters
    ARR,     // static arrays
    VEC,     // dynamic arrays
    MAP,     // hashmaps
    SET,     // hashsets

    //~ Control Flow
    //* Conditionals
    IF,    // check a condition
    ELIF,  // if previous condition was false, check this
    ELSE,  // if all previous conditions were false, do this

    //* Pattern Matching
    SWITCH,   // value to compare against
    MATCH,    // value to compare against (more advanced)
    CASE,     // specific value
    DEFAULT,  // if no values matched, do this

    //* Loops
    FOR,
    WHILE,
    REPEAT,    // execute a block of code `n` times -- don't care about index
    DO,        // run once before checking condition (in while loop)
    BREAK,     // stop loop
    CONTINUE,  // skip this iteration

    //~ Functions
    FUNC,    // function
    LAMBDA,  // anonymous function
    RETURN,  // function returns

    //~ Modules & Scoping
    IMPORT,  // bring in another module
    MODULE,  // declare as a module
    AS,      // alias a module

    //~ Encapsulation
    BUNDLE,  // like C's struct
    ENUM,
    BLUEPRINT,  // like a class

    //~ Misc
    ALIAS,    // type aliasing (alias `a` as `b`)
    TYPEOF,   // get the type of a variable
    CAST,     // cast to a new type (cast<"new type">)
    GARBAGE,  // prevent default initialization
};

extern std::unordered_map<std::string, const KeywordType> keyword_map;

/**
 * @brief Convert a string to KeywordType enum
 * @param keyword The string to convert
 * @return std::optional<KeywordType> The corresponding KeywordType, or std::nullopt if not found
 */
std::optional<KeywordType> keywordFromString(const std::string& keyword);

/**
 * @brief Convert KeywordType enum to string representation
 * @param keyword The KeywordType to convert
 * @return String representation of the KeywordType
 * @details Only used for debugging purposes. (if the debug flag is not set, this function will be empty)
 */
std::string keywordToString(KeywordType keyword);


}  // namespace lexer
MANG_END

#endif // KEYWORDS_H