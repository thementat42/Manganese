#ifndef MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_HPP

#include <core.hpp>
#include <cstddef>
#include <cstdint>
#include <mnstl/enum_matches.hxx>
#include <string>
#include <string_view>
#include <utility>

namespace Manganese::lexer {

enum class TokenType : std::uint8_t {
#define TOKEN(name, text)    name,
#define KEYWORD(name, text)  name,
#define OPERATOR(name, text) name,

#include <frontend/lexer/tokens.def>

#undef TOKEN
#undef KEYWORD
#undef OPERATOR
};

std::string tokenTypeToString(TokenType type);

class Token {
   private:
    bool _isInvalid;
    TokenType _type;
    std::string _lexeme;
    std::size_t _line, _column;

   public:
    Token() noexcept = default;
    Token(TokenType type, std::string&& lexeme, std::size_t line, std::size_t column, bool isInvalid = false) :
        _isInvalid(isInvalid), _type(type), _lexeme(std::move(lexeme)), _line(line), _column(column) {
        // Special lexeme override cases
        if (_type == TokenType::Int32) {
            _lexeme = "int32";
        } else if (_type == TokenType::Float32) {
            _lexeme = "float32";
        }
    }
    ~Token() noexcept = default;

    constexpr bool isKeyword() const noexcept {
        return BETWEEN(_type, TokenType::_keywordStart, TokenType::_keywordEnd);
    }
    constexpr bool isOperator() const noexcept {
        return BETWEEN(_type, TokenType::_operatorStart, TokenType::_operatorEnd);
    }

    constexpr bool isInvalid() const noexcept { return _isInvalid; }
    constexpr TokenType getType() const noexcept { return _type; }
    constexpr std::string getLexeme() const noexcept { return _lexeme; }
    constexpr std::size_t getLine() const noexcept { return _line; }
    constexpr std::size_t getColumn() const noexcept { return _column; }

    constexpr bool isPrefixOperator() const noexcept {
        using enum TokenType;
        return mnstl::enum_matches<TokenType>(_type, Inc, Dec, BitAnd, Mul, AddressOf, Dereference);
    }
    constexpr bool isLiteral() const noexcept {
        using enum TokenType;
        return mnstl::enum_matches<TokenType>(_type, IntegerLiteral, FloatLiteral, StrLiteral, CharLiteral, True,
                                              False);
    }
    constexpr bool isBracket() const noexcept {
        using enum TokenType;
        return mnstl::enum_matches<TokenType>(_type, LeftParen, RightParen, LeftBrace, RightBrace, LeftSquare,
                                              RightSquare);
    }
    constexpr bool isPrimitiveType() const noexcept {
        using enum TokenType;
        return mnstl::enum_matches<TokenType>(_type, Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Float32,
                                              Float64, Int128, UInt128, Char, Bool, String);
    }
    constexpr bool isInteger() const noexcept {
        using enum TokenType;
        return mnstl::enum_matches<TokenType>(_type, Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Int128,
                                              UInt128);
    }
    constexpr bool hasUnaryCounterpart() const noexcept {
        using enum TokenType;
        return mnstl::enum_matches<TokenType>(_type, Plus,  // + can be addition or unary plus
                                              Minus,  // - can be subtraction or unary minus
                                              BitAnd,  // & can be bitwise AND or address-of operator
                                              Mul);  // * can be multiplication or dereference operator
    }

    /**
     * @note Parser only: be careful
     */
    void overrideType(TokenType newType, std::string newLexeme);

    // These functions are long, so are implemented in a separate header
    TokenType getUnaryCounterpart() const NOEXCEPT_IF_RELEASE;
    std::string toString() const;
};

TokenType keywordLookup(std::string_view s) noexcept;

}  // namespace Manganese::lexer
#endif  // MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_HPP
