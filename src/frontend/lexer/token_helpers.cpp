#include <core.hpp>
#include <frontend/lexer/token.hpp>
#include <string>
#include <utils/type_names.hpp>

namespace Manganese::lexer {

TokenType Token::getUnaryCounterpart() const NOEXCEPT_IF_RELEASE {
    switch (_type) {
        case TokenType::Plus: return TokenType::UnaryPlus;
        case TokenType::Minus: return TokenType::UnaryMinus;
        case TokenType::BitAnd: return TokenType::AddressOf;
        case TokenType::Mul: return TokenType::Dereference;
        default: ASSERT_UNREACHABLE("No unary counterpart for token type: " + tokenTypeToString(_type));
    }
}

std::string tokenTypeToString(TokenType type) {
    switch (type) {
#define TOKEN(name, text) \
    case TokenType::name: return text;

#define KEYWORD(name, text) \
    case TokenType::name: return text;

#define OPERATOR(name, text) \
    case TokenType::name: return text;

#include <frontend/lexer/tokens.def>

#undef TOKEN
#undef KEYWORD
#undef OPERATOR

        default:
            ASSERT_UNREACHABLE("No string representation for TokenType: "
                               + std ::to_string(static_cast<std ::underlying_type<TokenType>::type>(type)));
    }
}
}  // namespace Manganese::lexer