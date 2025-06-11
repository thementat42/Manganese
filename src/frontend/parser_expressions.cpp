/**
 * @file parser_expressions.cpp
 * @brief This file contains the implementation of expression parsing in the parser. It is split into its own file for readability and maintainability.
 */

#include "../global_macros.h"
#include "../io/include/logging.h"
#include "include/parser.h"

MANGANESE_BEGIN

namespace parser {
constexpr uint8_t BINARY = 2;
constexpr uint8_t OCTAL = 8;
constexpr uint8_t DECIMAL = 10;
constexpr uint8_t HEXADECIMAL = 16;

ExpressionPtr Parser::parseExpression(OperatorBindingPower bindingPower) {
    TokenType type = currentToken().getType();
    auto nudIterator = nullDenotationLookup.find(type);
    if (nudIterator == nullDenotationLookup.end()) {
        UNREACHABLE("No null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    ExpressionPtr left = nudIterator->second(this);
    
    //! NOTE: For some bizzare reason, the only way this works is to first update the type outside the loop once, then update it inside the loop.
    //! Updating the type at the start of the loop but not outside it also doesn't work
    // ! I don't know why this is the case, but DO NOT REMOVE THIS OUTSIDE UPDATE

    type = currentToken().getType();  // update the type since the null denotation handler will have advanced the token
    while (!done()) {
        auto bindingPowerIterator = bindingPowerLookup.find(type);
        if (bindingPowerIterator == bindingPowerLookup.end() ||
        bindingPowerIterator->second <= bindingPower) {
            break;
        }
        
        auto ledIterator = leftDenotationLookup.find(type);
        if (ledIterator == leftDenotationLookup.end()) {
            UNREACHABLE("No left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }
        left = ledIterator->second(this, std::move(left), bindingPowerLookup[type]);
        type = currentToken().getType();  // update the type since the left denotation handler will have advanced the token
    }
    return left;
}

ExpressionPtr Parser::parseExponentiationExpression(ExpressionPtr left, OperatorBindingPower bindingPower) {
    auto operatorToken = advance();

    // For right associativity we need to parse the right-hand side with one less binding power
    // e.g. 2 ** 3 ** 4 should be parsed as 2 ** (3 ** 4) not (2 ** 3) ** 4
    auto rightBindingPower = static_cast<OperatorBindingPower>(static_cast<int>(bindingPower) - 1);

    auto right = parseExpression(rightBindingPower);

    return std::make_unique<ast::BinaryExpression>(
        std::move(left), operatorToken.getType(), std::move(right));
}

ExpressionPtr Parser::parseBinaryExpression(ExpressionPtr left, OperatorBindingPower bindingPower) {
    auto op = advance().getType();
    auto right = parseExpression(bindingPower);

    return std::make_unique<ast::BinaryExpression>(
        std::move(left), op, std::move(right));
}

ExpressionPtr Parser::parsePrimaryExpression() {
    using lexer::TokenType;
    using std::stof, std::stod;
    using std::stoi, std::stol, std::stoll, std::stoul, std::stoull;
    using std::tolower;
    auto token = advance();
    str lexeme = token.getLexeme();

    switch (token.getType()) {
        case TokenType::StrLiteral:
            return make_unique<ast::StringExpression>(lexeme);
        case TokenType::Identifier:
            return make_unique<ast::SymbolExpression>(lexeme);
        case TokenType::IntegerLiteral: {
            // Extract integer suffix (u, l, ll, ul, ull, etc.)
            int base = determineNumberBase(lexeme);
            if (base != 10) {
                // Strip the base prefix (the first two characters: 0x, 0b, 0o) from the lexeme
                lexeme.erase(0, 2);
            }
            std::string numericPart = lexeme;
            std::string suffix;
            // Scan backwards for suffix letters
            while (!numericPart.empty() && (isalpha(numericPart.back()) || numericPart.back() == '_')) {
                char c = static_cast<char>(tolower(numericPart.back()));
                suffix = std::string(1, c) + suffix;  // prepend to suffix, ignoring case for simplicity
                numericPart.pop_back();
            }

            return createIntegerLiteralNode(suffix, numericPart, base);
        }
        case TokenType::FloatLiteral:
            // Check for floating-point suffixes
            return make_unique<ast::NumberExpression>(
                tolower(lexeme.back()) == 'f' ? stof(lexeme) : stod(lexeme));

        default:
            UNREACHABLE("Invalid Token Type in parsePrimaryExpression: " + lexer::tokenTypeToString(token.getType()));
    }
}

const ExpressionPtr Parser::createIntegerLiteralNode(str& suffix, str& numericPart, int base) {
    if (suffix == "ull" || suffix == "llu") {
        return make_unique<ast::NumberExpression>(stoull(numericPart, nullptr, base));
    } else if (suffix == "ll") {
        return make_unique<ast::NumberExpression>(stoll(numericPart, nullptr, base));
    } else if (suffix == "ul" || suffix == "lu") {
        return make_unique<ast::NumberExpression>(stoul(numericPart, nullptr, base));
    } else if (suffix == "u") {
        return make_unique<ast::NumberExpression>(stoul(numericPart, nullptr, base));
    } else if (suffix == "l") {
        return make_unique<ast::NumberExpression>(stol(numericPart, nullptr, base));
    } else {
        return make_unique<ast::NumberExpression>(stoi(numericPart, nullptr, base));
    }
}

int Parser::determineNumberBase(const str& lexeme) {
    if (lexeme.length() <= 2) {
        return 10;  // Default to decimal if the lexeme is too short to be a valid base-prefixed number
    }
    if (lexeme[0] != '0') {
        // No base prefix, assume decimal
        return 10;
    }
    switch (lexeme[1]) {
        case 'x':
        case 'X':
            return HEXADECIMAL;
        case 'b':
        case 'B':
            return BINARY;
        case 'o':
        case 'O':
            return OCTAL;
        default:  // No valid base prefix, assume decimal
            return DECIMAL;
    }
}

}  // namespace parser

MANGANESE_END