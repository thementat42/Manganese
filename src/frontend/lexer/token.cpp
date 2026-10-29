#include <algorithm>
#include <format>
#include <frontend/lexer/token.hpp>
#include <io/logging.hpp>
#include <string>

namespace Manganese::lexer {

std::string tokenTypeToString(TokenType type) {
    switch (type) {
#define SPECIAL_TOKEN(name, text) \
    case TokenType::name: return text;

#define LITERAL_TOKEN(name, text) \
    case TokenType::name: return text;

#define PUNCTUATION_TOKEN(name, text) \
    case TokenType::name: return text;

#define KEYWORD_TOKEN(name, text) \
    case TokenType::name: return text;

#define OPERATOR_TOKEN(name, text) \
    case TokenType::name: return text;

#include <frontend/lexer/tokens.def>

#undef SPECIAL_TOKEN
#undef LITERAL_TOKEN
#undef PUNCTUATION_TOKEN
#undef KEYWORD_TOKEN
#undef OPERATOR_TOKEN
    }
    ASSERT_UNREACHABLE_FMT("No string representation for TokenType: {}", static_cast<int>(type));
}

TokenType Token::getUnaryCounterpart() const {
    switch (_type) {
        case TokenType::Plus: return TokenType::UnaryPlus;
        case TokenType::Minus: return TokenType::UnaryMinus;
        case TokenType::BitAnd: return TokenType::AddressOf;
        case TokenType::Mul: return TokenType::Dereference;
        default: ASSERT_UNREACHABLE_FMT("No unary counterpart for token type: {}", tokenTypeToString(_type));
    }
}

void Token::overrideType(TokenType newType) {
    logging::logInternal(logging::LogLevel::Info, "Overriding token type from {} to {}", tokenTypeToString(_type),
                         tokenTypeToString(newType));

    _type = newType;
}

namespace {
struct keyword_map_entry {
    std::string_view str;
    TokenType type;
};

struct operator_map_entry {
    std::string_view str;
    TokenType type;
};

// let template argument deduction figure out the size (more flexible for adding/removing keywords)
constexpr std::array _keywordTableHelper = {
#define SPECIAL_TOKEN(name, text)
#define LITERAL_TOKEN(name, text)
#define PUNCTUATION_TOKEN(name, text)
#define OPERATOR_TOKEN(name, text)

#define KEYWORD_TOKEN(name, text) keyword_map_entry{.str = (text), .type = TokenType::name},

#include <frontend/lexer/tokens.def>

#undef SPECIAL_TOKEN
#undef LITERAL_TOKEN
#undef PUNCTUATION_TOKEN
#undef KEYWORD_TOKEN
#undef OPERATOR_TOKEN
    // if no width is specified, default to a 32-bit float
    keyword_map_entry{.str = "float", .type = TokenType::Float32},
    // if no width is specified, default to a 32-bit integer
    keyword_map_entry{.str = "int", .type = TokenType::Int32},
    // if no width is specified, default to a 32-bit unsigned integer
    keyword_map_entry{.str = "uint", .type = TokenType::UInt32},
};

constexpr std::array _operatorTableHelper{
#define SPECIAL_TOKEN(name, text)
#define LITERAL_TOKEN(name, text)
#define PUNCTUATION_TOKEN(name, text) operator_map_entry{.str = (text), .type = TokenType::name},
#define OPERATOR_TOKEN(name, text)    operator_map_entry{.str = (text), .type = TokenType::name},
#define KEYWORD_TOKEN(name, text)

#include <frontend/lexer/tokens.def>

#undef SPECIAL_TOKEN
#undef LITERAL_TOKEN
#undef PUNCTUATION_TOKEN
#undef KEYWORD_TOKEN
#undef OPERATOR_TOKEN
};

}  // namespace

// Sort the helper table to allow for binary search lookups
constexpr auto keywordTable = []() {
    auto arr = _keywordTableHelper;
    std::ranges::sort(arr, {}, &keyword_map_entry::str);
    return arr;
}();

constexpr auto operatorTable = []() {
    // Note: sorting by length helps satisfy maximal munch
    auto arr = _operatorTableHelper;
    std::ranges::sort(arr, [](const auto& a, const auto& b) {
        if (a.str.length() != b.str.length()) { return a.str.length() > b.str.length(); }
        return a.str < b.str;
    });
    return arr;
}();

TokenType keywordLookup(std::string_view s) noexcept {
    if (const auto* entry = std::ranges::lower_bound(keywordTable, s, {}, &keyword_map_entry::str);
        entry != keywordTable.end() && entry->str == s) {
        return entry->type;
    }
    return TokenType::Unknown;
}

OperatorMatch operatorLookup(std::string_view s) noexcept {
    for (const auto& entry : operatorTable) {
        if (s.starts_with(entry.str)) { return {.type = entry.type, .length = entry.str.length()}; }
    }
    return {.type = TokenType::Unknown, .length = 1};
}

std::string Token::toString() const {
    return std::format("Token: {} (lexeme: '{}') at line {}, column {}", tokenTypeToString(_type), _lexeme, _line,
                       _column);
}

}  // namespace Manganese::lexer
