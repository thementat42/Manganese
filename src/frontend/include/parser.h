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
    lexer::OperatorBindingPower,
    lexer::Token;
using str = std::string;
using std::make_unique;

class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    std::vector<Token> tokenCache;   // Old tokens (for lookbehind)

   public:   // public variables
   private:  // private methods

   StatementPtr parseStatement();

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

   bool done() const {
        return tokenCache.empty() && lexer->done();  // read all tokens in cache and no more tokens are left
   }

   public:   // public methods
    Parser() = default;
    Parser(const str& source, lexer::Mode mode);
    ~Parser() noexcept = default;

    ast::Block parse();
};
}  // namespace parser
MANGANESE_END

#endif  // PARSER_H