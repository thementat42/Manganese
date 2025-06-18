#ifndef PARSER_H
#define PARSER_H

#include <global_macros.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "lexer.h"
#include "token.h"

namespace Manganese {
namespace parser {
using ast::StatementPtr, ast::ExpressionPtr;
using lexer::TokenType,
    lexer::Token;
using str = std::string;
using std::make_unique;

enum class OperatorBindingPower : uint8_t;

//~ Helper functions that don't depend on the parser class's methods/variables
int determineNumberBase(const str &lexeme);

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    size_t tokenCachePosition;
    ast::Visibility defaultVisibility;
    bool hasError;

    std::vector<Token> tokenCache;  // Old tokens (for lookbehind)

   public:   // public variables
   private:  // private methods
    using statementHandler_t = std::function<StatementPtr(Parser *)>;
    using nullDenotationHandler_t = std::function<ExpressionPtr(Parser *)>;
    using leftDenotationHandler_t = std::function<ExpressionPtr(Parser *, ExpressionPtr, OperatorBindingPower)>;

    //~ Lookups
    std::unordered_map<TokenType, statementHandler_t> statementLookup;
    std::unordered_map<TokenType, nullDenotationHandler_t> nullDenotationLookup;
    std::unordered_map<TokenType, leftDenotationHandler_t> leftDenotationLookup;
    std::unordered_map<TokenType, OperatorBindingPower> bindingPowerLookup;

    //~ Parsing functions

    //* Expression Parsing
    ExpressionPtr parseExpression(OperatorBindingPower bindingPower);
    bool isUnaryContext() const;
    ExpressionPtr parsePrimaryExpression();

    ExpressionPtr parseBinaryExpression(ExpressionPtr left, OperatorBindingPower bindingPower);
    ExpressionPtr parseExponentiationExpression(ExpressionPtr left, OperatorBindingPower bindingPower);

    ExpressionPtr parseAssignmentExpression(ExpressionPtr left, OperatorBindingPower bindingPower);
    ExpressionPtr parsePrefixExpression();
    ExpressionPtr parseParenthesizedExpression();

    //* Statement Parsing

    StatementPtr parseStatement();
    StatementPtr parseVariableDeclarationStatement();

    // ~ Helpers
    [[nodiscard]] Token currentToken();
    [[nodiscard]] Token advance();
    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, const str &errorMessage);

    void led(TokenType type, OperatorBindingPower bindingPower, leftDenotationHandler_t handler);
    void nud(TokenType type, nullDenotationHandler_t handler);
    void stmt(TokenType type, statementHandler_t handler);
    void initializeLookups();

    bool done() { return currentToken().getType() == TokenType::EndOfFile; }

   public:  // public methods
    Parser() = default;
    Parser(const str &source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ast::Block parse();
};

/**
 * See the operator precedence table in the operators documentation
 * Note: This enum is the reverse of the table (operators with higher precedences have bigger values in this enum)
 */
enum class OperatorBindingPower : uint8_t {
    Default = 0,
    // Arrow = 1,   // Not really needed
    Assignment = 2,
    LogicalOr = 3,
    LogicalAnd = 4,
    BitwiseOr = 5,
    BitwiseXor = 6,
    BitwiseAnd = 7,
    Equality = 8,
    Relational = 9,
    BitwiseShift = 10,
    Additive = 11,
    Multiplicative = 12,
    Exponential = 13,
    Unary = 14,
    Postfix = 15,
    Member = 16,
    Scope = 17,
    Generic = 17,
    Primary = 18
};

}  // namespace parser
}  // namespace Manganese

#endif  // PARSER_H