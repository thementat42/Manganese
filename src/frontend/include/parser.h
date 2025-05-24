#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <optional>

#include "../../global_macros.h"
#include "lexer.h"
#include "ast.h"

MANG_BEGIN
namespace parser {
constexpr auto NONE = std::nullopt;
using core::Token;
using str = std::string;
class Parser {
   private:  // private variables
    std::unique_ptr<lexer::Lexer> lexer;
    std::vector<Token> tokenCache;  // store tokens fromthe current cache

   private:  // private functions
    bool doneParsing();
    // ast::StatementPtr parseStatement();

   public:   // public functions
    Parser(const str& source, const lexer::Mode mode);
    ~Parser() = default;

    // ast::Block parse();
};

}  // namespace parser
MANG_END

#endif  // PARSER_H