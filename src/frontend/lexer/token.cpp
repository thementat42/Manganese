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
Token::Token(const TokenType _type, const std::string _lexeme, const size_t _line, const size_t _column) : type(_type), lexeme(std::move(_lexeme)), line(_line), column(_column) {
    if (_type == TokenType::Operator) {
        auto op = operatorFromString(lexeme);
        if (op.has_value()) {
            specialType = op.value();
        }
    } else if (_type == TokenType :: Keyword) {
        auto kw = keywordFromString(lexeme);
        if (kw.has_value()) {
            specialType = kw.value();
        }
    } else {
        specialType = std::monostate();
    }
}

Token::Token(const TokenType _type, const char _lexeme, const size_t _line, const size_t _column) : Token(_type, std::string(1, _lexeme), _line, _column) {}  // Defer to string constructor to avoid duplicating code

TokenType Token::getType() const {
    return type;
}

std::optional<OperatorType> Token::getOperatorType() const {
    if (std::holds_alternative<OperatorType>(specialType)) {
        return std::get<OperatorType>(specialType);
    }
    return std::nullopt;
}

std::optional<KeywordType> Token::getKeywordType() const {
    if (std::holds_alternative<KeywordType>(specialType)) {
        return std::get<KeywordType>(specialType);
    }
    return std::nullopt;
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
#endif  // DEBUG
    type = _type;
    // Reset specialType if needed
    if (_type != TokenType::Operator && _type != TokenType::Keyword) {
        specialType = std::monostate{};
    }
}

void Token::overrideOperatorType(OperatorType _type) {
    #if DEBUG
    printf(
        "Warning: overriding token operator type from %s to %s (line %d in file %s)\n",
        operatorToString(std::get<OperatorType>(specialType)).c_str(),
        operatorToString(_type).c_str(),
        __LINE__,
        __FILE__);
#endif  // DEBUG
    specialType = _type;
    type = TokenType::Operator;
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
    std::cout << "Token: Type: " << tokenTypeToString(type) << ", Lexeme: \"" << lexeme << "\"";

    if (std::holds_alternative<OperatorType>(specialType)) {
        std::cout << ", Operator: " << operatorToString(std::get<OperatorType>(specialType));
    } else if (std::holds_alternative<KeywordType>(specialType)) {
        std::cout << ", Keyword: " << keywordToString(std::get<KeywordType>(specialType));
    }

    std::cout << " (Line " << line << ", Column " << column << ")\n";
#endif  // DEBUG
}

void Token::log(Token token) {
    token.log();
}

}  // namespace lexer
MANG_END
