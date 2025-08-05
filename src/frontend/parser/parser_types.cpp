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

TypeSPtr_t Parser::parseType(Precedence precedence) noexcept_if_release {
    TokenType type = currentTokenType();

    auto nudIterator = nudLookup_types.find(type);
    if (nudIterator == nudLookup_types.end()) {
        ASSERT_UNREACHABLE("No type null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    TypeSPtr_t left = nudIterator->second(this);

    while (!done()) {
        type = currentTokenType();

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

TypeSPtr_t Parser::parseAggregateType() {
    DISCARD(advance());  // Consume the 'aggregate' token
    if (currentTokenType() == TokenType::Identifier) {
        logging::logWarning("Aggregate names are ignored in aggregate type declarations", currentToken().getLine(),
                            currentToken().getColumn());
        DISCARD(advance());  // Skip the identifier token
    }

    expectToken(TokenType::LeftBrace, "Expected a '{' to start aggregate type declaration");
    std::vector<ast::TypeSPtr_t> fieldTypes;

    while (currentTokenType() != TokenType::RightBrace) {
        if (currentTokenType() == TokenType::Identifier) {
            logging::logWarning("Variable names are ignored in aggregate type declarations", currentToken().getLine(),
                                currentToken().getColumn());
            DISCARD(advance());  // Skip the token
            expectToken(TokenType::Colon, "Expected ':' after field name in aggregate type declaration");
            continue;
        }
        fieldTypes.push_back(parseType(Precedence::Default));  // Parse the type of the field
        if (currentTokenType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma,
                        "Expected ',' to separate fields in aggregate type declaration or '}' to end the declaration");
        }
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end aggregate type declaration");
    return std::make_shared<ast::AggregateType>(std::move(fieldTypes));
}

TypeSPtr_t Parser::parseArrayType(TypeSPtr_t left, Precedence precedence) {
    ExpressionUPtr_t lengthExpression = nullptr;
    DISCARD(precedence);  // Avoid unused variable warning
    DISCARD(advance());  // Consume the left square bracket '['
    if (currentTokenType() != TokenType::RightSquare) {
        // If the next token is not a right square bracket, it's a length expression
        lengthExpression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::RightSquare, "Expected ']' to close array type declaration");
    return std::make_shared<ast::ArrayType>(std::move(left), std::move(lengthExpression));
}

TypeSPtr_t Parser::parseFunctionType() {
    DISCARD(advance());  // consume the 'func' token

    expectToken(TokenType::LeftParen, "Expected '( after 'func' in a function type");
    std::vector<ast::FunctionParameterType> parameterTypes;
    while (!done()) {
        if (currentTokenType() == TokenType::RightParen) {
            break;  // Done with parameter types
        }
        bool isConst = false;
        if (currentTokenType() == TokenType::Const) {
            isConst = true;
            DISCARD(advance());
        }
        parameterTypes.emplace_back(isConst, parseType(Precedence::Default));

        if (currentTokenType() != TokenType::RightParen) {
            expectToken(TokenType::Comma, "Expected ',' to separate parameter types or ')' to end parameter list");
        }
    }
    expectToken(TokenType::RightParen, "Expected ')' to end parameter type list");

    TypeSPtr_t returnType = nullptr;
    if (currentTokenType() == TokenType::Arrow) {
        DISCARD(advance());  // Consume the '->' token
        returnType = parseType(Precedence::Default);
    }

    return std::make_shared<ast::FunctionType>(std::move(parameterTypes), std::move(returnType));
}

TypeSPtr_t Parser::parseGenericType(TypeSPtr_t left, Precedence precedence) {
    DISCARD(advance());
    DISCARD(precedence);  // Avoid unused variable warning
    expectToken(TokenType::LeftSquare, "Expected a '[' to start generic type parameters");
    std::vector<TypeSPtr_t> typeParameters;
    while (!done()) {
        if (currentTokenType() == TokenType::RightSquare) {
            break;  // Done with type parameters
        }
        auto nextPrecedence = static_cast<std::underlying_type<Precedence>::type>(Precedence::Assignment) + 1;
        typeParameters.push_back(parseType(static_cast<Precedence>(nextPrecedence)));
        if (currentTokenType() != TokenType::RightSquare) {
            expectToken(TokenType::Comma, "Expected ',' to separate generic types");
        }
    }
    expectToken(TokenType::RightSquare, "Expected ']' to end generic type parameters");
    return std::make_shared<ast::GenericType>(std::move(left), std::move(typeParameters));
}

TypeSPtr_t Parser::parsePointerType() {
    DISCARD(advance());  // Consume `ptr`
    return std::make_shared<ast::PointerType>(parseType(Precedence::Default));
}

TypeSPtr_t Parser::parseSymbolType() {
    Token token = currentToken();
    if (token.isPrimitiveType()) {
        // If the token is a primitive type, we can directly create a SymbolType
        DISCARD(advance());
        return std::make_shared<ast::SymbolType>(token.getLexeme());
    }
    // If it's not a primitive type, expect an identifier (i.e., a user-defined type)
    return std::make_shared<ast::SymbolType>(expectToken(TokenType::Identifier).getLexeme());
}

}  // namespace parser
}  // namespace Manganese
