#include <format>
#include <frontend/lexer/token.hpp>
#include <frontend/lexer/token_base.hpp>
#include <utils/type_names.hpp>


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

}  // namespace lexer

}  // namespace Manganese