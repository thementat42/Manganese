#ifndef MANGANESE_INCLUDE_FRONTEND_PARSER_LOOKUP_TABLE
#define MANGANESE_INCLUDE_FRONTEND_PARSER_LOOKUP_TABLE 1

#include <array>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/lexer/token.hpp>
#include <frontend/parser/operators.hpp>
#include <frontend/parser/parser_base.hpp>

namespace Manganese::parser {

using statementHandler_t = ast::Statement* (Parser::*)();
using nudHandler_t = ast::Expression* (Parser::*)();
using nudHandler_types_t = ast::Type* (Parser::*)();
using ledHandler_t = ast::Expression* (Parser::*)(ast::Expression*, Precedence);
using ledHandler_types_t = ast::Type* (Parser::*)(ast::Type*, Precedence);

struct LookupTable {
    constexpr static inline auto _lookupSize = static_cast<std::size_t>(lexer::TokenType::_tokenCount);
    std::array<statementHandler_t, _lookupSize> statementLookup{};
    std::array<nudHandler_t, _lookupSize> nudLookup{};
    std::array<ledHandler_t, _lookupSize> ledLookup{};
    std::array<Operator, _lookupSize> operatorPrecedenceMap{};

    std::array<nudHandler_types_t, _lookupSize> nudLookup_types{};
    std::array<ledHandler_types_t, _lookupSize> ledLookup_types{};
    std::array<Operator, _lookupSize> operatorPrecedenceMap_type{};
};

// ~ Helpers for lookups
constexpr std::size_t tokenToIndex(lexer::TokenType t) noexcept { return static_cast<std::size_t>(t); }
consteval Precedence precedenceAbove(Precedence p) noexcept {
    return static_cast<Precedence>(static_cast<std::underlying_type_t<Precedence>>(p) + 1);
}

constexpr void registerLedHandler_binary(LookupTable& table, lexer::TokenType type, Precedence precedence,
                                         ledHandler_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);
    table.operatorPrecedenceMap[index] = Operator::binary(precedence);
    table.ledLookup[index] = handler;
}

constexpr void registerLedHandler_postfix(LookupTable& table, lexer::TokenType type, Precedence precedence,
                                          ledHandler_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);
    table.operatorPrecedenceMap[index] = Operator::postfix(precedence);
    table.ledLookup[index] = handler;
}

constexpr void registerLedHandler_prefix(LookupTable& table, lexer::TokenType type, Precedence precedence,
                                         ledHandler_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);
    table.operatorPrecedenceMap[index] = Operator::prefix(precedence);
    table.ledLookup[index] = handler;
}

constexpr void registerNudHandler_binary(LookupTable& table, lexer::TokenType type,
                                         nudHandler_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);
    table.operatorPrecedenceMap[index] = Operator::prefix(Precedence::Default);
    table.nudLookup[index] = handler;
}

constexpr void registerNudHandler_prefix(LookupTable& table, lexer::TokenType type,
                                         nudHandler_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);
    table.operatorPrecedenceMap[index] = Operator::prefix(Precedence::Default);
    table.nudLookup[index] = handler;
}

constexpr void registerLedHandler_type(LookupTable& table, lexer::TokenType type, Precedence precedence,
                                       ledHandler_types_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);
    table.operatorPrecedenceMap_type[index] = Operator::binary(precedence);
    table.ledLookup_types[index] = handler;
}

constexpr void registerNudHandler_type(LookupTable& table, lexer::TokenType type,
                                       nudHandler_types_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);
    table.operatorPrecedenceMap_type[index]
        = Operator{.leftBindingPower = Precedence::Primary, .rightBindingPower = Precedence::Default};
    table.nudLookup_types[index] = handler;
}

constexpr void registerStmtHandler(LookupTable& table, lexer::TokenType type,
                                   statementHandler_t handler) noexcept {
    const std::size_t index = tokenToIndex(type);

    table.operatorPrecedenceMap[index]
        = Operator{.leftBindingPower = Precedence::Default, .rightBindingPower = Precedence::Default};
    table.statementLookup[index] = handler;
}

consteval void initializeLookups(LookupTable& table) noexcept {
    using enum lexer::TokenType;
    //~ Assignments (updating variables, not initializing them)
    registerLedHandler_binary(table, Assignment, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, BitAndAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, BitLShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, BitOrAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, BitRShiftAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, BitXorAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, DivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, FloorDivAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, MinusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, ModAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, MulAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);
    registerLedHandler_binary(table, PlusAssign, Precedence::Assignment, &Parser::parseAssignmentExpression);

    //~ Bitwise Operators
    registerLedHandler_binary(table, BitAnd, Precedence::BitwiseAnd, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, BitLShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, BitOr, Precedence::BitwiseOr, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, BitRShift, Precedence::BitwiseShift, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, BitXor, Precedence::BitwiseXor, &Parser::parseBinaryExpression);

    //~ Relational
    registerLedHandler_binary(table, Equal, Precedence::Equality, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, NotEqual, Precedence::Equality, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, GreaterThan, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, GreaterThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, LessThan, Precedence::Relational, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, LessThanOrEqual, Precedence::Relational, &Parser::parseBinaryExpression);

    //~ Additive, Multiplicative, Exponential, Logical
    registerLedHandler_binary(table, And, Precedence::LogicalAnd, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, Div, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, FloorDiv, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, Minus, Precedence::Additive, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, Mod, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, Mul, Precedence::Multiplicative, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, Or, Precedence::LogicalOr, &Parser::parseBinaryExpression);
    registerLedHandler_binary(table, Plus, Precedence::Additive, &Parser::parseBinaryExpression);

    //~ Literals and Symbols
    registerNudHandler_binary(table, CharLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(table, False, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(table, FloatLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(table, Identifier, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(table, IntegerLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(table, LeftParen, &Parser::parseParenthesizedExpression);
    registerNudHandler_binary(table, StrLiteral, &Parser::parsePrimaryExpression);
    registerNudHandler_binary(table, True, &Parser::parsePrimaryExpression);

    //~ Prefix Operators
    registerNudHandler_prefix(table, AddressOf, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(table, BitNot, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(table, Dec, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(table, Dereference, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(table, Inc, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(table, Not, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(table, UnaryMinus, &Parser::parsePrefixExpression);
    registerNudHandler_prefix(table, UnaryPlus, &Parser::parsePrefixExpression);

    //~ PostFix Expression
    registerLedHandler_postfix(table, Dec, Precedence::Postfix, &Parser::parsePostfixExpression);
    registerLedHandler_postfix(table, Inc, Precedence::Postfix, &Parser::parsePostfixExpression);

    //~ Call/Member Expressions
    registerLedHandler_binary(table, At, Precedence::Postfix, &Parser::parseGenericInstantiationExpression);
    registerLedHandler_binary(table, LeftBrace, Precedence::Postfix, &Parser::parseAggregateInstantiationExpression);
    registerLedHandler_binary(table, LeftParen, Precedence::Postfix, &Parser::parseFunctionCallExpression);
    registerNudHandler_binary(table, LeftSquare, &Parser::parseArrayInstantiationExpression);
    registerLedHandler_binary(table, LeftSquare, Precedence::Postfix, &Parser::parseIndexingExpression);
    registerLedHandler_binary(table, MemberAccess, Precedence::Member, &Parser::parseMemberAccessExpression);
    registerLedHandler_binary(table, ScopeResolution, Precedence::ScopeResolution,
                              &Parser::parseScopeResolutionExpression);

    //~ Statements
    registerStmtHandler(table, Alias, &Parser::parseAliasStatement);
    registerStmtHandler(table, Break, &Parser::parseBreakStatement);
    registerStmtHandler(table, Aggregate, &Parser::parseAggregateDeclarationStatement);
    registerStmtHandler(table, Continue, &Parser::parseContinueStatement);
    registerStmtHandler(table, Do, &Parser::parseDoWhileLoopStatement);
    registerStmtHandler(table, Enum, &Parser::parseEnumDeclarationStatement);
    registerStmtHandler(table, For, &Parser::parseForLoopStatement);
    registerStmtHandler(table, Func, &Parser::parseFunctionDeclarationStatement);
    registerStmtHandler(table, If, &Parser::parseIfStatement);
    registerStmtHandler(table, Import, &Parser::parseImportStatement);
    registerStmtHandler(table, Let, &Parser::parseVariableDeclarationStatement);
    registerStmtHandler(table, Module, &Parser::parseModuleDeclarationStatement);
    registerStmtHandler(table, Namespace, &Parser::parseNamespace);
    registerStmtHandler(table, Private, &Parser::parseVisibilityAffectedStatement);
    registerStmtHandler(table, Public, &Parser::parseVisibilityAffectedStatement);
    registerStmtHandler(table, Return, &Parser::parseReturnStatement);
    registerStmtHandler(table, Switch, &Parser::parseSwitchStatement);
    registerStmtHandler(table, While, &Parser::parseWhileLoopStatement);

    //~ Misc
    registerLedHandler_binary(table, As, Precedence::TypeCast, &Parser::parseTypeCastExpression);
    registerStmtHandler(table, Semicolon, &Parser::parseRedundantSemicolon);
    registerNudHandler_binary(table, Aggregate, &Parser::parseAggregateLiteralExpression);
    registerNudHandler_binary(table, Sizeof, &Parser::parseSizeofExpression);
    registerNudHandler_binary(table, Alignof, &Parser::parseAlignofExpression);
}

consteval void initializeTypeLookups(LookupTable& table) noexcept {
    using enum lexer::TokenType;
    //~ Variable declarations with primitive types
    registerNudHandler_type(table, Identifier, &Parser::parseSymbolType);
    registerNudHandler_type(table, Int8, &Parser::parseSymbolType);
    registerNudHandler_type(table, UInt8, &Parser::parseSymbolType);
    registerNudHandler_type(table, Int16, &Parser::parseSymbolType);
    registerNudHandler_type(table, UInt16, &Parser::parseSymbolType);
    registerNudHandler_type(table, Int32, &Parser::parseSymbolType);
    registerNudHandler_type(table, UInt32, &Parser::parseSymbolType);
    registerNudHandler_type(table, Int64, &Parser::parseSymbolType);
    registerNudHandler_type(table, UInt64, &Parser::parseSymbolType);
    registerNudHandler_type(table, Float32, &Parser::parseSymbolType);
    registerNudHandler_type(table, Float64, &Parser::parseSymbolType);
    registerNudHandler_type(table, Int128, &Parser::parseSymbolType);
    registerNudHandler_type(table, UInt128, &Parser::parseSymbolType);
    registerNudHandler_type(table, Char, &Parser::parseSymbolType);
    registerNudHandler_type(table, Bool, &Parser::parseSymbolType);
    registerNudHandler_type(table, String, &Parser::parseSymbolType);
    registerNudHandler_type(table, Ptr, &Parser::parsePointerType);

    //~ Complex types
    registerNudHandler_type(table, Aggregate, &Parser::parseAggregateType);
    registerLedHandler_type(table, At, Precedence::Generic, &Parser::parseGenericInstantiationType);
    registerNudHandler_type(table, Func, &Parser::parseFunctionType);
    registerLedHandler_type(table, LeftSquare, Precedence::Postfix, &Parser::parseArrayType);
    registerNudHandler_type(table, LeftParen, &Parser::parseParenthesizedType);
    registerNudHandler_type(table, Typeof, &Parser::parseTypeofType);
    registerLedHandler_type(table, ScopeResolution, Precedence::ScopeResolution, &Parser::parseScopedType);
}

consteval LookupTable makeLookupTable() noexcept {
    LookupTable table{};
    initializeLookups(table);
    initializeTypeLookups(table);
    return table;
}

constexpr inline LookupTable lookupTable = makeLookupTable();

}  // namespace Manganese::parser

#endif  // MANGANESE_INCLUDE_FRONTEND_PARSER_LOOKUP_TABLE