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
          _type == TokenType::OPERATOR ? operatorFromString(_lexeme) : std::nullopt),
      keywordType(  // If it's a keyword, keep track of which one it is
          _type == TokenType::KEYWORD ? keywordFromString(_lexeme) : std::nullopt),
      line(_line),
      column(_column) {}

Token::Token(const TokenType _type, const char _lexeme, const size_t _line, const size_t _column)
    : type(_type),
      lexeme(std::string(1, _lexeme)),  // Convert char to string
      operatorType(                     // If it's an operator, keep track of which one it is
          _type == TokenType::OPERATOR ? operatorFromString(std::string(1, _lexeme)) : std::nullopt),
      keywordType(  // If it's a keyword, keep track of which one it is
          _type == TokenType::KEYWORD ? keywordFromString(std::string(1, _lexeme)) : std::nullopt),
      line(_line),
      column(_column) {}

std::string Token::tokenTypeToString(TokenType type) {
#ifdef DEBUG
    switch (type) {
        // Basic
        case TokenType::KEYWORD:
            return "TokenType::KEYWORD";
        case TokenType::IDENTIFIER:
            return "TokenType::IDENTIFIER";
        case TokenType::STRING_LITERAL:
            return "TokenType::STRING_LITERAL";
        case TokenType::CHARACTER:
            return "TokenType::CHARACTER";
        case TokenType::OPERATOR:
            return "TokenType::OPERATOR";

        // Numbers
        case TokenType::INTEGER:
            return "TokenType::INTEGER";
        case TokenType::FLOAT:
            return "TokenType::FLOAT";

        // Brackets
        case TokenType::LEFT_PAREN:
            return "TokenType::LEFT_PAREN";
        case TokenType::RIGHT_PAREN:
            return "TokenType::RIGHT_PAREN";
        case TokenType::LEFT_BRACE:
            return "TokenType::LEFT_BRACE";
        case TokenType::RIGHT_BRACE:
            return "TokenType::RIGHT_BRACE";
        case TokenType::LEFT_SQUARE:
            return "TokenType::LEFT_SQUARE";
        case TokenType::RIGHT_SQUARE:
            return "TokenType::RIGHT_SQUARE";
        case TokenType::LEFT_ANGLE:
            return "TokenType::LEFT_ANGLE";
        case TokenType::RIGHT_ANGLE:
            return "TokenType::RIGHT_ANGLE";

        // Punctuation
        case TokenType::SEMICOLON:
            return "TokenType::SEMICOLON";
        case TokenType::COMMA:
            return "TokenType::COMMA";
        case TokenType::COLON:
            return "TokenType::COMMA";

        // Misc
        case TokenType::END_OF_FILE:
            return "TokenType::END_OF_FILE";
        case TokenType::INVALID:
            return "TokenType::INVALID";

        default:
            return "Unknown TokenType";
    }
#else  // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif // DEBUG */
}

void Token::log() {
#ifdef DEBUG
    std::cout << "Token: " << tokenTypeToString(type) << " \"" << lexeme << "\"";

    if (operatorType.has_value()) {
        std::cout << " Operator: " << operatorToString(operatorType.value());
    }

    if (keywordType.has_value()) {
        std::cout << " Keyword: " << keywordToString(keywordType.value());
    }

    std::cout << " Line: " << line << " Column: " << column << '\n';
#endif // DEBUG
}

void Token::log(Token token) {
    token.log();
}

}  // namespace lexer
MANG_END
