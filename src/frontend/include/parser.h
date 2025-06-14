#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../../global_macros.h"
#include "ast.h"
#include "lexer.h"
#include "token.h"

MANGANESE_BEGIN
namespace parser {
using ast::StatementPtr, ast::ExpressionPtr;
using lexer::TokenType,
    lexer::Token;
using str = std::string;
using std::make_unique;

enum class OperatorBindingPower : uint8_t;


//~ Helper functions that don't depend on the parser class's methods/variables
int determineNumberBase(const str &lexeme);
ExpressionPtr createIntegerLiteralNode(str &suffix, str &numericPart, int base);


class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    size_t tokenCachePosition = 0;
    std::vector<Token> tokenCache;  // Old tokens (for lookbehind)
    ast::Visibility defaultVisibility = ast::Visibility::ReadOnly;

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

enum class OperatorBindingPower : uint8_t {
    Default = 0,
    Comma = 1,
    Assignment = 2,
    Logical = 3,
    Relational = 4,
    Additive = 5,
    Multiplicative = 6,
    Exponential = 7,
    Unary = 8,
    Call = 9,
    Member = 10,
    Primary = 11,
};

}  // namespace parser
MANGANESE_END

#endif  // PARSER_H