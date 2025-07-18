#include <frontend/ast.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include <memory>
#include <utility>

namespace Manganese {
namespace parser {

TypePtr_t Parser::parseType(Precedence precedence) noexcept_if_release {
    TokenType type = currentToken().getType();

    auto nudIterator = nudLookup_types.find(type);
    if (nudIterator == nudLookup_types.end()) {
        ASSERT_UNREACHABLE("No type null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    TypePtr_t left = nudIterator->second(this);

    while (!done()) {
        type = currentToken().getType();

        auto operatorPrecedenceIterator = operatorPrecedenceMap_type.find(type);
        if (operatorPrecedenceIterator == operatorPrecedenceMap_type.end() ||
            operatorPrecedenceIterator->second.leftBindingPower <= precedence) {
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

TypePtr_t Parser::parseArrayType(TypePtr_t left, Precedence precedence) {
    ExpressionPtr_t lengthExpression = nullptr;
    DISCARD(precedence);  // Avoid unused variable warning
    DISCARD(advance());   // Consume the left square bracket '['
    if (currentToken().getType() != TokenType::RightSquare) {
        // If the next token is not a right square bracket, it's a length expression
        lengthExpression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::RightSquare, "Expected ']' to close array type declaration");
    return std::make_unique<ast::ArrayType>(std::move(left), std::move(lengthExpression));
}

TypePtr_t Parser::parseFunctionType() {
    DISCARD(advance());  // consume the 'func' token

    expectToken(TokenType::LeftParen, "Expected '( after 'func' in a function type");
    std::vector<ast::FunctionParameterType> parameterTypes;
    while (!done()) {
        if (currentToken().getType() == TokenType::RightParen) {
            break;  // Done with parameter types
        }
        bool isConst = false;
        if (currentToken().getType() == TokenType::Const) {
            isConst = true;
            DISCARD(advance());
        }
        parameterTypes.emplace_back(isConst, parseType(Precedence::Default));

        if (currentToken().getType() != TokenType::RightParen) {
            expectToken(TokenType::Comma, "Expected ',' to separate parameter types or ')' to end parameter list");
        }
    }
    expectToken(TokenType::RightParen, "Expected ')' to end parameter type list");

    TypePtr_t returnType = nullptr;
    if (currentToken().getType() == TokenType::Arrow) {
        DISCARD(advance());  // Consume the '->' token
        returnType = parseType(Precedence::Default);
    }

    return std::make_unique<ast::FunctionType>(std::move(parameterTypes), std::move(returnType));
}

TypePtr_t Parser::parseGenericType(TypePtr_t left, Precedence precedence) {
    DISCARD(advance());
    DISCARD(precedence);  // Avoid unused variable warning
    expectToken(TokenType::LeftSquare, "Expected a '[' to start generic type parameters");
    std::vector<TypePtr_t> typeParameters;
    while (!done()) {
        if (currentToken().getType() == TokenType::RightSquare) {
            break;  // Done with type parameters
        }
        auto nextPrecedence = static_cast<std::underlying_type<Precedence>::type>(Precedence::Assignment) + 1;
        typeParameters.push_back(parseType(static_cast<Precedence>(nextPrecedence)));
        if (currentToken().getType() != TokenType::RightSquare) {
            expectToken(TokenType::Comma, "Expected ',' to separate generic types");
        }
    }
    expectToken(TokenType::RightSquare, "Expected ']' to end generic type parameters");
    return std::make_unique<ast::GenericType>(std::move(left), std::move(typeParameters));
}

TypePtr_t Parser::parsePointerType() {
    DISCARD(advance());  // Consume `ptr`
    return std::make_unique<ast::PointerType>(parseType(Precedence::Default));
}

TypePtr_t Parser::parseSymbolType() {
    Token token = currentToken();
    if (token.isPrimitiveType()) {
        // If the token is a primitive type, we can directly create a SymbolType
        DISCARD(advance());
        return std::make_unique<ast::SymbolType>(token.getLexeme());
    }
    // If it's not a primitive type, expect an identifier (i.e., a user-defined type)
    return std::make_unique<ast::SymbolType>(expectToken(TokenType::Identifier).getLexeme());
}

// ===== Lookup Initialization =====

void Parser::registerLedHandler_type(TokenType type, Precedence precedence,
                                     ledHandler_types_t handler) {
    operatorPrecedenceMap_type[type] = Operator::binary(precedence);
    ledLookup_types[type] = handler;
}

void Parser::registerNudHandler_type(TokenType type, nudHandler_types_t handler) {
    operatorPrecedenceMap_type[type] = Operator{
        .leftBindingPower = Precedence::Primary,
        .rightBindingPower = Precedence::Default};
    nudLookup_types[type] = handler;
}

void Parser::initializeTypeLookups() {
    //~ Variable declarations with primitive types
    registerNudHandler_type(TokenType::Identifier, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Int8, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::UInt8, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Int16, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::UInt16, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Int32, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::UInt32, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Int64, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::UInt64, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Float32, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Float64, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Char, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Bool, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::String, &Parser::parseSymbolType);
    registerNudHandler_type(TokenType::Ptr, &Parser::parsePointerType);

    //~ Complex types
    registerLedHandler_type(TokenType::LeftSquare, Precedence::Postfix, &Parser::parseArrayType);
    registerLedHandler_type(TokenType::At, Precedence::Generic, &Parser::parseGenericType);
    registerNudHandler_type(TokenType::Func, &Parser::parseFunctionType);
}

}  // namespace parser
}  // namespace Manganese
