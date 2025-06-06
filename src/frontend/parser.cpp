#include "include/parser.h"

#include <stdio.h>

#include <iostream>

#include "../global_macros.h"
#include "include/ast.h"

//! For generics, might have to do two passes
//! First pass: just identify what the function/bundle/blueprint names are and keep those in a table
//! Then re-parse the file using that table to handle the "less-than vs generic" ambiguity
//! On the second pass, any time you see a function/bundle/blueprint name followed by a left angle bracket (<), treat it as a generic type (it's invalid to compare a function/bundle/blueprint name with something else)

namespace manganese {
namespace parser {

Parser::Parser(const str& source, lexer::Mode mode) : lexer(std::make_unique<lexer::Lexer>(source, mode)) {
    initializeLookups();
}

ast::Block Parser::parse() {
    ast::Block body;
    while (!done()) {
        body.push_back(parseStatement());

        // Flush the cache so there are no holdover tokens
        tokenCache.clear();
        tokenCachePosition = 0;
    }
    return body;
}

ExpressionPtr Parser::parsePrimaryExpression() {
    using lexer::TokenType;
    using std::make_unique, std::stoi, std::stof, std::stod;
    auto token = consumeToken();
    str lexeme = token.getLexeme();
    switch (token.getType()) {
        case TokenType::IntegerLiteral:
            return make_unique<ast::NumberExpression>(stoi(lexeme));
        case TokenType::FloatLiteral:
            if (lexeme.back() == 'f' || lexeme.back() == 'F') {
                return make_unique<ast::NumberExpression>(stof(lexeme));
            } else {
                return make_unique<ast::NumberExpression>(stod(lexeme));
            }
        case TokenType::StrLiteral:
            return make_unique<ast::StringExpression>(lexeme);
        case TokenType::Identifier:
            return make_unique<ast::SymbolExpression>(lexeme);
        default:
            std::cerr << ("Invalid Token Type in parsePrimaryExpression: " + lexer::tokenTypeToString(token.getType()));
            abort();
    }
}

ExpressionPtr Parser::parseExpression(OperatorBindingPower bindingPower) {
    // First, parse the null denotated expression
    lexer::TokenType type = peekToken().getType();
    auto it = nullDenotationLookup.find(type);
    if (it == nullDenotationLookup.end()) {
        // TODO: Replace with proper error message
        std::cerr << ("No null denotation function for token type: " + lexer::tokenTypeToString(type));
        abort();
    }
    nullDenotationHandler_t nullDenotationFunction = it->second;
    ExpressionPtr left = nullDenotationFunction(this);

    // While we have a left denoted expression and (current binding power) < (current token binding power), continue parsing
    while (!done()) {
        type = peekToken().getType();  // the null denotation function advances the lexer so update the type
        if (type == TokenType::Semicolon) {
            // End of the expression -- don't keep parsing
            break;
        }

        // Check if current token's binding power is high enough
        auto bindingPowerIt = bindingPowerLookup.find(type);
        if (bindingPowerIt == bindingPowerLookup.end() || bindingPowerIt->second <= bindingPower) {
            break;
        }

        auto it = leftDenotationLookup.find(type);
        if (it == leftDenotationLookup.end()) {
            // TODO: Replace with proper error message
            std::cerr << ("No left denotation function for token type: " + lexer::tokenTypeToString(type));
            abort();
        }
        leftDenotationHandler_t leftDenotationFunction = it->second;
        left = leftDenotationFunction(this, std::move(left), bindingPowerLookup[type]);
    }
    return left;
}

ExpressionPtr Parser::parseBinaryExpression(ExpressionPtr left, OperatorBindingPower bindingPower) {
    auto operatorToken = consumeToken();
    auto right = parseExpression(bindingPower);

    return std::make_unique<ast::BinaryExpression>(std::move(left), operatorToken.getType(), std::move(right));
}

inline void Parser::initializeLookups() {
    using lexer::TokenType;

    //~ Logical
    led(TokenType::And, OperatorBindingPower::Logical, parseBinaryExpression);
    led(TokenType::Or, OperatorBindingPower::Logical, parseBinaryExpression);

    //~ Relational
    led(TokenType::LessThan, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::LessThanOrEqual, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::GreaterThan, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::GreaterThanOrEqual, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::Equal, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::NotEqual, OperatorBindingPower::Relational, parseBinaryExpression);

    //~ Additive & Multiplicative
    led(TokenType::Plus, OperatorBindingPower::Additive, parseBinaryExpression);
    led(TokenType::Minus, OperatorBindingPower::Additive, parseBinaryExpression);
    led(TokenType::Mul, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Div, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::FloorDiv, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Mod, OperatorBindingPower::Multiplicative, parseBinaryExpression);

    // ~ Literals & Symbols
    nud(TokenType::IntegerLiteral, parsePrimaryExpression);
    nud(TokenType::FloatLiteral, parsePrimaryExpression);
    nud(TokenType::StrLiteral, parsePrimaryExpression);
    nud(TokenType::Identifier, parsePrimaryExpression);
}

ast::StatementPtr Parser::parseStatement() {
    lexer::TokenType type = peekToken().getType();
    auto it = statementLookup.find(type);
    if (it != statementLookup.end()) {
        return (it->second)(this);
    }
    auto expression = parseExpression(OperatorBindingPower::Default);
    expectToken(TokenType::Semicolon);
    return std::make_unique<ast::ExpressionStatement>(std::move(expression));
}

Token Parser::expectToken(TokenType expectedType) {
    auto token = peekToken();
    auto tokenType = token.getType();
    if (tokenType != expectedType) {
        fprintf(stderr, "Expected %s, got %s intstead\n",
                lexer::tokenTypeToString(expectedType).c_str(),
                lexer::tokenTypeToString(tokenType).c_str());
        exit(EXIT_FAILURE);
    }
    return consumeToken();
}
Token Parser::expectToken(const std::initializer_list<TokenType>& expectedTypes) {
    auto token = peekToken();
    auto tokenType = token.getType();
    for (auto expectedType : expectedTypes) {
        if (tokenType == expectedType) {
            return consumeToken();
        }
    }
    fprintf(stderr, "Expected one of: ");
    for (auto expectedType : expectedTypes) {
        fprintf(stderr, "%s, ", lexer::tokenTypeToString(expectedType).c_str());
    }
    fprintf(stderr, "but got %s instead\n", lexer::tokenTypeToString(tokenType).c_str());
    exit(EXIT_FAILURE);
}
}  // namespace parser
}  // namespace manganese