#include "include/parser.h"

#include <stdio.h>

#include "../global_macros.h"
#include "include/ast.h"

//! For generics, might have to do two passes
//! First pass: just identify what the function/bundle/blueprint names are and keep those in a table
//! Then re-parse the file using that table to handle the "less-than vs generic" ambiguity
//! On the second pass, any time you see a function/bundle/blueprint name followed by a left angle bracket (<), treat it as a generic type (it's invalid to compare a function/bundle/blueprint name with something else)

namespace manganese {
namespace parser {

inline void Parser::initializeLookups() {
    // TODO: Update this to account for different sizes
//     nud(core::TokenType::IntegerLiteral, [](Parser* parser) {
//         double value = std::stod(parser->consumeToken().getLexeme());
//         return std::make_unique<ast::NumberExpression>(value);
//     });
//     nud(core::TokenType::FloatLiteral, [](Parser* parser) {
//         std::string lexeme = parser->consumeToken().getLexeme();
//         if (lexeme.back() == 'f' || lexeme.back() == 'F') {
//             return std::make_unique<ast::NumberExpression>(std::stof(lexeme));
//         } else {
//             return std::make_unique<ast::NumberExpression>(std::stod(lexeme));
//         }
//     });
//     nud(core::TokenType::StrLiteral, [](Parser* parser) {
//         std::string value = parser->consumeToken().getLexeme();
//         return std::make_unique<ast::StringExpression>(value);
//     });
// }

// Parser::Parser(const std::string& source, lexer::Mode mode) : lexer(std::make_unique<lexer::Lexer>(source, mode)) {
//     initializeLookups();
// }

// ast::Block Parser::parse() {
//     ast::Block body;
//     while (!done()) {
//         body.push_back(parseStatement());
//     }
//     return body;
// }
}  // namespace parser
}  // namespace manganese