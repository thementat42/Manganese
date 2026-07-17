#ifndef MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_TYPE_HPP
#define MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_TYPE_HPP

#include <stdint.h>

#include <core.hpp>

namespace Manganese {

namespace lexer {

enum class TokenType : uint8_t {
#define TOKEN(name, text)    name,
#define KEYWORD(name, text)  name,
#define OPERATOR(name, text) name,

#include <frontend/lexer/tokens.def>

#undef TOKEN
#undef KEYWORD
#undef OPERATOR
};

}  // namespace lexer

}  // namespace Manganese

#endif  // MANGANESE_INCLUDE_FRONTEND_AST_LEXER_TOKEN_TYPE_HPP