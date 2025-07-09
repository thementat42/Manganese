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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "operators.h"

namespace Manganese {
namespace parser {
using ast::StatementPtr, ast::ExpressionPtr, ast::TypePtr;
using lexer::TokenType, lexer::Token;

struct Import {
    std::string imported, alias;
};

struct ParsedFile {
    std::string moduleName;
    std::vector<Import> imports;
    ast::Block program;
};

//~ Helper functions that don't depend on the parser class's methods/variables
int determineNumberBase(const std::string &lexeme);
void extractSuffix(std::string &numericPart, std::string &suffix);

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    ast::Visibility defaultVisibility = ast::Visibility::Private;
    size_t tokenCachePosition = 0;
    std::vector<Token> tokenCache;  // Old tokens (for lookbehind)

    // Some flags
    bool hasError = false;
    bool hasCriticalError_ = false;
    bool isParsingBlockPrecursor = false;  // Used to determine if we are parsing a block precursor (if/for/while, etc.)

   public:  // public methods
    Parser() = default;
    Parser(const std::string &source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ast::Block parse();
    bool hasCriticalError() const noexcept { return hasCriticalError_; }

   private:  // private methods
    using statementHandler_t = std::function<StatementPtr(Parser *)>;
    using nudHandler_t = std::function<ExpressionPtr(Parser *)>;
    using nudHandler_types_t = std::function<TypePtr(Parser *)>;
    using ledHandler_t = std::function<ExpressionPtr(Parser *, ExpressionPtr, Precedence)>;
    using ledHandler_types_t = std::function<TypePtr(Parser *, TypePtr, Precedence)>;

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
    ExpressionPtr parseExpression(Precedence bindingPower) noexcept_debug;
    ExpressionPtr parseArrayInstantiationExpression();
    ExpressionPtr parseAssignmentExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseBinaryExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseBundleInstantiationExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseFunctionCallExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseGenericExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseIndexingExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseMemberAccessExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseParenthesizedExpression();
    ExpressionPtr parsePostfixExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parsePrefixExpression();
    ExpressionPtr parsePrimaryExpression() noexcept_debug;
    ExpressionPtr parseScopeResolutionExpression(ExpressionPtr left, Precedence bindingPower);
    ExpressionPtr parseTypeCastExpression(ExpressionPtr left, Precedence bindingPower);

    // ===== Statement Parsing =====

    StatementPtr parseStatement();
    StatementPtr parseBundleDeclarationStatement();
    StatementPtr parseDoWhileLoopStatement();
    StatementPtr parseEnumDeclarationStatement();
    StatementPtr parseFunctionDeclarationStatement();
    StatementPtr parseIfStatement();
    StatementPtr parseRepeatLoopStatement();
    StatementPtr parseReturnStatement();
    StatementPtr parseSwitchStatement();
    StatementPtr parseVariableDeclarationStatement();
    StatementPtr parseWhileLoopStatement();

    //* Type Parsing

    TypePtr parseType(Precedence bindingPower);
    TypePtr parseArrayType(TypePtr left, Precedence rightBindingPower);
    TypePtr parseSymbolType();
    TypePtr parseGenericType(TypePtr left, Precedence rightBindingPower);

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
     * @param bindingPower How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_binary(TokenType type, Precedence bindingPower, ledHandler_t handler);

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a right-associative operator)
     * @param bindingPower How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_rightAssoc(TokenType type, Precedence bindingPower,
                                  ledHandler_t handler);

    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a postfix operator)
     * @param bindingPower How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_postfix(TokenType type, Precedence bindingPower,
                               ledHandler_t handler);
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a prefix operator)
     * @param bindingPower How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_prefix(TokenType type, Precedence bindingPower,
                              ledHandler_t handler);
    /**
     * @brief Register a left denotation handler for `type`
     * @param type The token type associated with the handler (a token indicating a type)
     * @param bindingPower How strongly that operator binds to its neighbour(s)
     * @param handler The function to call when the token type is encountered
     */
    void registerLedHandler_type(TokenType type, Precedence bindingPower, ledHandler_types_t handler);

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
     * @brief Registes a handler function for a specific statement token type.
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
