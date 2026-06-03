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

#include <core.hpp>
#include <string>

#include "token_type.hpp"

namespace Manganese {
namespace lexer {

/**
 * @brief Representation of a token
 */
class Token {
   private:
    bool _isInvalid;
    TokenType _type;
    std::string _lexeme;
    size_t line, _column;

    template <class... TokenTypes>
    constexpr inline bool typeMatchesOneOf(TokenTypes&&... types) const noexcept {
        return ((_type == types) || ...);
    }

   public:
    Token() noexcept = default;
    Token(const TokenType type, const std::string& lexeme, const size_t line, const size_t column,
          bool isInvalid = false) :
        _isInvalid(isInvalid), _type(type), _lexeme(lexeme), line(line), _column(column) {
        // Special lexeme override cases
        if (_type == TokenType::Int32) {
            _lexeme = "int32";
        } else if (_type == TokenType::Float32) {
            _lexeme = "float32";
        }
    };
    ~Token() noexcept = default;

    constexpr bool isKeyword() const noexcept {
        return _type >= TokenType::_keywordStart && _type <= TokenType::_keywordEnd;
    }
    constexpr bool isOperator() const noexcept {
        return _type >= TokenType::_operatorStart && _type <= TokenType::_operatorEnd;
    }

    constexpr bool isInvalid() const noexcept { return _isInvalid; }
    constexpr TokenType getType() const noexcept { return _type; }
    constexpr std::string getLexeme() const noexcept { return _lexeme; }
    constexpr size_t getLine() const noexcept { return line; }
    constexpr size_t getColumn() const noexcept { return _column; }

    constexpr bool isPrefixOperator() const noexcept {
        using enum TokenType;
        return typeMatchesOneOf(Inc, Dec, BitAnd, Mul, AddressOf, Dereference);
    }
    constexpr bool isLiteral() const noexcept {
        using enum TokenType;
        return typeMatchesOneOf(IntegerLiteral, FloatLiteral, StrLiteral, CharLiteral, True, False);
    }
    constexpr bool isBracket() const noexcept {
        using enum TokenType;
        return typeMatchesOneOf(LeftParen, RightParen, LeftBrace, RightBrace, LeftSquare, RightSquare);
    }
    constexpr bool isPrimitiveType() const noexcept {
        using enum TokenType;
        return typeMatchesOneOf(Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Float32, Float64, Int128,
                                UInt128, Char, Bool, String);
    }
    constexpr bool hasUnaryCounterpart() const noexcept {
        using enum TokenType;
        return typeMatchesOneOf(Plus,  // + can be addition or unary plus
                                Minus,  // - can be subtraction or unary minus
                                BitAnd,  // & can be bitwise AND or address-of operator
                                Mul);  // * can be multiplication or dereference operator
    }

    /**
     * @note Parser only: be careful
     */
    void overrideType(TokenType, std::string = "");

    // These functions are long, so are implemented in a separate header
    TokenType getUnaryCounterpart() const NOEXCEPT_IF_RELEASE;
    std::string toString() const noexcept;
};

//~ Helpers, not tied to the Token class
std::string tokenTypeToString(TokenType type);
TokenType keywordLookup(const std::string_view& s) noexcept;

}  // namespace lexer
}  // namespace Manganese
#endif  // MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_BASE_HPP
