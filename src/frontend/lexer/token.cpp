/**
 * @file token.cpp
 * @brief This file contains the implementation of the Token struct for the Manganese compiler.
 */

#include <frontend/lexer/token.hpp>
#include <global_macros.hpp>
#include <io/logging.hpp>
#include <string>
#include <utility>

namespace Manganese {
namespace lexer {

void Token::overrideType(TokenType type_, std::string lexeme_) {
    logging::logInternal(std::format("Overriding token type from {} to {} with lexeme '{}'", tokenTypeToString(type),
                                     tokenTypeToString(type_), lexeme_));

    type = type_;
    if (lexeme != "") { lexeme = std::move(lexeme_); }
}

}  // namespace lexer
}  // namespace Manganese
