#include <algorithm>
#include <core.hpp>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <io/logging.hpp>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace Manganese::parser {

ast::Statement* Parser::parseStatement() {
    const Token startToken = peekToken();
    const TokenType type = peekTokenType();

    if (type == TokenType::LeftBrace) {
        return makeNode<ast::NestedBlockStatement>(startToken, parseBlock("nested block"));
    }

    // Handle bare semicolons
    if (type == TokenType::Semicolon) { return makeNode<ast::EmptyStatement>(consumeToken()); }

    const statementHandler_t handler = lookupTable[type].statementHandler;
    if (handler != nullptr) { return (this->*handler)(); }

    // Parse out an expression then convert it to a statement
    ast::Expression* expr = parseExpression(Precedence::Default);
    expectToken(TokenType::Semicolon, "Expected semicolon after expression");
    return makeNode<ast::ExpressionStatement>(startToken, expr);
}

// Specific statement parsing methods

ast::Statement* Parser::parseAggregateDeclarationStatement() {
    const Token startToken = consumeToken();
    const auto name = expectToken(TokenType::Identifier, "Expected aggregate name after 'aggregate'").getLexeme();

    std::vector<utils::StringID> genericTypes = parseGenericsList(name);

    expectToken(TokenType::LeftBrace, "Expected a '{'");

    std::vector<ast::AggregateField> fields;
    while (!done() && peekTokenType() != TokenType::RightBrace) {
        if (auto field = parseAggregateField(name, fields)) { fields.push_back(*field); }
    }

    expectToken(TokenType::RightBrace);

    return makeNode<ast::AggregateDeclarationStatement>(startToken, name, std::move(genericTypes), std::move(fields));
}

ast::Statement* Parser::parseAliasStatement() {
    flags.parsingAliasStatement = true;
    const Token startToken = consumeToken();  // consume alias
    const auto alias = expectToken(TokenType::Identifier, "Expected an alias name").getLexeme();
    expectToken(TokenType::Assignment, "Expected '=' after an alias name to introduce the aliased type.");
    ast::Type* baseType = parseType(Precedence::Default);
    expectToken(TokenType::Semicolon, "Expected a ';' after an alias statement");
    flags.parsingAliasStatement = false;
    return makeNode<ast::AliasStatement>(startToken, baseType, alias);
}

ast::Statement* Parser::parseBreakStatement() {
    const Token startToken = consumeToken();
    expectToken(TokenType::Semicolon);
    return makeNode<ast::BreakStatement>(startToken);
}

ast::Statement* Parser::parseContinueStatement() {
    const Token startToken = consumeToken();
    expectToken(TokenType::Semicolon);
    return makeNode<ast::ContinueStatement>(startToken);
}

ast::Statement* Parser::parseDoWhileLoopStatement() {
    const Token startToken = consumeToken();
    ast::Block body = parseBlock("do-while body");
    expectToken(TokenType::While, "Expected 'while' after a 'do' block");
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    ast::Expression* condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end a while condition");
    expectToken(TokenType::Semicolon, "Expected a ';' after a while clause");
    return makeNode<ast::WhileLoopStatement>(startToken, std::move(body), condition, /*isDoWhile=*/true);
}

ast::Statement* Parser::parseEnumDeclarationStatement() {
    const Token startToken = consumeToken();  // Consume 'enum'
    const auto name = expectToken(TokenType::Identifier, "Expected enum name after 'enum'").getLexeme();
    ast::Type* baseType = nullptr;

    if (peekTokenType() == TokenType::Colon) {
        DISCARD(consumeToken());
        baseType = parseType(Precedence::Default);
        if (baseType == nullptr) {
            Token& tmp = peekToken();
            logError(tmp, "Expected valid underlying type after ':' for enum '{}'", interner.get_view(name));
            baseType = makeNode<ast::PoisonedType>(tmp);
        }
    }

    expectToken(TokenType::LeftBrace, "Expected '{' to start the enum body");

    std::vector<ast::EnumValue> values;
    while (!done() && peekTokenType() != TokenType::RightBrace) {
        ast::EnumValue member = parseEnumMember();

        if (auto duplicate = std::ranges::find(values, member.name, &ast::EnumValue::name); duplicate != values.end()) {
            logError(member, "Duplicate member '{}' in enum '{}' (previously declared at line {}, column {})",
                     interner.get_view(member.name), interner.get_view(name), duplicate->line, duplicate->column);
        } else {
            values.push_back(member);
        }

        if (peekTokenType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma, "Expected ',' between enum members");
        }
    }

    expectToken(TokenType::RightBrace, "Expected '}' to close the enum body");

    return makeNode<ast::EnumDeclarationStatement>(startToken, name, baseType, std::move(values));
}

ast::Statement* Parser::parseForLoopStatement() {
    const Token startToken = consumeToken();  // consume 'for'

    expectToken(TokenType::LeftParen, "Expected '(' to introduce for loop");

    // Initialization Clause
    ast::Statement* init = nullptr;
    if (peekTokenType() != TokenType::Semicolon) {
        if (peekTokenType() == TokenType::Let) {
            init = parseVariableDeclarationStatement();
        } else {
            ast::Expression* initExpr = parseExpression(Precedence::Default);
            init = makeNode<ast::ExpressionStatement>(startToken, initExpr);
            expectToken(TokenType::Semicolon, "Expected ';' after for-loop initializer");
        }
    } else {
        expectToken(TokenType::Semicolon, "Expected ';' after for-loop initializer");
    }

    // Stop condition
    ast::Expression* condition = nullptr;
    if (peekTokenType() != TokenType::Semicolon) { condition = parseExpression(Precedence::Default); }
    expectToken(TokenType::Semicolon, "Expected ';' after for-loop condition");

    // Post clause (what runs after each loop)
    ast::Expression* post = nullptr;
    if (peekTokenType() != TokenType::RightParen) { post = parseExpression(Precedence::Default); }
    expectToken(TokenType::RightParen, "Expected ')' to end for loop header");

    ast::Block body = parseBlock("for loop body");

    return makeNode<ast::ForLoopStatement>(startToken, init, condition, post, std::move(body));
}

ast::Statement* Parser::parseFunctionDeclarationStatement() {
    const Token startToken = consumeToken();
    const auto name = expectToken(TokenType::Identifier, "Expected function name").getLexeme();

    std::vector<utils::StringID> genericTypes = parseGenericsList(name);

    expectToken(TokenType::LeftParen);

    std::vector<ast::FunctionParameter> params;
    bool hasDefaultParameter = false;
    bool hasVariadicParameter = false;

    while (!done() && peekTokenType() != TokenType::RightParen) {
        if (auto param = parseFunctionParameter(name, params, hasDefaultParameter, hasVariadicParameter)) {
            params.push_back(*param);
        }

        if (peekTokenType() != TokenType::RightParen && peekTokenType() != TokenType::EndOfFile) {
            expectToken(TokenType::Comma,
                        "Expected a ',' to separate function parameters, or a ) to close the parameter list");
        }
    }

    expectToken(TokenType::RightParen);

    ast::Type* returnType = nullptr;
    if (peekTokenType() == TokenType::Arrow) {
        DISCARD(consumeToken());
        returnType = parseType(Precedence::Default);
    }

    return makeNode<ast::FunctionDeclarationStatement>(startToken, name, std::move(genericTypes), std::move(params),
                                                       returnType, parseBlock("function body"));
}

ast::Statement* Parser::parseIfStatement() {
    const Token startToken = consumeToken();

    expectToken(TokenType::LeftParen, "Expected '(' to introduce if condition");
    ast::Expression* condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end if condition");
    ast::Block body = parseBlock("if body");

    std::vector<ast::ElifClause> elifs;
    while (peekTokenType() == TokenType::Elif) {
        DISCARD(consumeToken());

        expectToken(TokenType::LeftParen, "Expected '(' to introduce elif condition");
        ast::Expression* elifCondition = parseExpression(Precedence::Default);

        expectToken(TokenType::RightParen, "Expected ')' to end elif condition");

        elifs.emplace_back(elifCondition, parseBlock("elif body"));
    }
    ast::Block elseBody;
    if (peekTokenType() == TokenType::Else) {
        DISCARD(consumeToken());
        elseBody = parseBlock("else body");
        if (elseBody.empty()) { elseBody.push_back(ast::getEmptyStatement()); }
    }
    return makeNode<ast::IfStatement>(startToken, condition, std::move(body), std::move(elifs), std::move(elseBody));
}

ast::Statement* Parser::parseImportStatement() {
    const Token startToken = consumeToken();

    std::vector<utils::StringID> path = parseImportPath();
    utils::OptionalStringID alias = parseImportAlias();

    expectToken(TokenType::Semicolon, "Expected a ';' to end an import statement");

    if (flags.hasParsedFileHeader) { logError(startToken, "Import statements must go at the top of the file"); }

    return makeNode<ast::ImportStatement>(startToken, std::move(path), alias);
}

ast::Statement* Parser::parseModuleDeclarationStatement() {
    const lexer::Token temp = consumeToken();
    if (flags.hasParsedFileHeader || flags.hasImports) {
        logError(temp, "Module declarations must be the first line of a file");
    }

    const auto name = expectToken(TokenType::Identifier, "Expected a module name").getLexeme();

    std::string fullName = std::string(interner.get_view(name));
    while (peekTokenType() == TokenType::ScopeResolution) {
        DISCARD(consumeToken());
        fullName += "::"
            + std::string(interner.get_view(
                expectToken(TokenType::Identifier, "Expected identifier after '::'").getLexeme()));
    }

    expectToken(TokenType::Semicolon, "Expected a ';' after a module declaration");

    if (flags.hasModuleDeclaration) {
        logError(temp, "This file already has a module declaration. Files can only have one module declaration.");
    }

    return makeNode<ast::ModuleDeclarationStatement>(temp, interner.intern(fullName));
}

ast::Statement* Parser::parseNamespace() {
    const Token startToken = consumeToken();  // skip 'namespace'
    const auto name = expectToken(TokenType::Identifier, "Expected a namespace name after 'namespace'").getLexeme();
    ast::Block body = parseBlock("namespace " + std::string(interner.get_view(name)));
    return makeNode<ast::NamespaceStatement>(startToken, name, std::move(body));
}

ast::Statement* Parser::parseRedundantSemicolon() {
    DISCARD(consumeToken());
    return ast::getEmptyStatement();
}

ast::Statement* Parser::parseReturnStatement() {
    const Token startToken = consumeToken();
    ast::Expression* expression = nullptr;
    if (peekTokenType() != TokenType::Semicolon) { expression = parseExpression(Precedence::Default); }
    expectToken(TokenType::Semicolon, "Expected semicolon after return statement");
    return makeNode<ast::ReturnStatement>(startToken, expression);
}

ast::Statement* Parser::parseSwitchStatement() {
    const Token startToken = consumeToken();

    expectToken(TokenType::LeftParen, "Expected '(' to introduce switch variable");
    ast::Expression* variable = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end switch variable");

    expectToken(TokenType::LeftBrace, "Expected '{' to start the switch body");

    std::vector<ast::CaseClause> cases;
    while (peekTokenType() == TokenType::Case) { cases.push_back(parseCaseClause()); }

    ast::Block defaultBody;
    bool hasDefault = false;
    if (peekTokenType() == TokenType::Default) {
        hasDefault = true;
        defaultBody = parseDefaultClause();
    }

    if (cases.empty() && !hasDefault) { logWarning(startToken, "Switch statement has no cases or default body"); }

    expectToken(TokenType::RightBrace, "Expected '}' to end the switch body");

    return makeNode<ast::SwitchStatement>(startToken, variable, std::move(cases), std::move(defaultBody));
}

ast::Statement* Parser::parseWhileLoopStatement() {
    const Token startToken = consumeToken();
    expectToken(TokenType::LeftParen, "Expected '(' to introduce while condition");
    ast::Expression* condition = parseExpression(Precedence::Default);
    expectToken(TokenType::RightParen, "Expected ')' to end while condition");

    return makeNode<ast::WhileLoopStatement>(startToken, parseBlock("while loop body"), condition);
}

ast::Statement* Parser::parseVariableDeclarationStatement() {
    ast::Type* explicitType = nullptr;
    ast::Expression* value = nullptr;
    ast::Visibility visibility = defaultVisibility;

    const Token startToken = consumeToken();  // Consume the 'let' token
    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        DISCARD(consumeToken());  // Consume the 'mut' token
        isMutable = true;
    }
    const auto name = expectToken(TokenType::Identifier,
                                  std::format("Expected variable name after '{}'", isMutable ? "let mut" : "let"))
                          .getLexeme();

    // Type declaration
    if (peekTokenType() == TokenType::Colon) {
        DISCARD(consumeToken());  // Consume the colon
        if (peekTokenType() == TokenType::Public) {
            visibility = ast::Visibility::Public;
            DISCARD(consumeToken());  // Consume the public keyword
        } else if (peekTokenType() == TokenType::Private) [[unlikely]] {
            visibility = ast::Visibility::Private;
            DISCARD(consumeToken());  // Consume the private keyword
        }
        explicitType = parseType(Precedence::Default);
    }

    // Initializer
    if (peekTokenType() == TokenType::Assignment) {
        DISCARD(consumeToken());  // consume '='
        value = parseExpression(Precedence::Default);
    }

    expectToken(TokenType::Semicolon, "Expected semicolon after variable declaration");

    return makeNode<ast::VariableDeclarationStatement>(startToken, isMutable, name, visibility, value, explicitType);
}

// Helpers

ast::EnumValue Parser::parseEnumMember() {
    auto valueToken = expectToken(TokenType::Identifier, "Expected enum value name");
    const auto valueName = valueToken.getLexeme();
    ast::Expression* valueExpression = nullptr;

    if (peekTokenType() == TokenType::Assignment) {
        DISCARD(consumeToken());
        valueExpression = parseExpression(Precedence::Default);
    }

    return ast::EnumValue{
        .name = valueName, .value = valueExpression, .line = valueToken.getLine(), .column = valueToken.getColumn()};
}

std::vector<utils::StringID> Parser::parseGenericsList(utils::StringID contextName) {
    if (peekTokenType() != TokenType::LeftSquare) { return {}; }
    DISCARD(consumeToken());  // Consume '['

    std::vector<utils::StringID> genericTypes;
    return parseCommaSeparatedList<utils::StringID>(
        TokenType::RightSquare, "Expected a ',' to separate generic types, or a ']' to close the generic type list",
        [this, &genericTypes, contextName]() { return parseGenericTypeParameter(genericTypes, contextName); });
}

std::optional<ast::AggregateField> Parser::parseAggregateField(utils::StringID aggregateName,
                                                               std::span<ast::AggregateField> existingFields) {
    if (peekTokenType() != TokenType::Identifier) {
        logError(peekToken(), "Unexpected token '{}' in aggregate declaration. Expected field name.",
                 interner.get_view(peekToken().getLexeme()));
        DISCARD(consumeToken());  // Skip unexpected token to avoid an infinite loop
        return std::nullopt;
    }

    Token fieldToken = consumeToken();
    const auto fieldName = fieldToken.getLexeme();
    expectToken(TokenType::Colon, "Expected a ':' to declare an aggregate field type.");

    bool isMutable = false;
    if (peekTokenType() == TokenType::Mut) {
        isMutable = true;
        DISCARD(consumeToken());
    }

    ast::Type* type = parseType(Precedence::Default);
    expectToken(TokenType::Semicolon, "Expected a ';'");

    if (auto duplicate = std::ranges::find(existingFields, fieldName, &ast::AggregateField::name);
        duplicate != existingFields.end()) {
        logError(fieldToken, "Duplicate field '{}' in aggregate '{}' (previously declared at line {}, column {})",
                 interner.get_view(fieldName), interner.get_view(aggregateName), duplicate->line, duplicate->column);
        return std::nullopt;
    }

    return ast::AggregateField{.name = fieldName,
                               .type = type,
                               .line = fieldToken.getLine(),
                               .column = fieldToken.getColumn(),
                               .isMutable = isMutable};
}

std::optional<ast::FunctionParameter> Parser::parseFunctionParameter(utils::StringID functionName,
                                                                     std::span<ast::FunctionParameter> existingParams,
                                                                     bool& hasDefaultParameter,
                                                                     bool& hasVariadicParameter) {
    Token t = expectToken(TokenType::Identifier, "Expected a variable name");
    const auto paramName = t.getLexeme();

    bool isMutable = false;
    bool isVariadic = false;
    ast::Expression* defaultValue = nullptr;

    if (peekTokenType() == TokenType::Ellipsis) {
        DISCARD(consumeToken());
        if (hasVariadicParameter) {
            logError(t, "Only one variadic parameter is allowed in function '{}'", interner.get_view(functionName));
        } else {
            isVariadic = true;
            hasVariadicParameter = true;
        }
    } else if (hasVariadicParameter) {
        logError(t, "Parameter '{}' cannot follow a variadic parameter", interner.get_view(paramName));
    }

    expectToken(TokenType::Colon);
    if (peekTokenType() == TokenType::Mut) {
        DISCARD(consumeToken());
        isMutable = true;
    }
    ast::Type* paramType = parseType(Precedence::Default);

    if (peekTokenType() == TokenType::Assignment) {
        DISCARD(consumeToken());
        hasDefaultParameter = true;
        defaultValue = parseExpression(Precedence::Default);

        if (isVariadic) {
            logError(t, "Variadic parameter '{}' cannot have a default value", interner.get_view(paramName));
        }
    } else if (hasDefaultParameter) {
        logError(t, "Non-default parameter '{}' cannot follow a default parameter", interner.get_view(paramName));
    }

    if (auto duplicate = std::ranges::find(existingParams, paramName, &ast::FunctionParameter::name);
        duplicate != existingParams.end()) {
        logError(t, "Duplicate parameter '{}' in function '{}' (previously declared at line {}, column {})",
                 interner.get_view(paramName), interner.get_view(functionName), duplicate->line, duplicate->column);
        return std::nullopt;
    }

    return ast::FunctionParameter{.name = paramName,
                                  .type = paramType,
                                  .defaultValue = defaultValue,
                                  .line = t.getLine(),
                                  .column = t.getColumn(),
                                  .isMutable = isMutable,
                                  .isVariadic = isVariadic};
}

ast::CaseClause Parser::parseCaseClause() {
    DISCARD(consumeToken());  // consume 'case'
    std::vector<ast::Expression*> caseValues;

    do {
        caseValues.push_back(parseExpression(Precedence::Default));
        if (peekTokenType() == TokenType::Comma) {
            DISCARD(consumeToken());
        } else {
            break;
        }
    } while (true);

    expectToken(TokenType::Colon, std::format("Expected ':' after case value{}", (caseValues.size() > 1 ? "s" : "")));

    ast::Block caseBody;
    while (peekTokenType() != TokenType::Case && peekTokenType() != TokenType::Default
           && peekTokenType() != TokenType::RightBrace) {
        caseBody.push_back(parseStatement());
    }

    return ast::CaseClause{.values = std::move(caseValues), .body = std::move(caseBody)};
}

ast::Block Parser::parseDefaultClause() {
    DISCARD(consumeToken());  // consume 'default'
    expectToken(TokenType::Colon, "Expected ':' after default case");

    ast::Block defaultBody;
    while (peekTokenType() != TokenType::RightBrace) { defaultBody.push_back(parseStatement()); }

    if (defaultBody.empty()) { defaultBody.push_back(ast::getEmptyStatement()); }

    return defaultBody;
}

std::vector<utils::StringID> Parser::parseImportPath() {
    std::vector<utils::StringID> path;
    path.push_back(expectToken(TokenType::Identifier, "Expected a module name or path").getLexeme());

    while (peekTokenType() == TokenType::ScopeResolution) {
        DISCARD(consumeToken());  // Consume '::'
        path.push_back(expectToken(TokenType::Identifier, "Expected identifier after '::'").getLexeme());
    }

    return path;
}

utils::OptionalStringID Parser::parseImportAlias() {
    if (peekTokenType() == TokenType::As) {
        DISCARD(consumeToken());  // Consume 'as'
        return {.id = expectToken(TokenType::Identifier, "Expected an identifier as an import alias").getLexeme()};
    }

    return {};
}

utils::StringID Parser::parseGenericTypeParameter(std::vector<utils::StringID>& existingGenerics,
                                                  utils::StringID contextName) {
    Token genericToken = expectToken(TokenType::Identifier, "Expected a generic type name");
    const auto genericName = genericToken.getLexeme();

    if (std::ranges::find(existingGenerics, genericName) != existingGenerics.end()) {
        logError(genericToken, "Duplicate generic type '{}' in '{}'", interner.get_view(genericName),
                 interner.get_view(contextName));
        return {};
    }
    return genericName;
}

}  // namespace Manganese::parser