#include "include/parser.h"

#include "../core/include/token.h"
#include "../global_macros.h"
#include "include/ast.h"
#include "include/lexer.h"

MANG_BEGIN
namespace parser {

Parser::Parser(const str& source, const lexer::Mode mode) {
    this->lexer = std::make_unique<lexer::Lexer>(source, mode);
}

inline bool Parser::doneParsing() {
    return lexer->peekToken().getType() != core::TokenType::EndOfFile;
}

}  // namespace parser
MANG_END