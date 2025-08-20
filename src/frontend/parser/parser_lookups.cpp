/**
 * @file Initializes the lookups for the pratt parser
 * @note This file is mostly boilerplate
 */

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <frontend/lexer/token_type.hpp>

namespace Manganese {
namespace parser {
// ===== Lookup Registration Methods =====

void Parser::registerLedHandler_binary(TokenType type, Precedence bindingPower, ledHandler_t handler) noexcept {
    operatorPrecedenceMap[type] = Operator::binary(bindingPower);
    ledLookup[type] = handler;
}
void Parser::registerLedHandler_rightAssoc(TokenType type, Precedence bindingPower, ledHandler_t handler) noexcept {
    operatorPrecedenceMap[type] = Operator::rightAssociative(bindingPower);
    ledLookup[type] = handler;
}
void Parser::registerLedHandler_postfix(TokenType type, Precedence bindingPower, ledHandler_t handler) noexcept {
    operatorPrecedenceMap[type] = Operator::postfix(bindingPower);
    ledLookup[type] = handler;
}
void Parser::registerLedHandler_prefix(TokenType type, Precedence bindingPower, ledHandler_t handler) noexcept {
    operatorPrecedenceMap[type] = Operator::prefix(bindingPower);
    ledLookup[type] = handler;
}

void Parser::registerNudHandler_binary(TokenType type, nudHandler_t handler) noexcept {
    operatorPrecedenceMap[type] = Operator::binary(Precedence::Default);
    nudLookup[type] = handler;
}

void Parser::registerNudHandler_prefix(TokenType type, nudHandler_t handler) noexcept {
    operatorPrecedenceMap[type] = Operator::prefix();
    nudLookup[type] = handler;
}

void Parser::registerStmtHandler(TokenType type, statementHandler_t handler) noexcept {
    operatorPrecedenceMap[type]
        = Operator{.leftBindingPower = Precedence::Default, .rightBindingPower = Precedence::Default};
    statementLookup[type] = handler;
}

// ===== Type Lookup Registration Methods =====

void Parser::registerLedHandler_type(TokenType type, Precedence precedence, ledHandler_types_t handler) noexcept {
    operatorPrecedenceMap_type[type] = Operator::binary(precedence);
    ledLookup_types[type] = handler;
}

void Parser::registerNudHandler_type(TokenType type, nudHandler_types_t handler) noexcept {
    operatorPrecedenceMap_type[type]
        = Operator{.leftBindingPower = Precedence::Primary, .rightBindingPower = Precedence::Default};
    nudLookup_types[type] = handler;
}

// ===== Actually register the lookups =====

//! Really long stuff

void Parser::initializeLookups() noexcept {
    using enum lexer::TokenType;
    //~ Assignments (updating variables, not initializing them)
    registerLedHandler_binary(Assignment, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(BitAndAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(BitLShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(BitNotAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(BitOrAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(BitRShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(BitXorAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(DivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(ExpAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(FloorDivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(MinusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(ModAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(MulAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(PlusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);

    //~ Bitwise Operators
    registerLedHandler_binary(BitAnd, Precedence::BitwiseAnd, &Parser::parseBinaryExpression);
    registerLedHandler_binary(BitLShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    registerLedHandler_binary(BitOr, Precedence::BitwiseOr, &Parser::parseBinaryExpression);
    registerLedHandler_binary(BitRShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    registerLedHandler_binary(BitXor, Precedence::BitwiseXor, &Parser::parseBinaryExpression);

    //~ Relational
    registerLedHandler_binary(Equal, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(GreaterThan, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(GreaterThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(LessThan, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(LessThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(NotEqual, Precedence::Relational, &Parser::parseBinaryExpression);

    //~ Additive, Multiplicative, Exponential, Logical
    registerLedHandler_binary(And, Precedence::LogicalAnd, &Parser::parseBinaryExpression);
    registerLedHandler_binary(Div, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(FloorDiv, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(Minus, Precedence::Additive, &Parser::parseBinaryExpression);
    registerLedHandler_binary(Mod, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(Mul, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_rightAssoc(Exp, Precedence::Exponential, &Parser::parseBinaryExpression);
    registerLedHandler_binary(Or, Precedence::LogicalOr, &Parser::parseBinaryExpression);
    registerLedHandler_binary(Plus, Precedence::Additive, &Parser::parseBinaryExpression);

    //~ Literals and Symbols
    registerNudHandler_binary(CharLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(False, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(FloatLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(Identifier, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(IntegerLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(LeftParen, &Parser::parseParenthesizedExpression);
    registerNudHandler_binary(StrLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(True, &Parser::parsePrimaryExpression);

    //~ Prefix Operators
    registerNudHandler_prefix(AddressOf, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(BitNot, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(Dec, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(Dereference, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(Inc, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(Not, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(UnaryMinus, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(UnaryPlus, &Parser::parsePrefixExpression);

    //~ PostFix Expression
    registerLedHandler_postfix(Dec, Precedence::Postfix, &Parser::parsePostfixExpression);
    registerLedHandler_postfix(Inc, Precedence::Postfix, &Parser::parsePostfixExpression);

    //~ Call/Member Expressions
    registerLedHandler_binary(At, Precedence::Postfix, &Parser::parseGenericExpression);
    registerLedHandler_binary(LeftBrace, Precedence::Postfix, &Parser::parseAggregateInstantiationExpression);
    registerLedHandler_binary(LeftParen, Precedence::Postfix, &Parser::parseFunctionCallExpression);
    registerNudHandler_binary(LeftSquare, &Parser::parseArrayInstantiationExpression);
    registerLedHandler_binary(LeftSquare, Precedence::Postfix, &Parser::parseIndexingExpression);
    registerLedHandler_binary(MemberAccess, Precedence::Member, &Parser::parseMemberAccessExpression);
    registerLedHandler_binary(ScopeResolution, Precedence::ScopeResolution,
                              &Parser::parseScopeResolutionExpression);

    //~ Statements
    registerStmtHandler(Alias, &Parser::parseAliasStatement);
    registerStmtHandler(Break, &Parser::parseBreakStatement);
    registerStmtHandler(Aggregate, &Parser::parseAggregateDeclarationStatement);
    registerStmtHandler(Continue, &Parser::parseContinueStatement);
    registerStmtHandler(Do, &Parser::parseDoWhileLoopStatement);
    registerStmtHandler(Enum, &Parser::parseEnumDeclarationStatement);
    registerStmtHandler(Func, &Parser::parseFunctionDeclarationStatement);
    registerStmtHandler(If, &Parser::parseIfStatement);
    registerStmtHandler(Import, &Parser::parseImportStatement);
    registerStmtHandler(Let, &Parser::parseVariableDeclarationStatement);
    registerStmtHandler(Module, &Parser::parseModuleDeclarationStatement);
    registerStmtHandler(Private, &Parser::parseVisibilityAffectedStatement);
    registerStmtHandler(Public, &Parser::parseVisibilityAffectedStatement);
    registerStmtHandler(ReadOnly, &Parser::parseVisibilityAffectedStatement);
    registerStmtHandler(Repeat, &Parser::parseRepeatLoopStatement);
    registerStmtHandler(Return, &Parser::parseReturnStatement);
    registerStmtHandler(Switch, &Parser::parseSwitchStatement);
    registerStmtHandler(While, &Parser::parseWhileLoopStatement);

    //~ Misc
    registerLedHandler_binary(As, Precedence::TypeCast, &Parser::parseTypeCastExpression);
    registerStmtHandler(Semicolon, &Parser::parseRedundantSemicolon);
}

void Parser::initializeTypeLookups() noexcept {
    using enum lexer::TokenType;
    //~ Variable declarations with primitive types
    registerNudHandler_type(Identifier, &Parser::parseSymbolType);
    registerNudHandler_type(Int8, &Parser::parseSymbolType);
    registerNudHandler_type(UInt8, &Parser::parseSymbolType);
    registerNudHandler_type(Int16, &Parser::parseSymbolType);
    registerNudHandler_type(UInt16, &Parser::parseSymbolType);
    registerNudHandler_type(Int32, &Parser::parseSymbolType);
    registerNudHandler_type(UInt32, &Parser::parseSymbolType);
    registerNudHandler_type(Int64, &Parser::parseSymbolType);
    registerNudHandler_type(UInt64, &Parser::parseSymbolType);
    registerNudHandler_type(Float32, &Parser::parseSymbolType);
    registerNudHandler_type(Float64, &Parser::parseSymbolType);
    registerNudHandler_type(Char, &Parser::parseSymbolType);
    registerNudHandler_type(Bool, &Parser::parseSymbolType);
    registerNudHandler_type(String, &Parser::parseSymbolType);
    registerNudHandler_type(Ptr, &Parser::parsePointerType);

    //~ Complex types
    registerNudHandler_type(Aggregate, &Parser::parseAggregateType);
    registerLedHandler_type(At, Precedence::Generic, &Parser::parseGenericType);
    registerNudHandler_type(Func, &Parser::parseFunctionType);
    registerLedHandler_type(LeftSquare, Precedence::Postfix, &Parser::parseArrayType);
    registerNudHandler_type(LeftParen, &Parser::parseParenthesizedType);
}

}  // namespace parser

}  // namespace Manganese
