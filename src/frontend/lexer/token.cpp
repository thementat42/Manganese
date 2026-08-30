#include <algorithm>
#include <frontend/lexer/token.hpp>
#include <io/logging.hpp>
#include <string>
#include <utility>

namespace Manganese::lexer {

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

TokenType Token::getUnaryCounterpart() const NOEXCEPT_IF_RELEASE {
    switch (_type) {
        case TokenType::Plus: return TokenType::UnaryPlus;
        case TokenType::Minus: return TokenType::UnaryMinus;
        case TokenType::BitAnd: return TokenType::AddressOf;
        case TokenType::Mul: return TokenType::Dereference;
        default: ASSERT_UNREACHABLE("No unary counterpart for token type: " + tokenTypeToString(_type));
    }
}

void Token::overrideType(TokenType newType, std::string&& newLexeme) {
    logging::logInternal(logging::LogLevel::Info, "Overriding token type from {} to {}{} with lexeme '{}'",
                         tokenTypeToString(_type), tokenTypeToString(newType),
                         (hasUnaryCounterpart() ? " (unary)" : ""), newLexeme);

    _type = newType;
    if (!newLexeme.empty()) { _lexeme = std::move(newLexeme); }
}

namespace {
struct keyword_map_entry {
    std::string_view str;
    TokenType type;
};

// let template argument deduction figure out the size (more flexible for adding/removing keywords)
constexpr std::array _keywordTableHelper = {
#define TOKEN(name, text)
#define OPERATOR(name, text)

#define KEYWORD(name, text) keyword_map_entry{.str = text, .type = TokenType::name},

#include <frontend/lexer/tokens.def>

#undef TOKEN
#undef KEYWORD
#undef OPERATOR
    // if no width is specified, default to a 32-bit float
    keyword_map_entry{.str = "float", .type = TokenType::Float32},
    // if no width is specified, default to a 32-bit integer
    keyword_map_entry{.str = "int", .type = TokenType::Int32},
    // if no width is specified, default to a 32-bit unsigned integer
    keyword_map_entry{.str = "uint", .type = TokenType::UInt32},
};
}  // namespace

// Sort the helper table to allow for binary search lookups
constexpr auto keywordTable = []() {
    auto arr = _keywordTableHelper;
    std::ranges::sort(arr, {}, &keyword_map_entry::str);
    return arr;
}();

TokenType keywordLookup(std::string_view s) noexcept {
    if (auto entry = std::ranges::lower_bound(keywordTable, s, {}, &keyword_map_entry::str); entry != keywordTable.end() && entry->str == s) {
        return entry->type;
    }
    return TokenType::Unknown;
}

std::string Token::toString() const {
    return std::format("Token: {} (lexeme: '{}') at line {}, column {}", tokenTypeToString(_type), _lexeme, _line,
                       _column);
}

}  // namespace Manganese::lexer
