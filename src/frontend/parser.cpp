#include "include/parser.h"

#include <iostream>

#include "../global_macros.h"
#include "include/ast.h"

//! For generics, might have to do two passes
//! First pass: just identify what the function/bundle/blueprint names are and keep those in a table
//! Then re-parse the file using that table to handle the "less-than vs generic" ambiguity
//! On the second pass, any time you see a function/bundle/blueprint name followed by a left angle bracket (<), treat it as a generic type (it's invalid to compare a function/bundle/blueprint name with something else)

namespace manganese {
namespace parser {

Parser::Parser(const str& source, lexer::Mode mode) : lexer(std::make_unique<lexer::Lexer>(source, mode)) {
    initializeLookups();
}

ast::Block Parser::parse() {
    ast::Block body;
    while (!done()) {
        // Special cases for block visiblity modifiers (`public:`, `readonly:` and `private`: set the visibility for all future variables, until the next modifier is encountered)
        if (peekToken().getType() == TokenType::Public && peekToken(1).getType() == TokenType::Colon) {
            defaultVisibility = ast::Visibility::Public;
            DISCARD(consumeToken());  // Consume the Public token
            DISCARD(consumeToken());  // Consume the Colon token
        } else if (peekToken().getType() == TokenType::ReadOnly && peekToken(1).getType() == TokenType::Colon) {
            defaultVisibility = ast::Visibility::ReadOnly;
            DISCARD(consumeToken());  // Consume the ReadOnly token
            DISCARD(consumeToken());  // Consume the Colon token
        } else if (peekToken().getType() == TokenType::Private && peekToken(1).getType() == TokenType::Colon) {
            defaultVisibility = ast::Visibility::Private;
            DISCARD(consumeToken());  // Consume the Private token
            DISCARD(consumeToken());  // Consume the Colon token
        } else {
            body.push_back(parseStatement());
        }

        // Flush the cache so there are no holdover tokens
        tokenCache.clear();
        tokenCachePosition = 0;
    }
    return body;
}

ast::StatementPtr Parser::parseStatement() {
    lexer::TokenType type = peekToken().getType();
    auto it = statementLookup.find(type);
    if (it != statementLookup.end()) {
        return (it->second)(this);
    }
    auto expression = parseExpression(OperatorBindingPower::Default);
    expectToken(TokenType::Semicolon);
    return std::make_unique<ast::ExpressionStatement>(std::move(expression));
}

inline int determineNumberBase(const std::string& lexeme) {
    if (lexeme.length() <= 2) {
        return 10;  // Default to decimal if the lexeme is too short to be a valid base-prefixed number
    }
    if (lexeme[0] != '0') {
        // No base prefix, assume decimal
        return 10;
    }
    switch (lexeme[1]) {
        case 'x':
        case 'X':  // Hexadecimal
            return 16;
        case 'b':
        case 'B':  // Binary
            return 2;
        case 'o':
        case 'O':  // Octal
            return 8;
        default:  // No valid base prefix, assume decimal
            return 10;
    }
}

ExpressionPtr Parser::parsePrimaryExpression() {
    using lexer::TokenType;
    using std::make_unique, std::stoi, std::stol, std::stoll, std::stoul, std::stoull;
    using std::stof, std::stod, std::stold;
    auto token = consumeToken();
    str lexeme = token.getLexeme();
    switch (token.getType()) {
        case TokenType::StrLiteral:
            return make_unique<ast::StringExpression>(lexeme);
        case TokenType::Identifier:
            return make_unique<ast::SymbolExpression>(lexeme);
        case TokenType::IntegerLiteral: {
            // Extract integer suffix (u, l, ll, ul, ull, etc.)
            int base = determineNumberBase(lexeme);
            if (base != 10) {
                // Strip the base prefix (the first two characters: 0x, 0b, 0o) from the lexeme
                lexeme.erase(0, 2);
            }
            std::string numericPart = lexeme;
            std::string suffix;
            // Scan backwards for suffix letters
            while (!numericPart.empty() && (isalpha(numericPart.back()) || numericPart.back() == '_')) {
                suffix = std::string(1, tolower(numericPart.back())) + suffix;  // prepend to suffix, ignoring case for simplicity
                numericPart.pop_back();
            }

            if (suffix == "ull" || suffix == "llu") {
                return make_unique<ast::NumberExpression>(stoull(numericPart, nullptr, base));
            } else if (suffix == "ll") {
                return make_unique<ast::NumberExpression>(stoll(numericPart, nullptr, base));
            } else if (suffix == "ul" || suffix == "lu") {
                return make_unique<ast::NumberExpression>(stoul(numericPart, nullptr, base));
            } else if (suffix == "u") {
                return make_unique<ast::NumberExpression>(stoul(numericPart, nullptr, base));
            } else if (suffix == "l") {
                return make_unique<ast::NumberExpression>(stol(numericPart, nullptr, base));
            } else {
                return make_unique<ast::NumberExpression>(stoi(numericPart, nullptr, base));
            }
        }
        case TokenType::FloatLiteral: {
            // Check for floating-point suffixes
            char lastChar = lexeme.back();
            std::string numericPart = lexeme;

            // Process float suffixes (f/F, d/D, l/L)
            if (lastChar == 'f' || lastChar == 'F') {
                numericPart.pop_back();
                return make_unique<ast::NumberExpression>(stof(numericPart));
            } else {
                return make_unique<ast::NumberExpression>(stod(numericPart));
            }
        }
        default:
            UNREACHABLE("Invalid Token Type in parsePrimaryExpression: " + lexer::tokenTypeToString(token.getType()));
    }
}

ExpressionPtr Parser::parseBinaryExpression(ExpressionPtr left, OperatorBindingPower bindingPower) {
    auto operatorToken = consumeToken();
    auto right = parseExpression(bindingPower);

    return std::make_unique<ast::BinaryExpression>(std::move(left), operatorToken.getType(), std::move(right));
}

ExpressionPtr Parser::parseExponentiationExpression(ExpressionPtr left, OperatorBindingPower bindingPower) {
    auto operatorToken = consumeToken();

    // For right associativity, use one less binding power for the right operand
    // This will allow nested exponentiations to be parsed from right to left
    auto right = parseExpression(static_cast<OperatorBindingPower>(static_cast<int>(bindingPower) - 1));

    return std::make_unique<ast::BinaryExpression>(std::move(left), operatorToken.getType(), std::move(right));
}

ExpressionPtr Parser::parseExpression(OperatorBindingPower bindingPower) {
    // First, parse the null denoted expression
    lexer::TokenType type = peekToken().getType();
    auto nudIt = nullDenotationLookup.find(type);
    if (nudIt == nullDenotationLookup.end()) {
        UNREACHABLE("No null denotation function for token type: " + lexer::tokenTypeToString(type));
    }
    nullDenotationHandler_t nullDenotationFunction = nudIt->second;
    ExpressionPtr left = nullDenotationFunction(this);

    // While we have a left denoted expression and (current binding power) < (current token binding power), continue parsing
    while (!done()) {
        type = peekToken().getType();  // the null denotation function advances the lexer so update the type
        if (type == TokenType::Semicolon) {
            // End of the expression -- don't keep parsing
            break;
        }

        // Check if current token's binding power is high enough
        auto bindingPowerIt = bindingPowerLookup.find(type);
        if (bindingPowerIt == bindingPowerLookup.end() || bindingPowerIt->second <= bindingPower) {
            break;
        }

        auto ledIt = leftDenotationLookup.find(type);
        if (ledIt == leftDenotationLookup.end()) {
            UNREACHABLE("No left denotation function for token type: " + lexer::tokenTypeToString(type));
        }
        leftDenotationHandler_t leftDenotationFunction = ledIt->second;
        left = leftDenotationFunction(this, std::move(left), bindingPowerLookup[type]);
    }
    return left;
}

StatementPtr Parser::parseVariableDeclaration() {
    bool isConst = (consumeToken().getType() == TokenType::Const);

    std::string name = expectToken(TokenType::Identifier, "Expected a variable name after " + (isConst ? str("'const'") : str("'let'"))).getLexeme();

    expectToken({TokenType::Assignment, TokenType::Colon}, "Expected an '=' to assign a value to the variable, or a ':' to declare its type");

    ExpressionPtr assignedValue = parseExpression(OperatorBindingPower::Assignment);
    expectToken(TokenType::Semicolon, "Expected a ';' to end the variable declaration");

    return std::make_unique<ast::VariableDeclarationStatement>(name, isConst, defaultVisibility, std::move(assignedValue));
}

inline void Parser::initializeLookups() {
    using lexer::TokenType;

    //~ Logical
    led(TokenType::And, OperatorBindingPower::Logical, parseBinaryExpression);
    led(TokenType::Or, OperatorBindingPower::Logical, parseBinaryExpression);

    //~ Relational
    led(TokenType::LessThan, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::LessThanOrEqual, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::GreaterThan, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::GreaterThanOrEqual, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::Equal, OperatorBindingPower::Relational, parseBinaryExpression);
    led(TokenType::NotEqual, OperatorBindingPower::Relational, parseBinaryExpression);

    //~ Additive, Multiplicative & Exponential
    led(TokenType::Plus, OperatorBindingPower::Additive, parseBinaryExpression);
    led(TokenType::Minus, OperatorBindingPower::Additive, parseBinaryExpression);
    led(TokenType::Mul, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Div, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::FloorDiv, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Mod, OperatorBindingPower::Multiplicative, parseBinaryExpression);
    led(TokenType::Exp, OperatorBindingPower::Exponential, parseExponentiationExpression);

    // ~ Literals & Symbols
    nud(TokenType::IntegerLiteral, parsePrimaryExpression);
    nud(TokenType::FloatLiteral, parsePrimaryExpression);
    nud(TokenType::StrLiteral, parsePrimaryExpression);
    nud(TokenType::Identifier, parsePrimaryExpression);

    //~ Statements
    stmt(TokenType::Const, parseVariableDeclaration);
    stmt(TokenType::Let, parseVariableDeclaration);
}

Token Parser::expectToken(TokenType expectedType, const std::string& message) {
    auto token = peekToken();
    auto tokenType = token.getType();
    if (tokenType == expectedType) {
        return consumeToken();
    }
    if (!message.empty()) {
        std::cerr << message << "(";
    }
    std::cerr << "Expected " << lexer::tokenTypeToString(expectedType)
              << ", got " << lexer::tokenTypeToString(tokenType) << " instead\n";
    if (!message.empty()) {
        std::cerr << message << ")";
    }
    exit(EXIT_FAILURE);
}
Token Parser::expectToken(const std::initializer_list<TokenType>& expectedTypes, const std::string& message) {
    auto token = peekToken();
    auto tokenType = token.getType();
    for (auto expectedType : expectedTypes) {
        if (tokenType == expectedType) {
            return consumeToken();
        }
    }
    if (!message.empty()) {
        std::cerr << message << "(";
    }
    std::cerr << "Expected one of: ";
    for (auto expectedType : expectedTypes) {
        std::cerr << lexer::tokenTypeToString(expectedType) << ", ";
    }
    std::cerr << "but got " << lexer::tokenTypeToString(tokenType) << " instead\n";
    if (!message.empty()) {
        std::cerr << message << ")";
    }
    std::cerr << "(line " << token.getLine() << " column " << token.getColumn() << ")\n";
    exit(EXIT_FAILURE);
}
}  // namespace parser
}  // namespace manganese