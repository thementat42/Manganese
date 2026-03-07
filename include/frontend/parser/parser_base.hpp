/**
 * @file parser_base.hpp
 * @brief Base definitions and declarations for the parser.
 *
 * This header defines the core Parser class and related structures for parsing
 * source code into an AST, using Pratt parsing, via lookup tables.
 *
 * This file defines the various methods for parsing different sequences of tokens, grouped into the same categories as
 * the AST nodes (statements, expressions and types)
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
#define MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP

#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <global_macros.hpp>
#include <io/logging.hpp>
#include <memory>
#include <mnstl/chunk_allocator.hxx>
#include <string>
#include <utils/number_utils.hpp>
#include <vector>

#include "frontend/ast/ast_base.hpp"
#include "operators.hpp"


namespace Manganese {
namespace parser {
using ast::TypeSPtr_t;
using lexer::TokenType, lexer::Token;

struct Import {
    std::vector<std::string> path;
    std::string alias;
};

struct ParsedFile {
    std::string moduleName;
    std::vector<Import> imports;
    ast::Block program;
};

//~ Helper functions that don't depend on the parser class's methods/variables
constexpr Base determineNumberBase(const std::string& lexeme);
constexpr void extractSuffix(std::string& numericPart, std::string& suffix);
std::string importToString(const Import& import);

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    constexpr static inline ast::Visibility defaultVisibility = ast::Visibility::Private;
    std::optional<Token> previousToken;

    std::string moduleName;
    std::vector<Import> imports;
    mnstl::chunk_allocator& arena;

    // Some flags
    bool hasParsedFileHeader = false;  // Processing module and import
    bool hasError = false;
    bool hasCriticalError_ = false;
    bool isParsingBlockPrecursor = false;  // Used to determine if we are parsing a block precursor (if/for/while, etc.)

   public:  // public methods
    Parser(const std::string& source, lexer::Mode mode, mnstl::chunk_allocator& arena);

    // Avoid file ownership issues
    Parser(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser& operator=(Parser&&) = delete;

    ~Parser() noexcept = default;

    ParsedFile parse();
    bool hasCriticalError() const noexcept { return hasCriticalError_; }

   private:  // private methods
    using statementHandler_t = ast::Statement* (Parser::*)();
    using nudHandler_t = ast::Expression* (Parser::*)();
    using nudHandler_types_t = TypeSPtr_t (Parser::*)();
    using ledHandler_t = ast::Expression* (Parser::*)(ast::Expression*, Precedence);
    using ledHandler_types_t = TypeSPtr_t (Parser::*)(TypeSPtr_t, Precedence);

    //~ Lookups
    std::array<statementHandler_t, static_cast<size_t>(TokenType::_tokenCount)> statementLookup{};
    std::array<nudHandler_t, static_cast<size_t>(TokenType::_tokenCount)> nudLookup{};
    std::array<ledHandler_t, static_cast<size_t>(TokenType::_tokenCount)> ledLookup{};
    std::array<Operator, static_cast<size_t>(TokenType::_tokenCount)> operatorPrecedenceMap{};

    std::array<nudHandler_types_t, static_cast<size_t>(TokenType::_tokenCount)> nudLookup_types{};
    std::array<ledHandler_types_t, static_cast<size_t>(TokenType::_tokenCount)> ledLookup_types{};
    std::array<Operator, static_cast<size_t>(TokenType::_tokenCount)> operatorPrecedenceMap_type{};

    //~ Parsing functions

    // ===== Expression Parsing =====
    ast::Expression* parseExpression(Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseAggregateInstantiationExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseAggregateLiteralExpression() NOEXCEPT_IF_RELEASE;
    ast::Expression* parseArrayInstantiationExpression() NOEXCEPT_IF_RELEASE;
    ast::Expression* parseAssignmentExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseBinaryExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseFunctionCallExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseGenericExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseIndexingExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseMemberAccessExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseParenthesizedExpression() NOEXCEPT_IF_RELEASE;
    ast::Expression* parsePostfixExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parsePrefixExpression() NOEXCEPT_IF_RELEASE;
    ast::Expression* parsePrimaryExpression() NOEXCEPT_IF_RELEASE;
    ast::Expression* parseScopeResolutionExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    ast::Expression* parseTypeCastExpression(ast::Expression* left, Precedence precedence) NOEXCEPT_IF_RELEASE;

    // ===== Statement Parsing =====
    ast::Statement* parseStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseAggregateDeclarationStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseAliasStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseBreakStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseContinueStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseDoWhileLoopStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseEnumDeclarationStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseFunctionDeclarationStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseIfStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseImportStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseModuleDeclarationStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseRedundantSemicolon() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseRepeatLoopStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseReturnStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseSwitchStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseVariableDeclarationStatement() NOEXCEPT_IF_RELEASE;
    /**
     * @brief Parses statements that are preceded by a visibility modifier
     * @example (public/private) (function/aggregate/enum declaration)
     */
    ast::Statement* parseVisibilityAffectedStatement() NOEXCEPT_IF_RELEASE;
    ast::Statement* parseWhileLoopStatement() NOEXCEPT_IF_RELEASE;

    // ===== Type Parsing =====

    TypeSPtr_t parseType(Precedence precedence) NOEXCEPT_IF_RELEASE;
    TypeSPtr_t parseArrayType(TypeSPtr_t left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    TypeSPtr_t parseAggregateType() NOEXCEPT_IF_RELEASE;
    TypeSPtr_t parseFunctionType() NOEXCEPT_IF_RELEASE;
    TypeSPtr_t parseGenericType(TypeSPtr_t left, Precedence precedence) NOEXCEPT_IF_RELEASE;
    TypeSPtr_t parsePointerType() NOEXCEPT_IF_RELEASE;
    TypeSPtr_t parseParenthesizedType() NOEXCEPT_IF_RELEASE;
    TypeSPtr_t parseSymbolType() NOEXCEPT_IF_RELEASE;

    // ~ Helpers
    ast::Block parseBlock(std::string blockName) NOEXCEPT_IF_RELEASE;

    /**
     * @details The context is considered unary if the previous token was a left parenthesis
     * @details another operator (except ++, -- or ] (for indexing)) or nothing
     */
    bool isUnaryContext() const noexcept;

    /**
     * @details Get the current token, without consuming it
     */
    [[nodiscard]] inline Token peekToken() const noexcept { return lexer->peekToken(); }
    [[nodiscard]] inline TokenType peekTokenType() noexcept { return lexer->peekToken().getType(); }

    /**
     * @details Consume the current token
     */
    [[nodiscard]] inline Token consumeToken() noexcept {
        previousToken = peekToken();
        return lexer->consumeToken();
    }

    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, const std::string& errorMessage);

    /**
     * @brief A wrapper around logging::logError that sets the parser's hasError flag to true.
     */
    template <class... Args>
    inline void logError(size_t line, size_t col, std::format_string<Args...> message, Args&&... args) noexcept {
        logging::logError(line, col, message, std::forward<Args>(args)...);
        hasError = true;
    }

    inline bool done() noexcept { return peekTokenType() == TokenType::EndOfFile; }

    // ~ Helpers for lookups

    /**
     * @brief Convert a TokenType to an index for the lookup tables
     * @param t The token type to convert
     * @return The token type as an index
     */
    constexpr static size_t tokenToIndex(TokenType t) noexcept { return static_cast<size_t>(t); }

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a binary operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    constexpr void registerLedHandler_binary(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a postfix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    constexpr void registerLedHandler_postfix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    constexpr void registerLedHandler_prefix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    constexpr void registerLedHandler_type(TokenType type, Precedence precedence, ledHandler_types_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a binary operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    constexpr void registerNudHandler_binary(TokenType type, nudHandler_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have a prefix binding power
     */
    constexpr void registerNudHandler_prefix(TokenType type, nudHandler_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    constexpr void registerNudHandler_type(TokenType type, nudHandler_types_t handler) noexcept;

    /**
     * @brief Register a handler function for a specific statement token type.
     * @param type The token type for which the handler is to be registered.
     * @param handler The function to handle statements of the specified token type.
     */
    constexpr void registerStmtHandler(TokenType type, statementHandler_t handler) noexcept;

    void initializeLookups() noexcept;
    void initializeTypeLookups() noexcept;
};

}  // namespace parser
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
