/**
 * @file Initializes the lookups for the pratt parser
 * @note This file is mostly boilerplate
 */

#include <frontend/ast.h>
#include <frontend/parser.h>

namespace Manganese {
namespace parser {
// ===== Lookup Registration Methods =====

void Parser::registerLedHandler_binary(TokenType type, Precedence bindingPower, ledHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::binary(bindingPower);
    ledLookup[type] = handler;
}
void Parser::registerLedHandler_rightAssoc(TokenType type, Precedence bindingPower, ledHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::rightAssociative(bindingPower);
    ledLookup[type] = handler;
}
void Parser::registerLedHandler_postfix(TokenType type, Precedence bindingPower, ledHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::postfix(bindingPower);
    ledLookup[type] = handler;
}
void Parser::registerLedHandler_prefix(TokenType type, Precedence bindingPower, ledHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::prefix(bindingPower);
    ledLookup[type] = handler;
}

void Parser::registerNudHandler_binary(TokenType type, nudHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::binary(Precedence::Default);
    nudLookup[type] = handler;
}

void Parser::registerNudHandler_prefix(TokenType type, nudHandler_t handler) {
    operatorPrecedenceMap[type] = Operator::prefix();
    nudLookup[type] = handler;
}

void Parser::registerStmtHandler(TokenType type,
                                 statementHandler_t handler) {
    operatorPrecedenceMap[type] = Operator{
        .leftBindingPower = Precedence::Default,
        .rightBindingPower = Precedence::Default};
    statementLookup[type] = handler;
}

// ===== Actually register the lookups =====

//! Really long stuff

void Parser::initializeLookups() {

    //~ Assignments (updating variables, not initializing them)
    registerLedHandler_binary(TokenType::Assignment, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::BitAndAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::BitLShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::BitNotAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::BitOrAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::BitRShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::BitXorAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::DivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::ExpAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::FloorDivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::MinusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::ModAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::MulAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(TokenType::PlusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);

    //~ Bitwise Operators
    registerLedHandler_binary(TokenType::BitAnd, Precedence::BitwiseAnd, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::BitLShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::BitOr, Precedence::BitwiseOr, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::BitRShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::BitXor, Precedence::BitwiseXor, &Parser::parseBinaryExpression);

    //~ Relational
    registerLedHandler_binary(TokenType::Equal, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::GreaterThan, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::GreaterThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::LessThan, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::LessThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::NotEqual, Precedence::Relational, &Parser::parseBinaryExpression);

    //~ Additive, Multiplicative, Exponential, Logical
    registerLedHandler_binary(TokenType::And, Precedence::LogicalAnd, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::Div, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::FloorDiv, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::Minus, Precedence::Additive, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::Mod, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::Mul, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_rightAssoc(TokenType::Exp, Precedence::Exponential, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::Or, Precedence::LogicalOr, &Parser::parseBinaryExpression);
    registerLedHandler_binary(TokenType::Plus, Precedence::Additive, &Parser::parseBinaryExpression);

    //~ Literals and Symbols
    registerNudHandler_binary(TokenType::CharLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(TokenType::False, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(TokenType::FloatLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(TokenType::Identifier, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(TokenType::IntegerLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(TokenType::LeftParen, &Parser::parseParenthesizedExpression);
    registerNudHandler_binary(TokenType::StrLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(TokenType::True, &Parser::parsePrimaryExpression);

    //~ Prefix Operators
    registerNudHandler_prefix(TokenType::AddressOf, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(TokenType::BitNot, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(TokenType::Dec, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(TokenType::Dereference, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(TokenType::Inc, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(TokenType::Not, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(TokenType::UnaryMinus, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(TokenType::UnaryPlus, &Parser::parsePrefixExpression);

    //~ PostFix Expression
    registerLedHandler_postfix(TokenType::Dec, Precedence::Postfix, &Parser::parsePostfixExpression);
    registerLedHandler_postfix(TokenType::Inc, Precedence::Postfix, &Parser::parsePostfixExpression);

    //~ Call/Member Expressions
    registerLedHandler_binary(TokenType::At, Precedence::Postfix, &Parser::parseGenericExpression);
    registerLedHandler_binary(TokenType::LeftBrace, Precedence::Postfix, &Parser::parseBundleInstantiationExpression);
    registerLedHandler_binary(TokenType::LeftParen, Precedence::Postfix, &Parser::parseFunctionCallExpression);
    registerNudHandler_binary(TokenType::LeftSquare, &Parser::parseArrayInstantiationExpression);
    registerLedHandler_binary(TokenType::LeftSquare, Precedence::Postfix, &Parser::parseIndexingExpression);
    registerLedHandler_binary(TokenType::MemberAccess, Precedence::Member, &Parser::parseMemberAccessExpression);
    registerLedHandler_binary(TokenType::ScopeResolution, Precedence::ScopeResolution,
                              &Parser::parseScopeResolutionExpression);

    //~ Statements
    registerStmtHandler(TokenType::Break, [](Parser* p) -> ast::StatementPtr_t {
        DISCARD(p->advance());
        p->expectToken(TokenType::Semicolon);
        return std::make_unique<ast::BreakStatement>();
    });
    registerStmtHandler(TokenType::Bundle, &Parser::parseBundleDeclarationStatement);
    registerStmtHandler(TokenType::Const, &Parser::parseVariableDeclarationStatement);
    registerStmtHandler(TokenType::Continue, [](Parser* p) -> ast::StatementPtr_t {
        DISCARD(p->advance());
        p->expectToken(TokenType::Semicolon);
        return std::make_unique<ast::ContinueStatement>();
    });
    registerStmtHandler(TokenType::Do, &Parser::parseDoWhileLoopStatement);
    registerStmtHandler(TokenType::Enum, &Parser::parseEnumDeclarationStatement);
    registerStmtHandler(TokenType::Func, &Parser::parseFunctionDeclarationStatement);
    registerStmtHandler(TokenType::If, &Parser::parseIfStatement);
    registerStmtHandler(TokenType::Import, &Parser::parseImportStatement);
    registerStmtHandler(TokenType::Let, &Parser::parseVariableDeclarationStatement);
    registerStmtHandler(TokenType::Module, &Parser::parseModuleDeclarationStatement);
    registerStmtHandler(TokenType::Repeat, &Parser::parseRepeatLoopStatement);
    registerStmtHandler(TokenType::Return, &Parser::parseReturnStatement);
    registerStmtHandler(TokenType::Switch, &Parser::parseSwitchStatement);
    registerStmtHandler(TokenType::While, &Parser::parseWhileLoopStatement);
    registerStmtHandler(TokenType::Alias, &Parser::parseAliasStatement);

    //~ Misc
    registerLedHandler_binary(TokenType::As, Precedence::TypeCast, &Parser::parseTypeCastExpression);
}

}  // namespace parser

}  // namespace Manganese
