/**
 * @file token.h
 * @brief This file contains the definition of token functionality for the Manganese compiler.
 */

#ifndef TOKEN
#define TOKEN

#include <optional>
#include <string>
#include <variant>

#include "../../../global_macros.h"
#include "keywords.h"
#include "operators.h"

MANG_BEGIN
namespace lexer {

/**
 * @brief Defines all supported token types in Manganese
 *
 * This enum represents all tokens recognized by the compiler,
 * categorized by their functional purpose (keywords, identifiers, etc.).
 * Each token is documented with its corresponding
 */
enum class TokenType : unsigned char {
    //~ Basic
    KEYWORD,         // if, char, etc.
    IDENTIFIER,      // variables, functions
    STRING_LITERAL,  // "text"
    CHARACTER,       // 'a'
    OPERATOR,        // +, -, *, etc.

    // ~ Numbers
    INTEGER,  // Whole Number
    FLOAT,    // Floating point number

    //~ Brackets
    LEFT_PAREN,    // (
    RIGHT_PAREN,   // )
    LEFT_BRACE,    // {
    RIGHT_BRACE,   // }
    LEFT_SQUARE,   // [
    RIGHT_SQUARE,  // ]
    LEFT_ANGLE,    // <
    RIGHT_ANGLE,   // >

    //~ Punctuation
    SEMICOLON,  // ;
    COLON,      // :
    COMMA,      // ,

    //~ Misc
    END_OF_FILE,
    INVALID,
};

/**
 * @brief Representation of a token
 */
struct Token {
    const TokenType type;
    const std::string lexeme;  // TODO: Consider making this an std::string_view or just a char* (minimize memory use)

    const std::optional<OperatorType> operatorType;
    const std::optional<KeywordType> keywordType;
    const size_t line, column;

    Token(const TokenType type, const std::string lexeme, const size_t line, const size_t column);
    Token(const TokenType type, const char lexeme, const size_t line, const size_t column);

    /**
     * @brief Convert TokenType enum to string representation
     * @param type The TokenType to convert
     * @return String representation of the TokenType
     * @details Only used for debugging purposes. (if the debug flag is not set, this function will be empty)
     */
    static std::string tokenTypeToString(TokenType type);

    /**
     * @brief Print out a token
     * @details This function is used for debugging purposes. (if the debug flag is not set, this function will be empty)
     */
    void log();

    static void log(Token token);
};
}  // namespace lexer
MANG_END

#endif // TOKEN