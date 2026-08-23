#include <core.hpp>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <mnstl/number.hxx>
#include <string>
#include <utility>
#include <vector>
#include "frontend/parser/operators.hpp"

/**
 * Ambiguous cases:
 * Ambiguous case 1: `*`, `&`, `+` and `-`
 * All of these have unary versions and binary versions:
 * &: bitwise AND or address-of
 * *: multiplication or dereference
 * +: Addition or unary plus
 * -: Subtraction or unary minus
 *
 * Use the unary operator if the previous token is also an operator, or a `(`
 * Exception: postfix operators (++, --, [] and ()) don't count
 *      (though only ++ and -- are "officially" operators in the token type enum)
 * Otherwise use binary version
 *
 * Ambiguous case 2: `[]`
 * Used to declare an array type (e.g. int[])
 * Used for indexing (e.g. arr[a + b])
 * Used to declare an array (e.g. let x: int[] = [1,2,3])
 * Used in generics (e.g. foo@[int])  -- not really an issue (handled by parsing the `@` operator)
 * Array types handled in type parsing in variable declarations
 * To distinguish: Indexing is left denoted, array declarations are null denoted
 *
 * Ambiguous case 3: `()`
 * Used to group an expression (e.g. 3* (1+2))
 * Used to call a function (e.g. foo())
 * To distinguish: Calls are left denoted, groupings are null denoted
 *
 * Ambiguous case 4: `as`
 * Use for module/type aliasing (import `blah` as `x`, alias `a` as `b`)
 * Type casting
 * When parsing imports : check for `as` -- if present add an aliased name to the node
 * When parsing alias: require an `as` followed by an identifier
 * Everywhere else, interpret as an infix operator
 */

namespace Manganese::parser {

ast::Expression* Parser::parseExpression(Precedence precedence) {
    if (isUnaryContext()) {
        Token& lookahead = peekToken();
        if (lookahead.hasUnaryCounterpart()) {
            lookahead.overrideType(lookahead.getUnaryCounterpart(), lookahead.getLexeme());
        }
    }

    Token token = peekToken();
    TokenType type = token.getType();
    const std::size_t index = tokenToIndex(type);

    const nudHandler_t nudHandler = nudLookup[index];
    if (!nudHandler) {
        ASSERT_UNREACHABLE("No null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    // ast::Expression* left = nudIterator->second(this);
    ast::Expression* left = (this->*nudHandler)();

    while (!done()) {
        token = peekToken();

        if (isUnaryContext() && token.hasUnaryCounterpart()) {
            token.overrideType(token.getUnaryCounterpart(), token.getLexeme());
            precedence = Precedence::Unary;
        }
        type = token.getType();
        const std::size_t idx = tokenToIndex(type);
        const Operator& op = operatorPrecedenceMap[idx];

        if (op.leftBindingPower <= precedence) { break; }

        const ledHandler_t handler = ledLookup[idx];
        if (!handler) {
            ASSERT_UNREACHABLE("No left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }

        left = (this->*handler)(left, op.rightBindingPower);
    }
    return left;
}

// Specific expression parsing methods

ast::Expression* Parser::parseAggregateInstantiationExpression(ast::Expression* left, Precedence) {
    const Token startToken = peekToken();
    expectToken(lexer::TokenType::LeftBrace, "Expected '{' to start aggregate instantiation");
    std::vector<ast::AggregateInstantiationField> fields;

    while (!done()) {
        if (peekTokenType() == lexer::TokenType::RightBrace) { break; }
        Token token = expectToken(lexer::TokenType::Identifier, "Expected field name in aggregate instantiation");
        std::string propertyName = token.getLexeme();
        expectToken(lexer::TokenType::Assignment, "Expected '=' to assign value to aggregate field");
        constexpr auto precedence = precedenceAbove(Precedence::Assignment);
        ast::Expression* value = parseExpression(precedence);

        fields.push_back({.name = propertyName, .value = value, .line = token.getLine(), .column = token.getColumn()});

        if (peekTokenType() != lexer::TokenType::RightBrace) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate aggregate fields");
        }
    }
    expectToken(lexer::TokenType::RightBrace, "Expected '}' to end aggregate instantiation");
    return makeNode<ast::AggregateInstantiationExpression>(startToken, left, std::move(fields));
}

ast::Expression* Parser::parseAggregateLiteralExpression() {
    const Token startToken = consumeToken();  // go past the aggregate keyword

    expectToken(TokenType::LeftBrace, "Expected '{' to start an aggregate literal");

    std::vector<ast::Expression*> expressions;
    while (peekTokenType() != TokenType::RightBrace) {
        expressions.push_back(parseExpression(Precedence::Default));
        if (peekTokenType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma,
                        "Expected a ',' to separate aggregate literal fields, or a '}' to close the declaration");
        }
    }
    expectToken(TokenType::RightBrace, "Expected a '}' to end an aggreagate literal");
    return makeNode<ast::AggregateLiteralExpression>(startToken, std::move(expressions));
}

ast::Expression* Parser::parseAlignofExpression() {
    const Token startToken = consumeToken();  // discard alignof
    expectToken(lexer::TokenType::LeftParen, "Expected '(' after alignof");
    ast::Type* type = parseType(Precedence::Default);
    if (peekTokenType() != lexer::TokenType::RightParen) {
        logError(
            peekToken().getLine(), peekToken().getColumn(),
            "alignof expects a type as its argument. If you are trying to take the type of an expression, use alignof(typeof(...))");

        while (peekTokenType() != lexer::TokenType::RightParen && peekTokenType() != lexer::TokenType::Semicolon
               && peekTokenType() != lexer::TokenType::EndOfFile) {
            DISCARD(consumeToken());
        }

        // If we successfully skipped to the closing parenthesis, consume it
        if (peekTokenType() == lexer::TokenType::RightParen) { DISCARD(consumeToken()); }

        return makeNode<ast::AlignofExpression>(startToken, makeNode<ast::SymbolType>(startToken, "dummy"));
    }
    expectToken(lexer::TokenType::RightParen, "Expected ')' to enclose alignof");
    return makeNode<ast::AlignofExpression>(startToken, type);
}

ast::Expression* Parser::parseArrayInstantiationExpression() {
    const Token startToken = consumeToken();  // Consume the left square bracket
    std::vector<ast::Expression*> elements;
    while (!done()) {
        if (peekTokenType() == lexer::TokenType::RightSquare) {
            break;  // Done instantiation
        }
        constexpr auto precedence = precedenceAbove(Precedence::Assignment);
        elements.push_back(parseExpression(precedence));
        if (peekTokenType() != lexer::TokenType::RightSquare) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate array elements");
        }
    }
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end array instantiation");

    return makeNode<ast::ArrayLiteralExpression>(startToken, std::move(elements));
}

ast::Expression* Parser::parseAssignmentExpression(ast::Expression* left, Precedence precedence) {
    const Token startToken = consumeToken();
    const TokenType op = startToken.getType();
    ast::Expression* right = parseExpression(precedence);

    return makeNode<ast::AssignmentExpression>(startToken, left, op, right);
}

ast::Expression* Parser::parseBinaryExpression(ast::Expression* left, Precedence precedence) {
    const Token startToken = consumeToken();
    const lexer::TokenType op = startToken.getType();
    ast::Expression* right = parseExpression(precedence);

    return makeNode<ast::BinaryExpression>(startToken, left, op, right);
}

ast::Expression* Parser::parseFunctionCallExpression(ast::Expression* left, Precedence) {
    const Token startToken = consumeToken();
    std::vector<ast::Expression*> arguments;

    while (!done()) {
        if (peekTokenType() == lexer::TokenType::RightParen) {
            break;  // Done with arguments
        }
        arguments.push_back(parseExpression(Precedence::Assignment));
        if (peekTokenType() != lexer::TokenType::RightParen && peekTokenType() != lexer::TokenType::EndOfFile) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate function arguments");
        }
    }
    expectToken(lexer::TokenType::RightParen, "Expected ')' to end function call");
    return makeNode<ast::FunctionCallExpression>(startToken, left, std::move(arguments));
}

ast::Expression* Parser::parseGenericExpression(ast::Expression* left, Precedence) {
    const Token startToken = consumeToken();  // Consume the '@' token
    expectToken(lexer::TokenType::LeftSquare, "Expected '[' to start generic type parameters");
    std::vector<ast::Type*> typeParameters;
    while (!done()) {
        if (peekTokenType() == lexer::TokenType::RightSquare) {
            break;  // Done with type parameters
        }
        typeParameters.push_back(parseType(Precedence::Default));
        if (peekTokenType() != lexer::TokenType::RightSquare) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate generic types");
        }
    }
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end generic type parameters");
    return makeNode<ast::GenericExpression>(startToken, left, std::move(typeParameters));
}

ast::Expression* Parser::parseIndexingExpression(ast::Expression* left, Precedence) {
    const Token startToken = consumeToken();  // Consume the left square bracket
    constexpr auto precedence = precedenceAbove(Precedence::Assignment);
    ast::Expression* index = parseExpression(precedence);
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end indexing expression");
    return makeNode<ast::IndexExpression>(startToken, left, index);
}

ast::Expression* Parser::parseMemberAccessExpression(ast::Expression* left, Precedence) {
    const Token startToken = consumeToken();  // Consume the member access operator (.)
    return makeNode<ast::MemberAccessExpression>(
        startToken, left, expectToken(lexer::TokenType::Identifier, "Expected identifier after '.'").getLexeme());
}

ast::Expression* Parser::parseParenthesizedExpression() {
    const Token startToken = consumeToken();  // Consume the left parenthesis
    ast::Expression* expr = parseExpression(Precedence::Default);
    expectToken(lexer::TokenType::RightParen, "Expected a right parenthesis to close the expression");
    return expr;
}

ast::Expression* Parser::parsePostfixExpression(ast::Expression* left, Precedence precedence) {
    const Token startToken = consumeToken();
    TokenType op = startToken.getType();
    DISCARD(precedence);  // Avoid unused variable warning
    return makeNode<ast::PostfixExpression>(startToken, left, op);
}

ast::Expression* Parser::parsePrefixExpression() {
    Token startToken = peekToken();
    TokenType op = startToken.getType();

    // Check if we need to convert to a unary counterpart
    if (startToken.hasUnaryCounterpart() && isUnaryContext()) { op = startToken.getUnaryCounterpart(); }

    // Now advance past the token
    DISCARD(consumeToken());

    ast::Expression* right = parseExpression(Precedence::Unary);
    return makeNode<ast::PrefixExpression>(startToken, op, right);
}

ast::Expression* Parser::parsePrimaryExpression() {
    const lexer::Token startToken = consumeToken();
    std::string lexeme = startToken.getLexeme();

    switch (startToken.getType()) {
        case TokenType::CharLiteral:
            return makeNode<ast::CharLiteralExpression>(startToken, lexeme[0]);  // Single character
        case TokenType::StrLiteral: return makeNode<ast::StringLiteralExpression>(startToken, std::move(lexeme));
        case TokenType::Identifier: return makeNode<ast::IdentifierExpression>(startToken, std::move(lexeme));
        case TokenType::True: return makeNode<ast::BoolLiteralExpression>(startToken, true);
        case TokenType::False: return makeNode<ast::BoolLiteralExpression>(startToken, false);
        case TokenType::FloatLiteral: {
            const mnstl::string_conversion_result_t<mnstl::number_t> value = mnstl::str_to_num(lexeme, true);
            if (!value.exists) {
                logError(startToken.getLine(), startToken.getColumn(), "Invalid float literal '{}'", lexeme);
                return makeNode<ast::NumberLiteralExpression>(startToken, 0.0);
            } else if (value.overflowed) {
                logError(startToken.getLine(), startToken.getColumn(),
                         "Float literal {} cannot fit in its assigned type", lexeme);
            }
            return makeNode<ast::NumberLiteralExpression>(startToken, value.value);
        }
        case TokenType::IntegerLiteral: {
            const mnstl::string_conversion_result_t<mnstl::number_t> value = mnstl::str_to_num(lexeme, false);
            if (!value.exists) {
                logError(startToken.getLine(), startToken.getColumn(), "Invalid integer literal '{}'", lexeme);
                return makeNode<ast::NumberLiteralExpression>(startToken, 0);
            } else if (value.overflowed) {
                logError(startToken.getLine(), startToken.getColumn(),
                         "Integer literal {} cannot fit in its assigned type", lexeme);
            }
            return makeNode<ast::NumberLiteralExpression>(startToken, value.value);
        }
        default:
            ASSERT_UNREACHABLE("Invalid Token Type in parsePrimaryExpression: "
                               + lexer ::tokenTypeToString(startToken.getType()));
    }
}

ast::Expression* Parser::parseScopeResolutionExpression(ast::Expression* left, Precedence) {
    const Token startToken = consumeToken();  // Consume the scope resolution operator (::)
    if (peekTokenType() != lexer::TokenType::Identifier) {
        logError(peekToken().getLine(), peekToken().getColumn(), "Expected identifier after '::'");
    }
    return makeNode<ast::ScopeResolutionExpression>(startToken, left, parseExpression(Precedence::ScopeResolution));
}

ast::Expression* Parser::parseSizeofExpression() {
    const Token startToken = consumeToken();  // discard sizeof
    expectToken(lexer::TokenType::LeftParen, "Expected '(' after sizeof");
    ast::Type* type = parseType(Precedence::Default);
    if (peekTokenType() != lexer::TokenType::RightParen) {
        logError(
            peekToken().getLine(), peekToken().getColumn(),
            "sizeof expects a type as its argument. If you are trying to take the type of an expression, use sizeof(typeof(...))");

        while (peekTokenType() != lexer::TokenType::RightParen && peekTokenType() != lexer::TokenType::Semicolon
               && peekTokenType() != lexer::TokenType::EndOfFile) {
            DISCARD(consumeToken());
        }

        // If we successfully skipped to the closing parenthesis, consume it
        if (peekTokenType() == lexer::TokenType::RightParen) { DISCARD(consumeToken()); }

        return makeNode<ast::SizeofExpression>(startToken, makeNode<ast::SymbolType>(startToken, "dummy"));
    }
    expectToken(lexer::TokenType::RightParen, "Expected ')' to enclose sizeof");
    return makeNode<ast::SizeofExpression>(startToken, type);
}

ast::Expression* Parser::parseTypeCastExpression(ast::Expression* left, Precedence precedence) {
    const Token startToken = consumeToken();  // Consume the 'as' token
    ast::Type* type = parseType(precedence);
    return makeNode<ast::TypeCastExpression>(startToken, left, type);
}
}  // namespace Manganese::parser
