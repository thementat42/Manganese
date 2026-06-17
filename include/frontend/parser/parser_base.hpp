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

#include <core.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <io/logging.hpp>
#include <memory>
#include <mnstl/chunk_allocator.hxx>
#include <string>
#include <vector>

#include "operators.hpp"

namespace Manganese {
namespace parser {
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
    struct {
        bool hasParsedFileHeader : 1 = false;  // Processing module and import
        bool hasError : 1 = false;
        bool isParsingBlockPrecursor : 1 = false;  // if/for/while, etc.
    };

   public:  // public methods
    Parser(const std::string& source, lexer::Mode mode, mnstl::chunk_allocator& allocatorReference) :
        lexer(std::make_unique<lexer::Lexer>(source, mode)),
        arena(allocatorReference) {
        initializeLookups();
        initializeTypeLookups();
    }

    // Avoid file ownership issues
    Parser(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser& operator=(Parser&&) = delete;

    ~Parser() noexcept = default;

    ParsedFile parse();

   private:  // private methods
    using statementHandler_t = ast::Statement* (Parser::*)();
    using nudHandler_t = ast::Expression* (Parser::*)();
    using nudHandler_types_t = ast::Type* (Parser::*)();
    using ledHandler_t = ast::Expression* (Parser::*)(ast::Expression*, Precedence);
    using ledHandler_types_t = ast::Type* (Parser::*)(ast::Type*, Precedence);

    //~ Lookups
    static inline std::array<statementHandler_t, static_cast<size_t>(TokenType::_tokenCount)> statementLookup{};
    static inline std::array<nudHandler_t, static_cast<size_t>(TokenType::_tokenCount)> nudLookup{};
    static inline std::array<ledHandler_t, static_cast<size_t>(TokenType::_tokenCount)> ledLookup{};
    static inline std::array<Operator, static_cast<size_t>(TokenType::_tokenCount)> operatorPrecedenceMap{};

    static inline std::array<nudHandler_types_t, static_cast<size_t>(TokenType::_tokenCount)> nudLookup_types{};
    static inline std::array<ledHandler_types_t, static_cast<size_t>(TokenType::_tokenCount)> ledLookup_types{};
    static inline std::array<Operator, static_cast<size_t>(TokenType::_tokenCount)> operatorPrecedenceMap_type{};

    //~ Parsing functions

    // Expression Parsing
    ast::Expression* parseExpression(Precedence precedence);
    ast::Expression* parseAggregateInstantiationExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseAggregateLiteralExpression();
    ast::Expression* parseArrayInstantiationExpression();
    ast::Expression* parseAssignmentExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseBinaryExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseFunctionCallExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseGenericExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseIndexingExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseMemberAccessExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseParenthesizedExpression();
    ast::Expression* parsePostfixExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parsePrefixExpression();
    ast::Expression* parsePrimaryExpression();
    ast::Expression* parseScopeResolutionExpression(ast::Expression* left, Precedence precedence);
    ast::Expression* parseTypeCastExpression(ast::Expression* left, Precedence precedence);

    // Statement Parsing
    ast::Statement* parseStatement();
    ast::Statement* parseAggregateDeclarationStatement();
    ast::Statement* parseAliasStatement();
    ast::Statement* parseBreakStatement();
    ast::Statement* parseContinueStatement();
    ast::Statement* parseDoWhileLoopStatement();
    ast::Statement* parseEnumDeclarationStatement();
    ast::Statement* parseForLoopStatement();
    ast::Statement* parseFunctionDeclarationStatement();
    ast::Statement* parseIfStatement();
    ast::Statement* parseImportStatement();
    ast::Statement* parseModuleDeclarationStatement();
    ast::Statement* parseRedundantSemicolon();
    ast::Statement* parseReturnStatement();
    ast::Statement* parseSwitchStatement();
    ast::Statement* parseVariableDeclarationStatement();
    /**
     * @brief Parses statements that are preceded by a visibility modifier
     * @example (public/private) (function/aggregate/enum declaration)
     */
    ast::Statement* parseVisibilityAffectedStatement();
    ast::Statement* parseWhileLoopStatement();

    // Type Parsing

    ast::Type* parseType(Precedence precedence);
    ast::Type* parseArrayType(ast::Type* left, Precedence precedence);
    ast::Type* parseAggregateType();
    ast::Type* parseFunctionType();
    ast::Type* parseGenericType(ast::Type* left, Precedence precedence);
    ast::Type* parsePointerType();
    ast::Type* parseParenthesizedType();
    ast::Type* parseSymbolType();

    // ~ Helpers
    ast::Block parseBlock(const std::string& blockName);

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
    static void registerLedHandler_binary(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a postfix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    static void registerLedHandler_postfix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    static void registerLedHandler_prefix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    static void registerLedHandler_type(TokenType type, Precedence precedence, ledHandler_types_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a binary operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    static void registerNudHandler_binary(TokenType type, nudHandler_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have a prefix binding power
     */
    static void registerNudHandler_prefix(TokenType type, nudHandler_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    static void registerNudHandler_type(TokenType type, nudHandler_types_t handler) noexcept;

    /**
     * @brief Register a handler function for a specific statement token type.
     * @param type The token type for which the handler is to be registered.
     * @param handler The function to handle statements of the specified token type.
     */
    static void registerStmtHandler(TokenType type, statementHandler_t handler) noexcept;

    static void initializeLookups() noexcept;
    static void initializeTypeLookups() noexcept;
};

}  // namespace parser
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
