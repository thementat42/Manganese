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
    Token(const TokenType type_, const std::string lexeme_, const size_t line_, const size_t column_,
          bool invalid_ = false) NOEXCEPT_IF_RELEASE :
        type(type_),
        lexeme(lexeme_),
        line(line_),
        column(column_),
        invalid(invalid_) {
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
        return type == TokenType::Inc || type == TokenType::Dec || type == TokenType::BitAnd || type == TokenType::Mul
            || type == TokenType::AddressOf || type == TokenType::Dereference;
    }
    constexpr bool isLiteral() const noexcept {
        return type == TokenType::IntegerLiteral || type == TokenType::FloatLiteral || type == TokenType::StrLiteral
            || type == TokenType::CharLiteral || type == TokenType::True || type == TokenType::False;
    }
    constexpr bool isBracket() const noexcept {
        return type == TokenType::LeftParen || type == TokenType::RightParen || type == TokenType::LeftBrace
            || type == TokenType::RightBrace || type == TokenType::LeftSquare || type == TokenType::RightSquare;
    }
    constexpr bool isPrimitiveType() const noexcept {
        return type == TokenType::Int8 || type == TokenType::Int16 || type == TokenType::Int32
            || type == TokenType::Int64 || type == TokenType::UInt8 || type == TokenType::UInt16
            || type == TokenType::UInt32 || type == TokenType::UInt64 || type == TokenType::Float32
            || type == TokenType::Float64 || type == TokenType::Char || type == TokenType::Bool
            || type == TokenType::String;
    }
    constexpr bool hasUnaryCounterpart() const noexcept {
    return type == TokenType::Plus ||  // + can be addition or unary plus
        type == TokenType::Minus ||  // - can be subtraction or unary minus
        type == TokenType::BitAnd ||  // & can be bitwise AND or address-of operator
        type == TokenType::Mul;  // * can be multiplication or dereference operator
    }

        /**
     * @note Parser only: be careful
     */
    void overrideType(TokenType type_, std::string lexeme_ = "");

    // These functions are long, so are implemented in a separate header
    constexpr TokenType getUnaryCounterpart() const NOEXCEPT_IF_RELEASE;
    std::string toString() const noexcept;
};

//~ Helpers, not tied to the Token class
constexpr std::string tokenTypeToString(TokenType type) NOEXCEPT_IF_RELEASE;

constexpr TokenType keyword_lookup(const std::string_view& s);
constexpr TokenType operator_lookup(const std::string_view& s);

}  // namespace lexer
}  // namespace Manganese
#endif  // MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_BASE_HPP
