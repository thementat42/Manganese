/**
 * @file parser.cpp
 * Contains the implementation of some basic parser methods
 * @see parser_expressions.cpp
 * @see parser_lookups.cpp
 * @see parser_statements.cpp
 * @see parser_types.cpp
 */

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <global_macros.hpp>
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
    if (peekTokenType() == TokenType::Module) { DISCARD(parseModuleDeclarationStatement()); }
    while (peekTokenType() == TokenType::Import) { DISCARD(parseImportStatement()); }

    this->hasParsedFileHeader = true;  // Now, setting a module or import name should be a warning

    ast::Block program;
    while (!done()) {
        // No need to move thanks to copy elision
        program.push_back(parseStatement());

        // Lookbehind is only needed within a statement, not across them
        previousToken.reset();
    }
    program.shrink_to_fit();  // Avoid having a bunch of allocated but unused memory
    return ParsedFile{.moduleName = moduleName,
                      .imports = std::move(imports),
                      .program = std::move(program)};
}

// ===== Helper functions =====
bool Parser::isUnaryContext() const noexcept {
    if (!previousToken) {
        // No previous token (this is the start of an expression), so it's a unary context
        // e.g. -3
        return true;
    }
    const Token& lastToken = *previousToken;

    return lastToken.getType() == TokenType::LeftParen
        || (lastToken.isOperator() && lastToken.getType() != TokenType::Inc && lastToken.getType() != TokenType::Dec);
}

Token Parser::expectToken(TokenType expectedType) { return expectToken(expectedType, "Unexpected token: "); }

Token Parser::expectToken(TokenType expectedType, const std::string& errorMessage) {
    // TODO: Better error handling
    // Rather than blindly advancing, we want to continue in such a way that errors don't cascade
    // e.g., right now, in an if statement, if we expect a closing parenthesis (if (condition)), but
    // find an open brace (because the programmer forgot the closing parenthesis), everything breaks
    // -- the expression parsing fails, parsing the actual block fails, then the first statement in the block fails
    // since the first token in that statement got skipped, etc.
    // It might be worth designing different expectToken functions for different statements/contexts
    // e.g., in any block precursor, when a ) is missed, just keep going until we find a {, then parse a block from
    // there
    //, skipping any other logic in the conditional
    TokenType type = peekTokenType();
    if (type == expectedType) { return consumeToken(); }
    std::string message = errorMessage + " (expected " + lexer::tokenTypeToString(expectedType) + ", but found "
        + lexer::tokenTypeToString(type) + ")";
    logError(message, peekToken().getLine(), peekToken().getColumn());
    hasError = true;

    return consumeToken();
}

Token Parser::expectToken(std::initializer_list<TokenType> expectedTypes) {
    return expectToken(expectedTypes, "Unexpected token: ");
}
Token Parser::expectToken(std::initializer_list<TokenType> expectedTypes, const std::string& errorMessage) {
    TokenType type = peekTokenType();
    for (TokenType expectedType : expectedTypes) {
        if (type == expectedType) { return consumeToken(); }
    }
    std::string typesString;
    bool first = true;
    for (TokenType expectedType : expectedTypes) {
        if (!first) [[likely]] { typesString += ", "; }
        typesString += lexer::tokenTypeToString(expectedType);
        first = false;
    }

    std::string message = errorMessage + " (expected one of [" + typesString + "], but found "
        + lexer::tokenTypeToString(type) + ")";

    logError(message, peekToken().getLine(), peekToken().getColumn());
    hasError = true;
    return consumeToken();
}

std::string importToString(const Import& import) {
    std::string res = "import ";
    for (size_t i = 0; i < import.path.size(); ++i) {
        res += import.path[i];
        if (i < import.path.size() - 1) [[likely]] { res += "::"; }
    }
    if (!import.alias.empty()) { res += " as " + import.alias; }
    return res + ";";
}

}  // namespace parser
}  // namespace Manganese
