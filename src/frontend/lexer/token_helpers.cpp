#include <format>
#include <frontend/lexer/token_base.hpp>
#include <utils/type_names.hpp>

#include <frontend/lexer/token.hpp>

namespace Manganese {

namespace lexer {

//! Most of this is long

TokenType Token::getUnaryCounterpart() const NOEXCEPT_IF_RELEASE {
    switch (type) {
        case TokenType::Plus: return TokenType::UnaryPlus;
        case TokenType::Minus: return TokenType::UnaryMinus;
        case TokenType::BitAnd: return TokenType::AddressOf;
        case TokenType::Mul: return TokenType::Dereference;
        default: ASSERT_UNREACHABLE("No unary counterpart for token type: " + tokenTypeToString(type));
    }
}

TokenType getBinaryOperatorFromAssignmentOperator(lexer::TokenType assignmentOp) NOEXCEPT_IF_RELEASE {
    using enum lexer::TokenType;
    switch (assignmentOp) {
        case PlusAssign: return Plus;
        case MinusAssign: return Minus;
        case MulAssign: return Mul;
        case DivAssign: return Div;
        case FloorDivAssign: return FloorDiv;
        case ModAssign: return Mod;
        case BitAndAssign: return BitAnd;
        case BitOrAssign: return BitOr;
        case BitXorAssign: return BitXor;
        case BitLShiftAssign: return BitLShift;
        case BitRShiftAssign: return BitRShift;
        default:
            ASSERT_UNREACHABLE(std::format("Cannot convert assignment operator {} to binary operator",
                                           lexer ::tokenTypeToString(assignmentOp)));
            return Unknown;
    }
}

std::string tokenTypeToString(TokenType type) NOEXCEPT_IF_RELEASE {
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
        case TokenType::Int128: return int128_str;
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
        case TokenType::UInt128: return uint128_str;
        case TokenType::While: return "while";

        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Mul: return "*";
        case TokenType::Div: return "/";
        case TokenType::FloorDiv: return "//";
        case TokenType::Mod: return "%";
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

}  // namespace lexer

}  // namespace Manganese