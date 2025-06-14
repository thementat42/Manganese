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
    Token token = currentToken();

    // Handle operators which have a unary and a binary version (e.g. `-` can be a unary negation or a binary subtraction)
    if (isUnaryContext() && token.hasUnaryCounterpart()) {
        token.overrideType(token.getUnaryCounterpart());
        bindingPower = OperatorBindingPower::Unary;
    }
    TokenType type = token.getType();

    auto nudIterator = nullDenotationLookup.find(type);
    if (nudIterator == nullDenotationLookup.end()) {
        UNREACHABLE("No null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    ExpressionPtr left = nudIterator->second(this);

    while (!done()) {
        token = currentToken();
        
        if (isUnaryContext() && token.hasUnaryCounterpart()) {
            token.overrideType(token.getUnaryCounterpart());
            bindingPower = OperatorBindingPower::Unary;
        }
        type = token.getType();

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
    }
    return left;
}

ExpressionPtr Parser::parseAssignmentExpression(ExpressionPtr left, OperatorBindingPower bindingPower) {
    TokenType op = advance().getType();
    ExpressionPtr right = parseExpression(bindingPower);

    return std::make_unique<ast::AssignmentExpression>(std::move(left), op, std::move(right));
}

ExpressionPtr Parser::parseParenthesizedExpression() {
    DISCARD(advance());  // Consume the left parenthesis
    ExpressionPtr expr = parseExpression(OperatorBindingPower::Default);
    expectToken(lexer::TokenType::RightParen, "Expected a right parenthesis to close the expression");
    return expr;
}

ExpressionPtr Parser::parsePrefixExpression() {
    TokenType op = advance().getType();
    auto right = parseExpression(OperatorBindingPower::Default);

    return std::make_unique<ast::PrefixExpression>(op, std::move(right));
}

ExpressionPtr Parser::parseExponentiationExpression(ExpressionPtr left, OperatorBindingPower bindingPower) {
    auto operatorToken = advance();

    // For right associativity we need to parse the right-hand side with one less binding power
    // e.g. 2 ^^ 3 ^^ 4 should be parsed as 2 ^^ (3 ^^ 4) not (2 ^^ 3) ^^ 4
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
        case TokenType::FloatLiteral:
            // Check for floating-point suffixes
            return make_unique<ast::NumberExpression>(
                tolower(lexeme.back()) == 'f' ? stof(lexeme) : stod(lexeme));
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
            DISABLE_CONVERSION_WARNING
            while (!numericPart.empty() && (isalpha(numericPart.back()) || numericPart.back() == '_')) {
                suffix.insert(suffix.begin(), tolower(numericPart.back()));  // Convert to lowercase
                numericPart.pop_back();  // Remove the suffix letter
            }
            ENABLE_CONVERSION_WARNING

            return createIntegerLiteralNode(suffix, numericPart, base);
        }
        default:
            UNREACHABLE("Invalid Token Type in parsePrimaryExpression: " + lexer::tokenTypeToString(token.getType()));
    }
}

ExpressionPtr createIntegerLiteralNode(str& suffix, str& numericPart, int base) {
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

int determineNumberBase(const str& lexeme) {
    if (lexeme.length() <= 2) {
        return DECIMAL;  // Default to decimal if the lexeme is too short to be a valid base-prefixed number
    }
    if (lexeme[0] != '0') {
        // No base prefix, assume decimal
        return DECIMAL;
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