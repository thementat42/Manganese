#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <io/logging.hpp>
#include <string>
#include <utility>

namespace Manganese::parser {

Parser::Parser(const std::string& source, lexer::Mode mode, mnstl::chunk_allocator& allocatorReference) :
    lexer(std::make_unique<lexer::Lexer>(source, mode)), arena(allocatorReference), flags() {}

ParsedFile Parser::parse() {
    ast::ModuleDeclarationStatement* fileModule = nullptr;
    ast::Block program;
    std::vector<ast::ImportStatement*> imports;
    // Parse the header (module declaration and imports)
    if (peekTokenType() == TokenType::Module) {
        fileModule = static_cast<ast::ModuleDeclarationStatement*>(parseModuleDeclarationStatement());
        flags.hasModuleDeclaration = true;
    }
    while (peekTokenType() == TokenType::Import) {
        imports.push_back(static_cast<ast::ImportStatement*>(parseImportStatement()));
    }

    flags.hasParsedFileHeader = true;  // Now, setting a module or import name should be a warning

    while (!done()) {
        ast::Statement* stmt = parseStatement();
        if (stmt->kind == ast::StatementKind::ImportStatement) {
            imports.push_back(static_cast<ast::ImportStatement*>(stmt));
        } else if (stmt->kind == ast::StatementKind::ModuleDeclarationStatement && !flags.hasModuleDeclaration) {
            fileModule = static_cast<ast::ModuleDeclarationStatement*>(stmt);
            flags.hasModuleDeclaration = true;
        } else {
            program.push_back(stmt);
        }

        // Lookbehind is only needed within a statement, not across them
        previousToken.reset();
    }
    program.shrink_to_fit();  // Avoid having a bunch of allocated but unused memory
    return ParsedFile{.fileModule = fileModule, .imports = std::move(imports), .program = std::move(program)};
}

// Helper functions
bool Parser::isUnaryContext() const noexcept {
    if (!previousToken) { return true; /* Start of file */ }
    const TokenType lastType = previousToken->getType();

    // Statement or expression delimiters
    if (mnstl::enum_matches(lastType, TokenType::Semicolon, TokenType::LeftParen, TokenType::LeftBrace,
                            TokenType::LeftSquare, TokenType::Comma, TokenType::Colon, TokenType::Assignment)) {
        return true;
    }

    // 2. Binary / prefix operators (e.g. `1 + -2` or `return *ptr`)
    if (previousToken->isOperator() && lastType != TokenType::Inc && lastType != TokenType::Dec) { return true; }

    return false;
}

Token Parser::expectToken(TokenType expectedType) { return expectToken(expectedType, "Unexpected token: "); }

Token Parser::expectToken(TokenType expectedType, const std::string& errorMessage) {
    Token tok = peekToken();
    if (tok.getType() == expectedType) { return consumeToken(); }
    logging::logError(tok.getLine(), tok.getColumn(), "{} (expected '{}' but got '{}')", errorMessage,
                      lexer::tokenTypeToString(expectedType), lexer::tokenTypeToString(tok.getType()));
    flags.hasError = true;
    return lexer::Token{};
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

ast::Statement* Parser::parseVisibilityAffectedStatement() {
    ast::Visibility visibility;
    switch (consumeToken().getType()) {
        case TokenType::Private: visibility = ast::Visibility::Private; break;
        case TokenType::Public: visibility = ast::Visibility::Public; break;
        default:
            ASSERT_UNREACHABLE("Unexpected token type in parseVisibilityAffectedStatement: "
                               + lexer ::tokenTypeToString(peekTokenType()));
    }
    std::size_t startLine = peekToken().getLine(), startColumn = peekToken().getColumn();
    switch (peekTokenType()) {
        case TokenType::Alias: {
            auto* tempAlias = static_cast<ast::AliasStatement*>(parseAliasStatement());
            tempAlias->visibility = visibility;
            return tempAlias;
        }
        case TokenType::Aggregate: {
            auto* tempAggregate
                = static_cast<ast::AggregateDeclarationStatement*>(parseAggregateDeclarationStatement());
            tempAggregate->visibility = visibility;
            return tempAggregate;
        }
        case TokenType::Enum: {
            auto* tempEnum = static_cast<ast::EnumDeclarationStatement*>(parseEnumDeclarationStatement());
            tempEnum->visibility = visibility;
            return tempEnum;
        }
        case TokenType::Func: {
            auto* tempFunction = static_cast<ast::FunctionDeclarationStatement*>(parseFunctionDeclarationStatement());
            tempFunction->visibility = visibility;
            return tempFunction;
        }
        default:
            logError(startLine, startColumn, "{} cannot follow a visibility modifier",
                     lexer::tokenTypeToString(peekTokenType()));
            // Parse the statement as if it had no visibility modifier
            return parseStatement();
    }
}

}  // namespace Manganese::parser
