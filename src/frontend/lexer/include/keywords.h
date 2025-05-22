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
    Const,     // constant variable
    Ptr,       // pointer variables

    //~ Access Levels
    ReadOnly,  // readable, but can't be modified, outside the module
    Public,    // can be read and modified outside the module

    //~ Primitive Types
    Int8,     // 8 bit int
    Int16,    // 16 bit int
    Int32,    // 32 bit int
    Int64,    // 64 bit int
    UInt8,     // unsigned 8 bit int
    UInt16,    // unsigned 16 bit int
    UInt32,    // unsigned 32 bit int
    UInt64,    // unsigned 64 bit int
    Float,    // floating point
    Float32,  // 32-bit float (single precision)
    Float64,  // 64-bit float (double precision)
    Char,     // single character
    Bool,     // true or false
    True,     // boolean true
    False,    // boolean false

    //~ Data Structures
    Str,  // string of characters
    Arr,     // static arrays
    Vec,     // dynamic arrays
    Map,     // hashmaps
    Set,     // hashsets

    //~ Control Flow
    //* Conditionals
    If,    // check a condition
    Elif,  // if previous condition was false, check this
    Else,  // if all previous conditions were false, do this

    //* Pattern Matching
    Switch,   // value to compare against
    Case,     // specific value
    Default,  // if no values matched, do this

    //* Loops
    For,
    While,
    Repeat,    // execute a block of code `n` times -- don't care about index
    Do,        // run once before checking condition (in while loop)
    Break,     // stop loop
    Continue,  // skip this iteration

    //~ Functions
    Func,    // function
    Lambda,  // anonymous function
    Return,  // function returns

    //~ Modules & Scoping
    Import,  // bring in another module
    Module,  // declare as a module
    As,      // alias a module

    //~ Encapsulation
    Bundle,  // like C's struct
    Enum,
    Blueprint,  // like a class

    //~ Misc
    Alias,    // type aliasing (alias `a` as `b`)
    TypeOf,   // get the type of a variable
    Cast,     // cast to a new type (cast<"new type">)
    Garbage,  // prevent default initialization
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