/**
 * @file parser_types.cpp
 * @brief This file contains the implementation of type parsing in the parser. It is split into its own file for
 * readability and maintainability.
 */

#include <frontend/ast.hpp>
#include <frontend/parser.hpp>
#include <global_macros.hpp>
#include <memory>
#include <utility>

namespace Manganese {
namespace parser {

TypeSPtr_t Parser::parseType(Precedence precedence) NOEXCEPT_IF_RELEASE {
    TokenType type = peekTokenType();

    auto nudIterator = nudLookup_types.find(type);
    if (nudIterator == nudLookup_types.end()) {
        ASSERT_UNREACHABLE("No type null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    TypeSPtr_t left = nudIterator->second(this);

    while (!done()) {
        type = peekTokenType();

        auto operatorPrecedenceIterator = operatorPrecedenceMap_type.find(type);
        if (operatorPrecedenceIterator == operatorPrecedenceMap_type.end()
            || operatorPrecedenceIterator->second.leftBindingPower <= precedence) {
            break;
        }

        auto ledIterator = ledLookup_types.find(type);
        if (ledIterator == ledLookup_types.end()) {
            ASSERT_UNREACHABLE("No type left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }
        left = ledIterator->second(this, std::move(left), operatorPrecedenceMap_type[type].rightBindingPower);
    }
    return left;
}

// ===== Specific type parsing methods =====

TypeSPtr_t Parser::parseAggregateType() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());  // Consume the 'aggregate' token
    if (peekTokenType() == TokenType::Identifier) {
        logging::logWarning("Aggregate names are ignored in aggregate type declarations", peekToken().getLine(),
                            peekToken().getColumn());
        DISCARD(consumeToken());  // Skip the identifier token
    }

    expectToken(TokenType::LeftBrace, "Expected a '{' to start aggregate type declaration");
    std::vector<ast::TypeSPtr_t> fieldTypes;

    while (peekTokenType() != TokenType::RightBrace) {
        if (peekTokenType() == TokenType::Identifier) {
            logging::logWarning("Variable names are ignored in aggregate type declarations", peekToken().getLine(),
                                peekToken().getColumn());
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
    return std::make_shared<ast::AggregateType>(std::move(fieldTypes));
}

TypeSPtr_t Parser::parseArrayType(TypeSPtr_t left, Precedence precedence) NOEXCEPT_IF_RELEASE {
    ExpressionUPtr_t lengthExpression = nullptr;
    DISCARD(precedence);  // Avoid unused variable warning
    DISCARD(consumeToken());  // Consume the left square bracket '['
    if (peekTokenType() != TokenType::RightSquare) {
        // If the next token is not a right square bracket, it's a length expression
        lengthExpression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::RightSquare, "Expected ']' to close array type declaration");
    return std::make_shared<ast::ArrayType>(std::move(left), std::move(lengthExpression));
}

TypeSPtr_t Parser::parseFunctionType() NOEXCEPT_IF_RELEASE {
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

    TypeSPtr_t returnType = nullptr;
    if (peekTokenType() == TokenType::Arrow) {
        DISCARD(consumeToken());  // Consume the '->' token
        returnType = parseType(Precedence::Default);
    }

    return std::make_shared<ast::FunctionType>(std::move(parameterTypes), std::move(returnType));
}

TypeSPtr_t Parser::parseGenericType(TypeSPtr_t left, Precedence precedence) NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    DISCARD(precedence);  // Avoid unused variable warning
    expectToken(TokenType::LeftSquare, "Expected a '[' to start generic type parameters");
    std::vector<TypeSPtr_t> typeParameters;
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
    return std::make_shared<ast::GenericType>(std::move(left), std::move(typeParameters));
}

TypeSPtr_t Parser::parseParenthesizedType() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());  // Skip the '('
    TypeSPtr_t innerType = parseType(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to close parenthesized type");
    return innerType;
}

TypeSPtr_t Parser::parsePointerType() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());  // Consume `ptr`
    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        isMutable = true;
        DISCARD(consumeToken());  // Consume `mut`
    }
    return std::make_shared<ast::PointerType>(parseType(Precedence::Default), isMutable);
}

TypeSPtr_t Parser::parseSymbolType() NOEXCEPT_IF_RELEASE {
    using prim = ast::PrimitiveType_t;
    Token token = peekToken();
    if (token.isPrimitiveType()) {
        // If the token is a primitive type, we can directly create a SymbolType
        DISCARD(consumeToken());
        std::string lex = token.getLexeme();
        ast::PrimitiveType_t prim_t = prim::not_primitive;
        if (lex == int8_str) {
            prim_t = prim::i8;
        } else if (lex == int16_str) {
            prim_t = prim::i16;
        } else if (lex == int32_str) {
            prim_t = prim::i32;
        } else if (lex == int64_str) {
            prim_t = prim::i64;
        } else if (lex == uint8_str) {
            prim_t = prim::ui8;
        } else if (lex == uint16_str) {
            prim_t = prim::ui16;
        } else if (lex == uint32_str) {
            prim_t = prim::ui32;
        } else if (lex == uint64_str) {
            prim_t = prim::ui64;
        } else if (lex == float32_str) {
            prim_t = prim::f32;
        } else if (lex == float64_str) {
            prim_t = prim::f64;
        } else if (lex == string_str) {
            prim_t = prim::str;
        } else if (lex == char_str) {
            prim_t = prim::character;
        } else if (lex == bool_str) {
            prim_t = prim::boolean;
        }
        else {
            ASSERT_UNREACHABLE("Unknown primitive type " + lex);
        }
        auto symbol_type = std::make_shared<ast::SymbolType>(token.getLexeme());
        symbol_type->setPrimitiveType(prim_t);
        return symbol_type;
    }
    // If it's not a primitive type, expect an identifier (i.e., a user-defined type)
    return std::make_shared<ast::SymbolType>(expectToken(TokenType::Identifier).getLexeme());
}

}  // namespace parser
}  // namespace Manganese
