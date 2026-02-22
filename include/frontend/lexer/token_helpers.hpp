#ifndef MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL
#define MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL
/**
 * @file token_helpers.hpp
 * @brief Implementation of constexpr functions for the Token class
 */

#include <format>
#include <frontend/lexer/token_base.hpp>
#include <string_view>
#include <utils/type_names.hpp>

#include "token_type.hpp"

namespace Manganese {

namespace lexer {

//! Most of this is long

inline std::string Token::toString() const noexcept {
    return std::format("Token: {} ('{}') at line {}, column {}", tokenTypeToString(type), lexeme, line, column);
}

constexpr TokenType Token::getUnaryCounterpart() const NOEXCEPT_IF_RELEASE {
    switch (type) {
        case TokenType::Plus: return TokenType::UnaryPlus;
        case TokenType::Minus: return TokenType::UnaryMinus;
        case TokenType::BitAnd: return TokenType::AddressOf;
        case TokenType::Mul: return TokenType::Dereference;
        default: ASSERT_UNREACHABLE("No unary counterpart for token type: " + tokenTypeToString(type));
    }
}

constexpr TokenType getBinaryOperatorFromAssignmentOperator(lexer::TokenType assignmentOp) NOEXCEPT_IF_RELEASE {
    using enum lexer::TokenType;
    switch (assignmentOp) {
        case PlusAssign: return Plus;
        case MinusAssign: return Minus;
        case MulAssign: return Mul;
        case DivAssign: return Div;
        case FloorDivAssign: return FloorDiv;
        case ModAssign: return Mod;
        case ExpAssign: return Exp;
        case BitAndAssign: return BitAnd;
        case BitOrAssign: return BitOr;
        case BitXorAssign: return BitXor;
        case BitLShiftAssign: return BitLShift;
        case BitRShiftAssign: return BitRShift;
        default:
            ASSERT_UNREACHABLE(std ::format("Cannot convert assignment operator {} to binary operator",
                                            lexer ::tokenTypeToString(assignmentOp)));
            return Unknown;
    }
}

constexpr std::string tokenTypeToString(TokenType type) NOEXCEPT_IF_RELEASE {
    switch (type) {
        // Basic
        case TokenType::Identifier: return "Identifier";
        case TokenType::StrLiteral: return "String Literal";
        case TokenType::CharLiteral: return "Char Literal";

        // Numbers
        case TokenType::IntegerLiteral: return "Integer";
        case TokenType::FloatLiteral: return "Float";

        // Brackets
        case TokenType::LeftParen: return "Left Parenthesis";
        case TokenType::RightParen: return "Right Parenthesis";
        case TokenType::LeftBrace: return "Left Brace";
        case TokenType::RightBrace: return "Right Brace";
        case TokenType::LeftSquare: return "Left Square";
        case TokenType::RightSquare: return "Right Square";
        // Punctuation
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Comma: return "Comma";
        case TokenType::Colon: return "Colon";

        // Misc
        case TokenType::EndOfFile: return "End Of File";

        // Keywords
        case TokenType::Aggregate: return "aggregate";
        case TokenType::Alias: return "alias";
        case TokenType::As: return "as";
        case TokenType::Blueprint: return "blueprint";
        case TokenType::Bool: return "bool";
        case TokenType::Break: return "break";
        case TokenType::Case: return "case";
        case TokenType::Char: return "char";
        case TokenType::Continue: return "continue";
        case TokenType::Default: return "default";
        case TokenType::Do: return "do";
        case TokenType::Elif: return "elif";
        case TokenType::Else: return "else";
        case TokenType::Enum: return "enum";
        case TokenType::False: return "false";
        case TokenType::Float32: return "float32";
        case TokenType::Float64: return "float64";
        case TokenType::For: return "for";
        case TokenType::Func: return "func";
        case TokenType::If: return "if";
        case TokenType::Import: return "import";
        case TokenType::Int8: return int8_str;
        case TokenType::Int16: return int16_str;
        case TokenType::Int32: return int32_str;
        case TokenType::Int64: return int64_str;
        case TokenType::Lambda: return "lambda";
        case TokenType::Let: return "let";
        case TokenType::Module: return "module";
        case TokenType::Mut: return "mut";
        case TokenType::Ptr: return "ptr";
        case TokenType::Private: return "Private";
        case TokenType::Public: return "public";
        case TokenType::Repeat: return "repeat";
        case TokenType::Return: return "return";
        case TokenType::String: return "string";
        case TokenType::Switch: return "switch";
        case TokenType::True: return "true";
        // case TokenType::TypeOf: return "typeof";
        case TokenType::UInt8: return uint8_str;
        case TokenType::UInt16: return uint16_str;
        case TokenType::UInt32: return uint32_str;
        case TokenType::UInt64: return uint64_str;
        case TokenType::While: return "while";

        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Mul: return "*";
        case TokenType::Div: return "/";
        case TokenType::FloorDiv: return "//";
        case TokenType::Mod: return "%";
        case TokenType::Exp: return "^^";
        case TokenType::Inc: return "++";
        case TokenType::Dec: return "--";
        case TokenType::UnaryPlus: return "+";
        case TokenType::UnaryMinus: return "-";
        case TokenType::PlusAssign: return "+=";
        case TokenType::MinusAssign: return "-=";
        case TokenType::MulAssign: return "*=";
        case TokenType::DivAssign: return "/=";
        case TokenType::FloorDivAssign: return "//=";
        case TokenType::ModAssign: return "%=";
        case TokenType::ExpAssign: return "^^=";
        case TokenType::GreaterThan: return ">";
        case TokenType::GreaterThanOrEqual: return ">=";
        case TokenType::LessThan: return "<";
        case TokenType::LessThanOrEqual: return "<=";
        case TokenType::Equal: return "==";
        case TokenType::NotEqual: return "!=";
        case TokenType::And: return "&&";
        case TokenType::Or: return "||";
        case TokenType::Not: return "!";
        case TokenType::BitAnd: return "&";
        case TokenType::BitOr: return "|";
        case TokenType::BitNot: return "~";
        case TokenType::BitXor: return "^";
        case TokenType::BitLShift: return "<<";
        case TokenType::BitRShift: return ">>";
        case TokenType::BitAndAssign: return "&=";
        case TokenType::BitOrAssign: return "|=";
        case TokenType::BitNotAssign: return "~=";
        case TokenType::BitXorAssign: return "^=";
        case TokenType::BitLShiftAssign: return "<<=";
        case TokenType::BitRShiftAssign: return ">>=";
        case TokenType::AddressOf: return "&";
        case TokenType::Dereference: return "*";
        case TokenType::MemberAccess: return ".";
        case TokenType::Ellipsis: return "...";
        case TokenType::ScopeResolution: return "::";
        case TokenType::Assignment: return "=";
        case TokenType::Arrow: return "->";
        case TokenType::At: return "@";
        case TokenType::Unknown: return "Unknown Token";
        case TokenType::Keyword: return "Keyword";
        case TokenType::Operator: return "Operator";
        default:
            ASSERT_UNREACHABLE("No string representation for TokenType: "
                               + std ::to_string(static_cast<std ::underlying_type<TokenType>::type>(type)));
    }
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
    keyword_map_entry{"while", TokenType::While},
};

constexpr inline TokenType keyword_lookup(const std::string_view& s) noexcept {
    for (const auto& p : keywordTable) {
        if (p.str == s) { return p.type; }
    }
    return TokenType::Unknown;
}

}  // namespace lexer

}  // namespace Manganese

#endif  // MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL