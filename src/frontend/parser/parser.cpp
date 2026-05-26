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
#include <io/logging.hpp>
#include <memory>
#include <string>


namespace Manganese {
namespace parser {

Parser::Parser(const std::string& source, lexer::Mode mode, mnstl::chunk_allocator& _arena) :
    lexer(make_unique<lexer::Lexer>(source, mode)), arena(_arena) {
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
    return ParsedFile{.moduleName = moduleName, .imports = std::move(imports), .program = std::move(program)};
}

// Helper functions
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
    Token tok = peekToken();
    if (tok.getType() == expectedType) { return consumeToken(); }
    logging::logError(tok.getLine(), tok.getColumn(), "{} (expected '{}' but got '{}')", errorMessage,
                      lexer::tokenTypeToString(expectedType), lexer::tokenTypeToString(tok.getType()));
    this->hasError = true;

    return hasError ? lexer::Token{} : consumeToken();
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
