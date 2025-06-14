#include "include/parser.h"

#include <format>

#include "../global_macros.h"
#include "../io/include/logging.h"

MANGANESE_BEGIN
namespace parser {

Parser::Parser(const str& source, lexer::Mode mode) : lexer(make_unique<lexer::Lexer>(source, mode)) {
    initializeLookups();
}

ast::Block Parser::parse() {
    ast::Block program;

    while (!done()) {
        // Move since parseStatement() returns a unique_ptr
        program.push_back(std::move(parseStatement()));

        // We don't need to look back at old tokens from previous statements,
        // clear the cache to save memory
        tokenCache.clear();
        tokenCachePosition = 0;
    }
    return program;
}

inline void Parser::led(TokenType type, OperatorBindingPower bindingPower,
                        leftDenotationHandler_t handler) {
    bindingPowerLookup[type] = bindingPower;
    leftDenotationLookup[type] = handler;
}

inline void Parser::nud(TokenType type,
                        nullDenotationHandler_t handler) {
    bindingPowerLookup[type] = OperatorBindingPower::Primary;
    nullDenotationLookup[type] = handler;
}

inline void Parser::stmt(TokenType type,
                         statementHandler_t handler) {
    bindingPowerLookup[type] = OperatorBindingPower::Default;
    statementLookup[type] = handler;
}

inline void Parser::initializeLookups() {
    //~ Assignments (updating variables, not initializing them)
    led(TokenType::Assignment, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::PlusAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::MinusAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::MulAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::DivAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::FloorDivAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::ModAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::ExpAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::BitAndAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::BitOrAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::BitNotAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::BitXorAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::BitLShiftAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);
    led(TokenType::BitRShiftAssign, OperatorBindingPower::Assignment, parseAssignmentExpression);

    //~ Logical
    led(TokenType::And, OperatorBindingPower::Logical, parseBinaryExpression);
    led(TokenType::Or, OperatorBindingPower::Logical, parseBinaryExpression);

    //~ Relational
    led(TokenType::LessThan, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::GreaterThan, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::LessThanOrEqual, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::GreaterThanOrEqual, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::Equal, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::NotEqual, OperatorBindingPower::Relational, parseBinaryExpression);

    //~ Additive, Multiplicative, Exponential
    led(TokenType::Plus, OperatorBindingPower::Additive, parseBinaryExpression);
    led(TokenType::Minus, OperatorBindingPower::Additive, parseBinaryExpression);
    led(TokenType::Mul, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Div, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::FloorDiv, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Mod, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Exp, OperatorBindingPower::Exponential, parseExponentiationExpression);

    //~ Literals and Symbols
    nud(TokenType::IntegerLiteral, parsePrimaryExpression);
    nud(TokenType::FloatLiteral, parsePrimaryExpression);
    nud(TokenType::CharLiteral, parsePrimaryExpression);
    nud(TokenType::StrLiteral, parsePrimaryExpression);
    nud(TokenType::Identifier, parsePrimaryExpression);
    nud(TokenType::LeftParen, parseParenthesizedExpression);

    //~ Prefix Operators
    nud(TokenType::UnaryPlus, parsePrefixExpression);
    nud(TokenType::UnaryMinus, parsePrefixExpression);
    nud(TokenType::Not, parsePrefixExpression);
    nud(TokenType::AddressOf, parsePrefixExpression);
    nud(TokenType::Dereference, parsePrefixExpression);
    nud(TokenType::BitNot, parsePrefixExpression);
    nud(TokenType::Inc, parsePrefixExpression);
    nud(TokenType::Dec, parsePrefixExpression);

    //~ Statements
    stmt(TokenType::Const, parseVariableDeclarationStatement);
    stmt(TokenType::Let, parseVariableDeclarationStatement);
}

bool Parser::isUnaryContext() const {
    return (tokenCache.empty() || tokenCachePosition == 0 ||
            tokenCache[tokenCachePosition - 1].getType() == lexer::TokenType::LeftParen ||
            tokenCache[tokenCachePosition - 1].isOperator());
}

[[nodiscard]] Token Parser::currentToken() {
    // Make sure we have enough tokens in the cache
    while (tokenCachePosition >= tokenCache.size()) {
        tokenCache.push_back(lexer->consumeToken());
    }
    // Return the token at current position
    return tokenCache[tokenCachePosition];
}

[[nodiscard]] Token Parser::advance() {
    // Make sure we have enough tokens in the cache
    while (tokenCachePosition >= tokenCache.size()) {
        tokenCache.push_back(lexer->consumeToken());
    }
    return tokenCache[tokenCachePosition++];
}

Token Parser::expectToken(TokenType expectedType) {
    return expectToken(
        expectedType,
        std::format(
            "Expected: {}, but found: {}",
            lexer::tokenTypeToString(expectedType),
            lexer::tokenTypeToString(currentToken().getType())));
}

Token Parser::expectToken(TokenType expectedType, const str& errorMessage) {
    TokenType type = currentToken().getType();
    if (type != expectedType) {
        logging::logUser(
            errorMessage,
            logging::LogLevel::Error,
            currentToken().getLine(),
            currentToken().getColumn());
    }
    return advance();
}
}  // namespace parser
MANGANESE_END