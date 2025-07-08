#include <frontend/ast.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include <format>
#include <memory>
#include <string>

namespace Manganese {
namespace parser {

Parser::Parser(const std::string& source, lexer::Mode mode) : lexer(make_unique<lexer::Lexer>(source, mode)),
                                                              tokenCachePosition(0),
                                                              defaultVisibility(ast::Visibility::ReadOnly),
                                                              hasError(false) {
    initializeLookups();
    initializeTypeLookups();
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
    program.shrink_to_fit();  // Avoid having a bunch of allocated but unused memory
    return program;
}

// ===== Helper functions =====
bool Parser::isUnaryContext() const {
    if (tokenCache.empty() || tokenCachePosition == 0) {
        return true;  // If the cache is empty or we're at the start, it's a unary context
    }
    auto lastToken = tokenCache[tokenCachePosition - 1];

    return lastToken.getType() == TokenType::LeftParen ||
           (lastToken.isOperator() && lastToken.getType() != TokenType::Inc &&
            lastToken.getType() != TokenType::Dec);
}

[[nodiscard]] Token Parser::currentToken() {
    while (tokenCachePosition >= tokenCache.size()) {
        tokenCache.push_back(lexer->consumeToken());
    }
    return tokenCache[tokenCachePosition];
}

[[nodiscard]] Token Parser::advance() {
    while (tokenCachePosition >= tokenCache.size()) {
        tokenCache.push_back(lexer->consumeToken());
    }
    return tokenCache[tokenCachePosition++];
}

Token Parser::expectToken(TokenType expectedType) {
    return expectToken(
        expectedType, "Unexpected token: ");
}

Token Parser::expectToken(TokenType expectedType, const std::string& errorMessage) {
    TokenType type = currentToken().getType();
    if (type == expectedType) {
        return advance();
    }
    std::string message = errorMessage +
                          " (expected " + lexer::tokenTypeToString(expectedType) +
                          ", but found " + lexer::tokenTypeToString(type) + ")";
    logError(
        message,
        currentToken().getLine(),
        currentToken().getColumn());
    hasError = true;

    return advance();
}

// ===== Lookup Registration Methods =====

void Parser::led_binary(TokenType type, Precedence bindingPower, leftDenotationHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::binary(bindingPower);
    leftDenotationLookup[type] = handler;
}
void Parser::led_rightAssociative(TokenType type, Precedence bindingPower,
                                  leftDenotationHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::rightAssociative(bindingPower);
    leftDenotationLookup[type] = handler;
}
void Parser::led_postfix(TokenType type, Precedence bindingPower,
                         leftDenotationHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::postfix(bindingPower);
    leftDenotationLookup[type] = handler;
}
void Parser::led_prefix(TokenType type, Precedence bindingPower,
                        leftDenotationHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::prefix(bindingPower);
    leftDenotationLookup[type] = handler;
}

void Parser::nud_binary(TokenType type, nullDenotationHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::binary(Precedence::Default);
    nullDenotationLookup[type] = handler;
}

void Parser::nud_prefix(TokenType type, nullDenotationHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::prefix();
    nullDenotationLookup[type] = handler;
}

void Parser::stmt(TokenType type,
                  statementHandler_t handler) {
    operatorPrecedenceMap[type] = Operator{
        .leftBindingPower = Precedence::Default,
        .rightBindingPower = Precedence::Default};
    statementLookup[type] = handler;
}

void Parser::initializeLookups() {
    /*
    Things to implement:
    Attributes
    // Arithmetic Operators
    // Assignment Operators
    // Logical Operators
    // Bitwise Operators
    // Relational Operators
    // Grouping Operators
    // Unary Operators
    // Indexing Operators

    // Variable Declaration
    // Access control
    // Variable Assignment
    // Literals
        // Integer Literals
        // Float Literals
        // Char Literals
        // String Literals
        // Identifiers
        // Boolean Literals
        // Array Literals
    // Function Declarations
    // Function Calls
    // Return Statements
    Function type declarations
    ? Lambdas
    // Conditional Statements
    Loop Statements
        For Loops
        // While Loops
        // Do-While Loops
        // Repeat Loops
        // Break Statements
        // Continue Statements
    // Switch Statements
    Import Statements
        // Scope Resolution Operator
    Parsing multiple files
    // Bundle Declaration
    // Bundle Instantiation
    // Type Casting
    // Member Access
    // Enums
    Generics
        * Eventually: Constraints
    Error Handling
        Functions returning an error
    ? Macros
    ? Blueprints
        ? Constructor/Destructor
        ? Methods
        ? Static Methods
        ? Inheritance
        ? Operator Overloading


    Error Tolerance
    */

    //~ Assignments (updating variables, not initializing them)
    led_binary(TokenType::Assignment, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::BitAndAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::BitLShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::BitNotAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::BitOrAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::BitRShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::BitXorAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::DivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::ExpAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::FloorDivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::MinusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::ModAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::MulAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    led_binary(TokenType::PlusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);

    //~ Bitwise Operators
    led_binary(TokenType::BitAnd, Precedence::BitwiseAnd, &Parser::parseBinaryExpression);
    led_binary(TokenType::BitLShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    led_binary(TokenType::BitOr, Precedence::BitwiseOr, &Parser::parseBinaryExpression);
    led_binary(TokenType::BitRShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    led_binary(TokenType::BitXor, Precedence::BitwiseXor, &Parser::parseBinaryExpression);

    //~ Relational
    led_binary(TokenType::Equal, Precedence::Relational, &Parser::parseBinaryExpression);
    led_binary(TokenType::GreaterThan, Precedence::Relational, &Parser::parseBinaryExpression);
    led_binary(TokenType::GreaterThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);
    led_binary(TokenType::LessThan, Precedence::Relational, &Parser::parseBinaryExpression);
    led_binary(TokenType::LessThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);
    led_binary(TokenType::NotEqual, Precedence::Relational, &Parser::parseBinaryExpression);

    //~ Additive, Multiplicative, Exponential, Logical
    led_binary(TokenType::And, Precedence::LogicalAnd, &Parser::parseBinaryExpression);
    led_binary(TokenType::Div, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    led_binary(TokenType::FloorDiv, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    led_binary(TokenType::Minus, Precedence::Additive, &Parser::parseBinaryExpression);
    led_binary(TokenType::Mod, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    led_binary(TokenType::Mul, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    led_rightAssociative(TokenType::Exp, Precedence::Exponential, &Parser::parseBinaryExpression);
    led_binary(TokenType::Or, Precedence::LogicalOr, &Parser::parseBinaryExpression);
    led_binary(TokenType::Plus, Precedence::Additive, &Parser::parseBinaryExpression);

    //~ Literals and Symbols
    nud_binary(TokenType::CharLiteral, &Parser::parsePrimaryExpression);
    nud_binary(TokenType::False, &Parser::parsePrimaryExpression);
    nud_binary(TokenType::FloatLiteral, &Parser::parsePrimaryExpression);
    nud_binary(TokenType::Identifier, &Parser::parsePrimaryExpression);
    nud_binary(TokenType::IntegerLiteral, &Parser::parsePrimaryExpression);
    nud_binary(TokenType::LeftParen, &Parser::parseParenthesizedExpression);
    nud_binary(TokenType::StrLiteral, &Parser::parsePrimaryExpression);
    nud_binary(TokenType::True, &Parser::parsePrimaryExpression);

    //~ Prefix Operators
    nud_prefix(TokenType::AddressOf, &Parser::parsePrefixExpression);
    nud_prefix(TokenType::BitNot, &Parser::parsePrefixExpression);
    nud_prefix(TokenType::Dec, &Parser::parsePrefixExpression);
    nud_prefix(TokenType::Dereference, &Parser::parsePrefixExpression);
    nud_prefix(TokenType::Inc, &Parser::parsePrefixExpression);
    nud_prefix(TokenType::Not, &Parser::parsePrefixExpression);
    nud_prefix(TokenType::UnaryMinus, &Parser::parsePrefixExpression);
    nud_prefix(TokenType::UnaryPlus, &Parser::parsePrefixExpression);

    //~ PostFix Expression
    led_postfix(TokenType::Dec, Precedence::Postfix, &Parser::parsePostfixExpression);
    led_postfix(TokenType::Inc, Precedence::Postfix, &Parser::parsePostfixExpression);

    //~ Call/Member Expressions
    led_binary(TokenType::At, Precedence::Postfix, &Parser::parseGenericExpression);
    led_binary(TokenType::LeftBrace, Precedence::Postfix, &Parser::parseBundleInstantiationExpression);
    led_binary(TokenType::LeftParen, Precedence::Postfix, &Parser::parseFunctionCallExpression);
    nud_binary(TokenType::LeftSquare, &Parser::parseArrayInstantiationExpression);
    led_binary(TokenType::LeftSquare, Precedence::Postfix, &Parser::parseIndexingExpression);
    led_binary(TokenType::MemberAccess, Precedence::Member, &Parser::parseMemberAccessExpression);
    led_binary(TokenType::ScopeResolution, Precedence::ScopeResolution,
               &Parser::parseScopeResolutionExpression);

    //~ Statements
    stmt(TokenType::Break, [](Parser* p) -> ast::StatementPtr {
        DISCARD(p->advance());
        p->expectToken(TokenType::Semicolon);
        return std::make_unique<ast::BreakStatement>();
    });
    stmt(TokenType::Bundle, &Parser::parseBundleDeclarationStatement);
    stmt(TokenType::Const, &Parser::parseVariableDeclarationStatement);
    stmt(TokenType::Continue, [](Parser* p) -> ast::StatementPtr {
        DISCARD(p->advance());
        p->expectToken(TokenType::Semicolon);
        return std::make_unique<ast::ContinueStatement>();
    });
    stmt(TokenType::Do, &Parser::parseDoWhileLoopStatement);
    stmt(TokenType::Enum, &Parser::parseEnumDeclarationStatement);
    stmt(TokenType::Func, &Parser::parseFunctionDeclarationStatement);
    stmt(TokenType::If, &Parser::parseIfStatement);
    stmt(TokenType::Let, &Parser::parseVariableDeclarationStatement);
    stmt(TokenType::Repeat, &Parser::parseRepeatLoopStatement);
    stmt(TokenType::Return, &Parser::parseReturnStatement);
    stmt(TokenType::Switch, &Parser::parseSwitchStatement);
    stmt(TokenType::While, &Parser::parseWhileLoopStatement);

    //~ Misc
    led_binary(TokenType::As, Precedence::TypeCast, &Parser::parseTypeCastExpression);
}
}  // namespace parser
}  // namespace Manganese
