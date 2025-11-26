#ifndef MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL
#define MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL
/**
 * @file token_helpers.hpp
 * @brief Implementation of constexpr functions for the Token class
 */

#include <format>
#include <frontend/lexer/token_base.hpp>

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

constexpr TokenType Token::getUnaryCounterpart() const noexcept_if_release {
    switch (type) {
        case TokenType::Plus: return TokenType::UnaryPlus;
        case TokenType::Minus: return TokenType::UnaryMinus;
        case TokenType::BitAnd: return TokenType::AddressOf;
        case TokenType::Mul: return TokenType::Dereference;
        default: ASSERT_UNREACHABLE("No unary counterpart for token type: " + tokenTypeToString(type));
    }
}

constexpr TokenType getBinaryOperatorFromAssignmentOperator(lexer::TokenType assignmentOp) noexcept_if_release {
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

constexpr std::string tokenTypeToString(TokenType type) noexcept_if_release {
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

// TODO: Use gperf to make perfect hash
constexpr size_t _lexeme_hash(const std::string_view& s) {
    size_t _hash_val = 0;
    for (const char c : s) { _hash_val = _hash_val * 33 + (uint8_t)c; }
    return _hash_val;
}

constexpr TokenType keyword_lookup(const std::string_view& s) {
    switch (_lexeme_hash(s)) {
        case _lexeme_hash("aggregate"): return TokenType::Aggregate;
        case _lexeme_hash("alias"): return TokenType::Alias;
        case _lexeme_hash("as"): return TokenType::As;
        case _lexeme_hash("blueprint"): return TokenType::Blueprint;
        case _lexeme_hash("bool"): return TokenType::Bool;
        case _lexeme_hash("break"): return TokenType::Break;
        case _lexeme_hash("case"): return TokenType::Case;
        case _lexeme_hash("char"): return TokenType::Char;
        case _lexeme_hash("continue"): return TokenType::Continue;
        case _lexeme_hash("default"): return TokenType::Default;
        case _lexeme_hash("do"): return TokenType::Do;
        case _lexeme_hash("elif"): return TokenType::Elif;
        case _lexeme_hash("else"): return TokenType::Else;
        case _lexeme_hash("enum"): return TokenType::Enum;
        case _lexeme_hash("false"): return TokenType::False;
        case _lexeme_hash("float"): return TokenType::Float32;
        case _lexeme_hash("float32"): return TokenType::Float32;
        case _lexeme_hash("float64"): return TokenType::Float64;
        case _lexeme_hash("for"): return TokenType::For;
        case _lexeme_hash("func"): return TokenType::Func;
        case _lexeme_hash("if"): return TokenType::If;
        case _lexeme_hash("import"): return TokenType::Import;
        case _lexeme_hash("int"): return TokenType::Int32;
        case _lexeme_hash("int16"): return TokenType::Int16;
        case _lexeme_hash("int32"): return TokenType::Int32;
        case _lexeme_hash("int64"): return TokenType::Int64;
        case _lexeme_hash("int8"): return TokenType::Int8;
        case _lexeme_hash("lambda"): return TokenType::Lambda;
        case _lexeme_hash("let"): return TokenType::Let;
        case _lexeme_hash("module"): return TokenType::Module;
        case _lexeme_hash("mut"): return TokenType::Mut;
        case _lexeme_hash("private"): return TokenType::Private;
        case _lexeme_hash("ptr"): return TokenType::Ptr;
        case _lexeme_hash("public"): return TokenType::Public;
        case _lexeme_hash("readonly"): return TokenType::ReadOnly;
        case _lexeme_hash("repeat"): return TokenType::Repeat;
        case _lexeme_hash("return"): return TokenType::Return;
        case _lexeme_hash("string"): return TokenType::String;
        case _lexeme_hash("switch"): return TokenType::Switch;
        case _lexeme_hash("true"): return TokenType::True;
        case _lexeme_hash("uint"): return TokenType::UInt32;
        case _lexeme_hash("uint8"): return TokenType::UInt8;
        case _lexeme_hash("uint16"): return TokenType::UInt16;
        case _lexeme_hash("uint32"): return TokenType::UInt32;
        case _lexeme_hash("uint64"): return TokenType::UInt64;
        case _lexeme_hash("while"): return TokenType::While;
        default: return TokenType::Unknown;
    }
}

constexpr TokenType operator_lookup(const std::string_view& s) {
    switch (_lexeme_hash(s)) {
            // Arithmetic Operators
        case _lexeme_hash("+"): return TokenType::Plus;
        case _lexeme_hash("-"): return TokenType::Minus;
        case _lexeme_hash("*"): return TokenType::Mul;
        case _lexeme_hash("/"): return TokenType::Div;
        case _lexeme_hash("//"): return TokenType::FloorDiv;
        case _lexeme_hash("%"): return TokenType::Mod;
        case _lexeme_hash("^^"): return TokenType::Exp;
        case _lexeme_hash("++"): return TokenType::Inc;
        case _lexeme_hash("--"): return TokenType::Dec;

        // Arithmetic Assignment Operators
        case _lexeme_hash("+="): return TokenType::PlusAssign;
        case _lexeme_hash("-="): return TokenType::MinusAssign;
        case _lexeme_hash("*="): return TokenType::MulAssign;
        case _lexeme_hash("/="): return TokenType::DivAssign;
        case _lexeme_hash("//="): return TokenType::FloorDivAssign;
        case _lexeme_hash("%="): return TokenType::ModAssign;
        case _lexeme_hash("^^="): return TokenType::ExpAssign;

        // Comparison Operators
        case _lexeme_hash(">"): return TokenType::GreaterThan;
        case _lexeme_hash(">="): return TokenType::GreaterThanOrEqual;
        case _lexeme_hash("<"): return TokenType::LessThan;
        case _lexeme_hash("<="): return TokenType::LessThanOrEqual;
        case _lexeme_hash("=="): return TokenType::Equal;
        case _lexeme_hash("!="): return TokenType::NotEqual;

        // Boolean Operators
        case _lexeme_hash("&&"): return TokenType::And;
        case _lexeme_hash("||"): return TokenType::Or;
        case _lexeme_hash("!"): return TokenType::Not;

        // Bitwise Operators
        case _lexeme_hash("&"): return TokenType::BitAnd;
        case _lexeme_hash("|"): return TokenType::BitOr;
        case _lexeme_hash("~"): return TokenType::BitNot;
        case _lexeme_hash("^"): return TokenType::BitXor;
        case _lexeme_hash("<<"): return TokenType::BitLShift;
        case _lexeme_hash(">>"): return TokenType::BitRShift;

        // Bitwise Assignment Operators
        case _lexeme_hash("&="): return TokenType::BitAndAssign;
        case _lexeme_hash("|="): return TokenType::BitOrAssign;
        case _lexeme_hash("~="): return TokenType::BitNotAssign;
        case _lexeme_hash("^="): return TokenType::BitXorAssign;
        case _lexeme_hash("<<="): return TokenType::BitLShiftAssign;
        case _lexeme_hash(">>="): return TokenType::BitRShiftAssign;

        // Pointer Operators aren't here since they use the same symbols as bitwise AND and multiplication
        // that would cause a duplicate key error

        // Access Operators
        case _lexeme_hash("."): return TokenType::MemberAccess;
        case _lexeme_hash("..."): return TokenType::Ellipsis;
        case _lexeme_hash("::"): return TokenType::ScopeResolution;

        // Misc
        case _lexeme_hash("="): return TokenType::Assignment;
        case _lexeme_hash("->"): return TokenType::Arrow;
        case _lexeme_hash("@"): return TokenType::At;
        default: return TokenType::Unknown;
    }
}

}  // namespace lexer

}  // namespace Manganese

#endif  // MANGANGESE_INCLUDE_FRONTEND_LEXER_TOKEN_CONSTEXPR_IMPL_INL