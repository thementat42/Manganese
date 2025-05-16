/**
 * @file token.cpp
 * @brief This file contains the implementation of the Token struct for the Manganese compiler.
 */

#include "include/token.h"

#include <iostream>
#include <optional>
#include <string>

#include "../../global_macros.h"
#include "include/keywords.h"
#include "include/operators.h"

MANG_BEGIN
namespace lexer {
Token::Token(const TokenType _type, const std::string _lexeme, const size_t _line, const size_t _column)
    : type(_type),
      lexeme(std::move(_lexeme)),  // Move the lexeme string to avoid copying it again
      operatorType(                // If it's an operator, keep track of which one it is
          _type == TokenType::Operator ? operatorFromString(_lexeme) : std::nullopt),
      keywordType(  // If it's a keyword, keep track of which one it is
          _type == TokenType::Keyword ? keywordFromString(_lexeme) : std::nullopt),
      line(_line),
      column(_column) {}

Token::Token(const TokenType _type, const char _lexeme, const size_t _line, const size_t _column)
    : type(_type),
      lexeme(std::string(1, _lexeme)),  // Convert char to string
      operatorType(                     // If it's an operator, keep track of which one it is
          _type == TokenType::Operator ? operatorFromString(std::string(1, _lexeme)) : std::nullopt),
      keywordType(  // If it's a keyword, keep track of which one it is
          _type == TokenType::Keyword ? keywordFromString(std::string(1, _lexeme)) : std::nullopt),
      line(_line),
      column(_column) {}

TokenType Token::getType() const {
    return type;
}

std::optional<OperatorType> Token::getOperatorType() const {
    return operatorType;
}

std::optional<KeywordType> Token::getKeywordType() const {
    return keywordType;
}

std::string Token::getLexeme() const {
    return lexeme;
}

size_t Token::getLine() const {
    return line;
}

size_t Token::getColumn() const {
    return column;
}

void Token::overrideType(TokenType _type) {
#if DEBUG
    printf(
        "Warning: overriding token type from %s to %s (line %d in file %s)\n",
        tokenTypeToString(type).c_str(),
        tokenTypeToString(_type).c_str(),
        __LINE__,
        __FILE__);
#endif
    type = _type;
}

void Token::overrideOperatorType(OperatorType _type) {
#if DEBUG
    printf(
        "Warning: overriding operator from %s to %s (line %d in file %s)\n",
        operatorToString(operatorType).c_str(),
        operatorToString(_type).c_str(),
        __LINE__,
        __FILE__);
#endif
    operatorType = _type;
}

std::string Token::tokenTypeToString(TokenType type) {
#if DEBUG
    switch (type) {
        // Basic
        case TokenType::Keyword:
            return "TokenType::Keyword";
        case TokenType::Identifier:
            return "TokenType::Identifier";
        case TokenType::StrLiteral:
            return "TokenType::STRING_LITERAL";
        case TokenType::CharLiteral:
            return "TokenType::CharLiteral";
        case TokenType::Operator:
            return "TokenType::OPERATOR";

        // Numbers
        case TokenType::Integer:
            return "TokenType::Integer";
        case TokenType::Float:
            return "TokenType::Float";

        // Brackets
        case TokenType::LeftParen:
            return "TokenType::LeftParen";
        case TokenType::RightParen:
            return "TokenType::RightParen";
        case TokenType::LeftBrace:
            return "TokenType::LeftBrace";
        case TokenType::RightBrace:
            return "TokenType::RightBrace";
        case TokenType::LeftSquare:
            return "TokenType::LeftSquare";
        case TokenType::RightSquare:
            return "TokenType::RightSquare";
        case TokenType::LeftAngle:
            return "TokenType::LeftAngle";
        case TokenType::RightAngle:
            return "TokenType::RightAngle";

        // Punctuation
        case TokenType::Semicolon:
            return "TokenType::Semicolon";
        case TokenType::Comma:
            return "TokenType::Comma";
        case TokenType::Colon:
            return "TokenType::Colon";

        // Misc
        case TokenType::EndOfFile:
            return "TokenType::EndOfFile";
        case TokenType::Invalid:
            return "TokenType::Invalid";

        default:
            return "Unknown TokenType";
    }
#else   // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif  // DEBUG */
}

void Token::log() const {
#if DEBUG
    std::cout << "Token: " << tokenTypeToString(type) << " \"" << lexeme << "\"";

    if (operatorType.has_value()) {
        std::cout << " Operator: " << operatorToString(operatorType.value());
    }

    if (keywordType.has_value()) {
        std::cout << " Keyword: " << keywordToString(keywordType.value());
    }

    std::cout << " Line: " << line << " Column: " << column << '\n';
#endif  // DEBUG
}

void Token::log(Token token) {
    token.log();
}

}  // namespace lexer
MANG_END
