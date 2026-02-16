/**
 * @file parser_statements.cpp
 * @brief This file contains the implementation of statement parsing in the parser. It is split into its own file for
 * readability and maintainability.
 */

#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <global_macros.hpp>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace Manganese {
namespace parser {

StatementUPtr_t Parser::parseStatement() NOEXCEPT_IF_RELEASE {
    const TokenType type = peekTokenType();
    const auto index = tokenToIndex(type);

    auto handler = statementLookup[index];
    if (handler) { return (this->*handler)(); }

    // Parse out an expression then convert it to a statement
    ExpressionUPtr_t expr = parseExpression(Precedence::Default);
    if (!isParsingBlockPrecursor) { expectToken(TokenType::Semicolon, "Expected semicolon after expression"); }
    return std::make_unique<ast::ExpressionStatement>(std::move(expr));
}

// ===== Specific statement parsing methods =====

StatementUPtr_t Parser::parseAggregateDeclarationStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    std::vector<std::string> genericTypes;
    std::vector<ast::AggregateField> fields;
    std::string name = expectToken(TokenType::Identifier, "Expected aggregate name after 'aggregate'").getLexeme();

    if (peekTokenType() == TokenType::LeftSquare) {
        DISCARD(consumeToken());
        while (!done() && peekTokenType() != TokenType::RightSquare) {
            std::string genericName = (expectToken(TokenType::Identifier, "Expected a generic type name").getLexeme());
            if (std::find(genericTypes.begin(), genericTypes.end(), genericName) != genericTypes.end()) {
                logError(peekToken().getLine(), peekToken().getColumn(),
                         "Generic type '{}' in aggregate '{}' was already declared", genericName, name);
            } else {
                genericTypes.push_back(genericName);
            }
            if (peekTokenType() != TokenType::RightSquare) {
                expectToken(TokenType::Comma,
                            "Expected a ',' to separate generic types, or a ']' to close the generic type list");
            }
        }
        expectToken(TokenType::RightSquare, "Expected ']' to close generic type list");
    }
    expectToken(TokenType::LeftBrace, "Expected a '{'");

    while (!done()) {
        if (peekTokenType() == TokenType::RightBrace) {
            break;  // Done declaration
        }
        if (peekTokenType() != TokenType::Identifier) {
            logError(peekToken().getLine(), peekToken().getColumn(),
                     "Unexpected token '{}' in aggregate declaration. Expected field name.", peekToken().getLexeme());
            DISCARD(consumeToken());  // Skip the unexpected token to avoid infinite loop
        }
        Token t = consumeToken();
        std::string fieldName = t.getLexeme();
        expectToken(TokenType::Colon, "Expected a ':' to declare an aggregate field type.");
        bool isMutable = false;
        if (peekTokenType() == TokenType::Mut) {
            isMutable = true;
            DISCARD(consumeToken());
        }
        TypeSPtr_t type = parseType(Precedence::Default);
        expectToken(TokenType::Semicolon, "Expected a ';'");

        auto duplicate = std::find_if(fields.begin(), fields.end(), [fieldName](const ast::AggregateField& field) {
            return field.name == fieldName;
        });
        if (duplicate != fields.end()) {
            logError(t.getLine(), t.getColumn(), "Duplicate field '{}' in aggregate '{}'", fieldName, name);
        } else {
            fields.emplace_back(fieldName, std::move(type), isMutable);
        }
    }

    expectToken(TokenType::RightBrace);

    // Move since AggregateField contains a unique_ptr which is not copyable
    return std::make_unique<ast::AggregateDeclarationStatement>(name, std::move(genericTypes), std::move(fields));
}

StatementUPtr_t Parser::parseAliasStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    TypeSPtr_t baseType;
    if (peekToken().isPrimitiveType() || peekTokenType() == TokenType::Func || peekTokenType() == TokenType::Ptr) {
        // Primitive types are easy to alias -- just parse a regular type
        baseType = parseType(Precedence::Default);
    } else {
        // If it's not a primitive type, we expect an identifier.
        // This might be a path (e.g. alias foo::bar as baz, so we need to handle that)
        std::string path
            = expectToken(TokenType::Identifier, "Expected an identifier after 'alias', or a primitive type.")
                  .getLexeme();
        while (peekTokenType() == TokenType::ScopeResolution) {
            path += consumeToken().getLexeme();
            path += expectToken(TokenType::Identifier,
                                std::format("Expected an identifier after {}",
                                            lexer::tokenTypeToString(TokenType::ScopeResolution)))
                        .getLexeme();
        }

        if (peekTokenType() == TokenType::At) {
            // Generic Type
            baseType = parseGenericType(std::make_shared<ast::SymbolType>(path), Precedence::Default);
        } else if (peekTokenType() == TokenType::LeftSquare) {
            // Array type
            baseType = parseArrayType(std::make_shared<ast::SymbolType>(path), Precedence::Default);
        } else {
            // Regular (identifier) type
            baseType = std::make_shared<ast::SymbolType>(path);
        }
    }
    expectToken(TokenType::As, "Expected 'as' to introduce the type alias");
    std::string alias = expectToken(TokenType::Identifier, "Expected an alias name").getLexeme();
    expectToken(TokenType::Semicolon, "Expected a ';' after an alias statement");
    return std::make_unique<ast::AliasStatement>(std::move(baseType), std::move(alias));
}

StatementUPtr_t Parser::parseBreakStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    expectToken(TokenType::Semicolon);
    return std::make_unique<ast::BreakStatement>();
}

StatementUPtr_t Parser::parseContinueStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    expectToken(TokenType::Semicolon);
    return std::make_unique<ast::ContinueStatement>();
}

StatementUPtr_t Parser::parseDoWhileLoopStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    ast::Block body = parseBlock("do-while body");
    expectToken(TokenType::While, "Expected 'while' after a 'do' block");
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    auto condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end a while condition");
    expectToken(TokenType::Semicolon, "Expected a ';' after a while clause");
    return std::make_unique<ast::WhileLoopStatement>(std::move(body), std::move(condition), /*isDoWhile=*/true);
}

StatementUPtr_t Parser::parseEnumDeclarationStatement() NOEXCEPT_IF_RELEASE {
    Token enumStartToken = consumeToken();
    std::string name = expectToken(TokenType::Identifier, "Expected enum name after 'enum'").getLexeme();
    TypeSPtr_t baseType;
    std::vector<ast::EnumValue> values;
    if (peekTokenType() == TokenType::Colon) {
        DISCARD(consumeToken());
        if (!peekToken().isPrimitiveType()) {
            logError(peekToken().getLine(), peekToken().getColumn(),
                     "Enums can only have primitive types as their underlying type, not {}", peekToken().getLexeme());
        }
        baseType = std::make_shared<ast::SymbolType>(consumeToken().getLexeme());
    } else {
        baseType = std::make_shared<ast::SymbolType>("int32");  // Default to int32 if no base type is specified
    }
    expectToken(TokenType::LeftBrace, "Expected '{' to start the enum body");
    while (!done() && peekTokenType() != TokenType::RightBrace) {
        std::string valueName = expectToken(TokenType::Identifier, "Expected enum value name").getLexeme();
        ExpressionUPtr_t valueExpression = nullptr;
        if (peekTokenType() == TokenType::Assignment) {
            DISCARD(consumeToken());
            valueExpression = parseExpression(Precedence::Default);
        }
        // Check for duplicate enum value names
        auto duplicate = std::find_if(values.begin(), values.end(),
                                      [valueName](const ast::EnumValue& value) { return value.name == valueName; });

        if (duplicate != values.end()) {
            logError(peekToken().getLine(), peekToken().getColumn(),
                     "Enum value '{}' (in enum '{}') was previously declared", valueName, name);
        } else {
            values.emplace_back(valueName, std::move(valueExpression));
        }
        if (peekTokenType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma, "Expected ',' to separate enum values");
        }
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end the enum body");
    if (values.empty()) {
        logError(enumStartToken.getLine(), enumStartToken.getColumn(), "Enum '{}' has no values", name);
    }
    return std::make_unique<ast::EnumDeclarationStatement>(std::move(name), std::move(baseType), std::move(values));
}

StatementUPtr_t Parser::parseFunctionDeclarationStatement() NOEXCEPT_IF_RELEASE {
    // TODO: Handle function attributes
    // TODO: Handle function default parameters
    // TODO: Handle function variadic parameters
    // TODO: Handle function overloading (mangled names)
    DISCARD(consumeToken());
    std::string name = expectToken(TokenType::Identifier, "Expected function name").getLexeme();
    std::vector<ast::FunctionParameter> params;
    std::vector<std::string> genericTypes;
    TypeSPtr_t returnType = nullptr;
    ast::Block body;

    if (peekTokenType() == TokenType::LeftSquare) {
        // Generics
        DISCARD(consumeToken());
        while (!done()) {
            if (peekTokenType() == TokenType::RightSquare) {
                break;  // End of generics
            }
            if (peekTokenType() != TokenType::Identifier) {
                logError(peekToken().getLine(), peekToken().getColumn(), "Expected a generic type name");
                DISCARD(consumeToken());  // Skip the unexpected token to avoid infinite loop
            }
            Token genericToken = expectToken(TokenType::Identifier, "Expected a generic type name");
            std::string genericName = genericToken.getLexeme();
            if (std::find(genericTypes.begin(), genericTypes.end(), genericName) != genericTypes.end()) {
                logError(genericToken.getLine(), genericToken.getColumn(),
                         "Duplicate generic type '{}' in function '{}'", genericName, name);
            } else {
                genericTypes.push_back(std::move(genericName));
            }
            if (peekTokenType() != TokenType::RightSquare) {
                expectToken(TokenType::Comma,
                            "Expected a ',' to separate generic types, or a ']' to close the generic type list");
            }
        }
        expectToken(TokenType::RightSquare, "Expected ']' to close generic type list");
    }
    expectToken(TokenType::LeftParen);

    while (!done()) {
        if (peekTokenType() == TokenType::RightParen) { break; }
        bool isMutable = false;
        std::string param_name = expectToken(TokenType::Identifier, "Expected a variable name").getLexeme();
        expectToken(TokenType::Colon);
        if (peekTokenType() == TokenType::Mut) {
            DISCARD(consumeToken());
            isMutable = true;
        }
        TypeSPtr_t param_type = parseType(Precedence::Default);
        params.emplace_back(param_name, std::move(param_type), isMutable);
        if (peekTokenType() != TokenType::RightParen && peekTokenType() != TokenType::EndOfFile) {
            expectToken(TokenType::Comma,
                        "Expected a ',' to separate function parameters, or a ) to close the parameter list");
        }
    }
    expectToken(TokenType::RightParen);
    if (peekTokenType() == TokenType::Arrow) {
        DISCARD(consumeToken());
        returnType = parseType(Precedence::Default);
    }
    // Don't need to std::move body because of return value optimization
    return std::make_unique<ast::FunctionDeclarationStatement>(name, std::move(genericTypes), std::move(params),
                                                               std::move(returnType), parseBlock("function body"));
}

StatementUPtr_t Parser::parseIfStatement() NOEXCEPT_IF_RELEASE {
    this->isParsingBlockPrecursor = true;
    DISCARD(consumeToken());

    expectToken(TokenType::LeftParen, "Expected '(' to introduce if condition");
    auto condition = parseExpression(Precedence::Default);
    this->isParsingBlockPrecursor = false;
    expectToken(TokenType::RightParen, "Expected ')' to end if condition");
    ast::Block body = parseBlock("if body");

    std::vector<ast::ElifClause> elifs;
    while (peekTokenType() == TokenType::Elif) {
        DISCARD(consumeToken());

        this->isParsingBlockPrecursor = true;
        expectToken(TokenType::LeftParen, "Expected '(' to introduce elif condition");
        auto elifCondition = parseExpression(Precedence::Default);
        this->isParsingBlockPrecursor = false;
        expectToken(TokenType::RightParen, "Expected ')' to end elif condition");

        elifs.emplace_back(std::move(elifCondition), parseBlock("elif body"));
    }
    ast::Block elseBody;
    if (peekTokenType() == TokenType::Else) {
        DISCARD(consumeToken());
        elseBody = parseBlock("else body");
    }
    return std::make_unique<ast::IfStatement>(std::move(condition), std::move(body), std::move(elifs),
                                              std::move(elseBody));
}

StatementUPtr_t Parser::parseImportStatement() NOEXCEPT_IF_RELEASE {
    size_t startLine = peekToken().getLine();
    size_t startColumn = peekToken().getColumn();

    if (this->hasParsedFileHeader) {
        logging::logWarning(startLine, startColumn, "Imports should go at the top of the file");
    }
    DISCARD(consumeToken());
    std::vector<std::string> path;
    path.push_back(expectToken(TokenType::Identifier, "Expected a module name or path").getLexeme());
    while (peekTokenType() == TokenType::ScopeResolution) {
        DISCARD(consumeToken());  // Consume '::'
        path.push_back(expectToken(TokenType::Identifier, "Expected identifier after '::'").getLexeme());
    }
    std::string alias;
    if (peekTokenType() == TokenType::As) {
        DISCARD(consumeToken());
        alias = expectToken(TokenType::Identifier, "Expected an identifier as an import alias").getLexeme();
    }
    expectToken(TokenType::Semicolon, "Expected a ';' to end an import statement");

    bool duplicate = false;
    for (const auto& [existingPath, existingAlias] : imports) {
        if (path == existingPath) {
            std::string imported
                = std::accumulate(existingPath.begin() + 1, existingPath.end(),
                                  existingPath[0],  // existingPath should never be empty
                                  [](const std::string& a, const std::string& b) { return a + "::" + b; });

            logging::logWarning(startLine, startColumn, "Duplicate import of {}", imported);
            duplicate = true;
            break;

        } else if (alias == existingAlias && !alias.empty()) {
            logging::logWarning(startLine, startColumn, "Alias {} was already used", existingAlias);
            duplicate = true;
            break;
        }
    }
    if (!duplicate) { imports.emplace_back(path, alias); }
    // Dummy node since imports are stored separately
    return std::make_unique<ast::EmptyStatement>();
}

StatementUPtr_t Parser::parseModuleDeclarationStatement() NOEXCEPT_IF_RELEASE {
    auto temp = consumeToken();
    size_t startLine = temp.getLine(), startColumn = temp.getColumn();
    if (this->hasParsedFileHeader) {
        logging::logWarning(startLine, startColumn, "Module declarations should go at the top of the file");
    }
    std::string name = expectToken(TokenType::Identifier, "Expected a module name").getLexeme();
    expectToken(TokenType::Semicolon, "Expected a ';' after a module declaration");
    if (!this->moduleName.empty()) {
        logError(
            startLine, startColumn,
            "A module name has previously been declared in this file. Files can only have one module declaration.");
    } else {
        this->moduleName = name;
    }

    // Dummy node since there's no need to semantically analyze module declarations (that happens here)
    return std::make_unique<ast::EmptyStatement>();
}

StatementUPtr_t Parser::parseRedundantSemicolon() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    return std::make_unique<ast::EmptyStatement>();
}

StatementUPtr_t Parser::parseRepeatLoopStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    expectToken(TokenType::LeftParen, "Expected '(' to introduce a number of iterations");
    auto numIterations = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end the number of iterations");
    // Don't need to std::move because of return value optimization
    return std::make_unique<ast::RepeatLoopStatement>(std::move(numIterations), parseBlock("repeat loop body"));
}

StatementUPtr_t Parser::parseReturnStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    ExpressionUPtr_t expression = nullptr;
    if (peekTokenType() != TokenType::Semicolon) {
        // If the next token is not a semicolon, parse an expression
        // If it is, this is a null return statement
        expression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::Semicolon, "Expected semicolon after return statement");
    return make_unique<ast::ReturnStatement>(std::move(expression));
}

StatementUPtr_t Parser::parseSwitchStatement() NOEXCEPT_IF_RELEASE {
    Token temp = consumeToken();
    size_t startLine = temp.getLine(), startColumn = temp.getColumn();
    expectToken(TokenType::LeftParen, "Expected '(' to introduce switch variable");
    this->isParsingBlockPrecursor = true;
    ExpressionUPtr_t variable = parseExpression(Precedence::Default);
    this->isParsingBlockPrecursor = false;
    expectToken(TokenType::RightParen, "Expected ')' to end switch variable");
    expectToken(TokenType::LeftBrace, "Expected '{' to start the switch body");

    std::vector<ast::CaseClause> cases;
    ast::Block defaultBody;

    while (peekTokenType() == TokenType::Case) {
        DISCARD(consumeToken());
        auto caseValue = parseExpression(Precedence::Default);
        ast::Block caseBody;
        expectToken(TokenType::Colon, "Expected ':' after case value");
        while (peekTokenType() != TokenType::Case && peekTokenType() != TokenType::Default
               && peekTokenType() != TokenType::RightBrace) {
            caseBody.push_back(parseStatement());
        }
        cases.emplace_back(std::move(caseValue), std::move(caseBody));
    }
    if (peekTokenType() == TokenType::Default) {
        DISCARD(consumeToken());
        expectToken(TokenType::Colon, "Expected ':' after default case");
        while (peekTokenType() != TokenType::RightBrace) { defaultBody.push_back(parseStatement()); }
    }
    if (cases.empty() && defaultBody.empty()) {
        logging::logWarning(startLine, startColumn, "Switch statement has no cases or default body");
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end the switch body");

    return std::make_unique<ast::SwitchStatement>(std::move(variable), std::move(cases), std::move(defaultBody));
}

StatementUPtr_t Parser::parseVisibilityAffectedStatement() NOEXCEPT_IF_RELEASE {
    ast::Visibility visibility;
    switch (consumeToken().getType()) {
        case TokenType::Private: visibility = ast::Visibility::Private; break;
        case TokenType::Public: visibility = ast::Visibility::Public; break;
        default:
            ASSERT_UNREACHABLE("Unexpected token type in parseVisibilityAffectedStatement: "
                               + lexer ::tokenTypeToString(peekTokenType()));
    }
    size_t startLine = peekToken().getLine(), startColumn = peekToken().getColumn();
    switch (peekTokenType()) {
        case TokenType::Alias: {
            auto tempAlias = static_cast<ast::AliasStatement*>(parseAliasStatement().release());
            tempAlias->visibility = visibility;
            return std::unique_ptr<ast::AliasStatement>(tempAlias);
        }
        case TokenType::Aggregate: {
            auto tempAggregate
                = static_cast<ast::AggregateDeclarationStatement*>(parseAggregateDeclarationStatement().release());
            tempAggregate->visibility = visibility;
            return std::unique_ptr<ast::AggregateDeclarationStatement>(tempAggregate);
        }
        case TokenType::Enum: {
            auto tempEnum = static_cast<ast::EnumDeclarationStatement*>(parseEnumDeclarationStatement().release());
            tempEnum->visibility = visibility;
            return std::unique_ptr<ast::EnumDeclarationStatement>(tempEnum);
        }
        case TokenType::Func: {
            auto tempFunction
                = static_cast<ast::FunctionDeclarationStatement*>(parseFunctionDeclarationStatement().release());
            tempFunction->visibility = visibility;
            return std::unique_ptr<ast::FunctionDeclarationStatement>(tempFunction);
        }
        default:
            logError(startLine, startColumn, "{} cannot follow a visibility modifier",
                     lexer::tokenTypeToString(peekTokenType()));
            // Parse the statement as if it had no visibility modifier
            return parseStatement();
    }
}

StatementUPtr_t Parser::parseVariableDeclarationStatement() NOEXCEPT_IF_RELEASE {
    TypeSPtr_t explicitType;
    ExpressionUPtr_t value;
    ast::Visibility visibility = defaultVisibility;

    DISCARD(consumeToken());  // Consume the 'let' token
    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        DISCARD(consumeToken());  // Consume the 'mut' token
        isMutable = true;
    }
    std::string name = expectToken(TokenType::Identifier,
                                   std::format("Expected variable name after '{}'", isMutable ? "let mut" : "let"))
                           .getLexeme();
    if (peekTokenType() == TokenType::Colon) {
        DISCARD(consumeToken());  // Consume the colon
        if (peekTokenType() == TokenType::Public) {
            visibility = ast::Visibility::Public;
            DISCARD(consumeToken());  // Consume the public keyword
        } else if (peekTokenType() == TokenType::Private) [[unlikely]] {
            // private is the default so it'd mainly be used for emphasis
            visibility = ast::Visibility::Private;
            DISCARD(consumeToken());  // Consume the private keyword
        }
        explicitType = parseType(Precedence::Default);
    }
    if (peekTokenType() != TokenType::Semicolon) {
        expectToken(TokenType::Assignment, "Expected '=' or ';' after variable name");
        value = parseExpression(Precedence::Default);
    } else if (explicitType == nullptr) {
        // If no value is provided, we need to have a type
        expectToken(TokenType::Colon, "Expected ':' to specify type for variable without initial value");
        explicitType = parseType(Precedence::Default);
    }

    expectToken(TokenType::Semicolon, "Expected semicolon after variable declaration");

    return std::make_unique<ast::VariableDeclarationStatement>(isMutable, std::move(name), visibility, std::move(value),
                                                               std::move(explicitType));
}

StatementUPtr_t Parser::parseWhileLoopStatement() NOEXCEPT_IF_RELEASE {
    DISCARD(consumeToken());
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    auto condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end while condition");
    // Don't need to std::move because of return value optimization
    return std::make_unique<ast::WhileLoopStatement>(parseBlock("while loop body"), std::move(condition));
}

// ===== Helper Functions =====
ast::Block Parser::parseBlock(std::string blockName) NOEXCEPT_IF_RELEASE {
    expectToken(TokenType::LeftBrace, "Expected a '{' to start " + blockName);
    ast::Block block;
    while (!done()) {
        if (peekTokenType() == TokenType::RightBrace) {
            break;  // End of the block
        }
        block.push_back(parseStatement());
    }
    expectToken(TokenType::RightBrace, "Expected '}' to end " + blockName);
    if (block.empty()) {
        logging::logWarning(peekToken().getLine(), peekToken().getColumn(), "{} is empty", blockName);
    }
    return block;
}
}  // namespace parser
}  // namespace Manganese
