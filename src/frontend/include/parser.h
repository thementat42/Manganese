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

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
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

    inline void led(TokenType type, OperatorBindingPower bindingPower,
                    leftDenotationHandler_t handler);

    inline void nud(TokenType type,
                    nullDenotationHandler_t handler);

    inline void stmt(TokenType type,
                     statementHandler_t handler);

    inline void initializeLookups();
    static int determineNumberBase(const str &lexeme);

    //~ Parsing functions
    ExpressionPtr parseExpression(OperatorBindingPower bindingPower);

    ExpressionPtr parsePrimaryExpression();
    const ExpressionPtr createIntegerLiteralNode(str &suffix, str &numericPart, int base);

    ExpressionPtr parseBinaryExpression(ExpressionPtr left, OperatorBindingPower bindingPower);
    ExpressionPtr parseExponentiationExpression(ExpressionPtr left, OperatorBindingPower bindingPower);

    StatementPtr parseStatement();

    // ~ Helpers
    Token currentToken() {
        if (tokenCache.empty()) {
            tokenCache.push_back(lexer->consumeToken());
        }
        return tokenCache.front();
    }

    Token advance() {
        if (tokenCache.empty()) {
            tokenCache.push_back(lexer->consumeToken());
        }
        Token token = tokenCache.front();
        tokenCache.erase(tokenCache.begin());
        return token;
    }

    Token expectToken(TokenType expectedType);
    Token expectToken(TokenType expectedType, const str &errorMessage);

    bool done() const { return tokenCache.empty() && lexer->done(); }

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