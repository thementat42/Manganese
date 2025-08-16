/**
 * @file token.cpp
 * @brief This file contains the implementation of the Token struct for the Manganese compiler.
 */

#include <format>
#include <frontend/lexer/token.hpp>
#include <global_macros.hpp>
#include <io/logging.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <utils/number_utils.hpp>


namespace Manganese {
namespace lexer {
Token::Token(const TokenType type_, const std::string lexeme_, const size_t line_, const size_t column_,
             bool invalid_) noexcept_if_release :
    type(type_),
    lexeme(lexeme_),
    line(line_),
    column(column_),
    invalid(invalid_) {
    // Set a specific enum value for operators and keywords based on the lexeme
    if (type_ == TokenType::Operator) {
        type = operatorFromString(lexeme_, line_, column_);
    } else if (type_ == TokenType::Keyword) {
        type = keywordFromString(lexeme_, line_, column_);
    }
    // Special lexeme override cases
    if (type == TokenType::Int32) {
        lexeme = "int32";
    } else if (type == TokenType::Float32) {
        lexeme = "float32";
    }
}

TokenType operatorFromString(const std::string& op, const size_t line, const size_t column) {
    std::string op_str(op);
    auto it = operatorMap.find(op_str);
    if (it != operatorMap.end()) { return it->second; }
    ASSERT_UNREACHABLE(std::format("Unknown operator '{}' at line {}, column {}", op, line, column));
}

TokenType keywordFromString(const std::string& keyword, const size_t line, const size_t column) {
    auto it = keywordMap.find(keyword);
    if (it != keywordMap.end()) { return it->second; }
    ASSERT_UNREACHABLE(std::format("Unknown operator '{}' at line {}, column {}", keyword, line, column));
}

void Token::overrideType(TokenType type_, std::string lexeme_) {

    logging::logInternal(std::format("Overriding token type from {} to {} with lexeme '{}'", tokenTypeToString(type),
                                     tokenTypeToString(type_), lexeme_));

    type = type_;
    if (lexeme != "") { lexeme = std::move(lexeme_); }
}

std::unordered_map<std::string, const TokenType> operatorMap = {
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

    // Pointer Operators aren't here since they use the same symbols as bitwise AND and multiplication
    // that would cause a duplicate key error

    // Access Operators
    {".", TokenType::MemberAccess},
    {"...", TokenType::Ellipsis},
    {"::", TokenType::ScopeResolution},

    // Misc
    {"=", TokenType::Assignment},
    {"->", TokenType::Arrow},
    {"@", TokenType::At}};

std::unordered_map<std::string, const TokenType> keywordMap
    = {{"aggregate", TokenType::Aggregate},
       {"alias", TokenType::Alias},
       {"as", TokenType::As},
       {"blueprint", TokenType::Blueprint},
       {"bool", TokenType::Bool},
       {"break", TokenType::Break},
       {"case", TokenType::Case},
       {"char", TokenType::Char},
       {"const", TokenType::Const},
       {"continue", TokenType::Continue},
       {"default", TokenType::Default},
       {"do", TokenType::Do},
       {"elif", TokenType::Elif},
       {"else", TokenType::Else},
       {"enum", TokenType::Enum},
       {"false", TokenType::False},
       {"float", TokenType::Float32},  // default to float32 when floating point width isn't specified
       {"float32", TokenType::Float32},
       {"float64", TokenType::Float64},
       {"for", TokenType::For},
       {"func", TokenType::Func},
       {"if", TokenType::If},
       {"import", TokenType::Import},
       {"int", TokenType::Int32},  // default to int32 when integer width isn't specified
       {"int16", TokenType::Int16},
       {"int32", TokenType::Int32},
       {"int64", TokenType::Int64},
       {"int8", TokenType::Int8},
       {"lambda", TokenType::Lambda},
       {"let", TokenType::Let},
       {"module", TokenType::Module},
       {"private", TokenType::Private},
       {"ptr", TokenType::Ptr},
       {"public", TokenType::Public},
       {"readonly", TokenType::ReadOnly},
       {"repeat", TokenType::Repeat},
       {"return", TokenType::Return},
       {"string", TokenType::String},
       {"switch", TokenType::Switch},
       {"true", TokenType::True},
       {"typeof", TokenType::TypeOf},
       {"uint", TokenType::UInt32},  // default to uint32 when integer width isn't specified
       {"uint8", TokenType::UInt8},
       {"uint16", TokenType::UInt16},
       {"uint32", TokenType::UInt32},
       {"uint64", TokenType::UInt64},
       {"while", TokenType::While}};

}  // namespace lexer
}  // namespace Manganese
