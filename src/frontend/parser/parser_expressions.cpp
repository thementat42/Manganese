/**
 * @file expressions.cpp
 * @brief This file contains the implementation of expression parsing in the parser. It is split into its own file for
 * readability and maintainability.
 */

#include <algorithm>
#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <mnstl/number.hxx>
#include <string>
#include <utility>
#include <vector>

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

namespace Manganese {

namespace parser {

ast::Expression* Parser::parseExpression(Precedence precedence) {
    Token token = peekToken();

    // Handle operators which have a unary and a binary version
    // (e.g. `-` can be a unary negation or a binary subtraction)
    if (isUnaryContext() && token.hasUnaryCounterpart()) {
        token.overrideType(token.getUnaryCounterpart(), token.getLexeme());
        precedence = Precedence::Unary;
    }
    TokenType type = token.getType();
    const auto index = tokenToIndex(type);

    auto nudHandler = nudLookup[index];
    if (!nudHandler) {
        ASSERT_UNREACHABLE("No null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    // ast::Expression* left = nudIterator->second(this);
    ast::Expression* left = (this->*nudHandler)();

    // ! For some reason this works
    // ! Do not delete this
    if (type == TokenType::AddressOf || type == TokenType::Dereference) { precedence = Precedence::Default; }

    while (!done()) {
        token = peekToken();

        if (isUnaryContext() && token.hasUnaryCounterpart()) {
            token.overrideType(token.getUnaryCounterpart());
            precedence = Precedence::Unary;
        }
        type = token.getType();
        const auto idx = tokenToIndex(type);
        const Operator& op = operatorPrecedenceMap[idx];

        if (op.leftBindingPower <= precedence || !op.isValid) { break; }

        auto handler = ledLookup[idx];
        if (!handler) {
            ASSERT_UNREACHABLE("No left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }

        if (type == TokenType::LeftBrace && left->kind != ast::ExpressionKind::IdentifierExpression
            && left->kind != ast::ExpressionKind::GenericExpression) [[unlikely]] {
            if (isParsingBlockPrecursor) {
                // Left braces after an expression can either start a block or an aggregate instantiation
                // If we're parsing a block precursor (if/for/while, etc.) AND the previous expression is not an
                // identifier, we assume it's the start of a block and break If the previous expression IS an
                // identifier, it might be an inline aggregate instantiation
                return left;
            }
            logError(token.getLine(), token.getColumn(),
                     "Left brace after an expression must be preceded by an identifier (aggregate instantiation)"
                     " or a block precursor (if/for/while, etc.)");
        }
        left = (this->*handler)(left, op.rightBindingPower);
    }
    return left;
}

// Specific expression parsing methods

ast::Expression* Parser::parseAggregateInstantiationExpression(ast::Expression* left, Precedence precedence) {
    DISCARD(precedence);  // Avoid unused variable warning
    std::string aggregateName;
    std::vector<ast::Type*> genericTypes;
    // Parse out an aggregate instantiation even if there's an error
    expectToken(lexer::TokenType::LeftBrace, "Expected '{' to start aggregate instantiation");
    std::vector<ast::AggregateInstantiationField> fields;

    if (left->kind == ast::ExpressionKind::GenericExpression) {
        auto* genericExpr = static_cast<ast::GenericExpression*>(left);
        if (genericExpr->identifier->kind != ast::ExpressionKind::IdentifierExpression) {
            logError(left->getLine(), left->getColumn(),
                     "Generic aggregate instantiation must start with an aggregate name");
        } else {
            auto identifierExpr = static_cast<ast::IdentifierExpression*>(genericExpr->identifier);
            aggregateName = identifierExpr->value;
            genericTypes = std::move(genericExpr->types);
        }
    } else if (left->kind == ast::ExpressionKind::IdentifierExpression) {
        auto* underlying = static_cast<ast::IdentifierExpression*>(left);
        aggregateName = underlying->value;
    } else {
        logError(left->getLine(), left->getColumn(),
                 "Aggregate instantiation expression must start with an aggregate name, not {}", ast::toStringOr(left));
    }

    while (!done()) {
        if (peekTokenType() == lexer::TokenType::RightBrace) {
            break;  // Done instantiation
        }
        auto propertyName
            = expectToken(lexer::TokenType::Identifier, "Expected field name in aggregate instantiation").getLexeme();
        expectToken(lexer::TokenType::Assignment, "Expected '=' to assign value to aggregate field");
        // want precedence to be 1 higher than assignment (e.g. field = x = 10 is invalid)
        constexpr auto precedence_ = static_cast<std::underlying_type_t<Precedence>>(Precedence::Assignment) + 1;
        auto value = parseExpression(static_cast<Precedence>(precedence_));

        auto is_duplicate
            = [propertyName](const ast::AggregateInstantiationField& field) { return field.name == propertyName; };

        if (std::find_if(fields.begin(), fields.end(), is_duplicate) != fields.end()) {
            logError(value->getLine(), value->getColumn(), "Duplicate field '{}' in aggregate instantiation of '{}'",
                     propertyName, aggregateName);
        } else {
            fields.push_back({.name = propertyName, .value = value});
        }
        if (peekTokenType() != lexer::TokenType::RightBrace) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate aggregate fields");
        }
    }
    expectToken(lexer::TokenType::RightBrace, "Expected '}' to end aggregate instantiation");
    return arena.emplace<ast::AggregateInstantiationExpression>(std::move(aggregateName), std::move(genericTypes),
                                                                std::move(fields));
}

ast::Expression* Parser::parseAggregateLiteralExpression() {
    DISCARD(consumeToken());  // discard the aggregate keyword

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
    return arena.emplace<ast::AggregateLiteralExpression>(std::move(expressions));
}

ast::Expression* Parser::parseArrayInstantiationExpression() {
    DISCARD(consumeToken());  // Consume the left square bracket
    std::vector<ast::Expression*> elements;
    while (!done()) {
        if (peekTokenType() == lexer::TokenType::RightSquare) {
            break;  // Done instantiation
        }
        constexpr auto precedence = static_cast<std::underlying_type_t<Precedence>>(Precedence::Assignment) + 1;
        auto element = parseExpression(static_cast<Precedence>(precedence));
        elements.push_back(element);
        if (peekTokenType() != lexer::TokenType::RightSquare) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate array elements");
        }
    }
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end array instantiation");

    return arena.emplace<ast::ArrayLiteralExpression>(std::move(elements));
}

ast::Expression* Parser::parseAssignmentExpression(ast::Expression* left, Precedence precedence) {
    TokenType op = consumeToken().getType();
    ast::Expression* right = parseExpression(precedence);

    return arena.emplace<ast::AssignmentExpression>(left, op, right);
}

ast::Expression* Parser::parseBinaryExpression(ast::Expression* left, Precedence precedence) {
    auto op = consumeToken().getType();
    auto right = parseExpression(precedence);

    return arena.emplace<ast::BinaryExpression>(left, op, right);
}

ast::Expression* Parser::parseFunctionCallExpression(ast::Expression* left, Precedence precedence) {
    DISCARD(consumeToken());
    DISCARD(precedence);  // Avoid unused variable warning
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
    return arena.emplace<ast::FunctionCallExpression>(left, std::move(arguments));
}

ast::Expression* Parser::parseGenericExpression(ast::Expression* left, Precedence precedence) {
    DISCARD(consumeToken());  // Consume the '@' token
    DISCARD(precedence);  // Avoid unused variable warning
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
    return arena.emplace<ast::GenericExpression>(left, std::move(typeParameters));
}

ast::Expression* Parser::parseIndexingExpression(ast::Expression* left, Precedence precedence) {
    DISCARD(consumeToken());  // Consume the left square bracket
    DISCARD(precedence);  // Avoid unused variable warning
    constexpr auto precedence_ = static_cast<std::underlying_type_t<Precedence>>(Precedence::Assignment) + 1;
    ast::Expression* index = parseExpression(static_cast<Precedence>(precedence_));
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end indexing expression");
    return arena.emplace<ast::IndexExpression>(left, index);
}

ast::Expression* Parser::parseMemberAccessExpression(ast::Expression* left, Precedence precedence) {
    DISCARD(consumeToken());  // Consume the member access operator (.)
    DISCARD(precedence);  // Avoid unused variable warning
    return arena.emplace<ast::MemberAccessExpression>(
        std::move(left), expectToken(lexer::TokenType::Identifier, "Expected identifier after '.'").getLexeme());
}

ast::Expression* Parser::parseParenthesizedExpression() {
    DISCARD(consumeToken());  // Consume the left parenthesis
    ast::Expression* expr = parseExpression(Precedence::Default);
    expectToken(lexer::TokenType::RightParen, "Expected a right parenthesis to close the expression");
    return expr;
}

ast::Expression* Parser::parsePostfixExpression(ast::Expression* left, Precedence precedence) {
    TokenType op = consumeToken().getType();
    DISCARD(precedence);  // Avoid unused variable warning
    return arena.emplace<ast::PostfixExpression>(left, op);
}

ast::Expression* Parser::parsePrefixExpression() {
    Token token = peekToken();
    TokenType op = token.getType();

    // Check if we need to convert to a unary counterpart
    if (token.hasUnaryCounterpart() && isUnaryContext()) { op = token.getUnaryCounterpart(); }

    // Now advance past the token
    DISCARD(consumeToken());

    auto right = parseExpression(Precedence::Unary);
    return arena.emplace<ast::PrefixExpression>(op, right);
}

ast::Expression* Parser::parsePrimaryExpression() {
    auto token = consumeToken();
    std::string lexeme = token.getLexeme();

    switch (token.getType()) {
        case TokenType::CharLiteral: return arena.emplace<ast::CharLiteralExpression>(lexeme[0]);  // Single character
        case TokenType::StrLiteral: return arena.emplace<ast::StringLiteralExpression>(std::move(lexeme));
        case TokenType::Identifier: return arena.emplace<ast::IdentifierExpression>(std::move(lexeme));
        case TokenType::True: return arena.emplace<ast::BoolLiteralExpression>(true);
        case TokenType::False: return arena.emplace<ast::BoolLiteralExpression>(false);
        case TokenType::FloatLiteral: {
            mnstl::string_conversion_result_t<mnstl::number_t> value = mnstl::str_to_num(lexeme, true);
            if (!value.exists) {
                logError(token.getLine(), token.getColumn(), "Invalid float literal '{}'", lexeme);
                return arena.emplace<ast::NumberLiteralExpression>(0.0);
            } else if (value.overflowed) {
                logError(token.getLine(), token.getColumn(), "Float literal {} cannot fit in its assigned type",
                         lexeme);
            }
            return arena.emplace<ast::NumberLiteralExpression>(value.value);
        }
        case TokenType::IntegerLiteral: {
            mnstl::string_conversion_result_t<mnstl::number_t> value = mnstl::str_to_num(lexeme, false);
            if (!value.exists) {
                logError(token.getLine(), token.getColumn(), "Invalid integer literal '{}'", lexeme);
                return arena.emplace<ast::NumberLiteralExpression>(0);
            } else if (value.overflowed) {
                logError(token.getLine(), token.getColumn(), "Integer literal {} cannot fit in its assigned type",
                         lexeme);
            }
            return arena.emplace<ast::NumberLiteralExpression>(value.value);
        }
        default:
            ASSERT_UNREACHABLE("Invalid Token Type in parsePrimaryExpression: "
                               + lexer ::tokenTypeToString(token.getType()));
    }
}

ast::Expression* Parser::parseScopeResolutionExpression(ast::Expression* left, Precedence precedence) {
    DISCARD(consumeToken());  // Consume the scope resolution operator (::)
    DISCARD(precedence);  // Avoid unused variable warning
    auto element = expectToken(lexer::TokenType::Identifier, "Expected identifier after '::'").getLexeme();
    return arena.emplace<ast::ScopeResolutionExpression>(left, std::move(element));
}

ast::Expression* Parser::parseTypeCastExpression(ast::Expression* left, Precedence precedence) {
    DISCARD(consumeToken());  // Consume the 'as' token
    ast::Type* type = parseType(precedence);
    return arena.emplace<ast::TypeCastExpression>(left, std::move(type));
}
}  // namespace parser
}  // namespace Manganese
