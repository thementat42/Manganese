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

ParsedFile Parser::parse() {
    // Parse the header (module declaration and imports)
    if (currentToken().getType() == TokenType::Module) {
        DISCARD(parseModuleDeclarationStatement());
    }
    while (currentToken().getType() == TokenType::Import) {
        DISCARD(parseImportStatement());
    }

    this->hasParsedFileHeader = true;  // Now, setting a module or import name should be a warning

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
    lexer->blockComments.shrink_to_fit();
    return ParsedFile{
        .moduleName = moduleName,
        .imports = std::move(imports),
        .program = std::move(program),
        .blockComments = std::move(lexer->blockComments)
    };
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
    return expectToken(expectedType, "Unexpected token: ");
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

std::string importToString(const Import& import) {
    std::string res = "import ";
    for (size_t i = 0; i < import.path.size() ; ++i) {
        res += import.path[i];
        if (i < import.path.size() - 1) [[likely]] {
            res += "::";
        }
    }
    if (!import.alias.empty()) {
        res += " as " + import.alias;
    }
    return res + ";";
}

}  // namespace parser
}  // namespace Manganese
