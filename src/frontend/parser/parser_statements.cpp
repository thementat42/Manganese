#include <algorithm>
#include <core.hpp>
#include <cstddef>
#include <format>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <io/logging.hpp>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace Manganese::parser {

ast::Statement* Parser::parseStatement() {
    const Token startToken = peekToken();
    const TokenType type = peekTokenType();

    if (type == TokenType::LeftBrace) {
        // don't need to move thanks to copy elision
        return makeNode<ast::NestedBlockStatement>(startToken, parseBlock("nested block"));
    }

    // Handle bare semicolons
    if (type == TokenType::Semicolon) {
        DISCARD(consumeToken());
        // still want line and column information for these
        return makeNode<ast::EmptyStatement>(startToken);
    }
    const std::size_t index = tokenToIndex(type);

    const statementHandler_t handler = statementLookup[index];
    if (handler) { return (this->*handler)(); }

    // Parse out an expression then convert it to a statement
    ast::Expression* expr = parseExpression(Precedence::Default);
    expectToken(TokenType::Semicolon, "Expected semicolon after expression");
    return makeNode<ast::ExpressionStatement>(startToken, expr);
}

// Specific statement parsing methods

ast::Statement* Parser::parseAggregateDeclarationStatement() {
    const Token startToken = consumeToken();
    std::string name = expectToken(TokenType::Identifier, "Expected aggregate name after 'aggregate'").getLexeme();

    std::vector<std::string> genericTypes = parseGenericsList(name);

    expectToken(TokenType::LeftBrace, "Expected a '{'");

    std::vector<ast::AggregateField> fields;
    while (!done() && peekTokenType() != TokenType::RightBrace) {
        if (auto field = parseAggregateField(name, fields)) { fields.push_back(std::move(*field)); }
    }

    expectToken(TokenType::RightBrace);

    return makeNode<ast::AggregateDeclarationStatement>(startToken, std::move(name), std::move(genericTypes),
                                                        std::move(fields));
}

ast::Statement* Parser::parseAliasStatement() {
    flags.parsingAliasStatement = true;
    const Token startToken = consumeToken();  // consume alias
    std::string alias = expectToken(TokenType::Identifier, "Expected an alias name").getLexeme();
    expectToken(TokenType::Assignment, "Expected '=' after an alias name to introduce the aliased type.");
    ast::Type* baseType = parseType(Precedence::Default);
    expectToken(TokenType::Semicolon, "Expected a ';' after an alias statement");
    flags.parsingAliasStatement = false;
    return makeNode<ast::AliasStatement>(startToken, baseType, std::move(alias));
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
    return makeNode<ast::WhileLoopStatement>(startToken, std::move(body), std::move(condition), /*isDoWhile=*/true);
}

ast::Statement* Parser::parseEnumDeclarationStatement() {
    const Token startToken = consumeToken();  // Consume 'enum'
    std::string name = expectToken(TokenType::Identifier, "Expected enum name after 'enum'").getLexeme();
    ast::Type* baseType = nullptr;

    if (peekTokenType() == TokenType::Colon) {
        DISCARD(consumeToken());
        baseType = parseType(Precedence::Default);
        if (!baseType) {
            logError(peekToken().getLine(), peekToken().getColumn(),
                     "Expected valid underlying type after ':' for enum '{}'", name);
        }
    }

    expectToken(TokenType::LeftBrace, "Expected '{' to start the enum body");

    std::vector<ast::EnumValue> values;
    while (!done() && peekTokenType() != TokenType::RightBrace) {
        ast::EnumValue member = parseEnumMember();

        if (auto duplicate = std::ranges::find(values, member.name, &ast::EnumValue::name); duplicate != values.end()) {
            logError(member.line, member.column,
                     "Duplicate member '{}' in enum '{}' (previously declared at line {}, column {})", member.name,
                     name, duplicate->line, duplicate->column);
        } else {
            values.push_back(std::move(member));
        }

        if (peekTokenType() != TokenType::RightBrace) {
            expectToken(TokenType::Comma, "Expected ',' between enum members");
        }
    }

    expectToken(TokenType::RightBrace, "Expected '}' to close the enum body");

    return makeNode<ast::EnumDeclarationStatement>(startToken, std::move(name), baseType, std::move(values));
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
    // TODO: Handle function attributes
    const Token startToken = consumeToken();
    std::string name = expectToken(TokenType::Identifier, "Expected function name").getLexeme();

    std::vector<std::string> genericTypes = parseGenericsList(name);

    expectToken(TokenType::LeftParen);

    std::vector<ast::FunctionParameter> params;
    bool hasDefaultParameter = false;
    bool hasVariadicParameter = false;

    while (!done() && peekTokenType() != TokenType::RightParen) {
        if (auto param = parseFunctionParameter(name, params, hasDefaultParameter, hasVariadicParameter)) {
            params.push_back(std::move(*param));
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

    return makeNode<ast::FunctionDeclarationStatement>(startToken, std::move(name), std::move(genericTypes),
                                                       std::move(params), returnType, parseBlock("function body"));
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
    const std::size_t startLine = peekToken().getLine();
    const std::size_t startColumn = peekToken().getColumn();

    if (flags.hasParsedFileHeader) {
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
    if (!duplicate) { imports.push_back({.path = std::move(path), .alias = std::move(alias)}); }
    // Dummy node since imports are stored separately
    return ast::getEmptyStatement();
}

ast::Statement* Parser::parseModuleDeclarationStatement() {
    const lexer::Token temp = consumeToken();
    const std::size_t startLine = temp.getLine();
    const std::size_t startColumn = temp.getColumn();
    if (flags.hasParsedFileHeader) {
        logging::logWarning(startLine, startColumn, "Module declarations should go at the top of the file");
    }
    const std::string name = expectToken(TokenType::Identifier, "Expected a module name").getLexeme();
    expectToken(TokenType::Semicolon, "Expected a ';' after a module declaration");
    if (!this->moduleName.empty()) {
        logError(
            startLine, startColumn,
            "A module name has previously been declared in this file. Files can only have one module declaration.");
    } else {
        this->moduleName = name;
    }

    // Dummy node since there's no need to semantically analyze module declarations (that happens here)
    return ast::getEmptyStatement();
}

ast::Statement* Parser::parseNamespace() {
    const Token startToken = consumeToken();  // skip 'namespace'
    std::string name = expectToken(TokenType::Identifier, "Expected a namespace name after 'namespace'").getLexeme();
    ast::Block body = parseBlock("namespace " + name);
    return makeNode<ast::NamespaceStatement>(startToken, std::move(name), std::move(body));
}

ast::Statement* Parser::parseRedundantSemicolon() {
    DISCARD(consumeToken());
    return ast::getEmptyStatement();
}

ast::Statement* Parser::parseReturnStatement() {
    const Token startToken = consumeToken();
    ast::Expression* expression = nullptr;
    if (peekTokenType() != TokenType::Semicolon) {
        // If the next token is not a semicolon, parse an expression
        // If it is, this is a null return statement
        expression = parseExpression(Precedence::Default);
    }
    expectToken(TokenType::Semicolon, "Expected semicolon after return statement");
    return makeNode<ast::ReturnStatement>(startToken, expression);
}

ast::Statement* Parser::parseSwitchStatement() {
    const Token startToken = consumeToken();
    const std::size_t startLine = startToken.getLine();
    const std::size_t startColumn = startToken.getColumn();

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

    if (cases.empty() && !hasDefault) {
        logging::logWarning(startLine, startColumn, "Switch statement has no cases or default body");
    }

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

    return makeNode<ast::VariableDeclarationStatement>(startToken, isMutable, std::move(name), visibility, value,
                                                       explicitType);
}

// Helpers

ast::EnumValue Parser::parseEnumMember() {
    auto valueToken = expectToken(TokenType::Identifier, "Expected enum value name");
    std::string valueName = valueToken.getLexeme();
    ast::Expression* valueExpression = nullptr;

    if (peekTokenType() == TokenType::Assignment) {
        DISCARD(consumeToken());
        valueExpression = parseExpression(Precedence::Default);
    }

    return ast::EnumValue{.name = std::move(valueName),
                          .value = valueExpression,
                          .line = valueToken.getLine(),
                          .column = valueToken.getColumn()};
}

std::vector<std::string> Parser::parseGenericsList(std::string_view contextName) {
    if (peekTokenType() != TokenType::LeftSquare) { return {}; }
    DISCARD(consumeToken());  // Consume '['

    std::vector<std::string> genericTypes;
    return parseCommaSeparatedList<std::string>(
        TokenType::RightSquare, "Expected a ',' to separate generic types, or a ']' to close the generic type list",
        [this, &genericTypes, contextName]() { return parseGenericTypeParameter(genericTypes, contextName); });
}

std::optional<ast::AggregateField> Parser::parseAggregateField(const std::string& aggregateName,
                                                               const std::vector<ast::AggregateField>& existingFields) {
    if (peekTokenType() != TokenType::Identifier) {
        logError(peekToken().getLine(), peekToken().getColumn(),
                 "Unexpected token '{}' in aggregate declaration. Expected field name.", peekToken().getLexeme());
        DISCARD(consumeToken());  // Skip unexpected token to avoid an infinite loop
        return std::nullopt;
    }

    Token fieldToken = consumeToken();
    std::string fieldName = fieldToken.getLexeme();
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
        logError(fieldToken.getLine(), fieldToken.getColumn(),
                 "Duplicate field '{}' in aggregate '{}' (previously declared at line {}, column {})", fieldName,
                 aggregateName, duplicate->line, duplicate->column);
        return std::nullopt;
    }

    return ast::AggregateField{.name = std::move(fieldName),
                               .type = type,
                               .line = fieldToken.getLine(),
                               .column = fieldToken.getColumn(),
                               .isMutable = isMutable};
}

std::optional<ast::FunctionParameter> Parser::parseFunctionParameter(
    const std::string& functionName, const std::vector<ast::FunctionParameter>& existingParams,
    bool& hasDefaultParameter, bool& hasVariadicParameter) {
    Token t = expectToken(TokenType::Identifier, "Expected a variable name");
    std::string paramName = t.getLexeme();

    bool isMutable = false;
    bool isVariadic = false;
    ast::Expression* defaultValue = nullptr;

    // Handle Variadic (...)
    if (peekTokenType() == TokenType::Ellipsis) {
        DISCARD(consumeToken());
        if (hasVariadicParameter) {
            logError(t.getLine(), t.getColumn(), "Only one variadic parameter is allowed in function '{}'",
                     functionName);
        } else {
            isVariadic = true;
            hasVariadicParameter = true;
        }
    } else if (hasVariadicParameter) {
        logError(t.getLine(), t.getColumn(), "Parameter '{}' cannot follow a variadic parameter", paramName);
    }

    // Handle Type Annotations
    expectToken(TokenType::Colon);
    if (peekTokenType() == TokenType::Mut) {
        DISCARD(consumeToken());
        isMutable = true;
    }
    ast::Type* paramType = parseType(Precedence::Default);

    // Handle Default Values
    if (peekTokenType() == TokenType::Assignment) {
        DISCARD(consumeToken());
        hasDefaultParameter = true;
        defaultValue = parseExpression(Precedence::Default);

        if (isVariadic) {
            logError(t.getLine(), t.getColumn(), "Variadic parameter '{}' cannot have a default value", paramName);
        }
    } else if (hasDefaultParameter) {
        logError(t.getLine(), t.getColumn(), "Non-default parameter '{}' cannot follow a default parameter", paramName);
    }

    // Check Duplicates
    if (auto duplicate = std::ranges::find(existingParams, paramName, &ast::FunctionParameter::name);
        duplicate != existingParams.end()) {
        logError(t.getLine(), t.getColumn(),
                 "Duplicate parameter '{}' in function '{}' (previously declared at line {}, column {})", paramName,
                 functionName, duplicate->line, duplicate->column);
        return std::nullopt;
    }

    return ast::FunctionParameter{.name = std::move(paramName),
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

    // If the body was empty, this means there's a dangling else
    // We still want this printed out, but the toString method determines the presence of an else block
    // by checking the the body is empty
    // in this edge case, there is a body but it's empty, which makes the printing logic think there isn't one
    // to get around this, add a dummy statement (doesn't print anything) so the empty else block is still
    // printed
    if (defaultBody.empty()) { defaultBody.push_back(ast::getEmptyStatement()); }

    return defaultBody;
}

}  // namespace Manganese::parser
