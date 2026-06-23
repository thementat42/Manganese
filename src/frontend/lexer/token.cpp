#include <frontend/lexer/token.hpp>
#include <core.hpp>
#include <io/logging.hpp>
#include <string>
#include <utility>

namespace Manganese {
namespace lexer {

void Token::overrideType(TokenType type, std::string lexeme) {
    logging::logInternal(logging::LogLevel::Info, "Overriding token type from {} to {} with lexeme '{}'",
                         tokenTypeToString(_type), tokenTypeToString(type), lexeme);

    _type = type;
    if (lexeme != "") { _lexeme = std::move(lexeme); }
}

struct keyword_map_entry {
    std::string_view str;
    TokenType type;
};

// let template argument deduction figure out the size (more flexible for adding/removing keywords)
constexpr std::array keywordTable = {

#define TOKEN(name, text)
#define OPERATOR(name, text)

#define KEYWORD(name, text) keyword_map_entry{text, TokenType::name},

#include <frontend/lexer/tokens.def>

#undef TOKEN
#undef KEYWORD
#undef OPERATOR
    // if no width is specified, default to a 32-bit float
    keyword_map_entry{"float", TokenType::Float32},
    // if no width is specified, default to a 32-bit integer
    keyword_map_entry{"int", TokenType::Int32},
    // if no width is specified, default to a 32-bit unsigned integer
    keyword_map_entry{"uint", TokenType::UInt32},
};

TokenType keywordLookup(const std::string_view& s) noexcept {
    for (const auto& p : keywordTable) {
        if (p.str == s) { return p.type; }
    }
    return TokenType::Unknown;
}

std::string Token::toString() const noexcept {
    return std::format("Token: {} (lexeme: '{}') at line {}, column {}", tokenTypeToString(_type), _lexeme, _line, _column);
}

}  // namespace lexer
}  // namespace Manganese
