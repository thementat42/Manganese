#include "include/parser.h"

#include "../../global_macros.h"
#include "include/nodes.h"

MANG_BEGIN
namespace parser {

    Parser::Parser(const std::string& source, lexer::Mode mode) : lexer(std::make_unique<lexer::Lexer>(source, mode)) {
        currentToken = lexer->peekToken();
        nextToken = lexer->peekToken(1);
    }

    void Parser::advance() {

    }

    bool Parser::doneParsing() const {
        return currentToken.getType() == lexer::TokenType::EndOfFile;
    }

    NodePtr Parser::parse() {
        while (!doneParsing()) {
            // Based on what the current token's type is, call the appropriate parsing function

            currentToken = lexer->peekToken();
            nextToken = lexer->peekToken(1);
        }
    }

}  // namespace parser
MANG_END