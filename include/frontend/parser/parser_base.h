#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_H
#define MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_H

#include <global_macros.h>
#include <io/logging.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <frontend/ast.h>
#include <frontend/lexer.h>
#include "operators.h"

namespace Manganese {
namespace parser {
using ast::StatementPtr, ast::ExpressionPtr, ast::TypePtr;
using lexer::TokenType,
    lexer::Token;
using std::make_unique;

//~ Helper functions that don't depend on the parser class's methods/variables
int determineNumberBase(const std::string &lexeme);
void extractSuffix(std::string &numericPart, std::string &suffix);

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    size_t tokenCachePosition;
    ast::Visibility defaultVisibility;
    bool hasError;
    bool isParsingBlockPrecursor = false;  // Used to determine if we are parsing a block precursor (if/for/while, etc.)

    std::vector<Token> tokenCache;  // Old tokens (for lookbehind)

   public:  // public methods
    Parser() = default;
    Parser(const std::string &source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ast::Block parse();

   private:  // private methods
    using statementHandler_t = std::function<StatementPtr(Parser *)>;
    using nullDenotationHandler_t = std::function<ExpressionPtr(Parser *)>;
    using leftDenotationHandler_t = std::function<ExpressionPtr(Parser *, ExpressionPtr, Precedence)>;

    //~ Lookups
    std::unordered_map<TokenType, statementHandler_t> statementLookup;
    std::unordered_map<TokenType, nullDenotationHandler_t> nullDenotationLookup;
    std::unordered_map<TokenType, leftDenotationHandler_t> leftDenotationLookup;
    std::unordered_map<TokenType, Operator> operatorPrecedenceMap;

    using type_nullDenotationHandler_t = std::function<TypePtr(Parser *)>;
    using type_leftDenotationHandler_t = std::function<TypePtr(Parser *, TypePtr, Precedence)>;

    std::unordered_map<TokenType, type_nullDenotationHandler_t> type_nullDenotationLookup;
    std::unordered_map<TokenType, type_leftDenotationHandler_t> type_leftDenotationLookup;
    std::unordered_map<TokenType, Operator> type_operatorPrecedenceMap;

    //~ Parsing functions

    //* Expression Parsing
    /**
     * @brief The main function for parsing expressions.
     * @details Responsible for calling appropriate helper methods based on the current token type.
     * @details and handling operator precedence.
     */
    ExpressionPtr parseExpression(Precedence bindingPower) noexcept_except_catastrophic;

    /**
     * @brief Operators like + and - can be unary or binary, depending on the context.
     * @details The context is considered unary if the previous token was a left parenthesis
     * @details another operator (except ++, -- or ] (for indexing)) or nothing
     * @return true if the context is unary, false otherwise.
     */
    bool isUnaryContext() const;

    /**
     * @brief Parses a primary expression, which can be an identifier or literal
     */
    ExpressionPtr parsePrimaryExpression() noexcept_except_catastrophic;

    /**
     * @brief Parses expressions of the form `left op right`, where `op` is a binary operator.
     * @param left The parsed expression so far
     * @param bindingPower The precedence of the operator that binds this expression
     */
    ExpressionPtr parseBinaryExpression(ExpressionPtr left, Precedence bindingPower);

    /**
     * @brief Parses expressions of the form `left = right` or `left op= right` (where op= is a valid reassignment operator).
     * @param left The left-hand side expression that is being assigned to.
     */
    ExpressionPtr parseAssignmentExpression(ExpressionPtr left, Precedence bindingPower);

    /**
     * @brief Parses expressions of the form `left as type`
     * @param left The left-hand side expression that is being casted.
     */
    ExpressionPtr parseTypeCastExpression(ExpressionPtr left, Precedence bindingPower);

    /**
     * @brief Parses prefix expressions (unary operators applied to an expression, like !x or &x)
     * @return
     */
    ExpressionPtr parsePrefixExpression();

    /**
     * @brief Parses postfix expressions (like x++, x--)
     * @param left The left-hand side expression that is being operated on.
     */
    ExpressionPtr parsePostfixExpression(ExpressionPtr left, Precedence bindingPower);

    /**
     * @brief Parses an expression enclosed in parentheses, bypassing normal precedence rules.
     */
    ExpressionPtr parseParenthesizedExpression();

    /**
     * @brief Parse the instantiation of a bundle (identifier { field = value ...})
     */
    ExpressionPtr parseBundleInstantiationExpression(ExpressionPtr left,
                                                     Precedence bindingPower);

    /**
     * @brief Parses an array instantiation expression (e.g., [1, 2, 3])
     */
    ExpressionPtr parseArrayInstantiationExpression();

    /**
     * @brief Parse a function call expression (identifier(args...))
     */
    ExpressionPtr parseFunctionCallExpression(ExpressionPtr left, Precedence bindingPower);

    /**
     * @brief Parse a generic expression (e.g. `identifier@[type1, type2]`)
     */
    ExpressionPtr parseGenericExpression(ExpressionPtr left, Precedence bindingPower);

    /**
     * @brief Parse a member access expression (`object.member`)
     */
    ExpressionPtr parseMemberAccessExpression(ExpressionPtr left,
                                              Precedence bindingPower);

    /**
     * @brief Parse an indexing expression (e.g. `array[index]`)
     */
    ExpressionPtr parseIndexingExpression(ExpressionPtr left, Precedence bindingPower);

    /**
     * @brief Parse a scope resolution expression (e.g. `module::identifier`)
     */
    ExpressionPtr parseScopeResolutionExpression(ExpressionPtr left,
                                                 Precedence bindingPower);

    //* Statement Parsing

    /**
     * @brief The core parsing function -- responsible for parsing statements based on the current token type.
     * @details This also parses an expression and returns it as a statement
     */
    StatementPtr parseStatement();
    StatementPtr parseVariableDeclarationStatement();
    StatementPtr parseBundleDeclarationStatement();
    StatementPtr parseFunctionDeclarationStatement();
    StatementPtr parseReturnStatement();
    StatementPtr parseWhileLoopStatement();
    StatementPtr parseDoWhileLoopStatement();
    StatementPtr parseRepeatLoopStatement();
    StatementPtr parseIfStatement();
    StatementPtr parseEnumDeclarationStatement();
    StatementPtr parseSwitchStatement();

    //* Type Parsing

    /**
     * @brief Parses a type declaration
     */
    TypePtr parseType(Precedence bindingPower);
    TypePtr parseSymbolType();
    /**
     * @brief Parses an array type declaration (e.g. int[])
     */
    TypePtr parseArrayType(TypePtr left, Precedence rightBindingPower);

    TypePtr parseGenericType(TypePtr left, Precedence rightBindingPower);

    // ~ Helpers
    /**
     * @brief Gets the current token without consuming it.
     * @note Will refill the token cache if necessary
     */
    [[nodiscard]] Token currentToken();
    /**
     * @brief Consume the current token and return it.
     * @note Will refill the token cache if necessary
     */
    [[nodiscard]] Token advance();

    /**
     * @brief Expect a specific token type, printing an error on mismatch
     * @note Will always consume the token
     */
    Token expectToken(TokenType expectedType);

    /**
     * @brief Expect a specific token type, printing an error on mismatch
     * @param errorMessage A custom error message to print
     * @note Will always consume the token
     * @note Prints the given error message, followed by (expected x, received y)
     */
    Token expectToken(TokenType expectedType, const std::string &errorMessage);

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

    /**
     * @brief A wrapper around logging::logError that sets the parser's hasError flag to true.
     */
    inline void logError(const std::string &message, size_t line = 0, size_t col = 0) {
        logging::logError(message, line, col);
        hasError = true;
    }

    bool done() { return currentToken().getType() == TokenType::EndOfFile; }
};

}  // namespace parser
}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_PARSER_BASE_H
