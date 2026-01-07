#ifndef MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL
#define MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL
/**
 * @file token_helpers.hpp
 * @brief Implementation of constexpr functions for the Token class
 */

#include <format>
#include <frontend/lexer/token_base.hpp>
#include <string_view>
#include <utility>
#include <utils/type_names.hpp>

#include "token_type.hpp"

namespace Manganese {

namespace lexer {

//! Most of this is long

inline std::string Token::toString() const noexcept {
    return std::format("Token: {} ('{}') at line {}, column {}", tokenTypeToString(type), lexeme, line, column);
}

constexpr bool Token::isPrefixOperator() const noexcept {
    return type == TokenType::Inc || type == TokenType::Dec || type == TokenType::BitAnd || type == TokenType::Mul
        || type == TokenType::AddressOf || type == TokenType::Dereference;
}

constexpr bool Token::isLiteral() const noexcept {
    return type == TokenType::IntegerLiteral || type == TokenType::FloatLiteral || type == TokenType::StrLiteral
        || type == TokenType::CharLiteral || type == TokenType::True || type == TokenType::False;
}

constexpr bool Token::isBracket() const noexcept {
    return type == TokenType::LeftParen || type == TokenType::RightParen || type == TokenType::LeftBrace
        || type == TokenType::RightBrace || type == TokenType::LeftSquare || type == TokenType::RightSquare;
}

constexpr bool Token::isPrimitiveType() const noexcept {
    return type == TokenType::Int8 || type == TokenType::Int16 || type == TokenType::Int32 || type == TokenType::Int64
        || type == TokenType::UInt8 || type == TokenType::UInt16 || type == TokenType::UInt32
        || type == TokenType::UInt64 || type == TokenType::Float32 || type == TokenType::Float64
        || type == TokenType::Char || type == TokenType::Bool || type == TokenType::String;
}

constexpr bool Token::hasUnaryCounterpart() const noexcept {
    return type == TokenType::Plus ||  // + can be addition or unary plus
        type == TokenType::Minus ||  // - can be subtraction or unary minus
        type == TokenType::BitAnd ||  // & can be bitwise AND or address-of operator
        type == TokenType::Mul;  // * can be multiplication or dereference operator
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
            ASSERT_UNREACHABLE(std::format("Cannot convert assignment operator {} to binary operator",
                                           lexer::tokenTypeToString(assignmentOp)));
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
        case TokenType::ReadOnly: return "readonly";
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
                               + std::to_string(static_cast<std::underlying_type<TokenType>::type>(type)));
    }
}

constexpr std::pair<std::string_view, TokenType> keywordTable[] = {
    {"aggregate", TokenType::Aggregate},
    {"alias", TokenType::Alias},
    {"as", TokenType::As},
    {"blueprint", TokenType::Blueprint},
    {"bool", TokenType::Bool},
    {"break", TokenType::Break},
    {"case", TokenType::Case},
    {"char", TokenType::Char},
    {"continue", TokenType::Continue},
    {"default", TokenType::Default},
    {"do", TokenType::Do},
    {"elif", TokenType::Elif},
    {"else", TokenType::Else},
    {"enum", TokenType::Enum},
    {"false", TokenType::False},
    {"float", TokenType::Float32},
    {"float32", TokenType::Float32},
    {"float64", TokenType::Float64},
    {"for", TokenType::For},
    {"func", TokenType::Func},
    {"if", TokenType::If},
    {"import", TokenType::Import},
    {"int", TokenType::Int32},
    {"int16", TokenType::Int16},
    {"int32", TokenType::Int32},
    {"int64", TokenType::Int64},
    {"int8", TokenType::Int8},
    {"lambda", TokenType::Lambda},
    {"let", TokenType::Let},
    {"module", TokenType::Module},
    {"mut", TokenType::Mut},
    {"private", TokenType::Private},
    {"ptr", TokenType::Ptr},
    {"public", TokenType::Public},
    {"readonly", TokenType::ReadOnly},
    {"repeat", TokenType::Repeat},
    {"return", TokenType::Return},
    {"string", TokenType::String},
    {"switch", TokenType::Switch},
    {"true", TokenType::True},
    {"uint", TokenType::UInt32},
    {"uint8", TokenType::UInt8},
    {"uint16", TokenType::UInt16},
    {"uint32", TokenType::UInt32},
    {"uint64", TokenType::UInt64},
    {"while", TokenType::While},
};

constexpr std::pair<std::string_view, TokenType> operatorTable[] = {
    // Arithmetic Operators
    {"+", TokenType::Plus},
    {"-", TokenType::Minus},
    {"*", TokenType::Mul},
    {"/", TokenType::Div},
    {"//", TokenType::FloorDiv},
    {"%", TokenType::Mod},
    {"^^", TokenType::Exp},
    {"++", TokenType::Inc},
    {"--", TokenType::Dec},

    // Arithmetic Assignment Operators
    {"+=", TokenType::PlusAssign},
    {"-=", TokenType::MinusAssign},
    {"*=", TokenType::MulAssign},
    {"/=", TokenType::DivAssign},
    {"//=", TokenType::FloorDivAssign},
    {"%=", TokenType::ModAssign},
    {"^^=", TokenType::ExpAssign},

    // Comparison Operators
    {">", TokenType::GreaterThan},
    {">=", TokenType::GreaterThanOrEqual},
    {"<", TokenType::LessThan},
    {"<=", TokenType::LessThanOrEqual},
    {"==", TokenType::Equal},
    {"!=", TokenType::NotEqual},

    // Boolean Operators
    {"&&", TokenType::And},
    {"||", TokenType::Or},
    {"!", TokenType::Not},

    // Bitwise Operators
    {"&", TokenType::BitAnd},
    {"|", TokenType::BitOr},
    {"~", TokenType::BitNot},
    {"^", TokenType::BitXor},
    {"<<", TokenType::BitLShift},
    {">>", TokenType::BitRShift},

    // Bitwise Assignment Operators
    {"&=", TokenType::BitAndAssign},
    {"|=", TokenType::BitOrAssign},
    {"~=", TokenType::BitNotAssign},
    {"^=", TokenType::BitXorAssign},
    {"<<=", TokenType::BitLShiftAssign},
    {">>=", TokenType::BitRShiftAssign},

    // Pointer Operators  use the same symbols as bitwise AND and multiplication (distinction in parser)

    // Access Operators
    {".", TokenType::MemberAccess},
    {"...", TokenType::Ellipsis},
    {"::", TokenType::ScopeResolution},

    // Misc
    {"=", TokenType::Assignment},
    {"->", TokenType::Arrow},
    {"@", TokenType::At}

};

constexpr inline TokenType keyword_lookup(const std::string_view& s) {
    for (const auto& p : keywordTable) {
        if (p.first == s) { return p.second; }
    }
    return TokenType::Unknown;
}

constexpr inline TokenType operator_lookup(const std::string_view& s) {
    for (const auto& p : operatorTable) {
        if (p.first == s) { return p.second; }
    }
    return TokenType::Unknown;
}

}  // namespace lexer

}  // namespace Manganese

#endif  // MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL