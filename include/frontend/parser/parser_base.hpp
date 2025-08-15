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
#include <functional>
#include <global_macros.hpp>
#include <io/logging.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <utils/number_utils.hpp>
#include <vector>

#include "frontend/ast/ast_base.hpp"
#include "operators.hpp"

namespace Manganese {
namespace parser {
using ast::StatementUPtr_t, ast::ExpressionUPtr_t, ast::TypeSPtr_t;
using lexer::TokenType, lexer::Token;

struct Import {
    std::vector<std::string> path;
    std::string alias;
};

struct ParsedFile {
    std::string moduleName;
    std::vector<Import> imports;
    ast::Block program;
    std::vector<std::string> blockComments;  // These come from the lexer
};

//~ Helper functions that don't depend on the parser class's methods/variables
constexpr Base determineNumberBase(const std::string& lexeme);
constexpr void extractSuffix(std::string& numericPart, std::string& suffix);
std::string importToString(const Import& import);

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    ast::Visibility defaultVisibility = ast::Visibility::Private;
    std::optional<Token> previousToken;

    std::string moduleName;
    std::vector<Import> imports;

    // Some flags
    bool hasParsedFileHeader = false;  // Processing module and import
    bool hasError = false;
    bool hasCriticalError_ = false;
    bool isParsingBlockPrecursor = false;  // Used to determine if we are parsing a block precursor (if/for/while, etc.)

   public:  // public methods
    Parser() = default;
    Parser(const std::string& source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ParsedFile parse();
    bool hasCriticalError() const noexcept { return hasCriticalError_; }

   private:  // private methods
    using statementHandler_t = std::function<StatementUPtr_t(Parser*)>;
    using nudHandler_t = std::function<ExpressionUPtr_t(Parser*)>;
    using nudHandler_types_t = std::function<TypeSPtr_t(Parser*)>;
    using ledHandler_t = std::function<ExpressionUPtr_t(Parser*, ExpressionUPtr_t, Precedence)>;
    using ledHandler_types_t = std::function<TypeSPtr_t(Parser*, TypeSPtr_t, Precedence)>;

    //~ Lookups
    std::unordered_map<TokenType, statementHandler_t> statementLookup;
    std::unordered_map<TokenType, nudHandler_t> nudLookup;
    std::unordered_map<TokenType, ledHandler_t> ledLookup;
    std::unordered_map<TokenType, Operator> operatorPrecedenceMap;

    std::unordered_map<TokenType, nudHandler_types_t> nudLookup_types;
    std::unordered_map<TokenType, ledHandler_types_t> ledLookup_types;
    std::unordered_map<TokenType, Operator> operatorPrecedenceMap_type;

    //~ Parsing functions

    // ===== Expression Parsing =====
    ExpressionUPtr_t parseExpression(Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseAggregateInstantiationExpression(ExpressionUPtr_t left,
                                                           Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseArrayInstantiationExpression() noexcept_if_release;
    ExpressionUPtr_t parseAssignmentExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseBinaryExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseFunctionCallExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseGenericExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseIndexingExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseMemberAccessExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseParenthesizedExpression() noexcept_if_release;
    ExpressionUPtr_t parsePostfixExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parsePrefixExpression() noexcept_if_release;
    ExpressionUPtr_t parsePrimaryExpression() noexcept_if_release;
    ExpressionUPtr_t parseScopeResolutionExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;
    ExpressionUPtr_t parseTypeCastExpression(ExpressionUPtr_t left, Precedence precedence) noexcept_if_release;

    // ===== Statement Parsing =====
    StatementUPtr_t parseStatement() noexcept_if_release;
    StatementUPtr_t parseAggregateDeclarationStatement() noexcept_if_release;
    StatementUPtr_t parseAliasStatement() noexcept_if_release;
    StatementUPtr_t parseBreakStatement() noexcept_if_release;
    StatementUPtr_t parseContinueStatement() noexcept_if_release;
    StatementUPtr_t parseDoWhileLoopStatement() noexcept_if_release;
    StatementUPtr_t parseEnumDeclarationStatement() noexcept_if_release;
    StatementUPtr_t parseFunctionDeclarationStatement() noexcept_if_release;
    StatementUPtr_t parseIfStatement() noexcept_if_release;
    StatementUPtr_t parseImportStatement() noexcept_if_release;
    StatementUPtr_t parseModuleDeclarationStatement() noexcept_if_release;
    StatementUPtr_t parseRedundantSemicolon() noexcept_if_release;
    StatementUPtr_t parseRepeatLoopStatement() noexcept_if_release;
    StatementUPtr_t parseReturnStatement() noexcept_if_release;
    StatementUPtr_t parseSwitchStatement() noexcept_if_release;
    StatementUPtr_t parseVariableDeclarationStatement() noexcept_if_release;
    /**
     * @brief Parses statements that are preceded by a visibility modifier
     * @example (public/readonly/private) (function/aggregate/enum declaration)
     */
    StatementUPtr_t parseVisibilityAffectedStatement() noexcept_if_release;
    StatementUPtr_t parseWhileLoopStatement() noexcept_if_release;

    // ===== Type Parsing =====

    TypeSPtr_t parseType(Precedence precedence) noexcept_if_release;
    TypeSPtr_t parseArrayType(TypeSPtr_t left, Precedence precedence) noexcept_if_release;
    TypeSPtr_t parseAggregateType() noexcept_if_release;
    TypeSPtr_t parseFunctionType() noexcept_if_release;
    TypeSPtr_t parseGenericType(TypeSPtr_t left, Precedence precedence) noexcept_if_release;
    TypeSPtr_t parsePointerType() noexcept_if_release;
    TypeSPtr_t parseParenthesizedType() noexcept_if_release;
    TypeSPtr_t parseSymbolType() noexcept_if_release;

    // ~ Helpers
    ast::Block parseBlock(std::string blockName) noexcept_if_release;

    /**
     * @details The context is considered unary if the previous token was a left parenthesis
     * @details another operator (except ++, -- or ] (for indexing)) or nothing
     */
    bool isUnaryContext() const noexcept;

    /**
     * @details Get the current token, without consuming it
     */
    [[nodiscard]] inline Token currentToken() const noexcept { return lexer->peekToken(); }
    [[nodiscard]] inline TokenType currentTokenType() noexcept { return currentToken().getType(); }

    /**
     * @details Consume the current token
     */
    [[nodiscard]] inline Token advance() noexcept {
        previousToken = currentToken();
        return lexer->consumeToken();
    }

    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, const std::string& errorMessage);

    /**
     * @brief A wrapper around logging::logError that sets the parser's hasError flag to true.
     */
    inline void logError(const std::string& message, size_t line = 0, size_t col = 0) noexcept {
        logging::logError(message, line, col);
        hasError = true;
    }

    inline bool done() noexcept { return currentTokenType() == TokenType::EndOfFile; }

    // ~ Helpers for lookups

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a binary operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_binary(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a right-associative operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_rightAssoc(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a postfix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_postfix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_prefix(TokenType type, Precedence precedence, ledHandler_t handler) noexcept;
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_type(TokenType type, Precedence precedence, ledHandler_types_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a binary operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    void registerNudHandler_binary(TokenType type, nudHandler_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have a prefix binding power
     */
    void registerNudHandler_prefix(TokenType type, nudHandler_t handler) noexcept;

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    void registerNudHandler_type(TokenType type, nudHandler_types_t handler) noexcept;

    /**
     * @brief Register a handler function for a specific statement token type.
     * @param type The token type for which the handler is to be registered.
     * @param handler The function to handle statements of the specified token type.
     */
    void registerStmtHandler(TokenType type, statementHandler_t handler) noexcept;

    void initializeLookups() noexcept;
    void initializeTypeLookups() noexcept;
};

}  // namespace parser
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_HPP
