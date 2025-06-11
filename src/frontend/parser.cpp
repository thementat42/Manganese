#include "include/parser.h"

#include "../global_macros.h"

MANGANESE_BEGIN
namespace parser {

Parser::Parser(const str& source, lexer::Mode mode) : lexer(make_unique<lexer::Lexer>(source, mode)) {}

ast::Block Parser::parse() {
    ast::Block program;

    while (!done()) {
        // Move since parseStatement() returns a unique_ptr
        // and we want to avoid copying it into the vector
        program.push_back(std::move(parseStatement()));
    }
        return program;
}

}  // namespace parser
MANGANESE_END