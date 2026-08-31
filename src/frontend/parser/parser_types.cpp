#include <core.hpp>
#include <cstddef>
#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <mnstl/number.hxx>
#include <string>
#include <utility>
#include <utils/type_names.hpp>

namespace Manganese::parser {

ast::Type* Parser::parseType(Precedence precedence) {
    TokenType type = peekTokenType();
    const std::size_t index = tokenToIndex(type);

    const nudHandler_types_t nudHandler = lookupTable.nudLookup_types[index];
    if (!nudHandler) {
        ASSERT_UNREACHABLE("No type null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    ast::Type* left = (this->*nudHandler)();

    while (!done()) {
        type = peekTokenType();
        const std::size_t idx = tokenToIndex(type);

        const Operator& op = lookupTable.operatorPrecedenceMap_type[idx];
        if (op.leftBindingPower <= precedence) { break; }

        const ledHandler_types_t handler = lookupTable.ledLookup_types[idx];
        if (!handler) {
            ASSERT_UNREACHABLE("No type left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }
        left = (this->*handler)(left, op.rightBindingPower);
    }
    return left;
}

// Specific type parsing methods

ast::Type* Parser::parseAggregateType() {
    const Token startToken = consumeToken(); // Consume 'aggregate'
    if (peekTokenType() == TokenType::Identifier) {
        logging::logWarning(peekToken().getLine(), peekToken().getColumn(),
                            "Aggregate names are ignored in aggregate type declarations");
        DISCARD(consumeToken());
    }

    expectToken(TokenType::LeftBrace, "Expected a '{' to start aggregate type declaration");

    auto fieldTypes = parseCommaSeparatedList<ast::Type*>(
        TokenType::RightBrace,
        "Expected ',' to separate fields in aggregate type declaration or '}' to end the declaration",
        [this]() { return parseAggregateTypeField(); }
    );

    return makeNode<ast::AggregateType>(startToken, std::move(fieldTypes));
}

ast::Type* Parser::parseArrayType(ast::Type* left, Precedence) {
    ast::Expression* lengthExpression = nullptr;
    const Token startToken = consumeToken();  // Consume the left square bracket '['
    if (peekTokenType() != TokenType::RightSquare) {
        // If the next token is not a right square bracket, it's a length expression
        lengthExpression = parseExpression(Precedence::Default);
    }
    if (flags.parsingAliasStatement && lengthExpression == nullptr) {
        logError(left->line, left->column, "Arrays in alias statements must have an explicit length expression");
    }
    expectToken(TokenType::RightSquare, "Expected ']' to close array type declaration");
    return makeNode<ast::ArrayType>(startToken, left, lengthExpression);
}

ast::Type* Parser::parseFunctionType() {
    const Token startToken = consumeToken(); // Consume 'func'
    expectToken(TokenType::LeftParen, "Expected '(' after 'func' in a function type");

    bool seenVariadic = false;
    std::vector<ast::FunctionParameterType> parameterTypes;

    while (!done() && peekTokenType() != TokenType::RightParen) {
        parameterTypes.push_back(parseFunctionTypeParameter(seenVariadic));

        if (peekTokenType() != TokenType::RightParen) {
            expectToken(TokenType::Comma, "Expected ',' to separate parameter types or ')' to end parameter list");
        }
    }

    expectToken(TokenType::RightParen, "Expected ')' after function parameter type list");

    ast::Type* returnType = nullptr;
    if (peekTokenType() == TokenType::Arrow) {
        DISCARD(consumeToken());
        returnType = parseType(Precedence::Default);
    }

    return makeNode<ast::FunctionType>(startToken, std::move(parameterTypes), returnType);
}

ast::Type* Parser::parseGenericInstantiationType(ast::Type* left, Precedence) {
    const Token startToken = consumeToken();
    expectToken(TokenType::LeftSquare, "Expected a '[' to start generic type parameters");
    auto typeParameters
        = parseCommaSeparatedList<ast::Type*>(TokenType::RightSquare, "Expected ',' to separate generic types",
                                              [this]() { return parseType(precedenceAbove(Precedence::Assignment)); });
    return makeNode<ast::GenericInstantiationType>(startToken, left, std::move(typeParameters));
}

ast::Type* Parser::parseParenthesizedType() {
    const Token startToken = consumeToken();  // Skip the '('
    ast::Type* innerType = parseType(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to close parenthesized type");
    return innerType;
}

ast::Type* Parser::parsePointerType() {
    const Token startToken = consumeToken();  // Consume `ptr`
    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        isMutable = true;
        DISCARD(consumeToken());  // Consume `mut`
    }
    return makeNode<ast::PointerType>(startToken, parseType(Precedence::Default), isMutable);
}

ast::Type* Parser::parseScopedType(ast::Type* left, Precedence precedence) {
    const Token startToken = consumeToken();  // consume the `::`
    ast::Type* type = parseType(precedence);
    return makeNode<ast::ScopedType>(startToken, left, type);
}

ast::Type* Parser::parseSymbolType() {
    using enum ast::PrimitiveType_t;
    const Token startToken = peekToken();
    if (!startToken.isPrimitiveType()) {
        return makeNode<ast::SymbolType>(startToken, expectToken(TokenType::Identifier).getLexeme());
    }
    // If the token is a primitive type, we can directly create a SymbolType
    DISCARD(consumeToken());
    const std::string lexeme = startToken.getLexeme();
    ast::PrimitiveType_t prim_t = not_primitive;
    if (lexeme == int8_str) {
        prim_t = i8;
    } else if (lexeme == int16_str) {
        prim_t = i16;
    } else if (lexeme == int32_str) {
        prim_t = i32;
    } else if (lexeme == int64_str) {
        prim_t = i64;
    } else if (lexeme == int128_str) {
        prim_t = i128;
    } else if (lexeme == uint8_str) {
        prim_t = u8;
    } else if (lexeme == uint16_str) {
        prim_t = u16;
    } else if (lexeme == uint32_str) {
        prim_t = u32;
    } else if (lexeme == uint64_str) {
        prim_t = u64;
    } else if (lexeme == uint128_str) {
        prim_t = u128;
    } else if (lexeme == float32_str) {
        prim_t = f32;
    } else if (lexeme == float64_str) {
        prim_t = f64;
    } else if (lexeme == string_str) {
        prim_t = str;
    } else if (lexeme == char_str) {
        prim_t = character;
    } else if (lexeme == bool_str) {
        prim_t = boolean;
    } else {
        ASSERT_UNREACHABLE("Unknown primitive type " + lexeme);
    }
    return makeNode<ast::SymbolType>(startToken, startToken.getLexeme(), prim_t);
}

ast::Type* Parser::parseTypeofType() {
    const Token startToken = consumeToken();  // skip typeof
    expectToken(lexer::TokenType::LeftParen, "Expected '(' after typeof");
    ast::Expression* innerExpression = parseExpression(Precedence::Default);
    if (!innerExpression) {
        logError(peekToken().getLine(), peekToken().getColumn(), "Expected a valid expression inside 'typeof(...)'.");
        // Error recovery: give it a safe dummy fallback expression
        innerExpression = makeNode<ast::NumberLiteralExpression>(startToken, mnstl::number_t{std::int32_t{0}});
    }
    expectToken(lexer::TokenType::RightParen, "Expected ')' to close typeof");
    return makeNode<ast::TypeofType>(startToken, innerExpression);
}

// Helpers

ast::Type* Parser::parseAggregateTypeField() {
    if (peekTokenType() == TokenType::Identifier) {
        logging::logWarning(peekToken().getLine(), peekToken().getColumn(),
                            "Variable names are ignored in aggregate type declarations");
        DISCARD(consumeToken());
        expectToken(TokenType::Colon, "Expected ':' after field name in aggregate type declaration");
    }
    return parseType(Precedence::Default);
}

ast::FunctionParameterType Parser::parseFunctionTypeParameter(bool& seenVariadic) {
    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        isMutable = true;
        DISCARD(consumeToken());
    }

    ast::Type* parameterType = parseType(Precedence::Default);

    bool isVariadic = false;
    if (peekTokenType() == TokenType::Ellipsis) {
        DISCARD(consumeToken());
        isVariadic = true;

        if (seenVariadic) {
            logError(peekToken().getLine(), peekToken().getColumn(),
                     "A function type cannot have more than one variadic parameter");
        }
        seenVariadic = true;
    }

    if (isVariadic && peekTokenType() != TokenType::RightParen && peekTokenType() != TokenType::Comma) {
        logError(peekToken().getLine(), peekToken().getColumn(), "A variadic parameter must be the last parameter");
    }

    return ast::FunctionParameterType{.isMutable = isMutable, .isVariadic = isVariadic, .type = parameterType};
}

std::string Parser::parseGenericTypeParameter(std::vector<std::string>& existingGenerics, std::string_view contextName) {
    Token genericToken = expectToken(TokenType::Identifier, "Expected a generic type name");
    std::string genericName = genericToken.getLexeme();

    if (std::ranges::find(existingGenerics, genericName) != existingGenerics.end()) {
        logError(genericToken.getLine(), genericToken.getColumn(),
                 "Duplicate generic type '{}' in '{}'", genericName, contextName);
        return "";
    }
    return genericName;
}

}  // namespace Manganese::parser
