/**
 * @file parser_base.h
 * @brief Base definitions and declarations for the parser.
 *
 * This header defines the core Parser class and related structures for parsing
 * source code into an AST, using Pratt parsing, via lookup tables.
 * 
 * This file defines the various methods for parsing different sequences of tokens, grouped into the same categories as the AST nodes (statements, expressions and types)
 */

#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_H
#define MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_H

#include <frontend/ast.h>
#include <frontend/lexer.h>
#include <global_macros.h>
#include <io/logging.h>
#include <utils/number_utils.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "operators.h"

namespace Manganese {
namespace parser {
using ast::StatementPtr_t, ast::ExpressionPtr_t, ast::TypePtr_t;
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
Base determineNumberBase(const std::string &lexeme);
void extractSuffix(std::string &numericPart, std::string &suffix);
std::string importToString(const Import &import);

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    ast::Visibility defaultVisibility = ast::Visibility::Private;
    size_t tokenCachePosition = 0;
    std::vector<Token> tokenCache;  // Old tokens (for lookbehind)

    std::string moduleName;
    std::vector<Import> imports;

    // Some flags
    bool hasParsedFileHeader = false;  // Processing module and import
    bool hasError = false;
    bool hasCriticalError_ = false;
    bool isParsingBlockPrecursor = false;  // Used to determine if we are parsing a block precursor (if/for/while, etc.)

   public:  // public methods
    Parser() = default;
    Parser(const std::string &source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ParsedFile parse();
    bool hasCriticalError() const noexcept { return hasCriticalError_; }

   private:  // private methods
    using statementHandler_t = std::function<StatementPtr_t(Parser *)>;
    using nudHandler_t = std::function<ExpressionPtr_t(Parser *)>;
    using nudHandler_types_t = std::function<TypePtr_t(Parser *)>;
    using ledHandler_t = std::function<ExpressionPtr_t(Parser *, ExpressionPtr_t, Precedence)>;
    using ledHandler_types_t = std::function<TypePtr_t(Parser *, TypePtr_t, Precedence)>;

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
    ExpressionPtr_t parseExpression(Precedence precedence) noexcept_if_release;
    ExpressionPtr_t parseArrayInstantiationExpression();
    ExpressionPtr_t parseAssignmentExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseBinaryExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseBundleInstantiationExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseFunctionCallExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseGenericExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseIndexingExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseMemberAccessExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseParenthesizedExpression();
    ExpressionPtr_t parsePostfixExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parsePrefixExpression();
    ExpressionPtr_t parsePrimaryExpression() noexcept_if_release;
    ExpressionPtr_t parseScopeResolutionExpression(ExpressionPtr_t left, Precedence precedence);
    ExpressionPtr_t parseTypeCastExpression(ExpressionPtr_t left, Precedence precedence);

    // ===== Statement Parsing =====

    StatementPtr_t parseStatement();
    StatementPtr_t parseAliasStatement();
    StatementPtr_t parseBundleDeclarationStatement();
    StatementPtr_t parseDoWhileLoopStatement();
    StatementPtr_t parseEnumDeclarationStatement();
    StatementPtr_t parseFunctionDeclarationStatement();
    StatementPtr_t parseIfStatement();
    StatementPtr_t parseImportStatement();
    StatementPtr_t parseModuleDeclarationStatement();
    StatementPtr_t parseRepeatLoopStatement();
    StatementPtr_t parseReturnStatement();
    StatementPtr_t parseSwitchStatement();
    StatementPtr_t parseVariableDeclarationStatement();
    /**
     * Parses statements that are preceded by a visibility modifier
     * (public/readonly/private) (function/bundle/enum declaration)
     */
    StatementPtr_t parseVisibilityAffectedStatement();
    StatementPtr_t parseWhileLoopStatement();

    // ===== Type Parsing =====

    TypePtr_t parseType(Precedence precedence) noexcept_if_release;
    TypePtr_t parseArrayType(TypePtr_t left, Precedence precedence);
    TypePtr_t parseFunctionType();
    TypePtr_t parseGenericType(TypePtr_t left, Precedence precedence);
    TypePtr_t parsePointerType();
    TypePtr_t parseSymbolType();

    // ~ Helpers
    ast::Block parseBlock(std::string blockName);

    /**
     * @details The context is considered unary if the previous token was a left parenthesis
     * @details another operator (except ++, -- or ] (for indexing)) or nothing
     */
    bool isUnaryContext() const;

    /**
     * @details Get the current token, without consuming it
     * @details Will refill the tokenCache if needed
     */
    [[nodiscard]] Token currentToken();

    /**
     * @details Consume the current token
     * @details Will refill the tokenCache if needed
     */
    [[nodiscard]] Token advance();

    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, const std::string &errorMessage);

    /**
     * @brief A wrapper around logging::logError that sets the parser's hasError flag to true.
     */
    inline void logError(const std::string &message, size_t line = 0, size_t col = 0) {
        logging::logError(message, line, col);
        hasError = true;
    }

    inline bool done() { return currentToken().getType() == TokenType::EndOfFile; }

    // ~ Helpers for lookups

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a binary operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_binary(TokenType type, Precedence precedence, ledHandler_t handler);

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a right-associative operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_rightAssoc(TokenType type, Precedence precedence,
                                  ledHandler_t handler);

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a postfix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_postfix(TokenType type, Precedence precedence,
                               ledHandler_t handler);
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_prefix(TokenType type, Precedence precedence,
                              ledHandler_t handler);
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param precedence How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_type(TokenType type, Precedence precedence, ledHandler_types_t handler);

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a binary operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    void registerNudHandler_binary(TokenType type, nudHandler_t handler);

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have a prefix binding power
     */
    void registerNudHandler_prefix(TokenType type, nudHandler_t handler);

    /**
     * @brief Register a null denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param handler The function to call when the token type is encountered
     * @note all lookups registered using this have no binding power
     */
    void registerNudHandler_type(TokenType type, nudHandler_types_t handler);

    /**
     * @brief Register a handler function for a specific statement token type.
     * @param type The token type for which the handler is to be registered.
     * @param handler The function to handle statements of the specified token type.
     */
    void registerStmtHandler(TokenType type, statementHandler_t handler);

    void initializeLookups();
    void initializeTypeLookups();
};

}  // namespace parser
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_H
