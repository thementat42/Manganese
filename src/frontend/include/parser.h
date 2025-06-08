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

namespace manganese {
namespace parser {
using ast::StatementPtr, ast::ExpressionPtr;
using lexer::TokenType,
    lexer::OperatorBindingPower,
    lexer::Token;
using str = std::string;

class Parser {
   private:  // private variables
    bool hasError = false;
    ast::Visibility defaultVisibility = ast::Visibility::ReadOnly;  // If a variable is declared without an explicit visibility, it will default to this
    std::unique_ptr<lexer::Lexer> lexer;
    size_t tokenCachePosition = 0;
    std::vector<lexer::Token> tokenCache;  // store relevant tokens for the current parsing context

    using statementHandler_t = std::function<StatementPtr(Parser*)>;
    using nullDenotationHandler_t = std::function<ExpressionPtr(Parser*)>;
    using leftDenotationHandler_t = std::function<ExpressionPtr(Parser*, ExpressionPtr, OperatorBindingPower)>;

   public:   // public variables
   private:  // sub-parsing methods
    ExpressionPtr parseExpression(OperatorBindingPower bindingPower);
    ExpressionPtr parseBinaryExpression(ExpressionPtr left, OperatorBindingPower bindingPower);
    ExpressionPtr parseExponentiationExpression(ExpressionPtr left, OperatorBindingPower bindingPower);
    ExpressionPtr parsePrimaryExpression();

    StatementPtr parseVariableDeclaration();

   private:  // helpers

    /**
     * @brief Expect a specific token type
     * @param expectedType The token type to expect
     * @param message A custom error message to print
     * @return Consumes a token and returns it if it matches
     */
    Token expectToken(TokenType expectedType, const std::string& message = "");

    /**
     * @brief Expect a specific token type from a list of types
     * @param expectedTypes The list of token types to expect
     * @param message A custom error message to print
     * @return Consumes a token and returns it if it matches
     */
    Token expectToken(const std::initializer_list<TokenType>& expectedTypes, const std::string& message = "");

    ast::StatementPtr parseStatement();
    ast::ExpressionPtr nullDenotationHandler();
    ast::ExpressionPtr leftDenotationHandler(ExpressionPtr left, OperatorBindingPower bindingPower);

    //~ Lookup Tables
    std::unordered_map<TokenType, statementHandler_t> statementLookup;

    /**
     * @brief Null denotation: we don't expect anything to the left of the token
     */
    std::unordered_map<TokenType, nullDenotationHandler_t> nullDenotationLookup;

    /**
     * @brief Left denotation: we expect an expression to the left of the token
     */
    std::unordered_map<TokenType, leftDenotationHandler_t> leftDenotationLookup;
    std::unordered_map<TokenType, OperatorBindingPower> bindingPowerLookup;

    // TODO?: Inline these functions into the constructor later (removing the definitions)
    inline void led(TokenType type, OperatorBindingPower bindingPower,
                    leftDenotationHandler_t handler) {
        leftDenotationLookup[type] = handler;
        bindingPowerLookup[type] = bindingPower;
    }

    inline void nud(TokenType type,
                    nullDenotationHandler_t handler) {
        nullDenotationLookup[type] = handler;
        bindingPowerLookup[type] = OperatorBindingPower::Primary;  // Have to bind as tightly as possible since there's nothing else to bind to
    }

    inline void stmt(TokenType type, statementHandler_t handler) {
        statementLookup[type] = handler;
        bindingPowerLookup[type] = OperatorBindingPower::Default;
    }

    inline void initializeLookups();

    //~ Wrappers around the lexer
    /**
     * @brief Peek at the next token in the input stream without consuming it
     * @param offset How many tokens to look ahead (default is 0 -- the current token)
     * @return The peeked token
     */
    inline Token peekToken(size_t offset = 0) noexcept { return lexer->peekToken(offset); };

    /**
     * @brief Peek at the next lexeme in the input stream without consuming it
     * @param offset How many tokens to look ahead (default is 0 -- the current token)
     * @return The lexeme of the peeked token
     */
    inline str peekLexeme(size_t offset = 0) noexcept {
        return peekToken(offset).getLexeme();
    }

    /**
     * @brief Consume the next token in the input stream
     * @details This will advance the lexer position by 1
     * @return The consumed token
     */
    [[nodiscard]] inline Token consumeToken() noexcept { return lexer->consumeToken(); };

    /**
     * @brief Consume the next token in the input stream and return its lexeme
     * @return The lexeme of the consumed token
     */
    [[nodiscard]] inline str consumeLexeme() noexcept { return consumeToken().getLexeme(); }

    bool done() noexcept { return peekToken().getType() == TokenType::EndOfFile; }

   public:  // public methods
    Parser() = default;
    Parser(const str& source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ast::Block parse();
};
}  // namespace parser
}  // namespace manganese

#endif  // PARSER_H