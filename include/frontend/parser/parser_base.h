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
    size_t tokenCachePosition;
    ast::Visibility defaultVisibility;
    bool hasError;
    bool hasCriticalError_ = false;
    bool isParsingBlockPrecursor = false;  // Used to determine if we are parsing a block precursor (if/for/while, etc.)
    std::vector<Token> tokenCache;  // Old tokens (for lookbehind)

   public:  // public methods
    Parser() = default;
    Parser(const std::string &source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ast::Block parse();
    bool hasCriticalError() const noexcept { return hasCriticalError_; }

   private:  // private methods
    using statementHandler_t = std::function<StatementPtr(Parser *)>;
    using nullDenotationHandler_t = std::function<ExpressionPtr(Parser *)>;
    using type_nullDenotationHandler_t = std::function<TypePtr(Parser *)>;
    using leftDenotationHandler_t = std::function<ExpressionPtr(Parser *, ExpressionPtr, Precedence)>;
    using type_leftDenotationHandler_t = std::function<TypePtr(Parser *, TypePtr, Precedence)>;

    //~ Lookups
    std::unordered_map<TokenType, statementHandler_t> statementLookup;
    std::unordered_map<TokenType, nullDenotationHandler_t> nullDenotationLookup;
    std::unordered_map<TokenType, leftDenotationHandler_t> leftDenotationLookup;
    std::unordered_map<TokenType, Operator> operatorPrecedenceMap;

    std::unordered_map<TokenType, type_nullDenotationHandler_t> type_nullDenotationLookup;
    std::unordered_map<TokenType, type_leftDenotationHandler_t> type_leftDenotationLookup;
    std::unordered_map<TokenType, Operator> type_operatorPrecedenceMap;

    //~ Parsing functions

    // ===== Expression Parsing =====
    ExpressionPtr parseExpression(Precedence bindingPower) noexcept_except_catastrophic;
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
    ExpressionPtr parsePrimaryExpression() noexcept_except_catastrophic;
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

    /**
     * @details The context is considered unary if the previous token was a left parenthesis
     * @details another operator (except ++, -- or ] (for indexing)) or nothing
     */
    bool isUnaryContext() const;

    ast::Block parseBlock(std::string blockName);

    [[nodiscard]] Token currentToken();
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

    bool done() { return currentToken().getType() == TokenType::EndOfFile; }

    // ~ Helpers for lookups
    // TODO: Rename these to be clearer + add docstrings

    void led_binary(TokenType type, Precedence bindingPower, leftDenotationHandler_t handler);
    void led_rightAssociative(TokenType type, Precedence bindingPower,
                              leftDenotationHandler_t handler);
    void led_postfix(TokenType type, Precedence bindingPower,
                     leftDenotationHandler_t handler);
    void led_prefix(TokenType type, Precedence bindingPower,
                    leftDenotationHandler_t handler);

    void nud_binary(TokenType type, nullDenotationHandler_t handler);
    void nud_prefix(TokenType type, nullDenotationHandler_t handler);
    void stmt(TokenType type, statementHandler_t handler);

    void type_led(TokenType type, Precedence bindingPower, type_leftDenotationHandler_t handler);
    void type_nud(TokenType type, type_nullDenotationHandler_t handler);
    void type_postfix(TokenType type, type_leftDenotationHandler_t handler);

    void initializeLookups();
    void initializeTypeLookups();
};

}  // namespace parser
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_H
