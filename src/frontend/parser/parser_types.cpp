#include <frontend/ast.h>
#include <frontend/parser.h>
#include <global_macros.h>

#include <memory>
#include <utility>

namespace Manganese {
namespace parser {

TypePtr Parser::parseType(Precedence precedence) noexcept_except_catastrophic {
    TokenType type = currentToken().getType();

    auto nudIterator = type_nullDenotationLookup.find(type);
    if (nudIterator == type_nullDenotationLookup.end()) {
        ASSERT_UNREACHABLE("No type null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    TypePtr left = nudIterator->second(this);

    while (!done()) {
        type = currentToken().getType();

        auto operatorPrecedenceIterator = type_operatorPrecedenceMap.find(type);
        if (operatorPrecedenceIterator == type_operatorPrecedenceMap.end() ||
            operatorPrecedenceIterator->second.leftBindingPower <= precedence) {
            break;
        }

        auto ledIterator = type_leftDenotationLookup.find(type);
        if (ledIterator == type_leftDenotationLookup.end()) {
            ASSERT_UNREACHABLE("No type left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }
        left = ledIterator->second(this, std::move(left), type_operatorPrecedenceMap[type].rightBindingPower);
    }
    return left;
}

// ===== Specific type parsing methods =====

TypePtr Parser::parseArrayType(TypePtr left, Precedence rightBindingPower) {
    ExpressionPtr lengthExpression = nullptr;
    DISCARD(rightBindingPower);  // Avoid unused variable warning
    DISCARD(advance());          // Consume the left square bracket '['
    if (currentToken().getType() != TokenType::RightSquare) {
        // If the next token is not a right square bracket, it's a length expression
        lengthExpression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::RightSquare, "Expected ']' to close array type declaration");
    return std::make_unique<ast::ArrayType>(std::move(left), std::move(lengthExpression));
}

TypePtr Parser::parseGenericType(TypePtr left, Precedence rightBindingPower) {
    DISCARD(advance());
    expectToken(TokenType::LeftSquare, "Expected a '[' to start generic type parameters");
    std::vector<TypePtr> typeParameters;
    while (!done()) {
        if (currentToken().getType() == TokenType::RightSquare) {
            break;  // Done with type parameters
        }
        auto nextBindingPower = static_cast<std::underlying_type<Precedence>::type>(Precedence::Assignment) + 1;
        typeParameters.push_back(parseType(static_cast<Precedence>(nextBindingPower)));
        if (currentToken().getType() != TokenType::RightSquare) {
            expectToken(TokenType::Comma, "Expected ',' to separate generic types");
        }
    }
    expectToken(TokenType::RightSquare, "Expected ']' to end generic type parameters");
    DISCARD(rightBindingPower);  // Avoid unused variable warning
    return std::make_unique<ast::GenericType>(std::move(left), std::move(typeParameters));
}

TypePtr Parser::parseSymbolType() {
    Token token = currentToken();
    if (token.isPrimitiveType()) {
        // If the token is a primitive type, we can directly create a SymbolType
        DISCARD(advance());
        return std::make_unique<ast::SymbolType>(token.getLexeme());
    }
    // TODO: Allow `func` for lambda types (e.g., `func(int, int) -> int`)
    // If it's not a primitive type, expect an identifier (i.e., a user-defined type)
    return std::make_unique<ast::SymbolType>(expectToken(TokenType::Identifier).getLexeme());
}

// ===== Lookup Initialization =====

void Parser::type_led(TokenType type, Precedence precedence,
                      type_leftDenotationHandler_t handler) {
    type_operatorPrecedenceMap[type] = Operator::binary(precedence);
    type_leftDenotationLookup[type] = handler;
}

void Parser::type_nud(TokenType type, type_nullDenotationHandler_t handler) {
    type_operatorPrecedenceMap[type] = Operator{
        .leftBindingPower = Precedence::Primary,
        .rightBindingPower = Precedence::Default};
    type_nullDenotationLookup[type] = handler;
}

void Parser::type_postfix(TokenType type, type_leftDenotationHandler_t handler) {
    type_operatorPrecedenceMap[type] = Operator::postfix(Precedence::Primary);
    type_leftDenotationLookup[type] = handler;
}

void Parser::initializeTypeLookups() {
    //~ Variable declarations with primitive types
    type_nud(TokenType::Identifier, &Parser::parseSymbolType);
    type_nud(TokenType::Int8, &Parser::parseSymbolType);
    type_nud(TokenType::UInt8, &Parser::parseSymbolType);
    type_nud(TokenType::Int16, &Parser::parseSymbolType);
    type_nud(TokenType::UInt16, &Parser::parseSymbolType);
    type_nud(TokenType::Int32, &Parser::parseSymbolType);
    type_nud(TokenType::UInt32, &Parser::parseSymbolType);
    type_nud(TokenType::Int64, &Parser::parseSymbolType);
    type_nud(TokenType::UInt64, &Parser::parseSymbolType);
    type_nud(TokenType::Float32, &Parser::parseSymbolType);
    type_nud(TokenType::Float64, &Parser::parseSymbolType);
    type_nud(TokenType::Char, &Parser::parseSymbolType);
    type_nud(TokenType::Bool, &Parser::parseSymbolType);

    //~ Complex types
    type_led(TokenType::LeftSquare, Precedence::Postfix, &Parser::parseArrayType);
    type_led(TokenType::At, Precedence::Generic, &Parser::parseGenericType);
}

}  // namespace parser
}  // namespace Manganese
