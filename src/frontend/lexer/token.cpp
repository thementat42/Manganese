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
    logging::logInternal(logging::LogLevel::Info, "Overriding token type from {} to {} with lexeme '{}'", tokenTypeToString(type), tokenTypeToString(type_), lexeme_);

    type = type_;
    if (lexeme_ != "") { lexeme = std::move(lexeme_); }
}

struct keyword_map_entry {
    std::string_view str;
    TokenType type;
};

// let template argument deduction figure out the size (more flexible for adding/removing keywords)
constexpr std::array keywordTable = {
    keyword_map_entry{"aggregate", TokenType::Aggregate},
    keyword_map_entry{"alias", TokenType::Alias},
    keyword_map_entry{"as", TokenType::As},
    keyword_map_entry{"blueprint", TokenType::Blueprint},
    keyword_map_entry{"bool", TokenType::Bool},
    keyword_map_entry{"break", TokenType::Break},
    keyword_map_entry{"case", TokenType::Case},
    keyword_map_entry{"char", TokenType::Char},
    keyword_map_entry{"continue", TokenType::Continue},
    keyword_map_entry{"default", TokenType::Default},
    keyword_map_entry{"do", TokenType::Do},
    keyword_map_entry{"elif", TokenType::Elif},
    keyword_map_entry{"else", TokenType::Else},
    keyword_map_entry{"enum", TokenType::Enum},
    keyword_map_entry{"false", TokenType::False},
    // if no width is specified, default to a 32-bit float
    keyword_map_entry{"float", TokenType::Float32},
    keyword_map_entry{"float32", TokenType::Float32},
    keyword_map_entry{"float64", TokenType::Float64},
    keyword_map_entry{"for", TokenType::For},
    keyword_map_entry{"func", TokenType::Func},
    keyword_map_entry{"if", TokenType::If},
    keyword_map_entry{"import", TokenType::Import},
    // if no width is specified, default to a 32-bit integer
    keyword_map_entry{"int", TokenType::Int32},
    keyword_map_entry{"int16", TokenType::Int16},
    keyword_map_entry{"int32", TokenType::Int32},
    keyword_map_entry{"int64", TokenType::Int64},
    keyword_map_entry{"int8", TokenType::Int8},
    keyword_map_entry{"int128", TokenType::Int128},
    keyword_map_entry{"lambda", TokenType::Lambda},
    keyword_map_entry{"let", TokenType::Let},
    keyword_map_entry{"module", TokenType::Module},
    keyword_map_entry{"mut", TokenType::Mut},
    keyword_map_entry{"private", TokenType::Private},
    keyword_map_entry{"ptr", TokenType::Ptr},
    keyword_map_entry{"public", TokenType::Public},
    keyword_map_entry{"repeat", TokenType::Repeat},
    keyword_map_entry{"return", TokenType::Return},
    keyword_map_entry{"string", TokenType::String},
    keyword_map_entry{"switch", TokenType::Switch},
    keyword_map_entry{"true", TokenType::True},
    // if no width is specified, default to a 32-bit unsigned integer
    keyword_map_entry{"uint", TokenType::UInt32},
    keyword_map_entry{"uint8", TokenType::UInt8},
    keyword_map_entry{"uint16", TokenType::UInt16},
    keyword_map_entry{"uint32", TokenType::UInt32},
    keyword_map_entry{"uint64", TokenType::UInt64},
    keyword_map_entry{"uint128", TokenType::UInt128},
    keyword_map_entry{"while", TokenType::While},
};

TokenType keywordLookup(const std::string_view& s) noexcept {
    for (const auto& p : keywordTable) {
        if (p.str == s) { return p.type; }
    }
    return TokenType::Unknown;
}

}  // namespace lexer
}  // namespace Manganese
