/**
 * @file parser.cpp
 * Contains the implementation of some basic parser methods
 * @see parser_expressions.cpp
 * @see parser_lookups.cpp
 * @see parser_statements.cpp
 * @see parser_types.cpp
 */

#include <frontend/ast.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include <format>
#include <memory>
#include <string>

namespace Manganese {
namespace parser {

Parser::Parser(const std::string& source, lexer::Mode mode) : lexer(make_unique<lexer::Lexer>(source, mode)) {
    if (lexer->hasCriticalError()) {
        this->hasCriticalError_ = true;
        return;
    }
    initializeLookups();
    initializeTypeLookups();
}

ast::Block Parser::parse() {
    ast::Block program;
    while (!done()) {
        // No need to move thanks to copy elision
        program.push_back(parseStatement());

        // We don't need to look back at old tokens from previous statements,
        // clear the cache to save memory
        tokenCache.clear();
        tokenCachePosition = 0;
    }
    program.shrink_to_fit();  // Avoid having a bunch of allocated but unused memory
    return program;
}

// ===== Helper functions =====
bool Parser::isUnaryContext() const {
    if (tokenCache.empty() || tokenCachePosition == 0) {
        return true;  // If the cache is empty or we're at the start, it's a unary context
    }
    auto lastToken = tokenCache[tokenCachePosition - 1];

    return lastToken.getType() == TokenType::LeftParen ||
           (lastToken.isOperator() && lastToken.getType() != TokenType::Inc &&
            lastToken.getType() != TokenType::Dec);
}

[[nodiscard]] Token Parser::currentToken() {
    while (tokenCachePosition >= tokenCache.size()) {
        tokenCache.push_back(lexer->consumeToken());
    }
    return tokenCache[tokenCachePosition];
}

[[nodiscard]] Token Parser::advance() {
    while (tokenCachePosition >= tokenCache.size()) {
        tokenCache.push_back(lexer->consumeToken());
    }
    return tokenCache[tokenCachePosition++];
}

Token Parser::expectToken(TokenType expectedType) {
    return expectToken(
        expectedType, "Unexpected token: ");
}

Token Parser::expectToken(TokenType expectedType, const std::string& errorMessage) {
    TokenType type = currentToken().getType();
    if (type == expectedType) {
        return advance();
    }
    std::string message = errorMessage +
                          " (expected " + lexer::tokenTypeToString(expectedType) +
                          ", but found " + lexer::tokenTypeToString(type) + ")";
    logError(
        message,
        currentToken().getLine(),
        currentToken().getColumn());
    hasError = true;

    return advance();
}
}  // namespace parser
}  // namespace Manganese
