#include "include/parser.h"

#include "../global_macros.h"
#include "include/nodes.h"

MANG_BEGIN
namespace parser {

    Parser::Parser(const str& source, const lexer::Mode mode) {
        this->lexer = std::make_unique<lexer::Lexer>(source, mode);
    }

    inline bool Parser::doneParsing() {
        return lexer->peekToken().getType() != lexer::TokenType::EndOfFile;
    }

    Block Parser::parse() {
        Block program;
        while (!doneParsing()) {
            program.push_back(std::move(parseStatement()));  // move the statement to avoid duplicates
        }
        return program;
    }
    

}  // namespace parser
MANG_END