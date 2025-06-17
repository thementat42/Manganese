#include <frontend/parser.h>
#include <global_macros.h>
#include <io/logging.h>

#include <format>

namespace Manganese {
namespace parser {

Parser::Parser(const str& source, lexer::Mode mode) : lexer(make_unique<lexer::Lexer>(source, mode)) {
    initializeLookups();
}

ast::Block Parser::parse() {
    ast::Block program;

    while (!done()) {
        // No need to move thanks to copy elision
        program.push_back(parseStatement());

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
    led(TokenType::Assignment, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::PlusAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::MinusAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::MulAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::DivAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::FloorDivAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::ModAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::ExpAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::BitAndAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::BitOrAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::BitNotAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::BitXorAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::BitLShiftAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);
    led(TokenType::BitRShiftAssign, OperatorBindingPower::Assignment, &Parser::parseAssignmentExpression);

    //~ Logical
    led(TokenType::And, OperatorBindingPower::LogicalAnd, &Parser::parseBinaryExpression);
    led(TokenType::Or, OperatorBindingPower::LogicalOr, &Parser::parseBinaryExpression);

    //~ Relational
    led(TokenType::LessThan, OperatorBindingPower::Relational, &Parser::parseBinaryExpression);
    led(TokenType::GreaterThan, OperatorBindingPower::Relational, &Parser::parseBinaryExpression);
    led(TokenType::LessThanOrEqual, OperatorBindingPower::Relational, &Parser::parseBinaryExpression);
    led(TokenType::GreaterThanOrEqual, OperatorBindingPower::Relational, &Parser::parseBinaryExpression);
    led(TokenType::Equal, OperatorBindingPower::Relational, &Parser::parseBinaryExpression);
    led(TokenType::NotEqual, OperatorBindingPower::Relational, &Parser::parseBinaryExpression);

    //~ Additive, Multiplicative, Exponential
    led(TokenType::Plus, OperatorBindingPower::Additive, &Parser::parseBinaryExpression);
    led(TokenType::Minus, OperatorBindingPower::Additive, &Parser::parseBinaryExpression);
    led(TokenType::Mul, OperatorBindingPower::Multiplicative, &Parser::parseBinaryExpression);
    led(TokenType::Div, OperatorBindingPower::Multiplicative, &Parser::parseBinaryExpression);
    led(TokenType::FloorDiv, OperatorBindingPower::Multiplicative, &Parser::parseBinaryExpression);
    led(TokenType::Mod, OperatorBindingPower::Multiplicative, &Parser::parseBinaryExpression);
    led(TokenType::Exp, OperatorBindingPower::Exponential, &Parser::parseExponentiationExpression);

    //~ Literals and Symbols
    nud(TokenType::IntegerLiteral, &Parser::parsePrimaryExpression);
    nud(TokenType::FloatLiteral, &Parser::parsePrimaryExpression);
    nud(TokenType::CharLiteral, &Parser::parsePrimaryExpression);
    nud(TokenType::StrLiteral, &Parser::parsePrimaryExpression);
    nud(TokenType::Identifier, &Parser::parsePrimaryExpression);
    nud(TokenType::LeftParen, &Parser::parseParenthesizedExpression);

    //~ Prefix Operators
    nud(TokenType::UnaryPlus, &Parser::parsePrefixExpression);
    nud(TokenType::UnaryMinus, &Parser::parsePrefixExpression);
    nud(TokenType::Not, &Parser::parsePrefixExpression);
    nud(TokenType::AddressOf, &Parser::parsePrefixExpression);
    nud(TokenType::Dereference, &Parser::parsePrefixExpression);
    nud(TokenType::BitNot, &Parser::parsePrefixExpression);
    nud(TokenType::Inc, &Parser::parsePrefixExpression);
    nud(TokenType::Dec, &Parser::parsePrefixExpression);

    //~ Statements
    stmt(TokenType::Const, &Parser::parseVariableDeclarationStatement);
    stmt(TokenType::Let, &Parser::parseVariableDeclarationStatement);
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
}  // namespace Manganese