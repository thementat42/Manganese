#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <io/logging.hpp>
#include <string>
#include <utility>
#include "core.hpp"

namespace Manganese::parser {

ParsedFile Parser::parse() {
    // Parse the header (module declaration and imports)
    if (peekTokenType() == TokenType::Module) { parseModuleDeclarationStatement(); }
    while (peekTokenType() == TokenType::Import) { parseImportStatement(); }

    flags.hasParsedFileHeader = true;  // Now, setting a module or import name should be a warning

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

ast::Statement* Parser::parseVariableDeclarationStatement() {
    ast::Type* explicitType = nullptr;
    ast::Expression* value = nullptr;
    ast::Visibility visibility = defaultVisibility;

    DISCARD(consumeToken());  // Consume the 'let' token
    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        DISCARD(consumeToken());  // Consume the 'mut' token
        isMutable = true;
    }
    std::string name = expectToken(TokenType::Identifier,
                                   std::format("Expected variable name after '{}'", isMutable ? "let mut" : "let"))
                           .getLexeme();
    if (peekTokenType() == TokenType::Colon) {
        DISCARD(consumeToken());  // Consume the colon
        if (peekTokenType() == TokenType::Public) {
            visibility = ast::Visibility::Public;
            DISCARD(consumeToken());  // Consume the public keyword
        } else if (peekTokenType() == TokenType::Private) [[unlikely]] {
            // private is the default so it'd mainly be used for emphasis
            visibility = ast::Visibility::Private;
            DISCARD(consumeToken());  // Consume the private keyword
        }
        explicitType = parseType(Precedence::Default);
    }
    if (peekTokenType() != TokenType::Semicolon) {
        expectToken(TokenType::Assignment, "Expected '=' or ';' after variable name");
        value = parseExpression(Precedence::Default);
    } else if (explicitType == nullptr) {
        // If no value is provided, we need to have a type
        expectToken(TokenType::Colon, "Expected ':' to specify type for variable without initial value");
        explicitType = parseType(Precedence::Default);
    }

    expectToken(TokenType::Semicolon, "Expected semicolon after variable declaration");

    return arena.emplace<ast::VariableDeclarationStatement>(isMutable, std::move(name), visibility, value,
                                                            explicitType);
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
    flags.hasError = true;

    return flags.hasError ? lexer::Token{} : consumeToken();
}

std::string importToString(const Import& import) {
    std::string res = "import ";
    for (std::size_t i = 0; i < import.path.size(); ++i) {
        res += import.path[i];
        if (i < import.path.size() - 1) [[likely]] { res += "::"; }
    }
    if (!import.alias.empty()) { res += " as " + import.alias; }
    return res + ";";
}

ast::Block Parser::parseBlock(const std::string& blockName) {
    expectToken(TokenType::LeftBrace, "Expected a '{' to start " + blockName);
    ast::Block block;
    while (!done() && peekTokenType() != TokenType::RightBrace) {
        if (peekTokenType() == TokenType::Semicolon) {
            // skip bare semicolons
            DISCARD(consumeToken());
            continue;
        }
        block.push_back(parseStatement());
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end " + blockName);
    if (block.empty()) {
        logging::logWarning(peekToken().getLine(), peekToken().getColumn(), "{} is empty", blockName);
    }
    return block;
}

}  // namespace Manganese::parser
