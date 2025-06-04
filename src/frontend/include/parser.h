#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../../core/include/keywords.h"
#include "../../core/include/operators.h"
#include "../../core/include/token.h"
#include "../../global_macros.h"
#include "ast.h"
#include "lexer.h"

namespace manganese {
namespace parser {
using core::TokenType,
    core::OperatorBindingPower,
    core::Token;

class Parser {
   private:  // private variables
    bool hasError = false;
    std::unique_ptr<lexer::Lexer> lexer;
    std::vector<core::Token> tokenCache;  // store relevant tokens for the current parsing context

    using statementHandler_t = std::function<ast::StatementPtr(Parser*)>;
    using nullDenotationHandler_t = std::function<ast::ExpressionPtr(Parser*)>;
    using leftDenotationHandler_t = std::function<ast::ExpressionPtr(Parser*, ast::ExpressionPtr, core::OperatorBindingPower)>;

   public:   // public variables
   private:  // private methods
    ast::StatementPtr parseStatement();
    ast::ExpressionPtr nullDenotationHandler();
    ast::ExpressionPtr leftDenotationHandler(ast::ExpressionPtr left, core::OperatorBindingPower bindingPower);

    //~ Lookup Tables
    std::unordered_map<core::TokenType, statementHandler_t> statementLookup;

    /**
     * @brief Null denotation: we don't expect anything to the left of the token
     */
    std::unordered_map<core::TokenType, nullDenotationHandler_t> nullDenotationLookup;

    /**
     * @brief Left denotation: we expect an expression to the left of the token
     */
    std::unordered_map<core::TokenType, leftDenotationHandler_t> leftDenotationLookup;
    std::unordered_map<core::TokenType, core::OperatorBindingPower> bindingPowerLookup;

    // TODO: Inline these functions into the constructor later (removing the definitions)
    inline void led(core::TokenType type, core::OperatorBindingPower bindingPower,
                    leftDenotationHandler_t handler) {
        leftDenotationLookup[type] = handler;
        bindingPowerLookup[type] = bindingPower;
    }

    inline void nud(core::TokenType type,
                    nullDenotationHandler_t handler) {
        nullDenotationLookup[type] = handler;
        bindingPowerLookup[type] = core::OperatorBindingPower::Primary;  // Have to bind as tightly as possible since there's nothing else to bind to
    }

    inline void stmt(core::TokenType type, statementHandler_t handler) {
        statementLookup[type] = handler;
        bindingPowerLookup[type] = core::OperatorBindingPower::Default;
    }

    inline void initializeLookups();

    //~ Wrappers around the lexer
    inline lexer::Token peekToken(size_t offset = 0) { return lexer->peekToken(offset); };
    [[nodiscard]] inline lexer::Token consumeToken() { return lexer->consumeToken(); };

    bool done() { return peekToken().getType() == core::TokenType::EndOfFile; }

   public:  // public methods
    Parser() = default;
    Parser(const std::string& source, lexer::Mode mode);
    ~Parser();

    ast::Block parse();
};
}  // namespace parser
}  // namespace manganese

#endif  // PARSER_H