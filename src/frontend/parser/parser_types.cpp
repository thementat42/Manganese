/**
 * @file parser_types.cpp
 * @brief This file contains the implementation of type parsing in the parser. It is split into its own file for
 * readability and maintainability.
 */

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <core.hpp>
#include <utility>
#include <vector>

namespace Manganese {
namespace parser {

ast::Type* Parser::parseType(Precedence precedence) {
    TokenType type = peekTokenType();
    const auto index = tokenToIndex(type);

    auto nudHandler = nudLookup_types[index];
    if (!nudHandler) {
        ASSERT_UNREACHABLE("No type null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    // ast::Type* left = nudIterator->second(this);
    ast::Type* left = (this->*nudHandler)();

    while (!done()) {
        type = peekTokenType();
        const auto idx = tokenToIndex(type);

        const Operator& op = operatorPrecedenceMap_type[idx];
        if (op.leftBindingPower <= precedence) { break; }

        auto handler = ledLookup_types[idx];
        if (!handler) {
            ASSERT_UNREACHABLE("No type left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }
        // left = ledIterator->second(this, std::move(left), operatorPrecedenceMap_type[type].rightBindingPower);
        auto rbp = op.rightBindingPower;
        left = (this->*handler)(std::move(left), rbp);
    }
    return left;
}

// Specific type parsing methods

ast::Type* Parser::parseAggregateType() {
    DISCARD(consumeToken());  // Consume the 'aggregate' token
    if (peekTokenType() == TokenType::Identifier) {
        logging::logWarning(peekToken().getLine(), peekToken().getColumn(),
                            "Aggregate names are ignored in aggregate type declarations");
        DISCARD(consumeToken());  // Skip the identifier token
    }

    expectToken(TokenType::LeftBrace, "Expected a '{' to start aggregate type declaration");
    std::vector<ast::Type*> fieldTypes;

    while (peekTokenType() != TokenType::RightBrace) {
        if (peekTokenType() == TokenType::Identifier) {
            logging::logWarning(peekToken().getLine(), peekToken().getColumn(),
                                "Variable names are ignored in aggregate type declarations");
            DISCARD(consumeToken());  // Skip the token
            expectToken(TokenType::Colon, "Expected ':' after field name in aggregate type declaration");
            continue;
        }
        fieldTypes.push_back(parseType(Precedence::Default));  // Parse the type of the field
        if (peekTokenType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma,
                        "Expected ',' to separate fields in aggregate type declaration or '}' to end the declaration");
        }
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end aggregate type declaration");
    return arena.emplace<ast::AggregateType>(std::move(fieldTypes));
}

ast::Type* Parser::parseArrayType(ast::Type* left, Precedence precedence) {
    ast::Expression* lengthExpression = nullptr;
    DISCARD(precedence);  // Avoid unused variable warning
    DISCARD(consumeToken());  // Consume the left square bracket '['
    if (peekTokenType() != TokenType::RightSquare) {
        // If the next token is not a right square bracket, it's a length expression
        lengthExpression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::RightSquare, "Expected ']' to close array type declaration");
    return arena.emplace<ast::ArrayType>(std::move(left), std::move(lengthExpression));
}

ast::Type* Parser::parseFunctionType() {
    DISCARD(consumeToken());  // consume the 'func' token

    expectToken(TokenType::LeftParen, "Expected '( after 'func' in a function type");
    std::vector<ast::FunctionParameterType> parameterTypes;
    while (!done()) {
        if (peekTokenType() == TokenType::RightParen) {
            break;  // Done with parameter types
        }
        bool isMutable = false;
        if (peekTokenType() == TokenType::Mut) {
            isMutable = true;
            DISCARD(consumeToken());
        }
        parameterTypes.emplace_back(isMutable, parseType(Precedence::Default));

        if (peekTokenType() != TokenType::RightParen) {
            expectToken(TokenType::Comma, "Expected ',' to separate parameter types or ')' to end parameter list");
        }
    }
    expectToken(TokenType::RightParen, "Expected ')' to end parameter type list");

    ast::Type* returnType = nullptr;
    if (peekTokenType() == TokenType::Arrow) {
        DISCARD(consumeToken());  // Consume the '->' token
        returnType = parseType(Precedence::Default);
    }

    return arena.emplace<ast::FunctionType>(std::move(parameterTypes), std::move(returnType));
}

ast::Type* Parser::parseGenericType(ast::Type* left, Precedence precedence) {
    DISCARD(consumeToken());
    DISCARD(precedence);  // Avoid unused variable warning
    expectToken(TokenType::LeftSquare, "Expected a '[' to start generic type parameters");
    std::vector<ast::Type*> typeParameters;
    while (!done()) {
        if (peekTokenType() == TokenType::RightSquare) {
            break;  // Done with type parameters
        }
        auto nextPrecedence = static_cast<std::underlying_type_t<Precedence>>(Precedence::Assignment) + 1;
        typeParameters.push_back(parseType(static_cast<Precedence>(nextPrecedence)));
        if (peekTokenType() != TokenType::RightSquare) {
            expectToken(TokenType::Comma, "Expected ',' to separate generic types");
        }
    }
    expectToken(TokenType::RightSquare, "Expected ']' to end generic type parameters");
    return arena.emplace<ast::GenericType>(std::move(left), std::move(typeParameters));
}

ast::Type* Parser::parseParenthesizedType() {
    DISCARD(consumeToken());  // Skip the '('
    ast::Type* innerType = parseType(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to close parenthesized type");
    return innerType;
}

ast::Type* Parser::parsePointerType() {
    DISCARD(consumeToken());  // Consume `ptr`
    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        isMutable = true;
        DISCARD(consumeToken());  // Consume `mut`
    }
    return arena.emplace<ast::PointerType>(parseType(Precedence::Default), isMutable);
}

ast::Type* Parser::parseSymbolType() {
    using enum ast::PrimitiveType_t;
    Token token = peekToken();
    if (!token.isPrimitiveType()) {
        // If it's not a primitive type, expect an identifier (i.e., a user-defined type)
        return arena.emplace<ast::SymbolType>(expectToken(TokenType::Identifier).getLexeme());
    }
    // If the token is a primitive type, we can directly create a SymbolType
    DISCARD(consumeToken());
    std::string lex = token.getLexeme();
    ast::PrimitiveType_t prim_t = not_primitive;
    if (lex == int8_str) {
        prim_t = i8;
    } else if (lex == int16_str) {
        prim_t = i16;
    } else if (lex == int32_str) {
        prim_t = i32;
    } else if (lex == int64_str) {
        prim_t = i64;
    } else if (lex == int128_str) {
        prim_t = i128;
    } else if (lex == uint8_str) {
        prim_t = u8;
    } else if (lex == uint16_str) {
        prim_t = u16;
    } else if (lex == uint32_str) {
        prim_t = u32;
    } else if (lex == uint64_str) {
        prim_t = u64;
    } else if (lex == uint128_str) {
        prim_t = u128;
    } else if (lex == float32_str) {
        prim_t = f32;
    } else if (lex == float64_str) {
        prim_t = f64;
    } else if (lex == string_str) {
        prim_t = str;
    } else if (lex == char_str) {
        prim_t = character;
    } else if (lex == bool_str) {
        prim_t = boolean;
    } else {
        ASSERT_UNREACHABLE("Unknown primitive type " + lex);
    }
    auto symbol_type = arena.emplace<ast::SymbolType>(token.getLexeme());
    symbol_type->primitiveType = prim_t;
    return symbol_type;
}

}  // namespace parser
}  // namespace Manganese
