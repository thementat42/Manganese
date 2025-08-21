/**
 * @file token.hpp
 * @brief This file contains the definition of token functionality for the Manganese compiler.
 *
 * Defines the TokenType enumeration, Token class, and related helper functions for the Manganese language lexer.
 *
 * - TokenType: Enumerates all possible token types recognized by the lexer, including keywords, operators, literals,
 * brackets, punctuation, and special markers.
 * - Token: Represents a single token, storing its type, lexeme, position (line and column), and validity.
 * - Helper functions and maps: Provide utilities for mapping strings to token types, converting token types to strings,
 * and identifying keywords/operators.
 *
 * The design separates token type definitions from their usage in the Token class for clarity and maintainability.
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_BASE_HPP

#include <global_macros.hpp>
#include <string>
#include <unordered_map>
#include <utils/number_utils.hpp>

#include "token_type.hpp"

namespace Manganese {
namespace lexer {

/**
 * @brief Representation of a token
 */
class Token {
   private:
    TokenType type;
    std::string lexeme;
    size_t line, column;
    bool invalid;

   public:
    Token() noexcept = default;
    Token(const TokenType type, const std::string lexeme, const size_t line, const size_t column,
          bool invalid = false) noexcept_if_release;
    ~Token() noexcept = default;

    constexpr bool isKeyword() const noexcept {
        return type >= TokenType::__KeywordStart && type <= TokenType::__KeywordEnd;
    }
    constexpr bool isOperator() const noexcept {
        return type >= TokenType::__OperatorStart && type <= TokenType::__OperatorEnd;
    }

    constexpr bool isInvalid() const noexcept { return invalid; }
    constexpr TokenType getType() const noexcept { return type; }
    constexpr std::string getLexeme() const noexcept { return lexeme; }
    constexpr size_t getLine() const noexcept { return line; }
    constexpr size_t getColumn() const noexcept { return column; }

    // These functions are long, so are implemented below
    constexpr bool isPrefixOperator() const noexcept;
    constexpr bool isLiteral() const noexcept;
    constexpr bool isBracket() const noexcept;
    constexpr bool isPrimitiveType() const noexcept;
    constexpr bool hasUnaryCounterpart() const noexcept;
    constexpr TokenType getUnaryCounterpart() const noexcept_if_release;
    std::string toString() const noexcept;

    /**
     * @note Parser only: be careful
     */
    void overrideType(TokenType type_, std::string lexeme_ = "");
};

//~ Helpers, not tied to the Token class
constexpr std::string tokenTypeToString(TokenType type) noexcept_if_release;

extern std::unordered_map<std::string, const TokenType> keywordMap;
extern std::unordered_map<std::string, const TokenType> operatorMap;
TokenType keywordFromString(const std::string& keyword, const size_t line, const size_t column);
TokenType operatorFromString(const std::string& op, const size_t line, const size_t column);



}  // namespace lexer
}  // namespace Manganese
#endif  // MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_BASE_HPP
