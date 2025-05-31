/**
 * @file token.h
 * @brief This file contains the definition of token functionality for the Manganese compiler.
 */

#ifndef TOKEN
#define TOKEN

#include <optional>
#include <string>
#include <variant>

#include "../../global_macros.h"
#include "keywords.h"
#include "operators.h"

namespace manganese {
namespace core {

/**
 * @brief Defines all supported token types in Manganese
 *
 * This enum represents all tokens recognized by the compiler,
 * categorized by their functional purpose (keywords, identifiers, etc.).
 * Each token is documented with its corresponding
 */
enum class TokenType : uint8_t {
    //~ Basic
    Keyword,      // if, char, etc.
    Identifier,   // variables, functions
    StrLiteral,   // "text"
    CharLiteral,  // 'a'
    IntegerLiteral,  // Whole Number
    FloatLiteral,    // Floating point number
    Operator,     // +, -, *, etc.

    //~ Brackets
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftSquare,   // [
    RightSquare,  // ]
    LeftAngle,    // <
    RightAngle,   // >

    //~ Punctuation
    Semicolon,  // ;
    Colon,      // :
    Comma,      // ,

    //~ Misc
    EndOfFile,
    Invalid,
};

/**
 * @brief Representation of a token
 */
struct Token {
   private:
    TokenType type;

    // specific data about the token
    // std::string is for tokens with no enum representation (identifiers, numbers, etc.)
    std::variant<OperatorType, KeywordType, std::string> data;
    size_t line, column;

   public:
    Token() = default;
    Token(const TokenType type, const std::string lexeme, const size_t line, const size_t column);
    Token(const TokenType type, const char lexeme, const size_t line, const size_t column);
    Token(const Token& other) = default;
    Token(Token&& other) = default;
    Token& operator=(const Token& other) = default;
    Token& operator=(Token&& other) = default;
    ~Token() = default;

    /**
     * @brief Convert TokenType enum to string representation
     * @param type The TokenType to convert
     * @return String representation of the TokenType
     * @details Only used for debugging purposes. (if the debug flag is not set, this function will be empty)
     */
    static std::string tokenTypeToString(TokenType type);

    TokenType getType() const;
    std::optional<OperatorType> getOperatorType() const;
    std::optional<KeywordType> getKeywordType() const;
    std::string getLexeme() const;
    size_t getLine() const;
    size_t getColumn() const;
    void overrideType(TokenType _type, std::string _lexeme = "");
    void overrideOperatorType(OperatorType _type);

    /**
     * @brief Print out a token
     * @details This function is used for debugging purposes. (if the debug flag is not set, this function will be empty)
     */
    void log() const;

    static void log(const Token& token);
};
}  // namespace core
}

#endif  // TOKEN