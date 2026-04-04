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

#include "token_type.hpp"

namespace Manganese {
namespace lexer {

/**
 * @brief Representation of a token
 */
class Token {
   private:
    bool invalid;
    TokenType type;
    std::string lexeme;
    size_t line, column;

   public:
    Token() noexcept = default;
    Token(const TokenType type_, const std::string lexeme_, const size_t line_, const size_t column_,
          bool invalid_ = false) NOEXCEPT_IF_RELEASE :
        invalid(invalid_),
        type(type_),
        lexeme(lexeme_),
        line(line_),
        column(column_) {
        // Special lexeme override cases
        if (type == TokenType::Int32) {
            lexeme = "int32";
        } else if (type == TokenType::Float32) {
            lexeme = "float32";
        }
    };
    ~Token() noexcept = default;

    constexpr bool isKeyword() const noexcept {
        return type >= TokenType::_keywordStart && type <= TokenType::_keywordEnd;
    }
    constexpr bool isOperator() const noexcept {
        return type >= TokenType::_operatorStart && type <= TokenType::_operatorEnd;
    }

    constexpr bool isInvalid() const noexcept { return invalid; }
    constexpr TokenType getType() const noexcept { return type; }
    constexpr std::string getLexeme() const noexcept { return lexeme; }
    constexpr size_t getLine() const noexcept { return line; }
    constexpr size_t getColumn() const noexcept { return column; }

    constexpr bool isPrefixOperator() const noexcept {
        using enum TokenType;
        return type == Inc || type == Dec || type == BitAnd || type == Mul || type == AddressOf || type == Dereference;
    }
    constexpr bool isLiteral() const noexcept {
        using enum TokenType;
        return type == IntegerLiteral || type == FloatLiteral || type == StrLiteral || type == CharLiteral
            || type == True || type == False;
    }
    constexpr bool isBracket() const noexcept {
        using enum TokenType;
        return type == LeftParen || type == RightParen || type == LeftBrace || type == RightBrace || type == LeftSquare
            || type == RightSquare;
    }
    constexpr bool isPrimitiveType() const noexcept {
        using enum TokenType;
        return type == Int8 || type == Int16 || type == Int32 || type == Int64 || type == UInt8 || type == UInt16
            || type == UInt32 || type == UInt64 || type == Float32 || type == Float64 || type == Int128
            || type == UInt128 || type == Char || type == Bool || type == String;
    }
    constexpr bool hasUnaryCounterpart() const noexcept {
        using enum TokenType;
        return type == Plus ||  // + can be addition or unary plus
            type == Minus ||  // - can be subtraction or unary minus
            type == BitAnd ||  // & can be bitwise AND or address-of operator
            type == Mul;  // * can be multiplication or dereference operator
    }

    /**
     * @note Parser only: be careful
     */
    void overrideType(TokenType type_, std::string lexeme_ = "");

    // These functions are long, so are implemented in a separate header
    TokenType getUnaryCounterpart() const NOEXCEPT_IF_RELEASE;
    std::string toString() const noexcept;
};

//~ Helpers, not tied to the Token class
std::string tokenTypeToString(TokenType type) NOEXCEPT_IF_RELEASE;
TokenType keywordLookup(const std::string_view& s) noexcept;

}  // namespace lexer
}  // namespace Manganese
#endif  // MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_BASE_HPP
