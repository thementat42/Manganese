/**
 * @file token.cpp
 * @brief This file contains the implementation of the Token struct for the Manganese compiler.
 */

#include "include/token.h"

#include <iostream>
#include <optional>
#include <string>

#include "../global_macros.h"
#include "include/keywords.h"
#include "include/operators.h"

MANG_BEGIN
namespace core {
Token::Token(const TokenType _type, const std::string _lexeme, const size_t _line, const size_t _column) : type(_type), line(_line), column(_column) {
    if (_type == TokenType::Operator) {
        auto op = operatorFromString(_lexeme);
        if (op.has_value()) {
            data = op.value();
        }
    } else if (_type == TokenType::Keyword) {
        auto kw = keywordFromString(_lexeme);
        if (kw.has_value()) {
            data = kw.value();
        }
    } else {
        data = std::move(_lexeme);
    }
}

Token::Token(const TokenType _type, const char _lexeme, const size_t _line, const size_t _column) : Token(_type, std::string(1, _lexeme), _line, _column) {}  // Defer to string constructor to avoid duplicating code

TokenType Token::getType() const {
    return type;
}

std::optional<OperatorType> Token::getOperatorType() const {
    if (std::holds_alternative<OperatorType>(data)) {
        return std::get<OperatorType>(data);
    }
    return std::nullopt;
}

std::optional<KeywordType> Token::getKeywordType() const {
    if (std::holds_alternative<KeywordType>(data)) {
        return std::get<KeywordType>(data);
    }
    return std::nullopt;
}

std::string Token::getLexeme() const {
    if (std::holds_alternative<OperatorType>(data)) {
        return operatorToString(std::get<OperatorType>(data));
    } else if (std::holds_alternative<KeywordType>(data)) {
        return keywordToString(std::get<KeywordType>(data));
    }
    return std::get<std::string>(data);
}

size_t Token::getLine() const {
    return line;
}

size_t Token::getColumn() const {
    return column;
}

void Token::overrideType(TokenType _type, std::string _lexeme) {
#if DEBUG
    printf(
        "Warning: overriding token type from %s to %s (line %d in file %s)\n",
        tokenTypeToString(type).c_str(),
        tokenTypeToString(_type).c_str(),
        __LINE__,
        __FILE__);
#endif  // DEBUG
    type = _type;
    // Reset data if needed
    if (_type != TokenType::Operator && _type != TokenType::Keyword) {
        data = _lexeme;
    }
}

void Token::overrideOperatorType(OperatorType _type) {
#if DEBUG
    printf(
        "Warning: overriding token operator type from %s to %s (line %d in file %s)\n",
        operatorToString(std::get<OperatorType>(data)).c_str(),
        operatorToString(_type).c_str(),
        __LINE__,
        __FILE__);
#endif  // DEBUG
    data = _type;
    type = TokenType::Operator;
}

DEBUG_FUNC std::string Token::tokenTypeToString(TokenType type) {
#if DEBUG
    switch (type) {
        // Basic
        case TokenType::Keyword:
            return "Keyword";
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::StrLiteral:
            return "String Literal";
        case TokenType::CharLiteral:
            return "Char Literal";
        case TokenType::Operator:
            return "Operator";

        // Numbers
        case TokenType::Integer:
            return "Integer";
        case TokenType::Float:
            return "Float";

        // Brackets
        case TokenType::LeftParen:
            return "Left Parenthesis";
        case TokenType::RightParen:
            return "Right Parenthesis";
        case TokenType::LeftBrace:
            return "Left Brace";
        case TokenType::RightBrace:
            return "Right Brace";
        case TokenType::LeftSquare:
            return "Left Square";
        case TokenType::RightSquare:
            return "Right Square";
        case TokenType::LeftAngle:
            return "Left Angle";
        case TokenType::RightAngle:
            return "Right Angle";

        // Punctuation
        case TokenType::Semicolon:
            return "Semicolon";
        case TokenType::Comma:
            return "Comma";
        case TokenType::Colon:
            return "Colon";

        // Misc
        case TokenType::EndOfFile:
            return "End Of File";
        case TokenType::Invalid:
            return "Invalid";

        default:
            return "Unknown Token Type";
    }
#else   // ^ DEBUG ^ | v !DEBUG v
    return "";
#endif  // DEBUG */
}

DEBUG_FUNC void Token::log() const {
#if DEBUG
    std::cout << "Token: " << tokenTypeToString(type) << " ("
              << getLexeme() << ") at line " << line << ", column " << column;
    std::cout << '\n';
#endif  // DEBUG
}

void Token::log(const Token& token) {
    token.log();
}

}  // namespace core
MANG_END
