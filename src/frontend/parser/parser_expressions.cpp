/**
 * @file parser_expressions.cpp
 * @brief This file contains the implementation of expression parsing in the parser. It is split into its own file for
 * readability and maintainability.
 */

#include <frontend/ast.h>
#include <frontend/lexer.h>
#include <frontend/parser.h>
#include <global_macros.h>
#include <utils/number_utils.h>

#include <format>
#include <memory>
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

ExpressionUPtr_t Parser::parseExpression(Precedence precedence) noexcept_if_release {
    Token token = currentToken();

    // Handle operators which have a unary and a binary version
    // (e.g. `-` can be a unary negation or a binary subtraction)
    if (isUnaryContext() && token.hasUnaryCounterpart()) {
        token.overrideType(token.getUnaryCounterpart());
        precedence = Precedence::Unary;
    }
    TokenType type = token.getType();

    auto nudIterator = nudLookup.find(type);
    if (nudIterator == nudLookup.end()) {
        // TODO: This should be an error handled gracefully, not a throw
        ASSERT_UNREACHABLE("No null denotation handler for token type: " + lexer::tokenTypeToString(type));
    }
    ExpressionUPtr_t left = nudIterator->second(this);

    // ! For some reason this works
    // ! Do not delete this
    if (type == TokenType::AddressOf || type == TokenType::Dereference) { precedence = Precedence::Default; }

    while (!done()) {
        token = currentToken();

        if (isUnaryContext() && token.hasUnaryCounterpart()) {
            token.overrideType(token.getUnaryCounterpart());
            precedence = Precedence::Unary;
        }
        type = token.getType();

        auto precedenceIterator = operatorPrecedenceMap.find(type);
        if (precedenceIterator == operatorPrecedenceMap.end()
            || precedenceIterator->second.leftBindingPower <= precedence) {
            break;
        }

        auto ledIterator = ledLookup.find(type);
        if (ledIterator == ledLookup.end()) {
            // TODO: This should be an error handled gracefully, not a throw
            ASSERT_UNREACHABLE("No left denotation handler for token type: " + lexer::tokenTypeToString(type));
        }

        if (type == TokenType::LeftBrace && left->kind() != ast::ExpressionKind::IdentifierExpression
            && left->kind() != ast::ExpressionKind::GenericExpression) [[unlikely]] {
            if (isParsingBlockPrecursor) {
                // Left braces after an expression can either start a block or a bundle instantiation
                // If we're parsing a block precursor (if/for/while, etc.) AND the previous expression is not an
                // identifier, we assume it's the start of a block and break If the previous expression IS an
                // identifier, it might be an inline bundle instantiation
                return left;
            }
            logError("Left brace after an expression must be preceded by an identifier (bundle instantiation)"
                     " or a block precursor (if/for/while, etc.)",
                     token.getLine(), token.getColumn());
        }
        left = ledIterator->second(this, std::move(left), precedenceIterator->second.rightBindingPower);
    }
    return left;
}

// === Specific expression parsing methods ===

ExpressionUPtr_t Parser::parseArrayInstantiationExpression() {
    DISCARD(advance());  // Consume the left square bracket
    std::vector<ExpressionUPtr_t> elements;
    while (!done()) {
        if (currentToken().getType() == lexer::TokenType::RightSquare) {
            break;  // Done instantiation
        }
        constexpr auto precedence = static_cast<std::underlying_type<Precedence>::type>(Precedence::Assignment) + 1;
        auto element = parseExpression(static_cast<Precedence>(precedence));
        elements.push_back(std::move(element));
        if (currentToken().getType() != lexer::TokenType::RightSquare) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate array elements");
        }
    }
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end array instantiation");

    return make_unique<ast::ArrayLiteralExpression>(std::move(elements));
}

ExpressionUPtr_t Parser::parseAssignmentExpression(ExpressionUPtr_t left, Precedence precedence) {
    TokenType op = advance().getType();
    ExpressionUPtr_t right = parseExpression(precedence);

    return std::make_unique<ast::AssignmentExpression>(std::move(left), op, std::move(right));
}

ExpressionUPtr_t Parser::parseBinaryExpression(ExpressionUPtr_t left, Precedence precedence) {
    auto op = advance().getType();
    auto right = parseExpression(precedence);

    return std::make_unique<ast::BinaryExpression>(std::move(left), op, std::move(right));
}

ExpressionUPtr_t Parser::parseBundleInstantiationExpression(ExpressionUPtr_t left, Precedence precedence) {
    DISCARD(precedence);  // Avoid unused variable warning
    std::string bundleName;
    std::vector<TypeSPtr_t> genericTypes;
    // Parse out a bundle instantiation even if there's an error
    expectToken(lexer::TokenType::LeftBrace, "Expected '{' to start bundle instantiation");
    std::vector<ast::BundleInstantiationField> fields;

    if (left->kind() == ast::ExpressionKind::GenericExpression) {
        auto* genericExpr = static_cast<ast::GenericExpression*>(left.get());
        if (genericExpr->identifier->kind() != ast::ExpressionKind::IdentifierExpression) {
            logError("Generic bundle instantiation must start with a bundle name", left->getLine(), left->getColumn());
        } else {
            auto* identifierExpr = static_cast<ast::IdentifierExpression*>(genericExpr->identifier.get());
            bundleName = identifierExpr->value;
            genericTypes = genericExpr->moveTypeParameters();
        }
    } else if (left->kind() == ast::ExpressionKind::IdentifierExpression) {
        auto* underlying = static_cast<ast::IdentifierExpression*>(left.get());
        bundleName = underlying->value;
    } else {
        logError(std::format("Bundle instantiation expression must start with a bundle name, not {}", left->toString()),
                 left->getLine(), left->getColumn());
    }

    while (!done()) {
        if (currentToken().getType() == lexer::TokenType::RightBrace) {
            break;  // Done instantiation
        }
        auto propertyName
            = expectToken(lexer::TokenType::Identifier, "Expected field name in bundle instantiation").getLexeme();
        expectToken(lexer::TokenType::Assignment, "Expected '=' to assign value to bundle field");
        // want precedence to be 1 higher than assignment (e.g. field = x = 10 is invalid)
        constexpr auto precedence_ = static_cast<std::underlying_type<Precedence>::type>(Precedence::Assignment) + 1;
        auto value = parseExpression(static_cast<Precedence>(precedence_));

        auto duplicate = std::find_if(
            fields.begin(), fields.end(),
            [propertyName](const ast::BundleInstantiationField& field) { return field.name == propertyName; });
        if (duplicate != fields.end()) {
            logError(std::format("Duplicate field '{}' in bundle instantiation of '{}'", propertyName, bundleName),
                     value->getLine(), value->getColumn());
        } else {
            fields.emplace_back(propertyName, std::move(value));
        }
        if (currentToken().getType() != lexer::TokenType::RightBrace) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate bundle fields");
        }
    }
    expectToken(lexer::TokenType::RightBrace, "Expected '}' to end bundle instantiation");
    return std::make_unique<ast::BundleInstantiationExpression>(bundleName, genericTypes, std::move(fields));
}

ExpressionUPtr_t Parser::parseFunctionCallExpression(ExpressionUPtr_t left, Precedence precedence) {
    DISCARD(advance());
    DISCARD(precedence);  // Avoid unused variable warning
    std::vector<ExpressionUPtr_t> arguments;

    while (!done()) {
        if (currentToken().getType() == lexer::TokenType::RightParen) {
            break;  // Done with arguments
        }
        arguments.push_back(parseExpression(Precedence::Assignment));
        if (currentToken().getType() != lexer::TokenType::RightParen
            && currentToken().getType() != lexer::TokenType::EndOfFile) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate function arguments");
        }
    }
    expectToken(lexer::TokenType::RightParen, "Expected ')' to end function call");
    return std::make_unique<ast::FunctionCallExpression>(std::move(left), std::move(arguments));
}

ExpressionUPtr_t Parser::parseGenericExpression(ExpressionUPtr_t left, Precedence precedence) {
    DISCARD(advance());  // Consume the '@' token
    DISCARD(precedence);  // Avoid unused variable warning
    expectToken(lexer::TokenType::LeftSquare, "Expected '[' to start generic type parameters");
    std::vector<TypeSPtr_t> typeParameters;
    while (!done()) {
        if (currentToken().getType() == lexer::TokenType::RightSquare) {
            break;  // Done with type parameters
        }
        typeParameters.push_back(parseType(Precedence::Default));
        if (currentToken().getType() != lexer::TokenType::RightSquare) {
            expectToken(lexer::TokenType::Comma, "Expected ',' to separate generic types");
        }
    }
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end generic type parameters");
    return std::make_unique<ast::GenericExpression>(std::move(left), typeParameters);
}

ExpressionUPtr_t Parser::parseIndexingExpression(ExpressionUPtr_t left, Precedence precedence) {
    DISCARD(advance());  // Consume the left square bracket
    DISCARD(precedence);  // Avoid unused variable warning
    constexpr auto precedence_ = static_cast<std::underlying_type<Precedence>::type>(Precedence::Assignment) + 1;
    ExpressionUPtr_t index = parseExpression(static_cast<Precedence>(precedence_));
    expectToken(lexer::TokenType::RightSquare, "Expected ']' to end indexing expression");
    return std::make_unique<ast::IndexExpression>(std::move(left), std::move(index));
}

ExpressionUPtr_t Parser::parseMemberAccessExpression(ExpressionUPtr_t left, Precedence precedence) {
    DISCARD(advance());  // Consume the member access operator (.)
    DISCARD(precedence);  // Avoid unused variable warning
    return std::make_unique<ast::MemberAccessExpression>(
        std::move(left), expectToken(lexer::TokenType::Identifier, "Expected identifier after '.'").getLexeme());
}

ExpressionUPtr_t Parser::parseParenthesizedExpression() {
    DISCARD(advance());  // Consume the left parenthesis
    ExpressionUPtr_t expr = parseExpression(Precedence::Default);
    expectToken(lexer::TokenType::RightParen, "Expected a right parenthesis to close the expression");
    return expr;
}

ExpressionUPtr_t Parser::parsePostfixExpression(ExpressionUPtr_t left, Precedence precedence) {
    TokenType op = advance().getType();
    DISCARD(precedence);  // Avoid unused variable warning
    return std::make_unique<ast::PostfixExpression>(std::move(left), op);
}

ExpressionUPtr_t Parser::parsePrefixExpression() {
    TokenType op = advance().getType();
    auto right = parseExpression(Precedence::Unary);

    return std::make_unique<ast::PrefixExpression>(op, std::move(right));
}

ExpressionUPtr_t Parser::parsePrimaryExpression() noexcept_if_release {
    auto token = advance();
    std::string lexeme = token.getLexeme();

    switch (token.getType()) {
        case TokenType::CharLiteral:
            return std::make_unique<ast::CharLiteralExpression>(lexeme[0]);  // Single character
        case TokenType::StrLiteral: return std::make_unique<ast::StringLiteralExpression>(lexeme);
        case TokenType::Identifier: return std::make_unique<ast::IdentifierExpression>(lexeme);
        case TokenType::True: return std::make_unique<ast::BoolLiteralExpression>(true);
        case TokenType::False: return std::make_unique<ast::BoolLiteralExpression>(false);
        case TokenType::FloatLiteral:
            // Check for floating-point suffixes
            return std::make_unique<ast::NumberLiteralExpression>(lexeme.ends_with("f32") ? stof(lexeme)
                                                                                          : stod(lexeme));
        case TokenType::IntegerLiteral: {
            // Extract integer suffix
            Base base = determineNumberBase(lexeme);
            if (base != Base::Decimal) {
                // Strip the base prefix (the first two characters: 0x, 0b, 0o) from the lexeme
                lexeme.erase(0, 2);
            }
            std::string numericPart = lexeme;
            std::string suffix;
            extractSuffix(numericPart, suffix);

            std::optional<number_t> value = utils::stringToNumber(numericPart, base, false, suffix);
            if (!value) {
                logError(std::format("Invalid integer literal '{}'", lexeme), token.getLine(), token.getColumn());
                return std::make_unique<ast::NumberLiteralExpression>(0);
                // Error tolerance: return a default value of 0
            }
            return std::make_unique<ast::NumberLiteralExpression>(*value);
        }
        default:
            ASSERT_UNREACHABLE("Invalid Token Type in parsePrimaryExpression: "
                               + lexer::tokenTypeToString(token.getType()));
    }
}

ExpressionUPtr_t Parser::parseScopeResolutionExpression(ExpressionUPtr_t left, Precedence precedence) {
    DISCARD(advance());  // Consume the scope resolution operator (::)
    DISCARD(precedence);  // Avoid unused variable warning
    auto element = expectToken(lexer::TokenType::Identifier, "Expected identifier after '::'").getLexeme();
    return std::make_unique<ast::ScopeResolutionExpression>(std::move(left), element);
}

ExpressionUPtr_t Parser::parseTypeCastExpression(ExpressionUPtr_t left, Precedence precedence) {
    DISCARD(advance());  // Consume the 'as' token
    TypeSPtr_t type = parseType(precedence);
    return std::make_unique<ast::TypeCastExpression>(std::move(left), std::move(type));
}

// ===== Helper Functions =====

void extractSuffix(std::string& lexeme, std::string& suffix) {
    if (lexeme.ends_with("i8") || lexeme.ends_with("I8") || lexeme.ends_with("u8") || lexeme.ends_with("U8")) {
        suffix = lexeme.substr(lexeme.length() - 2);
        lexeme.erase(lexeme.length() - 2);
    } else if (lexeme.ends_with("i16") || lexeme.ends_with("I16") || lexeme.ends_with("u16")
               || lexeme.ends_with("U16")) {
        suffix = lexeme.substr(lexeme.length() - 3);
        lexeme.erase(lexeme.length() - 3);
    } else if (lexeme.ends_with("i32") || lexeme.ends_with("I32") || lexeme.ends_with("u32")
               || lexeme.ends_with("U32")) {
        suffix = lexeme.substr(lexeme.length() - 3);
        lexeme.erase(lexeme.length() - 3);
    } else if (lexeme.ends_with("i64") || lexeme.ends_with("I64") || lexeme.ends_with("u64")
               || lexeme.ends_with("U64")) {
        suffix = lexeme.substr(lexeme.length() - 3);
        lexeme.erase(lexeme.length() - 3);
    } else if (lexeme.ends_with("f32") || lexeme.ends_with("F32")) {
        suffix = "f32";
        lexeme.erase(lexeme.length() - 3);
    } else if (lexeme.ends_with("f64") || lexeme.ends_with("F64")) {
        suffix = "f64";
        lexeme.erase(lexeme.length() - 3);
    }
}

Base determineNumberBase(const std::string& lexeme) {
    if (lexeme.length() <= 2) {
        return Base::Decimal;  // Prefixed literals are at least 3 characters long (0x/0b/0o + at least one digit)
    }
    if (lexeme[0] != '0') {
        // No base prefix, assume decimal
        return Base::Decimal;
    }
    switch (lexeme[1]) {
        case 'x':
        case 'X': return Base::Hexadecimal;
        case 'b':
        case 'B': return Base::Binary;
        case 'o':
        case 'O': return Base::Octal;
        default:  // Not a base prefix (just leading zero), assume decimal
            return Base::Decimal;
    }
}

}  // namespace parser
}  // namespace Manganese
