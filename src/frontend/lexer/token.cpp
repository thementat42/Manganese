/**
 * @file token.cpp
 * @brief This file contains the implementation of the Token struct for the Manganese compiler.
 */

#include <frontend/lexer/token.h>
#include <global_macros.h>
#include <io/logging.h>

#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace Manganese {
namespace lexer {
Token::Token(const TokenType type_, const std::string lexeme_, const size_t line_, const size_t column_, bool invalid_)
    : type(type_), lexeme(lexeme_), line(line_), column(column_), invalid(invalid_) {
    // Set a specific enum value for operators and keywords based on the lexeme
    if (type_ == TokenType::Operator) {
        auto op = operatorFromString(lexeme_);
        if (op.has_value()) [[likely]] {
            type = op.value();
        } else {
            ASSERT_UNREACHABLE(
                std::format("Unknown operator '{}' at line {}, column {}", lexeme_, line_, column_));
        }
    } else if (type_ == TokenType::Keyword) {
        auto kw = keywordFromString(lexeme_);
        if (kw.has_value()) [[likely]] {
            type = kw.value();
        } else {
            ASSERT_UNREACHABLE(
                std::format("Unknown keyword '{}' at line {}, column {}", lexeme_, line_, column_));
        }
    }
    // Special lexeme override cases
    if (type == TokenType::Int32) {
        lexeme = "int32";
    } else if (type == TokenType::Float32) {
        lexeme = "float32";
    }
}

std::optional<TokenType> operatorFromString(const std::string& op) {
    std::string op_str(op);
    auto it = operatorMap.find(op_str);
    if (it != operatorMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<TokenType> keywordFromString(const std::string& keyword) {
    auto it = keywordMap.find(keyword);
    if (it != keywordMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool Token::isInvalid() const noexcept {
    return invalid;
}

bool Token::isKeyword() const noexcept {
    return type >= TokenType::__KeywordStart && type <= TokenType::__KeywordEnd;
}

bool Token::isOperator() const noexcept {
    return type >= TokenType::__OperatorStart && type <= TokenType::__OperatorEnd;
}

bool Token::isPrefixOperator() const noexcept {
    return type == TokenType::Inc ||
           type == TokenType::Dec ||
           type == TokenType::BitAnd ||
           type == TokenType::Mul ||
           type == TokenType::AddressOf ||
           type == TokenType::Dereference;
}

bool Token::isLiteral() const noexcept {
    return type == TokenType::IntegerLiteral ||
           type == TokenType::FloatLiteral ||
           type == TokenType::StrLiteral ||
           type == TokenType::CharLiteral ||
           type == TokenType::True ||
           type == TokenType::False;
}

bool Token::isBracket() const noexcept {
    return type == TokenType::LeftParen || type == TokenType::RightParen ||
           type == TokenType::LeftBrace || type == TokenType::RightBrace ||
           type == TokenType::LeftSquare || type == TokenType::RightSquare;
}

bool Token::isPrimitiveType() const noexcept {
    return type == TokenType::Int8 || type == TokenType::Int16 ||
           type == TokenType::Int32 || type == TokenType::Int64 ||
           type == TokenType::UInt8 || type == TokenType::UInt16 ||
           type == TokenType::UInt32 || type == TokenType::UInt64 ||
           type == TokenType::Float32 || type == TokenType::Float64 ||
           type == TokenType::Char || type == TokenType::Bool;
}

bool Token::hasUnaryCounterpart() const noexcept {
    return type == TokenType::Plus ||    // + can be addition or unary plus
           type == TokenType::Minus ||   // - can be subtraction or unary minus
           type == TokenType::BitAnd ||  // & can be bitwise AND or address-of operator
           type == TokenType::Mul;       // * can be multiplication or dereference operator
}

TokenType Token::getUnaryCounterpart() const noexcept_except_catastrophic {
    switch (type) {
        case TokenType::Plus:
            return TokenType::UnaryPlus;
        case TokenType::Minus:
            return TokenType::UnaryMinus;
        case TokenType::BitAnd:
            return TokenType::AddressOf;
        case TokenType::Mul:
            return TokenType::Dereference;
        default:
            ASSERT_UNREACHABLE("No unary counterpart for token type: " + tokenTypeToString(type));
    }
}

TokenType Token::getType() const noexcept {
    return type;
}

std::string Token::getLexeme() const noexcept {
    return lexeme;
}

size_t Token::getLine() const noexcept {
    return line;
}

size_t Token::getColumn() const noexcept {
    return column;
}

void Token::overrideType(TokenType type_, std::string lexeme_) {
#if DEBUG
    logging::logInternal(
        std::format("Overriding token type from {} to {} with lexeme '{}'", 
            tokenTypeToString(type), tokenTypeToString(type_), lexeme_));
#endif  // DEBUG
    type = type_;
    if (lexeme != "") {
        lexeme = std::move(lexeme_);
    }
}

void Token::log() const noexcept {
#if DEBUG
    std::cout << "Token: " << tokenTypeToString(type) << " ('"
              << getLexeme() << "') at line " << line << ", column " << column;
    std::cout << '\n';
#endif  // DEBUG
}

void Token::log(const Token& token) noexcept {
    token.log();
}

// ! === Really Long Stuff ===

// TODO: Make one master map of token types to strings.
// TODO: have helper functions that go through the map initialize the keyword and operator maps
// (by flipping the mappings in the master map)

std::string tokenTypeToString(TokenType type) noexcept_except_catastrophic {
    switch (type) {
        // Basic
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::StrLiteral:
            return "String Literal";
        case TokenType::CharLiteral:
            return "Char Literal";

        // Numbers
        case TokenType::IntegerLiteral:
            return "Integer";
        case TokenType::FloatLiteral:
            return "Float";

        // Brackets
        case TokenType::LeftParen:
            return "Left Parenthesis";
        case TokenType::RightParen:
            return "Right Parenthesis";
        case TokenType::LeftBrace:
            return "Left Brace";
        case TokenType::RightBrace:
            return "Right Brace";
        case TokenType::LeftSquare:
            return "Left Square";
        case TokenType::RightSquare:
            return "Right Square";
        // Punctuation
        case TokenType::Semicolon:
            return "Semicolon";
        case TokenType::Comma:
            return "Comma";
        case TokenType::Colon:
            return "Colon";

        // Misc
        case TokenType::EndOfFile:
            return "End Of File";

        // Keywords
        case TokenType::Alias:
            return "alias";
        case TokenType::As:
            return "as";
        case TokenType::Blueprint:
            return "blueprint";
        case TokenType::Bool:
            return "bool";
        case TokenType::Break:
            return "break";
        case TokenType::Bundle:
            return "bundle";
        case TokenType::Case:
            return "case";
        case TokenType::Char:
            return "char";
        case TokenType::Const:
            return "const";
        case TokenType::Continue:
            return "continue";
        case TokenType::Default:
            return "default";
        case TokenType::Do:
            return "do";
        case TokenType::Elif:
            return "elif";
        case TokenType::Else:
            return "else";
        case TokenType::Enum:
            return "enum";
        case TokenType::False:
            return "false";
        case TokenType::Float32:
            return "float32";
        case TokenType::Float64:
            return "float64";
        case TokenType::For:
            return "for";
        case TokenType::Func:
            return "func";
        case TokenType::If:
            return "if";
        case TokenType::Import:
            return "import";
        case TokenType::Int8:
            return "int8";
        case TokenType::Int16:
            return "int16";
        case TokenType::Int32:
            return "int32";
        case TokenType::Int64:
            return "int64";
        case TokenType::Lambda:
            return "lambda";
        case TokenType::Let:
            return "let";
        case TokenType::Module:
            return "module";
        case TokenType::Ptr:
            return "ptr";
        case TokenType::Private:
            return "Private";
        case TokenType::Public:
            return "public";
        case TokenType::ReadOnly:
            return "readonly";
        case TokenType::Repeat:
            return "repeat";
        case TokenType::Return:
            return "return";
        case TokenType::Switch:
            return "switch";
        case TokenType::Static:
            return "static";
        case TokenType::True:
            return "true";
        case TokenType::TypeOf:
            return "typeof";
        case TokenType::UInt8:
            return "uint8";
        case TokenType::UInt16:
            return "uint16";
        case TokenType::UInt32:
            return "uint32";
        case TokenType::UInt64:
            return "uint64";
        case TokenType::While:
            return "while";

        case TokenType::Plus:
            return "+";
        case TokenType::Minus:
            return "-";
        case TokenType::Mul:
            return "*";
        case TokenType::Div:
            return "/";
        case TokenType::FloorDiv:
            return "//";
        case TokenType::Mod:
            return "%";
        case TokenType::Exp:
            return "^^";
        case TokenType::Inc:
            return "++";
        case TokenType::Dec:
            return "--";
        case TokenType::UnaryPlus:
            return "+ (unary)";
        case TokenType::UnaryMinus:
            return "- (unary)";
        case TokenType::PlusAssign:
            return "+=";
        case TokenType::MinusAssign:
            return "-=";
        case TokenType::MulAssign:
            return "*=";
        case TokenType::DivAssign:
            return "/=";
        case TokenType::FloorDivAssign:
            return "//=";
        case TokenType::ModAssign:
            return "%=";
        case TokenType::ExpAssign:
            return "^^=";
        case TokenType::GreaterThan:
            return ">";
        case TokenType::GreaterThanOrEqual:
            return ">=";
        case TokenType::LessThan:
            return "<";
        case TokenType::LessThanOrEqual:
            return "<=";
        case TokenType::Equal:
            return "==";
        case TokenType::NotEqual:
            return "!=";
        case TokenType::And:
            return "&&";
        case TokenType::Or:
            return "||";
        case TokenType::Not:
            return "!";
        case TokenType::BitAnd:
            return "&";
        case TokenType::BitOr:
            return "|";
        case TokenType::BitNot:
            return "~";
        case TokenType::BitXor:
            return "^";
        case TokenType::BitLShift:
            return "<<";
        case TokenType::BitRShift:
            return ">>";
        case TokenType::BitAndAssign:
            return "&=";
        case TokenType::BitOrAssign:
            return "|=";
        case TokenType::BitNotAssign:
            return "~=";
        case TokenType::BitXorAssign:
            return "^=";
        case TokenType::BitLShiftAssign:
            return "<<=";
        case TokenType::BitRShiftAssign:
            return ">>=";
        case TokenType::AddressOf:
            return "&";
        case TokenType::Dereference:
            return "*";
        case TokenType::MemberAccess:
            return ".";
        case TokenType::Ellipsis:
            return "...";
        case TokenType::ScopeResolution:
            return "::";
        case TokenType::Assignment:
            return "=";
        case TokenType::Arrow:
            return "->";
        case TokenType::At:
            return "@";
        case TokenType::Unknown:
            return "Unknown Token";
        default:
            ASSERT_UNREACHABLE("No string representation for TokenType: " +
                               std::to_string(static_cast<std::underlying_type<TokenType>::type>(type)));
    }
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

std::unordered_map<std::string, const TokenType> keywordMap = {
    {"alias", TokenType::Alias},
    {"as", TokenType::As},
    {"blueprint", TokenType::Blueprint},
    {"bool", TokenType::Bool},
    {"break", TokenType::Break},
    {"bundle", TokenType::Bundle},
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
    {"static", TokenType::Static},
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
